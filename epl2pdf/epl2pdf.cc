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
int debug = 0;		//!< Are debugging messages enabled?

//!
//! \brief Container for parsing parsing epl2 commands.
//!
//! this class is used to parse out the individual parts of an
//! epl2 command.
//! Given a line containing a single epl2 command line, 
//! it splits the command between the command line, and the individual
//! comma seperated options.
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

	//!\brief Get cmd itemi as a std::string if available, else blank.
	//!
	//! Used to retrieve individual options without having to check
	//! that the option actually exists.
	//! This helps avoid the program from crashing.
	//!
	const std::string get(
		int item)	//!< Item number to try for
	{
		if (item < size())
		{
			return at(item);
		}
		else
		{
			return "";
		}
	}
	void dump();
};

//!
//! \brief Class to handle the EPL2 command parsing.
//!
//! Does the work of converting the EPL2 commands into PDF
//! commands.
//! This the the main class in this program.
//!
class epl2_class
{
public:
	float dpi;	//!< dots per inch (epl)
	char *ofile;	//!< File to process
	float href;	//!< Horizontal Rederence poinnt (epl)
	float vref;	//< Vertical Reference point (epl)
	float fadj;	//!< Adjustment for woth of characters to get
			//!< closer to EPL2 hight/width.

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
	std::vector<std::string> history;
					//!< Instruction history for this page.
	std::map<std::string,
		std::vector<std::string>>
		forms;			//!< Storage for all forms
	int inform;			//!< Are we in a form?
	std::string informname;		//!< While inform, name of current form
	int lock_history;		//!< Lock history during reuse to avoid
					//!< endless loop
	int lineno;			//!< Command line number.
					//!< Used by maybefirst.
	PdfRect pagesize;		//!< Page size to use

public:
	//! \brief constructor
	//!
	//! Initializes the class for use.
	//!
	epl2_class()
	{
		dpi = 200.0;		// Dots per inch
		document = 0;		// Not yet created
		ofile = 0;		// Not yet parsed
		href = 0.0;		// Reference point
		vref = 0.0;		// Reference point
		pFont = 0;		// Font
		lock_history = 0;
		lineno = 0;
		inform = 0;
		fadj = 0.93;

		// Shouldn't there be an easier way?
		pagesize.SetBottom(PdfPage::CreateStandardPageSize(
			ePdfPageSize_Letter).GetBottom());
		pagesize.SetLeft(PdfPage::CreateStandardPageSize(
			ePdfPageSize_Letter).GetLeft());
		pagesize.SetWidth(PdfPage::CreateStandardPageSize(
			ePdfPageSize_Letter).GetWidth());
		pagesize.SetHeight(PdfPage::CreateStandardPageSize(
			ePdfPageSize_Letter).GetHeight());
		if (debug)
		{
			std::cerr << "L" << pagesize.GetLeft() <<
				" B" << pagesize.GetBottom() <<
				" W" << pagesize.GetWidth() <<
				" H" << pagesize.GetHeight() << std::endl;
		}
	}
	int init_stream();
	void process_stream(std::istream &in);
	int finish_stream();
	void process_line(const std::string &buffer);

	//!\brief process a list of commands
	//!
	//! When given a list of EPL2 commands, 
	//! executes the entire list.
	//!
	//! This is used to redo a page started without a 'N' command,
	//! process forms, handle 'P' with multi-copies listed, etc.
	//!
	inline void process_list(
		const std::vector<std::string> &buffer)	//!< List conyaining EPL2 commands
	{
		for (auto item = buffer.begin(); item != buffer.end(); item++)
		{
			process_line(*item);
		}
	}
	//! \brief add command line to instruction history
	//!
	//! Stores all printing commands into 'history' for reuse if we need
	//! to reprint the page information.
	//! This history is used on pages that don't start with a 'N',
	//! and for 'P' commands that ask for multiple copies.
	//!
	void push_history(std::string command)
	{
		//
		// lock_history gets set to avoid adding to the historry when
		// we are processing a previously stored list.
		//
		if (lock_history == 0)
		{
			history.push_back(command);
		}
	}

	//!\brief clear history buffer.
	//!
	//! Clear the stored history.
	//! Usually because a 'N' (clear page' command is issued.
	//!
	void clear_history()
	{
		history.clear();
	}

	//!\brief Handle things necessary when a new page starts.
	//!
	//! Handle when a new page is being started.
	//! After a 'P' command, you can issue another 'P' command to reprint
	//! the page by leaving out the 'N' to clear the page buffer.
	//!
	void maybe_first()
	{
		if (lineno == 0)
		{
			//
			// Initialize for a podofo pdf output
			//
			pPage = document->CreatePage(pagesize);
			if( !pPage ) 
			{
				PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
			}
			painter.SetPage(pPage);

			lineno++;
			lock_history++;
			process_list(history);
			lock_history--;
		}
		else
		{
			lineno++;
		}
	}

public:
	//!\brief Convert a std::string value to a float
	//!
	//! If the string is blank, returns a default value,
	//! otherwise it converts the string to a float value.
	//! If the string contans non-numeric characters,
	//! they will halt the conversion without causing an error.
	//!
	inline float cvt_tofloat(
		const std::string &p,		//!< String to convert
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

	//!\brief Convert a std::string value to a integer
	//!
	//! If the string is blank, returns a default value,
	//! otherwise it converts the string to a float value.
	//! If the string contans non-numeric characters,
	//! they will halt the conversion without causing an error.
	//!
	inline float cvt_toint(
		const std::string &p,		//!< String to convert
		int def = 0)		//!< Default value
	{
		if (p == "")
		{
			return def;
		}
		else
		{
			return std::stoi(p);
		}
	}
	std::string cvt_tostring(
		const std::string &p);

	//!\brief Convert from EPl2 point measurement to PDF measurements
	//!
	//! EPL2 measures things in pixelss, normally at 203 pixelss to the inch,
	//! while PDF measures things in points, at 72 points to the inch.
	//! This converts from pixels to points.
	//
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
	//! The EPL2 printer, in addition using a different measuring system,
	//! also defines the (0,0) point differently.
	//! EPL2 puts the (0,0) point at the top left of the label,
	//! while PDF puts (0,0) at the bottom left of the label.
	//! This converts the EPL2 point to the corresponding PDF onei
	//! takinging the reference point into account.
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
	//! EPL2 and PDF agree that the horizontal for (0,0) is to the left
	//! of the label.
	//! This handles converting, and handling the referece point.
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
	int cvt_style(
		std::string p,
		pdf_barcode &pb);
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
//! \brief We only need one implementation of the epl2_class.
//!
//! This program emulates a single EPL  printer, This is it.
//!
static epl2_class epl2;


//!
//! \brief Main driver)
//!
//! Handles parsing the command line, and getting things started.
//!
int main(int argc, const char **argv)
{
	int c;			// popt command letter
	const char *filename;	// File to process
	float tmpwidth = 0.0;
	float tmpheight = 0.0;

	//
	// Handle command line arguements
	//
	poptContext optCon;

	const struct poptOption optionsTable[] =
	{
		{ "dpi", 'p', POPT_ARG_INT, &epl2.dpi, 0,
			 "dots per inch", "pixels" },
		{ "output", 'o', POPT_ARG_STRING, &epl2.ofile, 0,
			 "output file name", "filename" },
		{ "width", 'w', POPT_ARG_FLOAT, &tmpwidth, 0,
			 "Width of form", "points (1/72 inch)" },
		{ "length", 'l', POPT_ARG_FLOAT, &tmpheight, 0,
			 "Length of form", "points (1/72 inch)" },
		{ "output", 'f', POPT_ARG_STRING, &EPL2_FONTNAME, 0,
			 "font to use", "font name" },
		{ "fadjust", 'a', POPT_ARG_FLOAT, &epl2.fadj, 0,
			 "Adjustment for font width", "% adjustment"},
		{ "debug", 'd', POPT_ARG_NONE, 0, 'd',
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

	if (tmpwidth != 0.0)
	{
		epl2.pagesize.SetWidth(tmpwidth);
	}
	if (tmpheight != 0.0)
	{
		epl2.pagesize.SetHeight(tmpheight);
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

	epl2.init_stream();

	//
	// Process all file names passed on command ilne
	//
	while (filename = poptGetArg(optCon))
	{
		if (debug)
		{
			std::cerr << "File: " << filename << std::endl;
		}

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

	epl2.finish_stream();

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
//! \brief Initialize stream
//
int  epl2_class::init_stream()
{
	//
	// We need an utput file name.
	// Force it if we have to.
	//
	if (ofile == 0)
	{
		ofile = strdup("/tmp/epl2pdf.pdf");
	}
	pFont = document->CreateFont(EPL2_FONTNAME);
	if( !pFont )
	{
		PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
	}

	return 0;
}

//!
//! \brief Scan an open stream for commands
//!
//! This is the main loop for processing a file.
//! It reads in all lines, and processes the commands,
//!
void epl2_class::process_stream(
	std::istream &in)		//!< Stream to read spl2 commands from
{
	std::string buffer;


	while (getline(in, buffer))
	{
		process_line(buffer);
	}
}

//! \brief Finish stream
//!
//! Handles processes necessary to finish off the PDF file, and closes it.\
//!
int epl2_class::finish_stream()
{
	//
	// We have to flush out the last (blank) page,
	// because podofo requires it.
	// Can we work around this?
	//
	if (pPage != 0)
	{
		epl2.painter.FinishPage();
	}

	/*
	* Set some additional information on the PDF file.
	*/
	epl2.document->GetInfo()->SetCreator(PdfString("espl2pdf"));
	epl2.document->GetInfo()->SetAuthor(PdfString("spl2pdf"));
	epl2.document->GetInfo()->SetTitle(PdfString("spl2pdf"));
	epl2.document->GetInfo()->SetSubject(PdfString("spl2pdf") );
	epl2.document->GetInfo()->SetKeywords( PdfString("Test;PDF;spl2pdf;") );

	epl2.document->Close();

	return 0;
}

//!\brief Process one EPL2 command
//!
//! This is the main function to process EPL2 commands.
//! Processes one EPL2 command line.
//!
void epl2_class::process_line(
	const std::string &buffer)	//!< Line to process
{
	if (debug)
	{
		std::cerr << "Test Line: " << buffer << std::endl;
	}
	cmd_class thiscmd;

	// Split command into parsed blocks
	thiscmd.split_cmd(buffer);
	thiscmd.minsize(1);

	if (debug)
	{
		thiscmd.dump();
	}

	if (inform)
	{
		if (thiscmd[0] == "FE")
		{
			inform--;
		}
		else
		{
			if (debug)
			{
				std::cerr << "Inform: " << buffer << std::endl;
			}
			forms[informname].push_back(buffer);
		}
	}
	// Check for immediate action commands

	else if (thiscmd[0] == "")	// No command
	{
	}
	else if (thiscmd[0] == "A")	// ASCII text
	{
		maybe_first();
		push_history(buffer);
		//
		// P1 Horozintal start position
		// P2 Vertical start position
		// P3 Rotation
		//	0 = normal, 1 = 90, 2 = 180. 3 = 270
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
		float p1 = cvt_hpostohpos(cvt_tofloat(thiscmd.get(1)));
		float p2 = cvt_vpostovpos(cvt_tofloat(thiscmd.get(2)));
		char p3 = thiscmd[3][0];
		float p5 = std::max(1.0f, cvt_tofloat(thiscmd.get(5)));
		float p6 = std::max(1.0f, cvt_tofloat(thiscmd.get(6)));
		std::string p7 = cvt_tostring(thiscmd.get(7));
		std::string p8 = cvt_tostring(thiscmd.get(8));

		float fsize = 10.0;

		//	1=6pt, 2=7pt, 3=10pt, 4=12pt, 5=24pt, 6=9.5pt, 7=9.5pt
		switch (thiscmd[4][0])
		{
		case '1':
			fsize = 6.0;
			pFont->SetFontSize(fsize);
			painter.SetFont(pFont);
			break;
		case '2':
			fsize = 7.0;
			pFont->SetFontSize(fsize);
			painter.SetFont(pFont);
			break;
		case '3':
			fsize = 9.0;
			pFont->SetFontSize(fsize);
			painter.SetFont(pFont);
			break;
		case '4':
			fsize = 12.0;
			pFont->SetFontSize(fsize);
			painter.SetFont(pFont);
			break;
		case '5':
			fsize = 24.0;
			pFont->SetFontSize(fsize);
			painter.SetFont(pFont);
			break;
		case '6':
			fsize = 9.5;
			pFont->SetFontSize(fsize);
			painter.SetFont(pFont);
			break;
		case '7':
			fsize = 9.5;
			pFont->SetFontSize(fsize);
			painter.SetFont(pFont);
			break;
		}

		if (debug)
		{
			std::cerr << "Text (" <<  p1 << ", " << p2 <<
			       	")" << std::endl;
		}

//		float asize =
//			fsize +
//			painter.GetFont()->GetFontMetrics()->GetDescent();
//		float asize =
//			fsize +
//			painter.GetFont()->GetFontMetrics()->GetAscent();
//		float asize =
//			-painter.GetFont()->GetFontMetrics()->GetDescent();
//		float asize =
//			fsize * p5;
		float asize =
			painter.GetFont()->GetFontMetrics()->GetAscent() * p5;

		//
		// "p2-fsize": Since the epl2 points at the bottom of the
		// characters to print, when we switch it over to PDF it
		// is now pointing at the top, so we must shift it back
		// down to the bottom.
		//
		float a=1, b=0, c=0, d=1, e=p1, f=p2;	// Transformationmatrix
		switch(p3)
		{
		case '0':	// 0 degrees
			a = p5 * fadj;
			d = p6;
			f -= asize;	// fudge position, shift from top to bottom.
			break;
		case '1':	// 270 degrees
			a = 0;
			b = -p5 * fadj;
			c = p6;
			d = 0;
			e -= asize;	// fudge position. Shift from top to bottom.
			break;
		case '2':	// 180 degrees
			a = -p6;
			b = 0;
			c = 0;
			d = -p5 * fadj;
			break;
		case '3':	// 90 degrees
			a = 0;
			b = -p5 * fadj;
			c = p6;
			d = 0;
			break;
		};
		if (debug)
		{
			std::cerr << "Transform: " << a << "," << b << "," <<
				c << "," << d << "," << e << "," << f << std::endl;
		}

		painter.Save();
		float vdrop = 0.0;	// How far do we need to drop the black box
					// to make the white text look centered.
		if (p7 == "B")
		{
			vdrop = fsize / 4.0;
		}

		painter.SetTransformationMatrix(a, b, c, d, e, f - vdrop);
		if (p7 == "B")
		{
			float th = fsize;

			float tw =		// text width
				painter.GetFont()->GetFontMetrics()->StringWidth(p8);

			painter.Rectangle(0,
				0,
				tw,
				th + vdrop);
			painter.Fill();
			painter.SetColor(1.0, 1.0, 1.0);
		}

		painter.DrawText(0.0,
			0.0 + vdrop,
			p8);
		painter.Restore();

	}
	else if (thiscmd[0] == "B")	// Barcode
	{
		maybe_first();
		push_history(buffer);
		//
		// P1 Horozintal start position
		// P2 Vertical start position
		// P3 Rotation
		//	0 = normal, 1 = 90, 2 = 180. 3 = 270
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
		char p3 = thiscmd[3][0];
		float p7 = cvt_pttopt(cvt_tofloat(thiscmd[7])) / 2.0;
		std::string p8 = cvt_tostring(thiscmd[8]);
		std::string p9 = cvt_tostring(thiscmd[9]);

		if (debug)
		{
			std::cerr << "DrawBarcode: " << p1 << "," <<
				p2 << "," << 0 << "," << p9 << "." << p7 << std::endl;
		}

		pdf_barcode pb(document, &painter);

		bstyle = cvt_style(thiscmd[4], pb);

		//
		// Generate and place the barcode
		//
		pb.DrawBarcode(p2, p1, 0, p9, p7, p3, p8[0]);

	}
	else if (thiscmd[0] == "b")	// Second Barcode
	{
		maybe_first();
		push_history(buffer);
		//
		//	0 = normal, 1 = 90, 2 = 180. 3 = 270
		// P1 Horozintal start position
		// P2 Vertical start position
		// P3 Barcode selection
		// P4 - 
		// P5  Text to be encoded
		//
		if (debug)
		{
			std::cerr << "Barcode2" << std::endl;
		}
//		thiscmd.minsize(17);	// we can't use this here
		int bstyle;
		float p1 = cvt_hpostohpos(cvt_tofloat(thiscmd[1]));
		float p2 = cvt_vpostovpos(cvt_tofloat(thiscmd[2]));
		std::string p = thiscmd[3];
		//
		// The barcode data to encode is in the last field.
		// It can vlost in the 'b' style barcodes.
		//
		std::string text = cvt_tostring(thiscmd[thiscmd.size() - 1]);
		float height = 72;	// Desired height

		if (debug)
		{
			std::cerr << "DrawBarcode: " << p1 << "," <<
				p2 << "," << 0 << "," << text << std::endl;
		}

		pdf_barcode pb(document, &painter);

		// was in cvt_sty;eb, but the settings are weird enough
		// that we must have access to all the variables to correctly
		// parse the parameters.
		//
		if (p == "A")		// Aztec
		{
			pb.my_symbol->symbology = BARCODE_AZTEC;
		}
		else if (p == "AZ")		// Aztec Mesa
		{
			pb.my_symbol->symbology = BARCODE_AZTEC;
		}
		else if (p == "D")		// Data Matrix
		{
			pb.my_symbol->symbology = BARCODE_DATAMATRIX;
		}
		else if (p == "M")		// MaxiCode
		{
			pb.my_symbol->symbology = BARCODE_MAXICODE;
			height = 72;
		}
		else if (p == "P")		// pdf417
		{
			pb.my_symbol->symbology = BARCODE_PDF417;
		}
		else if (p == "Q")		// qrcode
		{
			pb.my_symbol->symbology = BARCODE_QRCODE;
		}

		//
		// Generate and place the barcode
		//
//		pb.DrawBarcode(p2 - height, p1, 0, text, height, '0', 0);
		pb.DrawBarcode(p2 + height, p1, 0, text, height, '0', 0);

	}
	else if (thiscmd[0] == "D")	// Density
	{
		//
		// What should we do with this density setting?
		// We currently just do B/W.
		// Should we greyscale?
		//
		thiscmd.minsize(2);
		float p1 = cvt_hpostohpos(cvt_tofloat(thiscmd[1]));
	}
	else if (thiscmd[0] == "FE")	// End Form
	{
		//
		// This command should not get executed, unless there is
		// a missing "FS" in the source.
		//
		if (inform > 0)
		{
			inform--;
		}
	}
	else if (thiscmd[0] == "FI")	// List Forms
	{
		//!
		//! \note FI: This sould output as a form, not to the
		//! console.
		//!
		std::cout << "Form Information" << std::endl;
		std::cout << forms.size() << std::endl;

		for (auto fn = forms.begin(); fn != forms.end(); fn++)
		{
			std::cout << fn->first << std::endl;

//			if (debug)
			{
				for (auto fl = fn->second.begin();
					fl != fn->second.end();
					fl++)
				{
					std::cout << "..." << *fl << std::endl;
				}
			}
		}
		std::cout << std::endl;
	}
	else if (thiscmd[0] == "FK")	// Clear Form
	{
		// p1 = Form name
		thiscmd.minsize(10);
		std::string p1 = cvt_tostring(thiscmd[1]);

		forms.erase(p1);
	}
	else if (thiscmd[0] == "FR")	// Retrieve Form
	{
		maybe_first();

		// p1 = Form name
		thiscmd.minsize(10);
		std::string p1 = cvt_tostring(thiscmd[1]);
		process_list(forms[p1]);
	}
	else if (thiscmd[0] == "FS")	// Store Form
	{
		// p1 = Form name
		thiscmd.minsize(10);
		std::string p1 = cvt_tostring(thiscmd[1]);

		std::vector<std::string> emptya;
		forms.erase(p1);
		forms.insert(std::make_pair(p1, emptya));
		inform++;
		informname = p1;
	}
	else if (thiscmd[0] == "LE")	// Line draw XOR
					// Don't know how to do xor in pdf,
					// so it duplicates black bar.
	{
		maybe_first();
		push_history(buffer);
		//
		// p1 = Horizotal start position
		// p2 = Vertical start position
		// p3 = horizontal lengt n in dots
		// p4 = vertical length in dots
		//
		if (debug)
		{
			std::cerr << "LineXor" << std::endl;
		}
		thiscmd.minsize(5);
		float p1 = cvt_hpostohpos(cvt_tofloat(thiscmd[1]));
		float p2 = cvt_vpostovpos(cvt_tofloat(thiscmd[2]));
		float p3 = cvt_pttopt(cvt_tofloat(thiscmd[3]));
		float p4 = cvt_pttopt(cvt_tofloat(thiscmd[4]));

		if (debug)
		{
			std::cerr << "Box (" <<  p1 << ", " << p2 << ", " <<
			p3 << ", " << p4 << ")" << std::endl;
		}

		painter.Rectangle(p1,
			p2,
			p3,
			-p4);
		painter.Fill();
	}
	else if (thiscmd[0] == "LO")	// Line draw black
	{
		maybe_first();
		push_history(buffer);
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
		thiscmd.minsize(5);
		float p1 = cvt_hpostohpos(cvt_tofloat(thiscmd[1]));
		float p2 = cvt_vpostovpos(cvt_tofloat(thiscmd[2]));
		float p3 = cvt_pttopt(cvt_tofloat(thiscmd[3]));
		float p4 = cvt_pttopt(cvt_tofloat(thiscmd[4]));

		if (debug)
		{
			std::cerr << "Box (" <<  p1 << ", " << p2 << ", " << p3 <<
			", " << p4 << ")" << std::endl;
		}

		painter.Rectangle(p1,
			p2,
			p3,
			-p4);
		painter.Fill();
	}
	else if (thiscmd[0] == "LS")	// Line draw diagnol
	{
		maybe_first();
		push_history(buffer);
		//
		// p1 = Horizotal start position
		// p2 = Vertical start position
		// p3 = horizontal lengt n in dots
		// p4 = vertical length in dots
		// p5 = vertical length in dots
		//
		if (debug)
		{
			std::cerr << "Diagonal" << std::endl;
		}
		thiscmd.minsize(5);
		float p1 = cvt_hpostohpos(cvt_tofloat(thiscmd[1]));
		float p2 = cvt_vpostovpos(cvt_tofloat(thiscmd[2]));
		float p3 = cvt_pttopt(cvt_tofloat(thiscmd[3]));
		float p4 = cvt_pttopt(cvt_tofloat(thiscmd[4]));
		float p5 = cvt_pttopt(cvt_tofloat(thiscmd[5]));

		if (debug)
		{
			std::cerr << "Line (" <<  p1 << ", " << p2 << ", " << p3 <<
			", " << p4 << ", " << p5 << ")" << std::endl;
		}

		painter.MoveTo(p1, p2);
		painter.LineTo(p1 + p3, p2);
		painter.LineTo(p1 + p3 + p4, p2 - p5);
		painter.LineTo(p1 + p4, p2 - p5);
		painter.LineTo(p1, p2);
		painter.ClosePath();
		painter.Fill();
	}
	else if (thiscmd[0] == "LW")	// Line draw white
	{
		maybe_first();
		push_history(buffer);
		//
		// p1 = Horizotal start position
		// p2 = Vertical start position
		// p3 = horizontal lengt n in dots
		// p4 = vertical length in dots
		//
		if (debug)
		{
			std::cerr << "LineWhite" << std::endl;
		}
		thiscmd.minsize(5);
		float p1 = cvt_hpostohpos(cvt_tofloat(thiscmd[1]));
		float p2 = cvt_vpostovpos(cvt_tofloat(thiscmd[2]));
		float p3 = cvt_pttopt(cvt_tofloat(thiscmd[3]));
		float p4 = cvt_pttopt(cvt_tofloat(thiscmd[4]));

		if (debug)
		{
			std::cerr << "Box (" <<  p1 << ", " << p2 << ", " << p3 <<
			", " << p4 << ")" << std::endl;
		}

		painter.Save();				// Save current settings
		painter.SetColor(1.0, 1.0, 1.0);	// White
		painter.Rectangle(p1,
			p2,
			p3,
			-p4);
		painter.Fill();
		painter.Restore();			// Restore settings
	}
	else if (thiscmd[0] == "N")	// New Page
	{
		clear_history();

		// So reprint will start at proper place
		//
		history.push_back(
			"R " +
			std::to_string(href) +
			"," +
			std::to_string(vref));
	}
	else if (thiscmd[0] == "P")	// Page
	{
		maybe_first();
		//
		// P1 = Number of label sets to print
		// P2 = Number of copies of each label set to print
		//
		// Both parameters are currently igbored
		//
		thiscmd.minsize(3);
		int p1 = cvt_toint(thiscmd.get(1));
		//
		// Close current page
		//
		epl2.painter.FinishPage();

		//
		// Handle additional pages
		//
		for (int loop = 2; loop <= p1; loop++)
		{
			//
			// Create new page
			//
			pPage = document->CreatePage(pagesize);
			if( !pPage ) 
			{
				PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
			}
			painter.SetPage(pPage);

			//
			// Repaint data
			//
			lock_history++;
			process_list(history);
			lock_history--;

			//
			// Close current page
			//
			epl2.painter.FinishPage();

		}
		lineno = 0;
		pPage = 0;
	}
	else if (thiscmd[0] == "q")	// Set label width
	{
		push_history(buffer);

		thiscmd.minsize(2);
		float p1 = cvt_pttopt(cvt_tofloat(thiscmd[1]));

		pagesize.SetWidth(p1);

		if (debug)
		{
			std::cerr << "q Width = " << p1 << std::endl;
			std::cerr << "L" << pagesize.GetLeft() <<
				" B" << pagesize.GetBottom() <<
				" W" << pagesize.GetWidth() <<
				" H" << pagesize.GetHeight() << std::endl;
		}

	}
	else if (thiscmd[0] == "Q")	// Set form length
	{
		push_history(buffer);

		thiscmd.minsize(4);
		float p1 = cvt_pttopt(cvt_tofloat(thiscmd[1]));
		float p2 = cvt_pttopt(cvt_tofloat(thiscmd[2]));
		float p3 = cvt_pttopt(cvt_tofloat(thiscmd[3]));

		//
		// Yes, the SetWidth sets the height
		//
		pagesize.SetHeight(p1);
		if (debug)
		{
			std::cerr << "Q height = " << p1 << std::endl;
			std::cerr << "L" << pagesize.GetLeft() <<
				" B" << pagesize.GetBottom() <<
				" W" << pagesize.GetWidth() <<
				" H" << pagesize.GetHeight() << std::endl;
		}
	}
	else if (thiscmd[0] == "R")	// Set reference Point
	{
//		maybe_first();
		push_history(buffer);
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
		maybe_first();
		push_history(buffer);
		//
		// p1 = Horizotal start position
		// p2 = Vertical start position
		// p3 = Line thickness
		// p4 = Horizontal length in dots
		// p5 = vertical length in dots
		//
		if (debug)
		{
			std::cerr << "BoxDraw" << std::endl;
		}
		thiscmd.minsize(6);
		float p1 = cvt_hpostohpos(cvt_tofloat(thiscmd[1]));
		float p2 = cvt_vpostovpos(cvt_tofloat(thiscmd[2]));
		float p3 = cvt_pttopt(cvt_tofloat(thiscmd[3]));
		float p4 = cvt_hpostohpos(cvt_tofloat(thiscmd[4]));
		float p5 = cvt_vpostovpos(cvt_tofloat(thiscmd[5]));

		//
		// normalize corners
		//
		if (p1 > p4)
		{
			float tmp = p1;
			p1 = p4;
			p4 = tmp;
		}
		if (p2 < p5)
		{
			float tmp = p2;
			p2 = p5;
			p5 = tmp;
		}

		if (debug)
		{
			std::cerr << "box (" <<  p1 << ", " << p2 << ", " << p3 <<
			", " << p4 << ", " << p5 << ")" << std::endl;
		}

		//
		// We have to do this using 4 bars.
		// Using two boxes would cover up anything underneath.
		//
		painter.MoveTo(p1, p2);			// Top
		painter.LineTo(p4, p2);
		painter.LineTo(p4, p2 - p3);
		painter.LineTo(p1, p2 - p3);
		painter.LineTo(p1, p2);
		painter.ClosePath();
		painter.Fill();

		painter.MoveTo(p1, p5);			// Bottom
		painter.LineTo(p4, p5);
		painter.LineTo(p4, p5 + p3);
		painter.LineTo(p1, p5 + p3);
		painter.LineTo(p1, p5);
		painter.ClosePath();
		painter.Fill();

		painter.MoveTo(p1, p2);			// Left
		painter.LineTo(p1, p5);
		painter.LineTo(p1 + p3, p5);
		painter.LineTo(p1 + p3, p2);
		painter.LineTo(p1, p2);
		painter.ClosePath();
		painter.Fill();

		painter.MoveTo(p4, p2);			// Right
		painter.LineTo(p4, p5);
		painter.LineTo(p4 - p3, p5);
		painter.LineTo(p4 - p3, p2);
		painter.LineTo(p4, p2);
		painter.ClosePath();
		painter.Fill();

	}
	else if (thiscmd[0] == "ZB")	// Flip entire page 180 degrees
	{
		// Not handled yet
		push_history(buffer);
	}
	else if (thiscmd[0] == "ZT")	// Flip entire page 0 degree cancel ZB
	{
		// Not handled yet
		push_history(buffer);
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
std::cerr << "** Unparsed command: " << buffer << " ***" << std::endl;
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
	const std::string &p)			//!< Unparsed string
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

//!\brief Convert from EPl2 Barcode Style to ZINT style`
//!
//! EPL2 uses a text string to define which barcode to use.
//! Zint uses an integer constant to define the barcode to use.
//! This attempts to convert from EPL2 strings to Zint numbers,
//! and to do any special operations required for the specific barcode.
//!
//! This operates on the "B" style barcodes.
//!
int epl2_class::cvt_style(
	std::string p,		//!< EPL2 symbol value to be converted
	pdf_barcode &pb)	//!< Symbol being created
{
	int result = 0.0;
	result = pb.my_symbol->symbology = BARCODE_EXCODE39;	// default

	if (p == "" || p == "3")	// Code 39
	{
		result = pb.my_symbol->symbology = BARCODE_EXCODE39;
	}
	else if (p == "3C")		// Code 39 with check digit
	{				// This isn't implemented yet
		result = pb.my_symbol->symbology = BARCODE_EXCODE39;
	}
	else if (p == "9")		// Code 39 with check digit
	{				// This isn't implemented yet
		result = pb.my_symbol->symbology = BARCODE_CODE93;
	}
	else if (p == "0")		// Code 128 special shipping
	{				//  container
					// This isn't zint compatable

		result = pb.my_symbol->symbology = BARCODE_CODE128;
	}
	else if (p == "1")		// Code 128 audo A,B,C
	{
		result = pb.my_symbol->symbology = BARCODE_CODE128;
	}
	else if (p == "1A")		// Code 128  A
	{				// Not Zint Compatable
		result = pb.my_symbol->symbology = BARCODE_CODE128;
	}
	else if (p == "1B")		// Code 128 B
	{
		result = pb.my_symbol->symbology = BARCODE_CODE128B;
	}
	else if (p == "1C")		// Code 128 C
	{				// Not Zint Compatable
		result = pb.my_symbol->symbology = BARCODE_CODE128;
	}
	else if (p == "K")		// Codabar
	{
		result = pb.my_symbol->symbology = BARCODE_CODABAR;
	}
	else if (p == "E80")		// EAN8
	{
		result = pb.my_symbol->symbology = BARCODE_EANX;
	}
	else if (p == "E82")		// EAN8 2 digit add-on
	{
		result = pb.my_symbol->symbology = BARCODE_EANX;
	}
	else if (p == "E85")		// EAN8 5 gdigit add-on
	{
		result = pb.my_symbol->symbology = BARCODE_EANX;
	}
	else if (p == "E30")		// EAN13
	{
		result = pb.my_symbol->symbology = BARCODE_EANX;
	}
	else if (p == "E32")		// EAN13 2 digit add-on
	{
		result = pb.my_symbol->symbology = BARCODE_EANX;
	}
	else if (p == "E35")		// EAN813 5 gdigit add-on
	{
		result = pb.my_symbol->symbology = BARCODE_EANX;
	}
	else if (p == "2G")		// German post code
	{
		result = pb.my_symbol->symbology = BARCODE_DPLEIT;
	}
	else if (p == "2")		// Interleaved 2-of-5
	{
		result = pb.my_symbol->symbology = BARCODE_C25INTER;
	}
	else if (p == "2C")		// Interleaved 2-of-5
	{
		result = pb.my_symbol->symbology = BARCODE_C25INTER;
	}
	else if (p == "2D")		// Interleaved 2-of-5
	{
		result = pb.my_symbol->symbology = BARCODE_C25INTER;
	}
	else if (p == "P")		// Postnet
	{
		result = pb.my_symbol->symbology = BARCODE_POSTNET;
	}
	else if (p == "PL")		// Planet 11 & 13 digit
	{
		result = pb.my_symbol->symbology = BARCODE_PLANET;
	}
	else if (p == "J")		// Japenese Postnet
	{
		result = pb.my_symbol->symbology = BARCODE_JAPANPOST;
	}
	else if (p == "1E ")		// UCC/EAN 128
	{
		result = pb.my_symbol->symbology = BARCODE_EAN128;
	}
	else if (p == "UA0 ")		// UPC A
	{
		result = pb.my_symbol->symbology = BARCODE_UPCA;
	}
	else if (p == "UA2 ")		// UPC A + check digits
	{
		result = pb.my_symbol->symbology = BARCODE_UPCA_CHK;
	}
	else if (p == "UA5 ")		// UPC A + 5 digit add-on
	{
		result = pb.my_symbol->symbology = BARCODE_UPCA;
	}
	else if (p == "UE0 ")		// UPC E
	{
		result = pb.my_symbol->symbology = BARCODE_UPCE;
	}
	else if (p == "UE2 ")		// UPC E + check digits
	{
		result = pb.my_symbol->symbology = BARCODE_UPCE_CHK;
	}
	else if (p == "UE5 ")		// UPC E + 5 digit add-on
	{
		result = pb.my_symbol->symbology = BARCODE_UPCE;
	}
	else if (p == "2U")		// UPC Interleaved 2-of-5
	{
		result = pb.my_symbol->symbology = BARCODE_C25INTER;
	}
	else if (p == "P")		// MSI Plessy
	{
		result = pb.my_symbol->symbology = BARCODE_MSI_PLESSEY;
	}
	else if (p == "M")		// MSI Plessy
	{
		result = pb.my_symbol->symbology = BARCODE_MSI_PLESSEY;
	}
	else if (p == "R14")		// RSSS 14
	{
		result = pb.my_symbol->symbology = BARCODE_RSS14;
	}
	else if (p == "RL")		// RSSS 14 Limited
	{
		result = pb.my_symbol->symbology = BARCODE_DBAR_LTD;
//		result = pb.my_symbol->symbology = BARCODE_RSS14;
	}
	else if (p == "RS")		// RSSS 14 Stacked
	{
//		result = pb.my_symbol->symbology = BARCODE_DBAR_EXP;
		result = pb.my_symbol->symbology = BARCODE_RSS14;
	}
	else if (p == "RT")		// RSSS 14 Truncated
	{
//		result = pb.my_symbol->symbology = BARCODE_RSS14;
		result = pb.my_symbol->symbology = BARCODE_RSS14;
	}

	return result;
}


