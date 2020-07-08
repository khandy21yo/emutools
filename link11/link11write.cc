//!\file link11write.cc
//! \brief Writing out of final data
//!
//!\author Kevin Handy, Jub 2020
//!


#include "link11.h"

#include <iomanip>
#include <ctime>

//!\brief Write out binary data in ccc format
//!
//! This writes the binary image in absolute format.
//!
//! Absolute format consists of blocks of data in the following format
//!
//!	1
//!	0
//!	BCL	Low order mits of byte count
//!	BCH	High orfder bits of byte count
//!	ADL	Low order bits of absolute load address
//!	ADH	High order bits of absolute load assress
//!	DATA	Data bytes (length bytes
//!	CHK	Shecksum byte
//!
int Link::WriteAbs(const std::string &filename)
{

	std::ofstream fout(filename.c_str(), std::ios_base::binary);
	if (fout)
	{
//		std::cout << "Open ok" << std::endl;
	}
	else
	{
//		std::cout << "Open failed" << std::endl;
		return ERROR_EOF;
	}

	//
	// Dump out all psects
	//
	for (auto loop = psectlist.begin();
		loop != psectlist.end();
		loop++)
	{
		//
		// We don't need to write it out if there is no data
		//
		if ((*loop).length != 0)
		{
			checksum = 0;
			putbyte(fout, 1);
			putbyte(fout, 0);
			putword(fout, (*loop).length + 6);
			putword(fout, (*loop).base);
			putbytes(fout, (*loop).data, (*loop).length);
			putchecksum(fout);
		}
	}

	//
	// Final block.
	// Start address.
	//
	checksum = 0;
	putbyte(fout, 1);
	putbyte(fout, 0);
	putword(fout, 6);
	putword(fout, start.absolute);		// Start address
	putchecksum(fout);

	return 0;
}

//!\brief Write out binary data in simh script format
//!
//! This writes the binary image in simh DO format.
//! Use the simh command 'DO <filename>" to load.
//!
int Link::WriteSimh(const std::string &filename)
{
	std::ofstream fout(filename.c_str());
	if (fout)
	{
//		std::cout << "Open ok" << std::endl;
	}
	else
	{
//		std::cout << "Open failed" << std::endl;
		return ERROR_EOF;
	}

	fout <<
		"#!/usr/bin/pdp11" << std::endl <<
		"; " << filename << " simh script to load binary" <<
		std::endl;

	fout <<
		"; Load in simh/pdp11 using 'do " << filename << "'" <<
		std::endl;

	//
	// Dump out all psects
	//
	for (auto loop = psectlist.begin();
		loop != psectlist.end();
		loop++)
	{
		//
		// We don't need to write it out if there is no data
		//
		if ((*loop).length != 0)
		{
			//
			// Add a note to the script to show what is being loaded
			//
			fout <<
				std::endl <<
				"; psect " <<
				derad504b((*loop).module) << " " <<
				derad504b((*loop).name) << " " <<
				std::oct << std::setw(6) << std::setfill('0') <<
				(*loop).base << " " <<
				std::oct << std::setw(6) << std::setfill('0') <<
				(*loop).length << " " <<
				psect_attr((*loop).flag) <<
				std::endl;

			for (int loop2 = 0; loop2 < (*loop).length; loop2 += 2)
			{
				fout << "d " <<
					std::oct << std::showbase <<
					(*loop).base + loop2 << " " <<
					std::oct << std::showbase <<
					deword((*loop).data + loop2) <<
					std::endl;
			}
		}
	}

	//
	// Final block.
	// Start address.
	//
	fout << std::endl <<
	       "; Set start address" << std::endl;
	fout << "d pc " <<
		start.absolute <<
		std::endl;		// Start address

	return 0;
}

//!\brief Write out mapping information
//!
//! This writes  a /map type of listing file.
//!
int Link::WriteMap(const std::string &filename)
{
	std::ofstream fout(filename.c_str());
	if (fout)
	{
//		std::cout << "Open ok" << std::endl;
	}
	else
	{
//		std::cout << "Open failed" << std::endl;
		return ERROR_EOF;
	}

	char outstr[200];
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL)
	{
		outstr[0] = 0;
	}
	else
	{
		if (strftime(outstr, sizeof(outstr), "%D %T", tmp) == 0)
		{
			outstr[0] = 0;
		}
	}

	//
	// File title
	//
	fout <<
		filename <<
		"    link11 map " <<
		outstr <<
		std::endl;

	//
	// Transfer address
	//
	fout <<
		std::endl <<
		"Transfer Address " <<
		std::oct << std::setw(6) << std::setfill('0') <<
			start.absolute <<
		std::endl;		// Start address

	//
	// Dump out all psects
	//
	fout <<
		std::endl <<
		"    Module Psect   Start Length Attributes" << std::endl;

	for (auto loop = psectlist.begin();
		loop != psectlist.end();
		loop++)
	{
		fout <<
			"    " <<
			derad504b((*loop).module) << " " <<
			derad504b((*loop).name) << " " <<
			std::oct << std::setw(6) << std::setfill('0') <<
			(*loop).base << " " <<
			std::oct << std::setw(6) << std::setfill('0') <<
			(*loop).length << " " <<
			psect_attr((*loop).flag) <<
			std::endl;
	}

	//
	// Global symbols
	//
	fout << std::endl <<
		"    Symbol Value " <<
		"    Symbol Value " <<
		"    Symbol Value " <<
		"    Symbol Value " <<
		std::endl;
	int vpos = 0;
	for (auto loopv = globalvars.begin();
		loopv != globalvars.end();
		loopv++)
	{
		if (vpos == 4)
		{
			vpos = 0;
			fout << std::endl;
		}
		fout <<
			"    " <<
			derad504b((*loopv).name) << " " <<
			std::oct << std::setw(6) << std::setfill('0') <<
			(*loopv).absolute;
	}
	if (vpos)
	{
		fout << std::endl;
	}

	return 0;
}

