//! \file
//! \brief Library to generate barcodes in PoDoFo using Zint
//!
//! This class is used to produce barcodes in a PoDoFo pdfPage
//! by using the Zint library to create the barcode.
//!
//! \author Kevin Handy, Sep 2021

#include "podofo_barcode.h"

//!\brief Draw a barcode on current page
//!
//! This function will draw a specified barcode on the current page
//!
//! Overall function to build and draw a barcode on the currenr painter object.
//!
//! \note currently does not work.
//!
int pdf_barcode::DrawBarcode(
	float y,		//!< y position of barcode
	float x,		//!< x position of barcode
	int barcode_style,	//!< Carcodre style (code39, code128, etc.)
	std::string &text,	//!< Text of barcode
	float size,		//!< sie of brcode
	char rotation		//!< Rotation (0=0, 1=90, 2=180, 3=270)
)
{
	struct zint_symbol *my_symbol;
	my_symbol = ZBarcode_Create();
//	my_symbol->symbology = BARCODE_EXCODE39;
	if (barcode_style != 0)
	{
		my_symbol->symbology = barcode_style;
	}
	my_symbol->height = size;
//	my_symbol->whitespace_height = size;
	my_symbol->show_hrt = 0;		// Hide text

	//
	// Set a save marker,
	// to make it easier to reset all parameters back to the origial
	// state when we exit generating the barcode.
	//
	painter->Save();

	//
	// Set options
	//
	float a=1, b=0, c=0, d=1, e=x, f=y;	// Transformation matrix

	switch(rotation)
	{
// [cos(x) sin(x) -sin(x) cos(x) 0 0]
// 	Rotate image
	case '0':	// 0 degrees
		break;
	case '3':	// 90 degrees
		a = 0;
		b = 1;
		c = -1;
		d = 0;
		break;
	case '2':	// 180 degrees
		a = -1;
		b = 0;
		c = 0;
		d = -1;
		break;
	case '1':	// 270 degrees
		a = 0;
		b = -1;
		c = 1;
		d = 0;
//		e -= size;	// fudge position of rotated barcode.
	       			// Why? I don't know, but it's necessary.
		break;
	};

// std::cerr << "Transform: " << a << "," << b << "," <<
// c << "," << d << "," << e << "," << f << std::endl;
	painter->SetTransformationMatrix(a, b, c, d, e, f);

	// Generate barcode
	ZBarcode_Encode(my_symbol, (const unsigned char *)text.c_str(), 0);
	ZBarcode_Buffer_Vector(my_symbol, /*rotate_angle*/ 0);



	//
	// Draw barcode
	//
	place_barcode(my_symbol, y, x);

	//
	// Finish, clean up, Restore from Save.
	//
	ZBarcode_Delete(my_symbol);
	painter->Restore();

	return 0;
}

//!\brief Last stage of building barcode
//!
//! Draws the rendered barcode onto the current painter object.
//! Requires that the barcode has been rendered previous to being called.
//!
//! This code based on the example in the zint /docs/manual.txt
//! And heavily modified for this application.
//!
int pdf_barcode::place_barcode(
	struct zint_symbol* symbol,	//!< Aint symbol interfce
	int y,				//!< y position (not used.
					//!< Handled using transformationmatrix)
	int x				//!< x position (not used.
					//!< Handled using transformationmatrix)
)
{
	struct zint_vector_rect *rect;
	rect = symbol->vector->rectangles;
	while (rect)
	{
// std::cerr << "draw_rect: " <<
// x + rect->x << "," <<
// y + rect->y << "," <<
// -rect->height << "," <<
// rect->width << "," << std::endl;

		draw_rect(
			rect->y,
			rect->x,
			-rect->height,
			rect->width);
		painter->Fill();
		rect = rect->next;
	}

	struct zint_vector_hexagon *hexagon;
	hexagon = symbol->vector->hexagons;
	while (hexagon)
	{
		draw_hexagon(
			hexagon->x,
			hexagon->y,
			hexagon->diameter);
		painter->Fill();
		hexagon = hexagon->next;
	}

	struct zint_vector_string *string;
	string = symbol->vector->strings;
	while (string)
	{
// std::cerr << "draw_string: " <<
// x + string->x << "," <<
// y + string->y << "," <<
// string->fsize << "," <<
// string->text << "," <<
// string->length << "," << std::endl;

		draw_string(
			string->y,
			string->x,
			string->fsize,
			string->text,
			string->length);
		string = string->next;
	}

	struct zint_vector_circle *circle;
	circle = symbol->vector->circles;
	while (circle)
	{
		draw_circle(
			circle->x,
			circle->y,
			circle->diameter);
		painter->Fill();
		circle = circle->next;
	}

	return 0;
}

int pdf_barcode::draw_rect(
	float x,
	float y,
	float width,
	float height)
{
	painter->Rectangle(y, x, -height, width);
	painter->Fill();

// std::cerr << "Rect: " << y << "," << x << "," << width << "," << height << std::endl;
	return 0;
}

int pdf_barcode::draw_hexagon(
	float x,		//!\FIXME is this position the center, or corner
	float y,
	float diameter)
{
//	pPainter->MoveTo( x, y );
//	pPainter->LineTo( x+dWidth, y-dHeight );
//	pPainter->LineTo( x-dWidth, y-dHeight );
//	pPainter->ClosePath();
//	pPainter->Fill();

	float cornerx = x - diameter / 2.0;
	float cornery = y - diameter / 2.0;
	float third = diameter / 3.0;

	//      1,3  --  2,3
	//          /  \
	//    0.2  |    | 3,2
	//         |    |
	//    0,1   \  /  3,1
	//      1,0  -- 2,0
	//
	painter->MoveTo(cornerx, cornery + third);
	painter->LineTo(cornerx, cornery + third * 2.0);		// |
	painter->LineTo(cornerx + third, cornery + third * 3.0);	// /
	painter->LineTo(cornerx + third * 2.0, cornery + third * 3.0);	// -
	painter->LineTo(cornerx + third * 3.0, cornery + third * 2.0);	// \
	painter->LineTo(cornerx + third * 3.0, cornery + third);	// |
	painter->LineTo(cornerx + third * 2.0, cornery);		// /
	painter->LineTo(cornerx + third * 1.0, cornery);		// -
	painter->ClosePath();						// \
	painter->Fill();

	return 0;
}

int pdf_barcode::draw_string(
	float x,
	float y,
	float fsize,
	unsigned char *text,
	float length)
{
	PdfFont* pFont;
	pFont = document->CreateFont("Courier Prime");
	if( !pFont )
	{
		PODOFO_RAISE_ERROR(ePdfError_InvalidHandle);
	}
	pFont->SetFontSize(fsize);
	painter->SetFont(pFont);

	painter->DrawText(
		x,
		y,
		text);

	return 0;
}

int pdf_barcode::draw_circle(
	float x,
	float y,
	float diameter)
{
	//!\TODO Circle x,y may need adjusting
	painter->Circle( x, y, diameter / 2.0);
	painter->Fill();

	return 0;
}

