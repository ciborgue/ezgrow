#!/usr/bin/ruby

require 'time'
require 'json'
require 'tempfile'
require 'getoptlong'

class SwitchedPDU
	OUTLET_COUNT = 8
	def initialize
		@conf=Hash.new
	end
	def run(opts)
		opts.each { |key,value| @conf[key.sub(/^--/,'')] = value }
		[ 'json', 'cmnd', 'addr' ].each { |arg|
			raise "No #{arg} is provided; aborting" unless @conf.has_key? arg
		}
		if File.exist? @conf['json']
			File.open(@conf['json']) { |f|
				@data = JSON.parse f.read
			}
		else
			@data = {
				'timestamp' => Time.new,
				'outlet' => {
					'name' => (0...OUTLET_COUNT).map { |i| "Outlet#{i}" },
					'status' => (0...OUTLET_COUNT).map { "on" },
				},
				'Amps' => OUTLET_COUNT * 1.0,
			}
			@data['outlet']['name'][0] = 'GrowLamp'
			@data['outlet']['name'][1] = 'ExhaustFan'
			@data['outlet']['name'][2] = 'WaterPump'
		end
		@conf['cmnd'] = @conf['cmnd'].downcase
		case @conf['cmnd']
			when 'on', 'off', 'cycle'
				indx=nil
				if @conf.has_key? 'indx'
					indx = @conf['indx'].to_i
				elsif @conf.has_key? 'name'
					indx = @data['outlet']['name'].index @conf['name']
				else
					raise RuntimeError, "No 'name' or 'indx' provided"
				end
				raise RuntimeError, "Outlet does not exist" \
					if indx == nil || @data['outlet']['status'][indx] == nil
				@data['outlet']['status'][indx] = @conf['cmnd']
			when 'status'
				# do nothing; just collect status and save it
			else
				raise RuntimeError, "Unknown command: [#{@conf['cmnd']}]"
		end
		@data['Amps'] = 0.0
		@data['outlet']['status'].each { |status|
			@data['Amps'] += 1.0 if status == 'on'
		}
		Tempfile.open { |f|
			f.puts JSON.generate @data
			f.fsync
			system "mv #{f.path} #{@conf['json']}"
		}
	end
end

opts = GetoptLong.new(
	[ '--json', '-j', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--name', '-n', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--indx', '-i', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--cmnd', '-c', GetoptLong::REQUIRED_ARGUMENT ],
	[ '--addr', '-a', GetoptLong::REQUIRED_ARGUMENT ],	# ignored
	[ '--slow', '-s', GetoptLong::NO_ARGUMENT ],		# ignored
)

SwitchedPDU.new.run opts
