//! \file
//! \brief Library to generate barcodes in PoDoFo using Zint
//!
//! This class is used to produce barcodes in a PoDoFo pdfPage
//! by using the Zint library to create the barcode.
//!
//! \author Kevin Handy, Sep 2021

#ifndef __podofo_barcode__
#define __podofo_barcode__

//
// Include files
//
#include <iostream>
#include <podofo/podofo.h>
#include <zint.h>

//! \brief Name of the font to use as the EPL2 font.
extern const char *EPL2_FONTNAME;

using namespace PoDoFo;

//!\brief Class to generate barcodes for podofo documents
//!
//! This class is used to generate barcodes in a podofo page.
//! It uses the Xint barcode library to generate the barcodes.
//!
class pdf_barcode
{
private:
	PdfDocument *document;		//<! podofo document reference
	PdfPainter *painter;		//!< podofo painter reference

public:
	struct zint_symbol *my_symbol;	//!< Symbol being built

public:
	pdf_barcode(				//!< Constructor
		PdfDocument *mydocument,	//!< Document pointer
		PdfPainter *mypainter)		//!< Painter to use
	{
		document = mydocument;
		painter = mypainter;
		my_symbol = ZBarcode_Create();
	};

	int DrawBarcode(
		float y,
		float x,
		int barcode_style,
		std::string &text,
		float size,
		char rotation,
		char hrcode,
		float tdadjust = 0.0
	);

	int place_barcode(
		struct zint_symbol *symbol
	);

private:
	int draw_rect(
		float x,
		float y,
		float width,
		float height);
	int draw_hexagon(
		float x,
		float y,
		float diameter);
	int draw_string(
		float x,
		float y,
		float fsize,
		unsigned char *text,
		float length);
	int draw_circle(
		float x,
		float y,
		float diameter);
};

#endif

