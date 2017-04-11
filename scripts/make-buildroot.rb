#!/usr/bin/ruby
# vim: set noai noexpandtab ts=4 sw=4:

class Build
	def initialize br2_path, br2_board
        @br2_path,@br2_board = br2_path,br2_board

        @ez_path = $0.sub(/(\/[^\/]+){2}$/, '')
		throw "Is this script taken out of context?" \
			unless File.exists? "#{@ez_path}/external.desc"
		puts "Setting BR2_EXTERNAL:\n\t#{@ez_path}"

        # load buildroot's config file; like raspberrypi_defconfig,
        # raspberrypi2_defconfig, raspberrypi3_defconfig etc.
		print "Loading #{@br2_board} from #{@br2_path} .."
        @br2_config=Hash.new
        File.open("#{@br2_path}/configs/#{@br2_board}_defconfig") { |f|
            f.each { |line|
                next if line.match /^$/ or line.match /^[[:space:]]*#/
                line=line.chomp.split(/=/, 2)
                @br2_config[line[0]] = line[1]
            }
        }
		puts "\n\t[#{@br2_config.size} line(s)] done"

        # parse project ('company') name; it's likely 'ezgrow'
		print "Loading project description file from #{@ez_path} .."
		File.open("#{@ez_path}/" +
				'external.desc').each { |line|
            next unless line.match /^name:[[:space]]*/
            @project = line.sub(/^name:[[:space:]]*/, '').chomp.downcase
            break
        }
		puts "\n\t[#{@project}] done"

        # project_config is what needs to be changed in the @br2_config
		print "Loading project configuration file(s) .."
		@project_config={
            # Default password is your project name; it's ok as password login
            # over the network is disabled. Change here if you're paranoid
            # Default hostname is your project name; there's no need to change
            # it. However, nothing is relied on it so you can do it if you want
            'BR2_TARGET_GENERIC_ROOT_PASSWD' => "\"#{@project}\"",
            'BR2_TARGET_GENERIC_HOSTNAME' => "\"#{@project}\"",
			'BR2_GLOBAL_PATCH_DIR' =>
				"\"$(BR2_EXTERNAL_#{@project.upcase}_PATH)/patches\"",
        }
		['common', "#{br2_board}"].each { |config|
			fragment="board/#{@project}/#{config}/buildroot.fragment"
			print "\n\t#{fragment} .. "
			if not File.exists? @ez_path + '/' + fragment
				print "not present"
				next
			end
			File.open("#{@ez_path}/#{fragment}").each { |line|
				next if line.match /^$/ or line.match /^[[:space:]]*#/
				line=line.chomp.split(/=/, 2)
				@project_config[line[0]] = line[1]
			}
			print "[now: #{@project_config.size}] ok"
		}
		puts "\n\tTotal: #{@project_config.size} line(s); done"
	end
    def writePackageLocations
        # this is to be used in post-*.sh scripts
		print "Saving 'project_locations' file .."
        File.open('project_locations', 'w') { |f|
            f.puts "PROJECT_BR2_PATH=\"#{@br2_path}\""
            f.puts "PROJECT_EZ_PATH=\"#{@ez_path}\""
            f.puts "PROJECT_NAME=\"#{@project}\""
            f.puts "PROJECT_BR2_BOARD=\"#{@br2_board}\""
			f.puts "PROJECT_EZ_SITE_SIZE=16"
			f.puts "PROJECT_EZ_NO_LOGO=\"logo.nologo\""
			#f.puts "PROJECT_EZ_NO_LOGO=\"\"" # uncomment to get ganja boot logo
        }
		puts " done"
	end
    def writePackageConfig
        # absolute path is, indeed, required by 'make'
        defconfig="#{ENV['PWD']}/#{@project}_defconfig"
        File.open(defconfig, 'w') { |f|
            f.puts @br2_config.merge(@project_config).map { |k,v|
                "#{k}=#{v}" }.join "\n"
        }
        makeArguments=[
            "BR2_EXTERNAL=#{@ez_path}",
            "BR2_DEFCONFIG=#{defconfig}",
            "-C #{@br2_path}",
            "O=#{ENV['PWD']}",
            "defconfig",
        ]
        puts "'make' arguments: " + makeArguments.join("\n\t")
		`make #{makeArguments.join ' '}`.each_line { |line|
			puts line
		}
		return unless $? == 0
		puts <<EOF

#     #     #     #    #  #######   ###   
##   ##    # #    #   #   #         ###   
# # # #   #   #   #  #    #         ###   
#  #  #  #     #  ###     #####      #    
#     #  #######  #  #    #               
#     #  #     #  #   #   #         ###   
#     #  #     #  #    #  #######   ###   

Now you can run it.
EOF
    end
	def writePackageGenimage
        # this is not universal but should work for RPi, RPi2, RPi3
        print "Making genimage.cfg for #{@br2_board} .."
        genimage=Array.new
		File.open("#{@br2_path}/board/" +
				"#{@br2_board}/genimage-#{@br2_board}.cfg") { |f|
            f.each_line { |line|
                line.chomp!
                break if line.match /^image[[:space:]]+sdcard.img/
                line.sub!(/=[[:space:]]*32M[[:space:]]*$/, '= 16M') \
                    if line.match /^[[:space:]]*size[[:space:]]*=/
                genimage.push line
            }
        }
        File.open('genimage.cfg', 'w') { |f|
            f.puts genimage.join "\n"
        }
		puts " done"
	end
	def run
		writePackageLocations
		writePackageGenimage
        writePackageConfig
	end
end
def usage
    puts "Usage: #{$0.sub /.*\//, ''} BuildrootPATH ModelConfigFile"
    puts "\ti.e. #{$0.sub /.*\//, ''} ~/buildroot raspberrypi3"
    exit 1
end
unless ARGV.length == 2 && File.exists?("#{ARGV[0]}/configs/#{ARGV[1]}_defconfig")
    usage
else
    Build.new(ARGV[0], ARGV[1]).run
end
