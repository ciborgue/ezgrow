#!/usr/bin/ruby

require 'snmp'
require 'net/smtp'
require 'syslog/logger'
require 'getoptlong'
require 'deep_merge'
require 'tempfile'
require 'fileutils'
require 'time'
require 'json' # use JSON for data import
require 'yaml' # use YAML for state save; as it doesn't convert Time to String
require 'zlib'

include SNMP

class SwitchedPDU
	MAXAGE = 15 * 60 # 15 minutes
	OUTLET_COUNT = 8
	PDU_MODEL = {
		'APC' => {
			'offset' => 1, # outlet #1 is at rPDUOutletStatusOutletState.1
			'power' => 'PowerNet-MIB::rPDUIdentDevicePowerVA.0',
			'name' => 'PowerNet-MIB::sPDUOutletName',
			'status' => 'PowerNet-MIB::sPDUOutletCtl',
			'command' => 'PowerNet-MIB::sPDUOutletCtl',
			'state' => { # from the MIB
				'on' => 'outletOn',
				'off' => 'outletOff',
				'reboot' => 'outletReboot',
				'unknown' => 'outletUnknown',
				'onPending' => 'outletOnWithDelay',
				'offPending' => 'outletOffWithDelay',
				'cyclePending' => 'outletRebootWithDelay',
			},
			'value' => {
				'on' => 'outletOn',
				'off' => 'outletOff',
			},
			'numeric' => {
				0 => 'off',
				1 => 'on',
			},
		},
		'Dataprobe' => {
			'offset' => 0, # outlet #1 is at outletStatus.0
			'power' => 'IBOOTBAR-MIB::currentLC1.0',
			'name' => 'IBOOTBAR-MIB::outletName',
			'status' => 'IBOOTBAR-MIB::outletStatus',
			'command' => 'IBOOTBAR-MIB::outletCommand',
			'state' => { # from the MIB
				'on' => 'on',
				'off' => 'off',
				'reboot' => 'reboot',
				'cycle' => 'cycle',
				'onPending' => 'onPending',
				'cyclePending' => 'cyclePending',
			},
			'value' => {
				'on' => 'on',
				'off' => 'off',
			},
			'numeric' => {
				0 => 'off',
				1 => 'on',
			},
		}
	}
	def initialize timestamp
		@timestamp = timestamp
		@conf = Hash.new
		PDU_MODEL.each_key { |maker|
			PDU_MODEL[maker]['numeric'].default = 'unknown'
		}
	end
	def snmpGet oid
		target = {
				:version => :SNMPv1,
				:community => @conf['--pass'],
				:host => @conf['--addr'],
		}
		mibObject = MIB.new
		mibObject.load_module oid.sub(/::.*/, '')
		reading = ObjectId.new(mibObject.oid(oid))
		Manager.open(target) { |manager|
				response = manager.get(reading)
				varbind = response.varbind_list.first
				return varbind.value
		}
		nil
	end
	def snmpSet oid, value
		target = {
			:version => :SNMPv1,
			:community => @conf['--pass'],
			:host => @conf['--addr'],
		}
		mibObject = MIB.new
		mibObject.load_module oid.sub(/::.*/, '')
		reading = ObjectId.new(mibObject.oid(oid))
		varbind = VarBind.new(reading, SNMP::Integer.new(value))
		Manager.open(target) { |manager|
			manager.set varbind
		}
	end
	def collectPowerFromSNMP data
		reading = snmpGet PDU_MODEL[data['maker']]['power']
		raise "Wrong variable type" \
			unless reading.class.to_s == 'SNMP::Integer'
		if data['maker'] == 'APC'
			# compared to data from the multimeter; idk why APC uses 125V
			data['Amp'] = (reading.to_f - 2.0) / 125.0
		elsif data['maker'] == 'Dataprobe'
			data['Amp'] = reading.to_f / 10.0
		else
			raise RuntimeError, "Unsupported PDU [internal error]"
		end
	end
	def collectNameFromSNMP data
		# internally all the outlets are kept in zero-based array, i.e.
		# outlet #1 is name[0]; use the offset to find the correct spot
		data['outlet']['name'] = value = Array.new
		(0...OUTLET_COUNT).each { |index|
			reading = snmpGet PDU_MODEL[data['maker']]['name'] +
				".#{index + PDU_MODEL[data['maker']]['offset']}"
			raise "Wrong variable type" \
				unless reading.class.to_s == 'SNMP::OctetString'
			value[index] = reading.to_s
		}
	end
	def collectStatusFromSNMP data
		data['outlet']['status'] = value = Array.new
		(0...OUTLET_COUNT).each { |index|
			reading = snmpGet PDU_MODEL[data['maker']]['status'] +
				".#{index + PDU_MODEL[data['maker']]['offset']}"
			raise "Wrong variable type" \
				unless reading.class.to_s == 'SNMP::Integer'
			value[index] = PDU_MODEL[data['maker']]['numeric'][reading.to_i]
		}
	end
	def collectFromSNMP
		data = {
			'timestamp' => @timestamp,
			'outlet' => {}, # data container
		}
		sysObjectID = snmpGet('SNMPv2-MIB::sysObjectID.0')
		raise RuntimeError, "Bad SNMP response; aborting" \
			unless sysObjectID.class.to_s == 'SNMP::ObjectId'
		sysObjectID = sysObjectID.to_s # convert sysObjectID from SNMP::*
		if sysObjectID == 'IBOOTBAR-MIB::iBootBarAgent' or
			sysObjectID == 'SNMPv2-SMI::enterprises.1418.4'
			data['maker'] = 'Dataprobe'
		elsif sysObjectID == 'PowerNet-MIB::masterSwitchrPDU' or
			sysObjectID == 'SNMPv2-SMI::enterprises.318.1.3.4.5'
			data['maker'] = 'APC'
		else
			raise RuntimeError, "Unsupported PDU: #{sysObjectID}"
		end
		collectPowerFromSNMP data
		collectNameFromSNMP data
		collectStatusFromSNMP data
		data
	end
	def snmpExecuteCommand data, zeroIndx, userCmnd
		return userCmnd if ! @conf.has_key?('slow') \
			and data['outlet']['status'][zeroIndx] == userCmnd

		zeroIndx += PDU_MODEL[data['maker']]['offset']
		target = PDU_MODEL[data['maker']]['command']

		rawValue = 0
		PDU_MODEL[data['maker']]['numeric'].each { |k,v|
			if v.eql? userCmnd
				rawValue = k
				break
			end
		}

		snmpSet "#{target}.#{zeroIndx}", rawValue

		data['outlet']['status'][zeroIndx] = userCmnd
	end
	def getDefaultPass list
		list.each { |f|
			next unless File.file? f
			File.open(f).each_line { |line|
				line = line.chomp.split
				if line[0].eql? 'includeFile'
					list.push line[1] unless list.include? line[1]
				elsif line[0].eql? 'defCommunity'
					return line[1]
				end
			}
		}
		'public' # nothing found, use 'public'
	end
	def run conf
		@conf = conf

		[ '--addr', '--json', '--cmnd' ].each { |requiredArg|
			raise "No #{requiredArg} is provided; aborting" \
				unless @conf.has_key? requiredArg
		}

		@conf['--pass'] = getDefaultPass [ '/etc/snmp/snmp.conf' ] \
			unless @conf.has_key? '--pass'

		data = { 'timestamp' => Time.at(0) } # trigger reload
		if ! @conf.has_key?('--slow') and File.file?(@conf['--json'])
			File.open(@conf['--json']) { |f|
				data = JSON.parse f.read
			}
			data['timestamp'] = Time.parse data['timestamp']
		end

		# reload data if it is too old or '--slow' is given
		data = collectFromSNMP if (@timestamp - data['timestamp']) > MAXAGE

		@conf['cmnd'] = @conf['--cmnd'].downcase
		case @conf['cmnd']
			when 'on', 'off'
				indx,outlet = nil,data['outlet']
				if @conf.has_key? '--indx'
					# we're zero-based, command line is one-based
					indx = @conf['--indx'].to_i - 1
				elsif @conf.has_key? '--name'
					# see if there's an outlet by this name
					indx = outlet['name'].index @conf['--name']
				else
					raise RuntimeError, "No 'name' or 'indx' provided"
				end

				raise RuntimeError, 'Outlet does not exist' \
					if indx == nil or indx >= OUTLET_COUNT

				# don't update amps here; delay it until next status
				snmpExecuteCommand data, indx, @conf['--cmnd'] \
					if outlet['status'][indx] != @conf['--cmnd']
			when 'status'
				# do nothing; status will be saved later anyway
			else
				raise RuntimeError, "Unknown command: [#{@conf['--cmnd']}]"
		end

		Tempfile.open('switched-pdu.', File.dirname(@conf['--json'])) { |f|
			f.puts JSON.pretty_generate data
			f.fsync
			File.rename f.path, @conf['--json']
		}
	end
end
#---------0---------0---------0---------0---------0---------0---------0---------
class Main
	DEFAULT_CONFIG		= '/etc/ezgrow/ezgrow.conf'

	TOLERANCE		= 180	# max. reading age
	INTERVAL		= 15	# hardware limit (and default) is 16s

	# these are the names of the outlets; you can use your own
	#	(but there's a little point doing so)
	GROWLAMP		=	'GrowLamp'
	EXHAUSTFAN		=	'ExhaustFan'
	INTERNALFAN		=	'InternalFan'
	ACREMOTE		=	'SHARP-CRMC-A810JBEZ'
	WATERPUMP		=	'WaterPump'
	AIRPUMP			=	'AirPump'
	SWITCHEDPDU		=	'SwitchedPDU'

	# these are the names of the sensors; you can use your own
	#	(but there's a little point doing so)
	INDOORTEMP		=	'IndoorTemperature'
	OUTDOORTEMP		=	'OutdoorTemperature'
	GROWZONETEMP		=	'GrowZoneTemperature'
	WATERLEVEL		=	'WaterLevel'
	WATERSENSOR		=	'WaterSensor'
	FIVEVOLT		=	'FiveVolt'

	attr_reader	:interval
	attr_reader	:history
	attr_reader	:prefs

	def initialize
		@interval = INTERVAL
		@prefs = Hash.new
		@updates = [
			method(:updateFiveVolt),
			method(:updatePduData),
			method(:updateAirPump),
			method(:updateWaterPump),
			method(:updateGrowLamp),
			method(:updateExhaustFan),
			method(:updateInternalFan),
			method(:updateAirConditioner),
			method(:updateWatchdog), # make sure watchdog is updated before exit
		]
		@state = {	# this is the bootstrap state
			'stage' => 'grow',
			'AC' => { 'mode' => 'OFF' },
		}
		@history = {
			Time.new => {
				'log' => [ 'Script restarted' ]
			}
		}
	end
	def loadJson(jsonName)
		return Hash.new unless File.file? jsonName
		File.open(jsonName) { |json|
			return JSON.parse json.read
		}
	end
	def parseCommandLine commandLine
		names, jsons, value = Array.new, Array.new, Hash.new
		commandLine.each { |k,v|
			value[k] = v
			case k
				when '--config'
					names.push v
			end
		}
		names.push DEFAULT_CONFIG if names.empty?
		names.each { |jsonName|
			if File.exists? jsonName and File.file? jsonName
				jsons.push loadJson jsonName
				next unless jsons.last.has_key? 'include'
				jsons.last['include'].each { |includeName|
					names.push includeName \
						unless names.include? includeName
				}
			elsif File.exists? jsonName and Dir.exists? jsonName
				Dir.entries(jsonName).each { |entry|
					jsonFullName = jsonName + '/' + entry
					next unless jsonFullName.match /\.conf$/ and
						File.file? jsonFullName
					names.push jsonFullName
				}
			else
				raise RuntimeError, "config: #{jsonName} not found"
			end
		}
		jsons.each { |jsonFile|
			@prefs.deep_merge! jsonFile
		}
		File.open('/tmp/prefs.dump.json', 'w') { |f| # this is for debug only
			f.puts JSON.pretty_generate @prefs
			f.fsync
		}
		updateWatchdog
		value
	end
	def debugLog text
		text.each_line { |line|
			$stderr.puts line.chomp
			@history[@timestamp]['log'].push line.chomp
		}
	end
	def historyDump prefix, interval = INTERVAL
		return unless @timestamp.min % interval == 0 and @timestamp.sec == 0

		baseDirectory = @prefs['path']['history']['perm']
		Dir.mkdir baseDirectory unless Dir.exists? baseDirectory

		saveDirectory = @timestamp.strftime("#{baseDirectory}/%Y-%m-%d")
		Dir.mkdir saveDirectory unless Dir.exists? saveDirectory

		Tempfile.open(prefix, saveDirectory) { |f|
			Zlib::GzipWriter.wrap(f) { |g|
				g.puts @history.to_yaml
				g.finish
			}
			f.fsync
			File.rename f.path,
				@timestamp.strftime("#{saveDirectory}/#{prefix}%H%M.gz")
		}

		@history.clear
	end
	def smtpNotifiedRecently name
		timeout = @prefs['smtp']['timeout'][name]
		timeout = @prefs['smtp']['timeout']['default'] if timeout == nil

		silenceDir = @prefs['path']['silence']
		Dir.mkdir silenceDir unless File.directory? silenceDir

		flag = silenceDir + '/' + name

		lastNotified = Time.at(0)
		lastNotified = File.ctime(flag) if File.file? flag

		if @timestamp == nil # test invocation with '--test-email'
			@timestamp = Time.new
			@history[@timestamp] = { 'log' => [] }
		end
		if (@timestamp - lastNotified) < timeout
			debugLog "====== smtpNotifiedRecently: supressed email for #{name}"
			return true
		end
		debugLog "====== smtpNotifiedRecently: sending email for #{name}"
		FileUtils::touch flag
		false
	end
	def emailNotify name, error
		### Notify user by email; argument is the message subject
		### 1. Exit if flag file present
		### 2. Send email
		### 3. Create flag file
		### Note: flag file should be on the persistent storage because
		###	system is going to be rebooted by the watchdog
		### Note: this file should be deleted if script terminates with no error
		### Note: it is possible that config files are misconfigured and
		###	email can't be sent yet. This situation is not covered; you
		###	must make your configuration files correct or else you won't
		###	be auto-notified.
		return if smtpNotifiedRecently name
		smtp = @prefs['smtp']
		msgstr = <<EOF
From: #{smtp['name from']} <#{smtp['mail from']}>
To: #{smtp['name to']} <#{smtp['mail to']}>
CC: #{smtp['cc'].join ','}
EOF
		if error.class.to_s.eql? 'String'
			msgstr += <<EOF
Subject: #{name}: #{error}

This is automated notification; application is still running but wants to
notify you.
EOF
		else
			msgstr += <<EOF
Subject: #{name}: #{error.message}

This is automated notification; application crashed, backtrace is attached.
Note that RPi is going to be rebooted by the watchdog timer and you might get
more notificatons soon.

=== exception backtrace ===
#{error.backtrace}
===========================
EOF
		end
		
		server = Net::SMTP.new(smtp['server'], smtp['port'])
		#server.set_debug_output $stderr
		server.enable_tls if smtp['enable_tls']
		server.start(smtp['ehlo'],
			smtp['login'], smtp['password'], :login) { |f|
			f.send_message msgstr, smtp['mail from'],
				[smtp['mail to']] + smtp['cc']
		}
	end
	def loadSensorData name
		raise RuntimeError, "Unknown sensor: #{name}" \
			if (sensor = @prefs['sensor'][name]) == nil

		data = @history[@timestamp]['sensor']

		### don't reload data for the same sensor/timestamp combination
		### it's only next time script runs data is going to be reloaded
		unless data.has_key? name
			# no cached data; attempt to reload
			if not File.file?(sensor['json']) or
				((@timestamp - File.ctime(sensor['json'])) > TOLERANCE)
				# No JSON data file or file ctime is too old
				return nil unless sensor['required']
				raise RuntimeError, "Essential sensor stuck: #{name}"
			end
			data[name] = loadJson sensor['json']
			# TODO: check embedded timestamp
		end
		
		### select the correct sensor
		data[name].each { |key,value|
			next unless key.match sensor['regex']
			value['timestamp'] = Time.parse value['timestamp'] \
				unless value['timestamp'].class.to_s == 'Time'
			return value
		}

		return nil unless sensor['required']
		raise RuntimeError, "Nothing matches #{sensor['regex']}" +
			" on required sensor #{sensor}"
	end
	def updateWatchdog
#		@watchdog = File.open(@prefs['path']['watchdog'], 'w') \
#			if @watchdog == nil
#		@watchdog.print 'DEADBEEF: ' + @timestamp.to_s
	end
	def outletGet name
		outlets = @history[@timestamp]['sensor'][SWITCHEDPDU]['outlet']
		index = outlets['name'].index name
		raise RuntimeError, "PDU: No outlet '#{name}'" if index == nil
		outlets['status'][index]
	end
	def outletSet name, state
		if state == outletGet(name)
			#debugLog "====== outletSet: outlet #{name} is already #{state}"
		else
			debugLog "====== outletSet: outlet #{name} switching to #{state}"
			SwitchedPDU.new(@timestamp).run({
				'--addr' => @prefs[SWITCHEDPDU]['addr'],
				'--json' => @prefs[SWITCHEDPDU]['json'],
				'--name' => name, '--cmnd' => state,
			})
			# Don't reload JSON; it takes some time to switch outlet so
			# just wait until the next cycle. It's not 100% accurate but
			# it's acceptable to me.
		end
		state
	end
	def updateGrowLamp
		### no need for dynamic cycle update; it's really 12/12 or 6/18
		###		(but you can permanently adjust it to 4/20 or even 0/24)
		stage = @state['stage']
		cycle = @prefs['light'][stage]['cycle']

		peak = @prefs['temperature']['peak'][stage]
		peak = @state['peak'][stage] \
			if @state.has_key? 'peak' and @state['peak'].has_key? stage
		peak = Time.parse(peak) unless peak.class.to_s == 'Time'

		lamp = (((@timestamp > (peak - cycle * 30 * 60)) and
			(@timestamp < (peak + cycle * 30 * 60)))) ? 'off' : 'on'

		debugLog "#{lamp}: #{cycle}, #{@timestamp} vs" +
			" #{peak - cycle * 30 * 60} / #{peak + cycle * 30 * 60}"

		lamp = 'off'

		outletSet GROWLAMP, lamp

		debugLog "<<< updateGrowLamp: out [#{lamp}; #{peak}; #{cycle}]"
	end
	def calculateAcMode degc
		limit = @prefs['temperature']['light'][outletGet(GROWLAMP)]
		mode = @state['AC']['mode']
		debugLog ">>> calculateAcMode: in, AC is #{mode}, temp is #{degc}"

		if degc > limit['COOL']['on']
			mode = 'COOL' # any mode -> COOL
			debugLog "it's hot -> #{degc} / #{mode}"
		elsif ['COOL'].include?(mode) and degc > limit['COOL']['off']
			mode = 'COOL' # keep COOL if we're on COOL
			debugLog "it's still hot -> #{degc} / #{mode}"
		else
			mode = 'VENT' # decide VENT/OFF later
			debugLog "it's colder -> #{degc} / #{mode}"
		end
		if not ['COOL'].include?(mode) and degc > limit['VENT']['on']
			mode = 'VENT' # any mode except COOL -> VENT
			debugLog "it's is warm -> #{degc} / #{mode}"
		elsif ['VENT'].include?(mode) and degc > limit['VENT']['off']
			mode = 'VENT' # keep VENT if we're on VENT
			debugLog "it's still warm -> #{degc} / #{mode}"
		else
			mode = 'OFF'
			debugLog "it's cold -> #{degc} / #{mode}"
		end
		debugLog "<<< calculateAcMode: out, mode is #{mode}"
		mode
	end
	def updateAirConditioner
		lamp,mode = outletGet(GROWLAMP),@state['AC']['mode']

		# use IndoorTemperature if avail; use GrowZoneTemperature as a backup
		#	(GrowZone is required and raises an exception is missing)
		reftemp = [
			loadSensorData(INDOORTEMP),
			loadSensorData(GROWZONETEMP),
			loadSensorData(OUTDOORTEMP),
		]
		degc = nil
		if reftemp[0] != nil
			degc = reftemp[0]['temperature']
		else
			debugLog "WARNING: IndoorTemperature is missing;" +
				" using backup GrowZoneTemperature .."
			degc = reftemp[1]['temperature']
		end
		mode = calculateAcMode degc

		### possibly change VENT -> OFF if outside is warmer
		if reftemp[2] != nil and 'VENT'.eql?(mode) and
			degc < reftemp[2]['temperature']
			mode = 'OFF'
			debugLog "it's colder in than out, switching off"
		end

		# see if IR has to be send; don't resend as the unit beeps every
		# time it receives ANY valid command. Send if AC mode has been changed
		# and at top & bottom of each hour
		if not mode.eql?(@state['AC']['mode']) or
			[0,30].include? @timestamp.min and @timestamp.sec == 0
			debugLog "AC mode has been changed (#{mode}); sending IR"
			debugLog @prefs['AC'][mode] % 2 # FAN=2
		else
			debugLog "AC mode hasn't been changed (#{mode}); NOT sending IR"
		end
		@state['AC']['mode'] = mode
		debugLog "<<< updateAirConditioner: out [AC is now #{mode}]"
	end
	def updateInternalFan
		debugLog '>>> updateInternalFan: in'
		value = 'off'
		if outletGet(GROWLAMP).eql? 'off'
			# ON for light-off time
			value = 'on'
		elsif outletGet(EXHAUSTFAN).eql? 'off'
			# ON when it's too cold for exhaust fan to run
			value = 'on'
		else
			value = updateFan(INTERNALFAN, GROWZONETEMP)
		end
		outletSet INTERNALFAN, value if ['on', 'off'].include? value
		debugLog "<<< updateInternalFan: out [#{outletGet INTERNALFAN}]"
	end
	def updateExhaustFan
		debugLog '>>> updateExhaustFan: in'
		value = updateFan(EXHAUSTFAN, GROWZONETEMP)
		outletSet EXHAUSTFAN, value if ['on', 'off'].include? value 
		debugLog "<<< updateExhaustFan: out [#{outletGet EXHAUSTFAN}]"
	end
	def updateFan outlet, sensor
		value, lampState, sensorData =
			outletGet(outlet), outletGet(GROWLAMP), loadSensorData(sensor)
		limit = @prefs['temperature']['light'][lampState][outlet]
		debugLog "updateFan: t = [#{sensorData['temperature']}]" +
			" on: #{limit['on']} off: #{limit['off']}"
		if limit['on'] < limit['off'] # is this device warming or cooling?
			if sensorData['temperature'] < limit['on']
				value = 'on'
			elsif sensorData['temperature'] > limit['off']
				value = 'off'
			else
				value = 'keep'
			end
		else
			if sensorData['temperature'] > limit['on']
				value = 'on'
			elsif sensorData['temperature'] < limit['off']
				value = 'off'
			else
				value = 'keep'
			end
		end
		value
	end
	def updateAirPump
		debugLog ">>> updateAirPump: in"
		value = 'on' # air is always on
		outletSet AIRPUMP, value
		debugLog "<<< updateAirPump: out [#{outletGet AIRPUMP}]"
	end
	def updateWaterPump
		# turn OFF if sensor is disabled/unavaliable
		# leaving pump running can be dangerous
		debugLog ">>> updateWaterPump: in"
		value = 'off'
		sensorConf = @prefs['sensor'][WATERSENSOR]
		while sensorConf != nil and not sensorConf['disabled']
			break if (sensorData = loadSensorData WATERSENSOR) == nil
			voltage = sensorData[sensorConf['use']]['voltage']
			debugLog "updateWaterPump: got voltage [#{voltage}]" +
				" vs #{@prefs['voltage'][WATERSENSOR]['voltage']}"
			unless voltage < @prefs['voltage'][WATERSENSOR]['voltage']
				value = 'on'
				break
			end
			emailNotify WATERSENSOR, "Water on the floor! Water pump is OFF"
			break
		end
		outletSet WATERPUMP, value
		debugLog "<<< updateWaterPump: out [#{outletGet WATERPUMP}]"
	end
	def updateFiveVolt
		# don't do anything; just send an email if 5V is not really 5.00
		debugLog ">>> updateFiveVolt: in"
		value = nil
		sensorConf = @prefs['sensor'][FIVEVOLT]
		while sensorConf != nil and not sensorConf['disabled']
			break if (sensorData = loadSensorData FIVEVOLT) == nil
			value = sensorData[sensorConf['use']]['voltage']
			break unless value < @prefs['voltage'][FIVEVOLT]['voltage']
			emailNotify FIVEVOLT, "Supply voltage is low: #{value}V"
			break
		end
		debugLog "<<< updateFiveVolt: out [#{value}]"
	end
	def updatePduData
		debugLog ">>> updatePduData: in"
		pdu = @prefs[SWITCHEDPDU] # get PDU setup
		pduObject = SwitchedPDU.new(@timestamp)
		pduObject.run({'--addr' => pdu['addr'], '--json' => pdu['json'],
			'--cmnd' => 'status',
			'--slow' => nil,
		})
		@history[@timestamp]['sensor'][SWITCHEDPDU] =
			json = loadJson pdu['json']
		amp = json['Amp']
		limit = pdu['Amp'][outletGet GROWLAMP]
		if amp > limit['hi']
			emailNotify SWITCHEDPDU, 'Power consumption is too high!' +
				'Powering off'
		elsif amp < limit['lo']
			emailNotify SWITCHEDPDU, 'Power consumption is too low!' +
				'Is main lamp failed?'
		end
		debugLog "<<< updatePduData: out; #{amp} amp; #{limit.inspect}"
	end
	def operate timestamp
		@timestamp = timestamp
		@history[@timestamp] = {
			'sensor' => Hash.new,
			'last-runtime' => nil,
			'log' => Array.new,
		}
		debugLog "========== operate: #{@timestamp} =========="
		@updates.each { |method|
			updateWatchdog
			method.call
		}
		@history[@timestamp]['last-runtime'] = Time.new - @timestamp
		@history[@timestamp].delete 'sensor'
		historyDump 'operation-'

		@history['last-runtime']
	end
	def archiver timestamp
		@timestamp = timestamp
		debugLog "========== archiver: #{@timestamp} =========="

		# load switched PDU JSON
		jsons = { @prefs[SWITCHEDPDU]['json'] => nil }

		# collect all JSON names used
		@prefs['sensor'].each { |name,data|
			jsons[data['json']] = nil \
				if data.has_key? 'json' and not jsons.has_key? data['json']
		}

		# load all JSON files
		jsons.keys.each { |k|
			jsons[k] = loadJson k if File.file?(k) and
				((@timestamp - File.ctime(k)) < TOLERANCE)
		}

		@history[@timestamp] = jsons # add data to the history

		historyDump 'sensors-', 60
	end
end

Syslog::Logger.new 'ezgrow::main' # mark syslog records

begin
	app = Main.new

	commandLine = app.parseCommandLine GetoptLong.new(
		[ '--config', '-c', GetoptLong::REQUIRED_ARGUMENT ],
		[ '--operate', '-o', GetoptLong::NO_ARGUMENT ],
		[ '--archiver', '-l', GetoptLong::NO_ARGUMENT ],
		[ '--test-email', '-t', GetoptLong::NO_ARGUMENT ],
	)

	if commandLine.has_key? '--test-email'
		app.emailNotify 'ApplicationTest', 'Email test#1, application notify'
		raise RuntimeError, 'Email test#2, exception caught'
	end

	while true
		seconds = app.interval - Time.new.sec % app.interval
		sleep seconds
		timestamp = Time.new # reference timestamp
		if commandLine.has_key? '--archiver'
			sleep 8 # actually archive 8 sec AFTER the timestamp
			app.archiver timestamp
		elsif commandLine.has_key? '--operate'
			app.operate timestamp
		else
			raise RuntimeError, 'Nothing to do; use --operate or --archiver'
		end
	end
rescue => e
	app.emailNotify 'Internal', e
	raise
end
# vim: set fdm=syntax tabstop=4 shiftwidth=4 noexpandtab modeline:
