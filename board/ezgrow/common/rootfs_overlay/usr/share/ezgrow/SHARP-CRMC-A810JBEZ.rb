#!/usr/bin/ruby
# vim: set ts=8 sw=8 noet:

class RC
	SLOT = 550
	DELAY = [ "#{SLOT}\t#{SLOT}", "#{SLOT}\t#{SLOT * 2}" ]
	HEADER = "3750\t1875"

	HEXES = {
		'0000' => 0x0,
		'1000' => 0x1,
		'0100' => 0x2,
		'1100' => 0x3,
		'0010' => 0x4,
		'1010' => 0x5,
		'0110' => 0x6,
		'1110' => 0x7,
		'0001' => 0x8,
		'1001' => 0x9,
		'0101' => 0xa,
		'1101' => 0xb,
		'0011' => 0xc,
		'1011' => 0xd,
		'0111' => 0xe,
		'1111' => 0xf,
	}
	def xor(data)
		nxor=0
		(0...data.length/4).each { |i|
			nxor ^= HEXES[data[i*4..i*4+3]]
		}
		(0..3).map { |i|
			value = nxor % 2
			nxor >>= 1
			value
		}.join ''
	end
	def dmp(code)
		code.split(//).map { |i|
			DELAY[i.to_i]
		}.join "\t"
	end
	def remote_begin
		value = <<EOF
# Due to the nature of AC remote controls it's not possible to fully
# implement it in LIRC. Almost every key sends almost the whole unit
# setup in a long (68bit) packet (+ 32 bit header and 4 bit XOR). On
# top of that, I don't know if LIRC allows you to encode more than
# 64 bits without resorting to the raw codes. So I decided to implement
# only the very basic codes, just enough to control grow room temperature
# Also, I'm not 100% sure if all the codes here valid or make sense
# Too bad that implementing flow management (a.k.a oscillationg louvers)
# will double the size of this file
begin remote
	name	SHARP-CRMC-A810JBEZ
	flags	RAW_CODES
	eps	30 
	aeps	100 

	gap	30000
	begin raw_codes
EOF
	end
	def remote_end
		value = <<EOF
	end raw_codes
end remote
# vim: set ts=8 noet:
EOF
	end
	def button(name, data)
		code = ("01010101.01011010.11110011.00001000" +
			data + "011111000").gsub(/[^01]/,'')
		hdr = <<EOF
		name	#{name}
		#{HEADER}
		#{(0...code.length/4).map { |i| dmp(code[i*4..i*4+3]) }.join "\n\t\t"}
		#{dmp(xor(code))}
		#{SLOT}
EOF
	end
end

rc = RC.new

SPEED = { '3' => '1110', '2' => '1010', '1' => '1100', 'AUTO' => '0100', }
FHEIT = {
	'64' => '100010',
	'65' => '010010',
	'66' => '110010',
	'67' => '001010',
	'68' => '101010',
	'69' => '011010',
	'70' => '111010',
	'71' => '000110',
	'72' => '100110',
	'73' => '010110',
	'74' => '110110',
	'75' => '001110',
	'76' => '101110',
	'77' => '011110',
	'78' => '111110',
	'79' => '100001',
	'81' => '010001',
	'80' => '110001',
	'82' => '001001',
	'83' => '101001',
	'84' => '011001',
	'85' => '111001',
	'86' => '000101',
}

puts rc.remote_begin
puts rc.button "OFF", \
  "101100.00100001001100.0100.00000000.000.1000000000001.00000000000"
puts rc.button "MAX_DENSITY", \
	"100000.00100001101010.0100.00000000.000.1000000000001.10000000001"
SPEED.each { |k,v|
	puts rc.button "ON_VENT_#{k}", \
	  "100000.00100010000011.#{v}.00000000.000.1000000000001.00000000000"
	puts rc.button "ON_ION_#{k}", \
	  "100000.00100010001010.#{v}.00000000.000.1000000000001.00000000001"
	puts rc.button "ON_DEHUM_#{k}", \
	  "101100.00100010001100.#{v}.00000000.000.1000000000001.00000000000"
	puts rc.button "ON_FAN_#{k}", \
	  "100000.00100010000010.#{v}.00000000.000.1000000000001.00000000000"
}
FHEIT.each { |tk,tv|
	puts rc.button "MAX_COOL_#{tk}", \
		"#{tv}.11100001100100.0100.00000000.000.1000000000001.10000000000"
	SPEED.each { |sk,sv|
		puts rc.button "ON_COOL_#{tk}_#{sk}", \
		  "#{tv}.11100010000100.#{sv}.00000000.000.1000000000001.00000000000"
	}
}
puts rc.remote_end
