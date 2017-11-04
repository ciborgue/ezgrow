#!/usr/bin/ruby
# vim: set noai noexpandtab ts=4 sw=4:
require 'pp'

class Build
	def initialize ez_project, br2_path, br2_board
		@cfg = {
			'ez_path' => $0.sub(/(\/[^\/]+){2}$/, ''),
			'ez_project' => ez_project,
			'br2_path' => br2_path,
			'br2_board' => br2_board,
		}

		throw "Project doesn't exist: #{ez_project}" \
			unless Dir.exists? "#{@cfg['ez_path']}/project/#{@cfg['ez_project']}"

        # load buildroot's config file; like 'raspberrypi3_defconfig'
        @cfg['br2_config'] =
			loadConfigFile("#{@cfg['br2_path']}/configs/#{@cfg['br2_board']}_defconfig")

		['common', ez_project].each { |project|
			['common', br2_board].each { |board|
				fragment="#{@cfg['ez_path']}/board/#{project}/#{board}/buildroot.fragment"
				@cfg['br2_config'].merge!(loadConfigFile(fragment))
			}
		}

		# add static config items
		@cfg['br2_config'].merge!({
            # Default password is your project name; it's ok as password login
            # over the network is disabled. Change here if you're paranoid
            # Default hostname is your project name; there's no need to change
            # it. However, nothing is relied on it so you can do it if you want
            'BR2_TARGET_GENERIC_ROOT_PASSWD' => "\"#{@cfg['br2_project']}\"",
            'BR2_TARGET_GENERIC_HOSTNAME' => "\"#{@cfg['br2_project']}\"",
			'BR2_GLOBAL_PATCH_DIR' => "\"$(BR2_EXTERNAL)/patches\"",
        })
	end
	def loadConfigFile pathname
		value = Hash.new
		File.exists?(pathname) && File.open(pathname) { |file|
            file.each { |line|
                next if line.match /^$/ or line.match /^[[:space:]]*#/
                line=line.chomp.split(/=/, 2)
                value[line[0]] = line[1]
            }
        }
		value
	end
    def writePackageLocations
        # this is to be used in post-*.sh scripts
        File.open('project_locations', 'w') { |f|
            f.puts "PROJECT_BR2_PATH=\"#{@cfg['br2_path']}\""
            f.puts "PROJECT_EZ_PATH=\"#{@cfg['ez_path']}\""
            f.puts "PROJECT_NAME=\"#{@cfg['ez_project']}\""
            f.puts "PROJECT_BR2_BOARD=\"#{@cfg['br2_board']}\""
			f.puts "PROJECT_EZ_SITE_SIZE=16"
			f.puts "PROJECT_EZ_NO_LOGO=\"logo.nologo\""
			#f.puts "PROJECT_EZ_NO_LOGO=\"\"" # uncomment to get boot logo
        }
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
unless ARGV.length == 3
    puts "Usage: #{$0.sub /.*\//, ''} project buildroot board"
    puts "\ti.e. #{$0.sub /.*\//, ''} ezgrow ~/buildroot ezgrow raspberrypi3"
    exit 1
end
Build.new(ARGV[0], ARGV[1], ARGV[2]).run
