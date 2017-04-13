#!/usr/bin/ruby

require 'time'
require 'json'
require 'syslog/logger'
require 'tempfile'
require 'getoptlong'

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
		},
	}
	def initialize timestamp
		@timestamp = timestamp
		@conf = Hash.new
	end
	def debugLog text
		text.each_line { |line|
			Syslog::debug line.chomp
		}
	end
	def collectPowerFromSNMP data
		debugLog 'entry: collectPowerFromSNMP'
		target = PDU_MODEL[data['maker']]['power']
		`snmpwalk #{@conf['addr']} #{target} 2> /dev/null`.each_line { |line|
			if data['maker'] == 'APC'
				# compared to data from the multimeter; idk why APC uses 125V
				data['Amp'] = (line.chomp.split[1].to_f - 2.0) / 125.0
			elsif data['maker'] == 'Dataprobe'
				data['Amp'] = line.chomp.split[1].to_f / 10.0
			else
				raise RuntimeError, "Unsupported PDU [internal error]"
			end
		}
		raise RuntimeError, "Can't get power consumption; aborting" \
			unless data.has_key? 'Amp'
	end
	def collectNameFromSNMP data
		debugLog 'entry: collectNameFromSNMP'
		# internally all the outlets are kept in zero-based array, i.e.
		# outlet #1 is name[0]; use the offset to find the correct spot
		data['outlet']['name'] = value = Array.new
		target = PDU_MODEL[data['maker']]['name']
		`snmpwalk #{@conf['addr']} #{target} 2> /dev/null`.each_line { |line|
			line = line.chomp.split(/[[:space:]]+/, 2)
			line[0] = line[0].split /\./
			index = line[0][-1].to_i - PDU_MODEL[data['maker']]['offset']
			name = line[1].sub(/^"/,'').sub(/"$/,'')
			break if index == OUTLET_COUNT
			value[index] = name
		}
	end
	def collectStatusFromSNMP data
		debugLog 'entry: collectStatusFromSNMP'
		data['outlet']['status'] = value = Array.new
		target = PDU_MODEL[data['maker']]['status']
		`snmpwalk #{@conf['addr']} #{target} 2> /dev/null`.each_line { |line|
			line = line.chomp.split(/[[:space:]]+/, 2)
			line[0] = line[0].split /\./
			index = line[0][-1].to_i - PDU_MODEL[data['maker']]['offset']
			break if index == OUTLET_COUNT
			PDU_MODEL[data['maker']]['state'].each { |k,v|
				value[index] = k if v == line[1]
			}
			raise RuntimeError, "Unknown outlet status #{line[1]}" \
				if value[index] == nil
		}
	end
	def collectFromSNMP
		debugLog  'entry: collectFromSNMP'
		data = {
			'timestamp' => @timestamp,
			'outlet' => {}, # data container
		}
		`snmpget #{@conf['addr']} sysObjectID.0 2> /dev/null`.each_line { |line|
			line = line.chomp.split
			raise RuntimeError, "Bad SNMP response; aborting" \
				unless line[0] == 'SNMPv2-MIB::sysObjectID.0'
			if line[1] == 'IBOOTBAR-MIB::iBootBarAgent'
				data['maker'] = 'Dataprobe'
			elsif line[1] == 'PowerNet-MIB::masterSwitchrPDU'
				data['maker'] = 'APC'
			else
				raise RuntimeError, "Unsupported PDU: #{line[1]}"
			end
		}
		collectPowerFromSNMP data # order is used as a delay; don't change
		collectNameFromSNMP data
		collectStatusFromSNMP data
		data
	end
	def snmpExecuteCommand data, zeroIndx, userCmnd
		return userCmnd if ! @conf.has_key?('slow') \
			and data['outlet']['status'][zeroIndx] == userCmnd

		zeroIndx += PDU_MODEL[data['maker']]['offset']
		target = PDU_MODEL[data['maker']]['command']
		value = PDU_MODEL[data['maker']]['value'][userCmnd]
		
		system "snmpset #{@conf['addr']} #{target}.#{zeroIndx}" +
			" = #{value} > /dev/null 2> /dev/null"

		data['outlet']['status'][zeroIndx] = userCmnd
	end
	def run opts
		opts.each { |key,value| @conf[key.sub(/^--/,'')] = value }
		[ 'addr', 'json', 'cmnd' ].each { |requiredArg|
			raise "No #{requiredArg} is provided; aborting" \
				unless @conf.has_key? requiredArg
		}

		data = { 'timestamp' => Time.at(0) } # trigger reload
		if ! @conf.has_key?('slow') and File.file?(@conf['json'])
			File.open(@conf['json']) { |f|
				data = JSON.parse f.read
			}
			data['timestamp'] = Time.parse data['timestamp']
		end

		# reload data if it is too old
		data = collectFromSNMP if (@timestamp - data['timestamp']) > MAXAGE

		@conf['cmnd'] = @conf['cmnd'].downcase
		case @conf['cmnd']
			when 'on', 'off'
				indx,outlet = nil,data['outlet']
				if @conf.has_key? 'indx'
					# we're zero-based, command line is one-based
					indx = @conf['indx'].to_i - 1
				elsif @conf.has_key? 'name'
					# see if there's an outlet by this name
					indx = outlet['name'].index @conf['name']
				else
					raise RuntimeError, "No 'name' or 'indx' provided"
				end

				raise RuntimeError, 'Outlet does not exist' \
					if indx == nil or indx >= OUTLET_COUNT

				# don't update amps here; delay it until next status
				snmpExecuteCommand data, indx, @conf['cmnd'] \
					if outlet['status'][indx] != @conf['cmnd']
			when 'status'
				# do nothing; status will be saved later anyway
			else
				raise RuntimeError, "Unknown command: [#{@conf['cmnd']}]"
		end

		Tempfile.open { |f|
			f.puts JSON.pretty_generate data
			f.fsync
			File.rename f.path, @conf['json']
		}
	end
end

Syslog::Logger.new 'switched-pdu' # mark syslog records

opts = GetoptLong.new(
	[ '--json', '-j', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--name', '-n', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--indx', '-i', GetoptLong::REQUIRED_ARGUMENT ], # 'natural' index: 1, 2
	[ '--cmnd', '-c', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--addr', '-a', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--slow', '-s', GetoptLong::NO_ARGUMENT ],
	# slow:	1. Don't use JSON file as source, always get fresh SNMP data
	#	2. Always send the command, even if there is no change present
)

SwitchedPDU.new(Time.new).run opts
