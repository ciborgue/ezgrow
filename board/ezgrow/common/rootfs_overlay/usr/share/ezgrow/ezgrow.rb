#!/usr/bin/ruby

require 'net/smtp'
require 'syslog/logger'
require 'getoptlong'
require 'deep_merge' # note that this has to be added to Ruby
require 'tempfile'
require 'time'
require 'json' # use JSON for data import
require 'yaml' # use YAML for state save; as it doesn't convert Time to String
require 'zlib'

class History
	def self.load prefs
		file = prefs['path']['history']['temp']
		return Hash.new unless File.exists? file
		Zlib::GzipReader.open(file) { |f|
			return YAML.load f.read
		}
	end
	def self.save prefs, data
		file = prefs['path']['history']['temp']
		Zlib::GzipWriter.open(file) { |f|
			f.puts data.to_yaml
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
		Zlib::GzipReader.open(file) { |f|
			return YAML.load f.read
		}
	end
	def self.save prefs, data
		Tempfile.open { |f|
			Zlib::GzipWriter.wrap(f) { |g|
				g.puts data.to_yaml
			}
			File.rename f.path, prefs['path']['state']
		}
	end
end
#---------0---------0---------0---------0---------0---------0---------0---------
class Main
	DEFAULT_CONFIG	= '/etc/ezgrow/ezgrow.conf'

	TOLERANCE		= 180	# max. reading age
	INTERVAL		= 10

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
		debugLog "====== outletGet: #{name} is #{outlets['status'][index]}"
		outlets['status'][index]
	end
	def outletSet name, state
		if state == outletGet(name)
			debugLog "====== outletSet: outlet #{name} is already #{state}"
		else
			debugLog "====== outletSet: outlet #{name} switching to #{state}"
			system @prefs[SWITCHEDPDU]['exec'] +
				" --name '#{name}'" +
				" --addr #{@prefs[SWITCHEDPDU]['addr']}" +
				" --json #{@prefs[SWITCHEDPDU]['json']}" +
				" --cmnd #{state}"
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

		if data.has_key? name
			debugLog "loadSensorData: reusing #{sensor['json']} .."
		else
			debugLog "loadSensorData: loading #{sensor['json']} .."
			if not File.exists? sensor['json'] or
				((@timestamp - File.ctime(sensor['json'])) > TOLERANCE)
				if sensor['required']
					raise RuntimeError, "Essential sensor stuck: #{name}"
				else
					return nil
				end
			end
			data[name] = loadJson sensor['json']
		end
		
		### select the correct sensor; don't recheck 'timestamp' here
		data[name].each { |key,value|
			next unless key.match sensor['regex']
			value['timestamp'] = Time.parse value['timestamp'] \
				unless value['timestamp'].class.to_s == 'Time'
			debugLog "loadSensorData: #{name}/#{sensor['regex']}"
			return value
		}

		return nil unless sensor['required']
		raise RuntimeError, "Essential sensor has no regex: #{name}"
	end
	def calculateAcMode degc, mode, limit
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
		mode
	end
	def updateAirConditioner
		lamp = outletGet GROWLAMP
		debugLog ">>> updateAirConditioner: in, lamp is: #{lamp}"

		mode,limit = @state['AC']['mode'],@prefs['temperature']['light'][lamp]

		# use IndoorTemperature if avail.; use GrowZoneTemperature as a backup
		#	(this one MUST be present)
		if (indoor = loadSensorData INDOORTEMP) != nil
			debugLog "Using IndoorTemperature .."
			mode = calculateAcMode indoor['temperature'], mode, limit
		elsif (growzonetemp = loadSensorData GROWZONETEMP) != nil
			debugLog "Using GrowZoneTemperature .."
			mode = calculateAcMode growzonetemp['temperature'], mode, limit
		end

		# possibly change VENT -> OFF if outside is hotter than inside
		if (outdoor = loadSensorData OUTDOORTEMP) != nil \
			and ['VENT'].include?(mode)
			degc = outdoor['temperature']

			if indoor != nil and degc > indoor['temperature']
				mode = 'OFF'
				debugLog "it's colder inside than out -> #{mode}"
			elsif growzonetemp != nil and degc > growzonetemp['temperature']
				mode = 'OFF' # weird, unlikely
				debugLog "it's colder in grow zone than out -> #{mode}"
			end
		end

		if mode == @state['AC']['mode']
			debugLog "AC mode hasn't changed (#{mode}); NOT sending IR"
		else
			@state['AC']['mode'] = mode # save AC mode; it can't be read back

			debugLog "AC mode has changed (#{mode}); sending IR"
			$stderr.puts @prefs['AC'][mode] % 2 # FAN=2
		end

		debugLog "<<< updateAirConditioner: out [#{mode}]"
	end
	def updateExhaustFan
		lamp = outletGet GROWLAMP
		debugLog ">>> updateExhaustFan: in, lamp is #{lamp}"
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
		status = system pdu['exec'] +
			' --addr ' + pdu['addr'] +
			' --json ' + pdu['json'] +
			' --cmnd status' +
			' --slow'
		raise RuntimeError, "Can't retrieve PDU status" \
			if not status or ! File.exists? pdu['json']

		@history[@timestamp]['sensor'][SWITCHEDPDU] =
			json = loadJson pdu['json']

		debugLog "<<< updatePduData: out; #{json['Amp']} amp"
	end
	def operate timestamp
		@timestamp = timestamp
		updateWatchdog

		debugLog "==============================================="
		debugLog "========== #{@timestamp} =========="


		@state = {	# this is the bootstrap state
			'stage' => 'grow',
			'AC' => { 'mode' => 'OFF' }
		} if (@state = State.load @prefs).empty?

		@history[@timestamp] = {
			'sensor' => Hash.new
		} unless (@history = History.load @prefs).has_key? @timestamp

		@updates.each { |method|
			method.call
			updateWatchdog
		}

		@history[@timestamp]['runtime'] = Time.new - @timestamp

		if @history.length >= @prefs['settings']['save-history-every']
			History.dump @prefs, @timestamp, @history
		else
			History.save @prefs, @history
		end

		State.save @prefs, @state

		updateWatchdog

		@history[@timestamp]['runtime']
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
		sleep seconds

		seconds = app.operate Time.new
		app.debugLog "Finished in: #{'%.2f' % seconds}s"
	end
rescue => e
	app.emailNotify e
	raise
end
# vim: set fdm=syntax tabstop=4 shiftwidth=4 noexpandtab modeline:
