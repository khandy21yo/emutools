//! \file
//! \brief epl2 to pdf emulation
//!
//! This program attempts to generate a pdf file from an epl2 script.
//! It does not emulate a specific epl2 printer.
//! It does not include non-volitile memory.
//! It is missing many commands.
//!
//! I am only implementing:
//! 1. commands I make use of
//! 2. Commands I can figure out how to emulate.
//!
//! Only parts of each command have been implemented.
//! Only those bits that exists in my tests have been handled.
//! and only those parts I have figured out how to do in podofo.
//!
//! \author Kevin Handy, Jan 2018
//!
//


//
// PDF transform matrix
//
// a scale in x
// b rotation
// c rotation
// d scale in y
// e translate in x
// f translate in y
//
// [1 0 0 1 tx ty]
// 	Translate (move) image base to tx ty
// [sx 0 0 sy 0 0]
// 	Scale image (resize based on sx sy)
// [cos(x) sin(x) -sin(x) cos(x) 0 0]
// 	Rotate image
// [1 tan(x)  tan(b) 1 0 0]
// 	skews the x by (x) and the y by (y)
//
//
//
//
//
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include <popt.h>

#include <podofo/podofo.h>
using namespace PoDoFo;

#include "epl2pdf.h"
#include "podofo_barcode.h"

//
// Local variables
//
static int debug = 0;		//!< Are debugging messages enabled?

//!
//! \brief Container for parsing parsing epl2 commands.
//!
class cmd_class : public std::vector<std::string>
{
public:
	//! \brief Resize to number of parameters for a command.
	//!
	//! Extends the number of parameters to a minimum size, so that
	//! we don't need to error trap everywhere for missing parameters.
	//! Fill trailing parameters as blanks.
	void minsize(int min)
	{
		if (size() < min)
		{
			resize(min);
		}
	}

	int split_cmd(const std::string &cmd);
	void dump();
};

//!
//! \brief Class to hold all the epl2 data together
//!
//! Container for all of the epl2 data.
//!
class epl2_class
{
public:
	float dpi;	//!< dots per inch (epl)
	char *ofile;	//!< File to process
	float href;	//!< Horizontal Rederence poinnt (epl)
	float vref;	//< Vertical Reference point (epl)

	//
	// PODOFO pdf objects
	//
	// Defined here because it makes them available to all functions
	// within this class without requiring a lot of static defines in
	// main code, or passing them through all the functions.
	//
public:
	PdfStreamedDocument *document;	//!< podofo document output stream
	PdfPage* pPage;			//!< podofo page pointer
	PdfPainter painter;		//!< podofo painter object
	PdfFont* pFont;			//!< podofo font.
					//!<  Name is currently hardcoded.

public:
	//! \brief constructor
	epl2_class()
	{
		dpi = 200.0;		// Dots per inch
		document = 0;		// Not yet created
		ofile = 0;		// Not yet parsed
		href = 0.0;		// Reference point
		vref = 0.0;		// Reference point
	}
	void process_stream(std::istream &in);
	void process_line(const std::string &buffer);

public:
	//!\brief Convert a std::string value to a float
	//!
	//! If the string is blank, returns a default value,
	//! otherwise it converts the string to a float value.
	//! If the string contans non-numeric characters,
	//! they will halt the conversion without causing an error.
	//!
	inline float cvt_tofloat(
		std::string &p,			//!< String to convert
		float def = 0.0)		//!< Default value
	{
		if (p == "")
		{
			return def;
		}
		else
		{
			return std::stof(p);
		}
	}
	std::string cvt_tostring(
		std::string &p);

	//!\brief Convert from EPl2 point measurement to PDF measurements
	//!
	inline float cvt_pttopt(
		float p)		//< Value to be converted
	{
		//
		// Divide the p (point size) by the number of points/in
		// to get the the size in inches, then multiply that by
		// the postscript 72 points/inch to get the right size.
		//
		return (p / dpi) * 72.0;
	}

	//!\brief Convert from EPl2 vertical position to PDF position`
	//!
	inline float cvt_vpostovpos(
		float p)		//< Value to be converted
	{
		//
		// EPL puts 0,0 at the top left,
		// while PDF puts it at the top left.
		//
		return pPage->GetPageSize().GetHeight() -
			(cvt_pttopt(p + vref));
	}

	//!\brief Convert from EPl2 horizontal position to PDF position`
	//!
	inline float cvt_hpostohpos(
		float p)		//< Value to be converted
	{
		//
		// Divide the p (point size) by the number of points/in
		// to get the the size in inches, then multiply that by
		// the postscript points/inch to get the right size.
		//
		return cvt_pttopt(p + href);
	}
	//!\brief Convert from EPl2 Barcode Style to ZINT style`
	//!
	inline float cvt_style(
		std::string p)		//< Value to be converted
	{
		if (p == "" || p == "3")	// Code 39
		{
			return BARCODE_EXCODE39;
		}
		else if (p == "3C")		// Code 39 with check digit
		{				// This isn't implemented yet
			return BARCODE_EXCODE39;
		}
		else if (p == "9")		// Code 39 with check digit
		{				// This isn't implemented yet
			return BARCODE_CODE93;
		}
		else if (p == "0")		// Code 128 special shipping
		{				//  container
						// This isn't zint compatable

			return BARCODE_CODE128;
		}
		else if (p == "1")		// Code 128 audo A,B,C
		{
			return BARCODE_CODE128;
		}
		else if (p == "1A")		// Code 128  A
		{				// Not Zint Compatable
			return BARCODE_CODE128;
		}
		else if (p == "1B")		// Code 128 B
		{
			return BARCODE_CODE128B;
		}
		else if (p == "1C")		// Code 128 C
		{				// Not Zint Compatable
			return BARCODE_CODE128;
		}
		else if (p == "K")		// Codabar
		{
			return BARCODE_CODABAR;
		}
		return BARCODE_EXCODE39;
	}
};

//!
//! \brief Print popt command line usage info and exit program
//!
void usage(
	poptContext optCon,	//!< popt Context
	int exitcode,		//!< Program abort code
	char *error,		//!< Error why we called the usage function
	char *addl)		//!< Additional messages to print
{
	poptPrintUsage(optCon, stderr, 0);
	if (error)
	{
		std::cerr << error << ": " <<  addl << std::endl;
	}
	exit(exitcode);
}

//
// Some local variables
//
//!
//! \brief We need one implementation of the epl2_class.
//!
//! This program (currently) emulates a single epl2  printer, This is it.
//!
static epl2_class epl2;


//!
//! \brief Main driver)
//!
int main(int argc, const char **argv)
{
	int c;			// popt command letter
	const char *filename;	// File to process

	//
	// Handle command line arguements
	//
	poptContext optCon;

	const struct poptOption optionsTable[] =
	{
		{ "dpi", 'p', POPT_ARG_INT, &epl2.dpi, 0,
			 "dots per inch", "BPS" },
		{ "output", 'o', POPT_ARG_STRING, &epl2.ofile, 0,
			 "utput file name", "filename" },
		{ "debug", 'L', POPT_ARG_NONE, 0, 'd',
			 "debugging messages", "" },
		POPT_AUTOHELP
		{ nullptr, 0, 0, nullptr, 0 }
	};

	optCon = poptGetContext(nullptr, argc, argv, optionsTable, 0);
	poptSetOtherOptionHelp(optCon, "[OPTIONS]* <file.epl>");

	if (argc < 2)
	{
		poptPrintUsage(optCon, stderr, 0);
		exit(1);
	}

	/* Now do options processing, get portname */
	while ((c = poptGetNextOpt(optCon)) >= 0)
	{
		switch (c)
		{
		case 'd':
			debug = 1;
			break;
		}
	}

	//
	// We need an utput file name.
	// Force it if we have to.
	//
	if (epl2.ofile == 0)
	{
		try
		{
			epl2.ofile = strdup("/tmp/epl2pdf.pdf");
		} catch(const PdfError & eCode)
		{
			eCode.PrintErrorMsg();
			return eCode.GetError();
		}
	}

	//
	// Initialize for a podofo pdf output
	//
	// We have to go through this off business because podofo doesn't
	// want to create a document without the file name. and we cannot
	// do that in the class before we get the output file name.
	//
	PdfStreamedDocument mydocument(epl2.ofile);
	epl2.document = &mydocument;

	//
	// Process all file names passed on command ilne
	//
	while (filename = poptGetArg(optCon))
	{
		std::cerr << "File: " << filename << std::endl;

		if (strcmp(filename, "-") == 0)
		{
			epl2.process_stream(std::cin);
		}
		else
		{
			std::ifstream ifs;
			ifs.open(filename);
			epl2.process_stream(ifs);
		}
	}

	poptFreeContext(optCon);

}

//******************************************************************
// Implement cmd_class
//******************************************************************

//!\brief Split epl2 command into seperate components
//!
//! Splits an EPL2 command line into individual components.
//!
int cmd_class::split_cmd(
	const std::string &cmd)		//! EPL2 command line to split
{
	std::string part;
	int ptr = 0;
	bool inquote = false;

	clear();

	if (cmd.size() == 0)
	{
		return -1;
	}

	//
	// 1st one or two characters is the main command
	//
	if (isalpha(cmd[0]) == 0)
	{
		// Not a valid command
		return -1;
	}

	if (cmd.size() > 1 && isalpha(cmd[1]))
	{
		// Two character command
		push_back(cmd.substr(0, 2));
		ptr = 2;
	}
	else
	{
		// One character command
		push_back(cmd.substr(0, 1));
		ptr = 1;
	}

	while (ptr < cmd.size())
	{
		switch(cmd[ptr])
		{
		// Special character
		case '\\':
			// Handle marker at tail end of string
			if ((ptr + 1) < cmd.size())
			{
				part += cmd[ptr + 1];
				ptr += 2;
			}
			else
			{
				part += cmd[ptr];
				ptr++;
			}
			break;

		// Quotation mark
		case '"':
			inquote = !inquote;
			part += cmd[ptr];
			ptr++;
			break;

		// Comma
		case ',':
			if (inquote)
			{
				part += cmd[ptr];
			}
			else
			{
				push_back(part);
				part = "";
			}
			ptr++;
			break;

		// Leading spaces?
		case ' ':
			if (part != "")
			{
				part += cmd[ptr];
			}
			ptr++;
			break;

		// Ordinary character
		default:
			part += cmd[ptr];
			ptr++;
			break;
		}
	}

	// Keep tail end of command
	if (part != "")
	{
		push_back(part);
	}

	return 0;
}

//! \brief dump contained values.
//!
//! Dumps (to cerr) the stored values in this class.
//! This is for debugging purposes.
//! It isn't meant to be pretty.
//!
void cmd_class::dump()
{
	int loop;

	if (size() > 0)
	{
		std::cerr << at(0);
	}

	if (size() > 1)
	{
		std::cerr << " " << at(1);
	}

	for (loop = 2; loop < this->size(); loop++)
	{
		std::cerr << ", " << at(loop);
	}
	std::cerr << std::endl;
}

//******************************************************************
// Implement epl2_class
//******************************************************************

//!
//! \brief Scan an open stream for commands
//!
void epl2_class::process_stream(
	std::istream &in)		//!< Stream to read spl2 commands from
{
	std::string buffer;

	//
	// We need an utput file name.
	// Force it if we have to.
	//
	if (ofile == 0)
	{
		ofile = strdup("/tmp/epl2pdf.pdf");
	}
	//
	// Initialize for a podofo pdf output
	//
	// We have to go through this off business because podofo doesn't
	// want to create a document without the file name. and we cannot
	// do that in the class before we get the output file name.
	//
	PdfStreamedDocument mydocument(ofile);
	document = &mydocument;

	pPage = mydocument.CreatePage(
		PdfPage::CreateStandardPageSize(ePdfPageSize_A4));
	if( !pPage ) 
	{
		PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
	}
	painter.SetPage(pPage);
	pFont = mydocument.CreateFont("Courier Prime");
	if( !pFont )
	{
		PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
	}
	pFont->SetFontSize(10.0);
	painter.SetFont(pFont);

	while (getline(in, buffer))
	{
		process_line(buffer);
	}

	epl2.painter.FinishPage();

	/*
	* Set some additional information on the PDF file.
	*/
	mydocument.GetInfo()->SetCreator(PdfString("espl2pdf"));
	mydocument.GetInfo()->SetAuthor(PdfString("spl2pdf"));
	mydocument.GetInfo()->SetTitle(PdfString("spl2pdf"));
	mydocument.GetInfo()->SetSubject(PdfString("spl2pdf") );
	mydocument.GetInfo()->SetKeywords( PdfString("Test;PDF;spl2pdf;") );

	mydocument.Close();
}

//!\brief Process one EPL2 command
//!
//! Processes on EPL2 command line.
//!
void epl2_class::process_line(
	const std::string &buffer)	//!< Line to process
{
	std::cerr << "Test Line: " << buffer << std::endl;
	cmd_class thiscmd;

	// Split command into parsed blocks
	thiscmd.split_cmd(buffer);

	thiscmd.dump();

	// Check for immediate action commands

	if (thiscmd.size() == 0)
	{
	}
	else if (thiscmd[0] == "A")	// ASCII text
	{
		//
		// P1 Horozintal start position
		// P2 Vertical start position
		// P3 Rotation
		//	0 = normal, 1 = 90, 2 = 180. 2 = 270
		// P4 Font
		//	1=6pt, 2=7pt, 3=10pt, 4=12pt, 5=24pt, 6=9.5pt, 7=9.5pt
		//	A-Z=soft fone, a-z=Soft Font
		//	8,9=Japanese,Chinese
		// P5 Horozental multiplier
		// P6 Vertical multiplier
		// P7 Reverse image
		//	N=Normal, R=reverse
		// Data  Text to be encoded
		//
		if (debug)
		{
			std::cerr << "Ascii Text" << std::endl;
		}
		thiscmd.minsize(9);
		float p1 = cvt_hpostohpos(cvt_tofloat(thiscmd[1]));
		float p2 = cvt_vpostovpos(cvt_tofloat(thiscmd[2]));
		float p5 = std::max(1.0f, cvt_tofloat(thiscmd[5]));
		float p6 = std::max(1.0f, cvt_tofloat(thiscmd[6]));
		std::string p8 = cvt_tostring(thiscmd[8]);

		float fscale = p5 / p6 * 100.0;
		float fsize = 10.0;

		//	1=6pt, 2=7pt, 3=10pt, 4=12pt, 5=24pt, 6=9.5pt, 7=9.5pt
		switch (thiscmd[4][0])
		{
		case '1':
			fsize = 6.0;
			pFont->SetFontSize(fsize * p6);
			pFont->SetFontScale(fscale);
			painter.SetFont(pFont);
			break;
		case '2':
			fsize = 7.0;
			pFont->SetFontSize(fsize * p6);
			pFont->SetFontScale(fscale);
			painter.SetFont(pFont);
			break;
		case '3':
			fsize = 10.0;
			pFont->SetFontSize(fsize * p6);
			pFont->SetFontScale(fscale);
			painter.SetFont(pFont);
			break;
		case '4':
			fsize = 12.0;
			pFont->SetFontSize(fsize * p6);
			pFont->SetFontScale(fscale);
			painter.SetFont(pFont);
			break;
		case '5':
			fsize = 24.0;
			pFont->SetFontSize(fsize * p6);
			pFont->SetFontScale(fscale);
			painter.SetFont(pFont);
			break;
		case '6':
			fsize = 9.5;
			pFont->SetFontSize(fsize * p6);
			pFont->SetFontScale(fscale);
			painter.SetFont(pFont);
			break;
		case '7':
			fsize = 9.5;
			pFont->SetFontSize(fsize * p6);
			pFont->SetFontScale(fscale);
			painter.SetFont(pFont);
			break;
		}

std::cerr << "Text (" <<  p1 << ", " << p2 <<  ")" << std::endl;
		//
		// "p2-fsize": Since the epl2 points at the bottom of the
		// characters to print, when we switch it over to PDF it
		// is now pointing at the top, so we must shift it back
		// down to the bottom.
		//
		painter.DrawText(p1,
			p2 - fsize,
			p8);

	}
	else if (thiscmd[0] == "B")	// Barcode
	{
		//
		// P1 Horozintal start position
		// P2 Vertical start position
		// P3 Rotation
		//	0 = normal, 1 = 90, 2 = 180. 2 = 270
		// P4 Barcode Selection
		// P5 Narrow Bar Width
		// P6 Wide Bar Width
		// P7 Bar code height
		// P8 Human readable code
		//    B = Yes, N = No
		// Data  Text to be encoded
		//
		if (debug)
		{
			std::cerr << "Barcode1" << std::endl;
		}
		thiscmd.minsize(10);
		int bstyle;
		float p1 = cvt_hpostohpos(cvt_tofloat(thiscmd[1]));
		float p2 = cvt_vpostovpos(cvt_tofloat(thiscmd[2]));
		bstyle = cvt_style(thiscmd[4]);
		float p7 = cvt_pttopt(cvt_tofloat(thiscmd[7]));
		std::string p8 = cvt_tostring(thiscmd[8]);

std::cerr << "DrawBarcode: " << p1 << "," << p2 << "," << 0 << "," << p8 << "." << p7 << std::endl;

	pdf_barcode pb(document, &painter);
		//
		// "p2-f7": Since the epl2 points at the bottom of the
		// barcode to print, when we switch it over to PDF it
		// is now pointing at the top, so we must shift it back
		// down to the bottom.
		//
	pb.DrawBarcode(p2 - p7, p1, 0, p8, p7);

if (0)
{
painter.Rectangle(p1,	// Stand in
	p2,
	p7/2,
	-p7/2);
painter.Fill();
std::cerr << "Rectangle: " << p1 << "," << p2 << "," << p7  << std::endl;
}
	}
	else if (thiscmd[0] == "b")	// Barcode
	{
std::cerr << "Barcode2" << std::endl;
	}
	else if (thiscmd[0] == "LE")	// Line draw XOR
	{
std::cerr << "LixeXor" << std::endl;
	}
	else if (thiscmd[0] == "LO")	// Line draw black
	{
		//
		// p1 = Horizotal start position
		// p2 = Vertical start position
		// p3 = horizontal lengt n in dots
		// p4 = vertical length in dots
		//
		if (debug)
		{
			std::cerr << "LineBlack" << std::endl;
		}
		thiscmd.minsize(9);
		float p1 = cvt_hpostohpos(cvt_tofloat(thiscmd[1]));
		float p2 = cvt_vpostovpos(cvt_tofloat(thiscmd[2]));
		float p3 = cvt_pttopt(cvt_tofloat(thiscmd[3]));
		float p4 = cvt_pttopt(cvt_tofloat(thiscmd[4]));

std::cerr << "Box (" <<  p1 << ", " << p2 << ", " << p3 << ", " << p4 << ")" << std::endl;

		painter.Rectangle(p1,
			p2,
			p3,
			-p4);
		painter.Fill();
	}
	else if (thiscmd[0] == "LS")	// Line draw diagnol
	{
std::cerr << "LixeDia" << std::endl;
	}
	else if (thiscmd[0] == "LW")	// Line draw white
	{
std::cerr << "LixeWhite" << std::endl;
	}
	else if (thiscmd[0] == "N")	// New Page
	{
std::cerr << "New Page" << std::endl;
	}
	else if (thiscmd[0] == "P")	// Page
	{
		//
		// P1 = Number of label sets to print
		// P2 = Number of copies of each label set to print
		//
		// Both parameters are currently igbored
		//
std::cerr << "Page" << std::endl;
		//
		// Close current page
		//
		epl2.painter.FinishPage();

		//
		// Create new page
		//
		pPage = document->CreatePage(
			PdfPage::CreateStandardPageSize(ePdfPageSize_A4));
		if( !pPage ) 
		{
			PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
		}
		painter.SetPage(pPage);
	}
	else if (thiscmd[0] == "R")	// Set reference Point
	{
		//
		// P1 Horozintal reference point
		// P2 Vertical reference point
		//
		if (debug)
		{
			std::cerr << "Reference Point" << std::endl;
		}
		thiscmd.minsize(3);
		href = cvt_tofloat(thiscmd[1]);
		vref = cvt_tofloat(thiscmd[2]);
	}
	else if (thiscmd[0] == "X")	// Box draw
	{
std::cerr << "BoxDraw" << std::endl;
	}
	else if (thiscmd[0] == ";")	// Comment
	{
		// Comment. do nothing.
		if (debug)
		{
			std::cerr << "Comment" << std::endl;
		}
	}
	else				// Unhandled command
	{
std::cerr << "** Unparsed command **" << std::endl;
	}
}

//!\brief Convert a string from an EPL2 command line into a parsed string.
//!
//! This converts a raw string given in an EPL2 command line into a parsed
//! string ready to print.
//! It strips off quotes '"'.
//! It handles characters flagged with a '\'.
//!
//! I'm using a very simple method to do this, which may not handdle
//! malformed strings very well.
//!
//!\returnes parsed string.
//!
std::string epl2_class::cvt_tostring(
	std::string &p)			//!< Unparsed string
{
	std::string result;		// String being built

	for (int loop = 0; loop < p.size(); loop++)
	{
		switch (p[loop])
		{
		case '"':
			break;

		case '\\':		// May go bad with malformed strings
			loop++;
			result += p[loop];
			break;

		default:
			result += p[loop];
			break;
		}
	}

	return result;
}
