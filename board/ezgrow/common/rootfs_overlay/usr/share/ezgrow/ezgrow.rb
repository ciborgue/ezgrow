#!/usr/bin/ruby

require 'snmp'
require 'net/smtp'
require 'syslog/logger'
require 'getoptlong'
require 'deep_merge' # note that this has to be added to Ruby
require 'tempfile'
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
		},
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
class History
	def self.load prefs
		file = prefs['path']['history']['temp']
		return Hash.new unless File.exists? file
		File.open(file) { |f|
			return YAML.load f.read
		}
	end
	def self.save prefs, data
		file = prefs['path']['history']['temp']
		Tempfile.open('history.', File.dirname(file)) { |f|
			f.puts data.to_yaml
			f.fsync
			File.rename f.path, file
		}
	end
	def self.dump prefs, timestamp, data
		baseDirectory = prefs['path']['history']['perm']
		Dir.mkdir baseDirectory unless Dir.exists? baseDirectory

		saveDirectory = timestamp.strftime("#{baseDirectory}/%Y-%m-%d")
		Dir.mkdir saveDirectory unless Dir.exists? saveDirectory

		Tempfile.open('history.', saveDirectory) { |f|
			Zlib::GzipWriter.wrap(f) { |g|
				g.puts data.to_yaml
			}
			File.rename f.path, timestamp.strftime("#{saveDirectory}/%H:%M.gz")
		}

		File.unlink prefs['path']['history']['temp'] \
			if File.exists? prefs['path']['history']['temp']
	end
end
class State
	def self.load prefs
		file = prefs['path']['state']
		return Hash.new unless File.exists? file
		File.open(file) { |f|
			return YAML.load f.read
		}
	end
	def self.save prefs, data
		file = prefs['path']['state']
		Tempfile.open('state.', File.dirname(file)) { |f|
			f.puts data.to_yaml
			f.fsync
			File.rename f.path, file
		}
	end
end
#---------0---------0---------0---------0---------0---------0---------0---------
class Main
	DEFAULT_CONFIG	= '/etc/ezgrow/ezgrow.conf'

	TOLERANCE		= 180	# max. reading age
	INTERVAL		= 15	# hardware limit (and default) is 16s

	# these are the names of the outlets; you can use your own
	#	(but there's a little point doing so)
	GROWLAMP		=	'GrowLamp'
	EXHAUSTFAN		=	'ExhaustFan'
	ACREMOTE		=	'SHARP-CRMC-A810JBEZ'
	WATERPUMP		=	'WaterPump'
	SWITCHEDPDU		=	'SwitchedPDU'

	# these are the names of the sensors; you can use your own
	#	(but there's a little point doing so)
	INDOORTEMP		=	'IndoorTemperature'
	OUTDOORTEMP		=	'OutdoorTemperature'
	GROWZONETEMP	=	'GrowZoneTemperature'
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
			method(:updatePduData),
			method(:updateFiveVolt),
			method(:updateWaterPump),
			method(:updateGrowLamp),
			method(:updateExhaustFan),
			method(:updateAirConditioner),
		]
		@history = Hash.new
	end
	def loadJson(jsonName)
		return Hash.new unless File.exists? jsonName
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
		}
		value
	end
	def debugLog text
		text.each_line { |line|
			$stderr.puts line.chomp
			#Syslog::debug line.chomp
		}
	end
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
	def emailNotify error
		$stderr.puts error
		$stderr.puts error.backtrace
		return if File.exists? @prefs['path']['silence']
		return # we're debugging
		msgstr = <<EOF
From: #{@prefs['name from']} <#{@prefs['mail from']}>
To: #{@prefs['name to']} <#{@prefs['mail to']}>
Message-Id: <#{SecureRandom.uuid}>
Subject: #{error.message}

Automated notification. Please don't reply.

=== backtrace ===
#{error.backtrace}
=================
EOF
		Net::SMTP.start(@prefs['smtp']['server'], 25,
			@prefs['smtp']['ehlo']) { |smtp|
			smtp.send_message msgstr,
				@prefs['smtp']['mail from'],
				@prefs['smtp']['mail to']
		}
		File.open(@prefs['path']['silence'], 'w') { |f|
			f.puts error.message
			f.fsync
		}
	end
	# ideally, this should also set watchdog timeout to a few minutes via
	# the IOCTL call; this is in my TODO list
	def updateWatchdog
		debugLog "========= updateWatchdog"
		File.open(@prefs['path']['watchdog'], 'w') { |watchdog|
			watchdog.print 'DEADBEEF'
		}
	end
	def outletGet name
		outlets = @history[@timestamp]['sensor'][SWITCHEDPDU]['outlet']
		index = outlets['name'].index name
		raise RuntimeError, "PDU: No outlet '#{name}'" \
			if index == nil
		#debugLog "====== outletGet: #{name} is #{outlets['status'][index]}"
		outlets['status'][index]
	end
	def outletSet name, state
		if state == outletGet(name)
			debugLog "====== outletSet: outlet #{name} is already #{state}"
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
		debugLog ">>> updateGrowLamp: in"

		stage = @state['stage']
		cycle = @prefs['light'][stage]['cycle']
		debugLog "Light cycle selected: #{cycle}/#{24 - cycle}hr for #{stage}"

		peak = @prefs['temperature']['peak'][stage]
		peak = @state['peak'][stage] \
			if @state.has_key? 'peak' and @state['peak'].has_key? stage
		peak = Time.parse(peak) unless peak.class.to_s == 'Time'
		debugLog "Peak selected: #{peak.strftime('%H:%M')}" +
			"; now: #{@timestamp.strftime('%H:%M')}"

		lamp = ((@timestamp > peak - cycle * 1800) and
			(@timestamp < peak + cycle * 1800)) ? 'off' : 'on'

		outletSet GROWLAMP, lamp

		debugLog "<<< updateGrowLamp: out [#{lamp}]"
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
		debugLog '>>> updateAirConditioner: in'
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
		# time it receives any valid command
		if mode.eql?(@state['AC']['mode'])
			debugLog "AC mode hasn't been changed (#{mode}); NOT sending IR"
		else
			debugLog "AC mode has been changed (#{mode}); sending IR"
			debugLog @prefs['AC'][mode] % 2 # FAN=2
		end
		@state['AC']['mode'] = mode
		debugLog "<<< updateAirConditioner: out [AC is now #{mode}]"
	end
	def updateExhaustFan
		debugLog '>>> updateExhaustFan: in'
		lamp = outletGet GROWLAMP
		value = 'on' # turn ON if sensor is disabled/unavaliable
		if @prefs['sensor'].has_key? GROWZONETEMP \
			and not @prefs['sensor'][GROWZONETEMP]['disabled']
			sensorData = loadSensorData GROWZONETEMP
			limit = @prefs['temperature']['light'][lamp]['FAN']
			debugLog "updateExhaustFan: got temperature" +
				" [#{sensorData['temperature']}]" +
				" vs #{limit['on']} .. #{limit['off']}"
			if sensorData['temperature'] > limit['on']
				value = 'on'
			elsif sensorData['temperature'] < limit['off']
				value = 'off'
			end
		end
		outletSet EXHAUSTFAN, value
		debugLog "<<< updateExhaustFan: out [#{value}]"
	end
	def updateWaterPump
		# turn OFF if sensor is disabled/unavaliable; this can be dangerous
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
			emailNotify "Water is detected on the floor! Water pump is OFF"
			break
		end
		outletSet WATERPUMP, value
		debugLog "<<< updateWaterPump: out [#{value}]"
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
			emailNotify "Supply voltage is low: #{value}V"
			break
		end
		debugLog "<<< updateFiveVolt: out [#{value}]"
	end
	def updatePduData
		# asks PDU control script to retrieve status of the unit and write JSON
		#	'slow' is a debug speed hack; should be ignored by the real script
		debugLog ">>> updatePduData: in"
		pdu = @prefs[SWITCHEDPDU]
		pduObject = SwitchedPDU.new(@timestamp)
		pduObject.run({
			'--addr' => pdu['addr'],
			'--json' => pdu['json'],
			'--cmnd' => 'status',
			'--slow' => nil,
		})
#		status = system pdu['exec'] +
#			' --addr ' + pdu['addr'] +
#			' --json ' + pdu['json'] +
#			' --cmnd status' +
#			' --slow'
		raise RuntimeError, "Can't retrieve PDU status" \
			unless File.file? pdu['json']

		@history[@timestamp]['sensor'][SWITCHEDPDU] =
			json = loadJson pdu['json']

		debugLog "<<< updatePduData: out; #{json['Amp']} amp"
	end
	def operate timestamp
		@timestamp = timestamp

		debugLog "========== #{@timestamp} =========="

		@state = {	# this is the bootstrap state
			'stage' => 'grow',
			'AC' => { 'mode' => 'OFF' },
		} if (@state = State.load @prefs).empty?
		@history[@timestamp] = {
			'sensor' => Hash.new
		}

		@updates.each { |method|
			updateWatchdog
			method.call
		}

		State.save @prefs, @state

		@history[@timestamp]['runtime'] = runtime = Time.new - @timestamp

		if @timestamp.min % @interval == 0 and @timestamp.sec == 0
			History.dump @prefs, @timestamp, @history
			@history.clear
		end

		runtime
	end
end

Syslog::Logger.new 'ezgrow::main' # mark syslog records

begin
	app = Main.new

	commandLine = app.parseCommandLine GetoptLong.new(
		[ '--config', '-c', GetoptLong::REQUIRED_ARGUMENT ],
	)

	Tempfile.open('prefs.') { |f|
		f.puts JSON.pretty_generate app.prefs
		f.fsync
		File.rename f.path, '/tmp/prefs.dump.json'
	}
	
	while true
		seconds = app.interval - Time.new.sec % app.interval
		app.debugLog "Sleeping for: #{'%.2f' % seconds}s"
		sleep seconds

		seconds = app.operate Time.new
		app.debugLog "Finished in: #{'%.2f' % seconds}s"
	end
rescue => e
	app.emailNotify e
	raise
end
# vim: set fdm=syntax tabstop=4 shiftwidth=4 noexpandtab modeline:
