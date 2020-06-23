//!\file link11write.cc
//! \brief Writing out of final data
//!
//!\author Kevin Handy, Jub 2020
//!


#include "link11.h"

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
	fout << "d pc " <<
		start.absolute <<
		std::endl;		// Start address

	return 0;
}
