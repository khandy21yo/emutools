#!/usr/bin/python3

from sys import argv

#romfile = sys.argv[1]
#fontname = argv[2]

romfile = "81-x015.rom"
fontname = "kpii81greek"

#romfile = "81-187.rom"     # 81-235.rom duplicate
#fontname = "kpii81"

rowsize = 16
colsize = 8

print("rom file =", romfile, ", font name =", fontname)

#
# Process one rom file
#
with open(romfile, "rb", encoding=None) as romf:

    #
    # We grab the entire rom in one go.
    # Its size should,nt be a problem, unless it is a bad file.

    romdata = romf.read()
    print("romzise =", len(romdata))

    #
    # Loop for all characters
    # We skip over all the control charaters (0,.31) because those
    # are blank (from inspection)
    #
    for charloop in range(0, 256):

        print("Starting", charloop)

        #
        # There are 8 rows (top to bottom) in each character.
        #
        for rowloop in range(0, rowsize):

            coltext = ''

            rowdata = romdata[charloop * rowsize + rowloop]

            #
            # There are 8 columns (left to right) in each character.
            #
            for colloop in range(0, colsize):

                if rowdata & (1 << colloop):
                        coltext = "*" + coltext
                else:
                        coltext = "." + coltext

            print(coltext, rowdata)

        print()

