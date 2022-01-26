//! \file
//! \brief Library to generate barcodes in PoDoFo using Zint
//!
//! This class is used to produce barcodes in a PoDoFo pdfPage
//! by using the Zint library to create the barcode.
//!
//! \author Kevin Handy, Sep 2021

#include "podofo_barcode.h"
#include <cstring>

extern int debug;
const char *EPL2_FONTNAME = "Courier Prime";

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
	char rotation,		//!< Rotation (0=0, 1=90, 2=180, 3=270)
	char hrcode		//!< Human readable text on barcode
)
{
	if (barcode_style != 0)
	{
		my_symbol->symbology = barcode_style;
	}

//	my_symbol->primary = text.c_str();
//	strncat(my_symbol->primary, text.c_str(), sizeof(my_symbol->primary));
//	my_symbol->option_1 = 3;

	//
	// Human readable text on barcode?
	//
	if (hrcode == 'B')
	{
		if (debug)
		{
			std::cerr << "SHOWHRT = YES" << std::endl;
		}
	}
	else
	{
		if (debug)
		{
			std::cerr << "SHOWHRT = NO" << std::endl;
		}
		my_symbol->show_hrt = 0;		// Hide text
	}

	//
	// Set a save marker,
	// to make it easier to reset all parameters back to the origial
	// state when we exit generating the barcode.
	//
	painter->Save();

	//
	// onfigure transformations.
	// - Rotation
	// - Scaling/size
	// - position
	//
	float a=1, b=0, c=0, d=1, e=x, f=y;	// Transformation matrix

	switch(rotation)
	{
	case '0':	// 0 degrees
		f -= size * 2.0;
		break;
	case '3':	// 90 degrees
		a = 0;
		b = 1;
		c = -1;
		d = 0;
		e += size * 2.0;
		break;
	case '2':	// 180 degrees
		a = -1;
		b = 0;
		c = 0;
		d = -1;
		f += size * 2.0;
		break;
	case '1':	// 270 degrees
		a = 0;
		b = -1;
		c = 1;
		d = 0;
		e -= size * 2.0;
		break;
	};

	//
	// Generate barcode
	//
	int error = ZBarcode_Encode_and_Buffer_Vector(my_symbol,
		(const unsigned char *)text.c_str(), 0, 0);
	if (error)
	{
		std::cerr << "Barcode error: " <<
			error << ":" <<
			my_symbol->errtxt << std::endl;
	}

	if (debug)
	{
		std::cerr << "BarcodeGen: Height=" << my_symbol->height <<
			" WWid=" << my_symbol->whitespace_width <<
			" WHgt=" << my_symbol->whitespace_height <<
			" bWid=" << my_symbol->border_width <<
			" rows=" << my_symbol->rows <<
			" width=" << my_symbol->width <<
			" bheight=" << my_symbol->bitmap_height <<
			std::endl;
	}

	//
	// Adjust barcode size
	//
	float height = my_symbol->height;
	switch(my_symbol->symbology)
	{
	case BARCODE_MAXICODE:	// Doesn't set height correctly. So we guess
		height = 72;
		break;
	}
	if (height != 0)
	{
		float adjust = (size / height);
		a = a * adjust;
		b = b * adjust;
		c = c * adjust;
		d = d * adjust;

		if (debug)
		{
			std::cerr << "Transform: " << a << "," << b << "," <<
				c << "," << d << "," << e << "," << f << std::endl;
		}
		painter->SetTransformationMatrix(a, b, c, d, e, f);

		//
		// Draw barcode
		//
		place_barcode(my_symbol);
	}
	else
	{
		std::cerr << "Barcode failed" << std::endl;
	}

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
	struct zint_symbol* symbol	//!< Aint symbol interfce
)
{
	struct zint_vector_rect *rect;
	rect = symbol->vector->rectangles;
	while (rect)
	{
		if (debug)
		{
			std::cerr << "draw_rect: " <<
				rect->x << "," <<
				rect->y << "," <<
				-rect->height << "," <<
				rect->width << "," << std::endl;
 		}

		draw_rect(
			rect->y,
			rect->x,
			rect->height,
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
		if (debug)
		{
			std::cerr << "draw_string: " <<
				string->x << "," <<
				string->y << "," <<
				string->fsize << "," <<
				string->text << "," <<
				string->length << "," << std::endl;
		}

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
		if (debug)
		{
			std::cerr << "draw_circle: " <<
				circle->x << "," <<
				circle->y << "," <<
				circle->colour << "," <<
				circle->diameter << "," << std::endl;
 		}
		//
		//Switch to white
		//
		if ((circle->colour & 1) == 1)
		{
			painter->SetColor(1.0, 1.0, 1.0);
		}
		draw_circle(
			circle->x,
			circle->y,
			circle->diameter);
		painter->Fill();

		//
		//Switch back to black
		//
		if ((circle->colour & 1) == 1)
		{
			painter->SetColor(0.0, 0.0, 0.0);
		}
		circle = circle->next;
	}

	return 0;
}

//! \brief Draw a rectandle (bar) on the page.
//!
//! Draws a rectangle on the screen.
//
int pdf_barcode::draw_rect(
	float x,
	float y,
	float width,
	float height)
{
	painter->Rectangle(y, x, height, width);
	painter->Fill();

// std::cerr << "Rect: " << y << "," << x << "," << width << "," << height << std::endl;
	return 0;
}

int pdf_barcode::draw_hexagon(
	float x,		//!\FIXME is this position the center, or corner
	float y,
	float diameter)
{
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
	painter->LineTo(cornerx + third * 3.0, cornery + third * 2.0);	// \.
	painter->LineTo(cornerx + third * 3.0, cornery + third);	// |
	painter->LineTo(cornerx + third * 2.0, cornery);		// /
	painter->LineTo(cornerx + third * 1.0, cornery);		// -
	painter->ClosePath();						// \
	painter->Fill();

	return 0;
}

//!\brief Draw characters on the barcode
//!
//! Draws strings (characters) on the page.
//! The x.y coordinates are the bootom left of the character.
//!
int pdf_barcode::draw_string(
	float x,
	float y,
	float fsize,
	unsigned char *text,
	float length)
{
	PdfFont* pFont;
	pFont = document->CreateFont(EPL2_FONTNAME);
	if( !pFont )
	{
		PODOFO_RAISE_ERROR(ePdfError_InvalidHandle);
	}
	pFont->SetFontSize(fsize);
	painter->SetFont(pFont);

	painter->DrawText(
		y,
		x,
		text);

	return 0;
}

//!\brief Draw circles on the page.
//!
//! Draws circles on the screen.
//! The x,y coordinates are the center of the circle..
//!
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

