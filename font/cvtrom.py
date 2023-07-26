#!/usr/bin/python3
# Script to convert a Kaypro II font rom into a modern font file.
#
# Requires python3 and fontforge be installed, and a copy of the rom
# image in the current directory.
#
# Kevin Handy, Juny 2023
# License: bsd
#
import fontforge

dodots = 1      # Use round dots instead of rectangles to make glyphs.

romfile = "81-187.rom"
fontname = "kpii81"

rowsize = 16
colsize = 8
rowwid = 60
colwid = 100

print("rom file =", romfile, ", font name =", fontname)

#
# Create a fontforge font
#
newfont=fontforge.font()
newfont.fontname = fontname

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
        newglyph = newfont.createChar(charloop)

        pen = newfont[charloop].glyphPen()

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

                        cbase = 800 - colloop * colwid;
                        rbase = 800 - 75 - rowloop * rowwid;

                        if dodots:
                            pen.moveTo((cbase, rbase))
                            pen.curveTo( \
                                (cbase + colwid / 2, rbase), \
                                (cbase + colwid / 2, rbase), \
                                (cbase + colwid / 2, rbase + rowwid / 2))
                            pen.curveTo( \
                                (cbase + colwid / 2, rbase + rowwid), \
                                (cbase + colwid / 2, rbase + rowwid), \
                                (cbase, rbase + rowwid))
                            pen.curveTo( \
                                (cbase - colwid / 2, rbase + rowwid), \
                                (cbase - colwid / 2, rbase + rowwid), \
                                (cbase - colwid / 2, rbase + rowwid / 2))
                            pen.curveTo( \
                                (cbase - colwid / 2, rbase), \
                                (cbase - colwid / 2, rbase), \
                                (cbase, rbase))
                            pen.closePath()
                        else:
                            pen.moveTo((cbase, rbase))
                            pen.lineTo((cbase, rbase + rowwid))
                            pen.lineTo((cbase + colwid + 1, rbase + rowwid))
                            pen.lineTo((cbase + colwid + 1, rbase))
                            pen.closePath()


                else:
                        coltext = "." + coltext

            print(coltext, rowdata)

        print()
        if dodots:
            pass
        else:
            newglyph.removeOverlap()
            newglyph.simplify()

        pen = None

#
# Save the font file
#
newfont.save(fontname + ".sfd")
