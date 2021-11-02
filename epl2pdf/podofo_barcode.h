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
#include <podofo/podofo.h>
#include <zint.h>


using namespace PoDoFo;

//!\brief Class to generate barcodes for podofo documents
//!
//! This class is used to generate barcodes in a podofo page.
//! It uses the Xint barcode library to generate the barcodes.
//!
class pdf_barcode
{
private:
	PdfDocument *document;
	PdfPage *pPage;
	PdfPainter *painter;
	PdfFont *pFont;

public:
	pdf_barcode(			//!< Constructor
		PdfDocument *mydocument,	//!< Document pointer
		PdfPage *mypPage)	//!< Page to parcode on
	{
		document = mydocument;
		pPage = mypPage;
		painter->SetPage( pPage );
		pFont = document->CreateFont("Couriur");
		if( !pFont )
		{
			PODOFO_RAISE_ERROR(ePdfError_InvalidHandle);
		}

	};

	int place_barcode(
		struct zint_symbol *symbol ,
		int y,
		int x,
		int barcode_style,
		float size
	);

private:
	int prepare_canvas(
		float width,
		float height,
		float scale);
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

