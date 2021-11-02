//! \file
//! \brief Library to generate barcodes in PoDoFo using Zint
//!
//! This class is used to produce barcodes in a PoDoFo pdfPage
//! by using the Zint library to create the barcode.
//!
//! \author Kevin Handy, Sep 2021

#include "podofo_barcode.h"

//!
//! This code based on the example in the zint /docs/manual.txt
//! And heavily modified for this application.
//!


int pdf_barcode::place_barcode(
	struct zint_symbol* symbol,
	int y,
	int x,
	int barcode_style,
	float size
)
{
	prepare_canvas(symbol->vector->width,
		symbol->vector->height,
		symbol->scale);

	struct zint_vector_rect *rect;
	rect = symbol->vector->rectangles;
	while (rect)
	{
		draw_rect(
			rect->x,
			rect->y,
			rect->width,
			rect->height);
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
		hexagon = hexagon->next;
	}

	struct zint_vector_string *string;
	string = symbol->vector->strings;
	while (string)
	{
		draw_string(
			string->x,
			string->y,
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
		circle = circle->next;
	}

	return 0;
}

int pdf_barcode::prepare_canvas(
	float width,
	float height,
	float scale)
{
	return 0;
}

int pdf_barcode::draw_rect(
	float x,
	float y,
	float width,
	float height)
{
	painter->Rectangle( x, y, width, height);
	painter->Fill();

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
	pFont->SetFontSize(fsize);

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
