import os


"""
Convert a gdb register dump to a binary register dump that can be used
in troll test drive sessions

The input file is expected to contain the output of the gdb 'info registers'
command, i.e.:
---------------------------------------------
r0             0x8                 8
r1             0x8018570           134317424
r2             0x2                 2
r3             0x8004cb5           134237365
r4             0x200012a8          536875688
r5             0x200017b0          536876976
r6             0x0                 0
r7             0x2                 2
r8             0x200012a8          536875688
r9             0x801a204           134324740
r10            0x20                32
r11            0x82                130
r12            0x2                 2
sp             0x20003f20          0x20003f20
lr             0x800db9d           0x800db9d <interpret+180>
pc             0x8004cb4           0x8004cb4 <do_vla_test>
xpsr           0x41000000          1090519040
msp            0x20003f20          0x20003f20
psp            0xfffffffc          0xfffffffc
primask        0x0                 0 '\000'
basepri        0x0                 0 '\000'
faultmask      0x0                 0 '\000'
control        0x0                 0 '\000'
---------------------------------------------
The first 16 lines of the file are scanned and the register contents
are written to a binary file

"""

def convert_gdb_register_dump_to_binary_dump(input_file):
	infile = open(input_file, 'r')
	outfile = open('registers.bin', 'wb')
	lines = infile.readlines()
	regs = []

	count = 0
	for register in range(16):
		reg = int(lines[count].split()[1], 0)
		print("register {}: {}".format(count, reg))
		regs.append(reg & 0xff)
		reg >>= 8
		regs.append(reg & 0xff)
		reg >>= 8
		regs.append(reg & 0xff)
		reg >>= 8
		regs.append(reg & 0xff)

		count += 1
	outfile.write(bytearray(regs))	

for i in range(100):
	os.chdir("coredump-" + str(i))
	convert_gdb_register_dump_to_binary_dump('gdb-registers.txt')
	os.chdir('../')

