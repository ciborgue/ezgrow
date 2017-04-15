#!/usr/bin/ruby

require 'snmp'
require 'time'
require 'json'
require 'syslog/logger'
require 'tempfile'
require 'getoptlong'

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
	def debugLog text
		text.each_line { |line|
			Syslog::debug line.chomp
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
		debugLog 'entry: collectPowerFromSNMP'
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
		debugLog 'entry: collectNameFromSNMP'
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
		debugLog 'entry: collectStatusFromSNMP'
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
		debugLog  'entry: collectFromSNMP'
		data = {
			'timestamp' => @timestamp,
			'outlet' => {}, # data container
		}
		sysObjectID = snmpGet('SNMPv2-MIB::sysObjectID.0').to_s
		raise RuntimeError, "Bad SNMP response; aborting" \
			if sysObjectID == nil
		if sysObjectID == 'IBOOTBAR-MIB::iBootBarAgent' or
			sysObjectID == 'SNMPv2-SMI::enterprises.1418.4'
			data['maker'] = 'Dataprobe'
		elsif sysObjectID == 'PowerNet-MIB::masterSwitchrPDU' or
			sysObjectID == 'SNMPv2-SMI::enterprises.318.1.3.4.5'
			data['maker'] = 'APC'
		else
			raise RuntimeError, "Unsupported PDU: #{sysObjectID}"
		end
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

		value = 0
		PDU_MODEL[data['maker']]['numeric'].each { |k,v|
			if v.eql? userCmnd
				value = k
				break
			end
		}

		snmpSet "#{target}.#{zeroIndx}", value

		data['outlet']['status'][zeroIndx] = userCmnd
	end
	def getDefaultPass list
		list.each { |f|
			next unless File.file? f
			File.open(f).each_line { |line|
				line = line.chomp.split
				if line[0].eql? 'includeFile'
					list.push line[1] \
						unless list.include? line[1]
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

		# reload data if it is too old
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

Syslog::Logger.new 'switched-pdu' # mark syslog records

opts = GetoptLong.new(
	[ '--json', '-j', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--name', '-n', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--indx', '-i', GetoptLong::REQUIRED_ARGUMENT ], # 'natural' index: 1, 2
	[ '--cmnd', '-c', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--addr', '-a', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--pass', '-p', GetoptLong::REQUIRED_ARGUMENT ], # SNMP community (password)
	[ '--slow', '-s', GetoptLong::NO_ARGUMENT ],
	# slow:	1. Don't use JSON file as source, always get fresh SNMP data
	#	2. Always send the command, even if there is no change present
)
args = Hash.new
opts.each { |key,value| args[key] = value }

SwitchedPDU.new(Time.new).run args
