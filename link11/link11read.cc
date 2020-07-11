//!\file link11read.cc
//!
//!\brief This module contains the code used to read in object files.
//!
//! The format of the object files is described in the
//! "RT-11 Volume and File Formats Manual", chapter 2.
//!
//!\author Kevin Handy, Apr 2020
//!

#include <iostream>
#include <fstream>

#include "link11.h"

//**********************************************************************
// ObjectBlock
//**********************************************************************

//!\brief Read one block from an object file
//!
//! Pulls one block of data from an RT11 object file.
//! It does not try to interpret the data,
//! it just reads in the raw blocks.
//!
//!\returns ErrorCode
//
//!\todo Handle RSX format object files.
//
int ObjectBlock::ReadBlock(
	std::ifstream &in)	//!< Stream to read object file from
{
	unsigned char byte1;	// 1st character read
	unsigned char byte2;	// 2nd character read

	if (in.eof())
	{
		return ERROR_EOF;
	}

	//
	// read in the start flag.
	// This should always be 1.
	//
	if (rt11)
	{
		//
		// rt11 has a 0,1 leadin to eacj record.
		// rsx does not have this.
		//
		byte1 = in.get();
		byte2 = in.get();
//std::cout << "Testget " << (int)byte1 << "," << (int)byte2 << std::endl;
		one = deword(byte1, byte2);
		if (one != 1)
		{
			return ERROR_BADDATA;
		}
	}

	//
	// Block length
	//
	byte1 = in.get();
	byte2 = in.get();
// std::cout << "Testget " << (int)byte1 << "," << (int)byte2 << std::endl;
	length = deword(byte1, byte2);
	if (rt11 == false)
	{
		//
		// rsx doesn't count the length as part of the length,
		// and it also lacks the 0,1 at the start of a record.
		//
		length += 4;
	}

	//
	// Type of block
	//
	type = in.get();
	byte2 = in.get();
// std::cout << "Type " << (int)type << "," << (int)byte2 << std::endl;
	if (byte2 != 0)
	{
		std::cout << "Errer, type2 = " << (int)byte2 << std::endl;
		exit(0);
	}

	//
	// Read data block
	//
	block = new unsigned char[length - 6];
	for (int loop = 0; loop < (length - 6); loop++)
	{
		block[loop] = in.get();
	}

	//
	// Read checksum
	//
	if (rt11)
	{
		//
		// rt11 ends with a checksum.
		//
		checksum = in.get();
	}
	else
	{
		//
		// rsx may need padding to an even length
		//
		if (length & 1)
		{
			checksum = in.get();
		}
	}

	return ERROR_OK;
}

//!\brief Dump ObjectBlocck data for debugging purposes
//!
void ObjectBlock::Dump(
	int level)	//!< Level of detail to display
{
	if (level == 0)
	{
		return;
	}

	std::cout << "ObjectBlock" << std::endl;
	std::cout << "   One:      " << one << std::endl;
	std::cout << "   Length:   " <<
	       std::oct << std::showbase <<
       	       length << std::endl;
	std::cout << "   Type:     " << type << std::endl;
	std::cout << "   Data:     ";
	int count = 0;
	for (int loop = 0; loop < length - 6; loop++)
	{
		if (count == 16)
		{
			count = 0;
			std::cout << std::endl << "             ";
		}
		std::cout << (int) block[loop] << " ";
		count++;
	}
	std::cout << std::endl;

	switch(type)
	{
	case BLOCK_GSD:
		DumpGSD(level);
		break;
	case BLOCK_ENDGSD:
		std::cout << "      GSD END" << std::endl;
		break;
	case BLOCK_TXT:
		std::cout << "      TXT Load Address " <<
			deword(block) << std::endl;
		break;
	case BLOCK_RLD:
		DumpRLD(level);
		break;

	}
	std::cout << "   Checksum: " << checksum << std::endl;
}

//!\brief Dump GSD data
//
void ObjectBlock::DumpGSD(
	int level)		//!< Level of detail to display
{
	if (level == 0)
	{
		return;
	}

	for (int loop = 0; loop < length - 6; loop += 8)
	{
		std::cout << "      GSD ";
		switch(block[loop + 5])
		{
		case GSD_MODNAM:
			std::cout << "module ";
			break;
		case GSD_CSECT:
			std::cout << "csect  ";
			break;
		case GSD_ISN:
			std::cout << "syngol ";
			break;
		case GSD_TA:
			std::cout << "tran   ";
			break;
		case GSD_GSN:
			std::cout << "global ";
			break;
		case GSD_PSECT:
			std::cout << "psect  ";
			break;
		case GSD_IDENT:
			std::cout << "ident  ";
			break;
		case GSD_VSECT:
			std::cout << "vsect  ";
			break;
		default:
			std::cout << "????   ";
			break;
		}
		std::cout <<
			derad504b(block + loop) << "  ";

		int attr = block[loop + 4];

		switch(block[loop + 5])
		{
		case GSD_PSECT:
			std::cout << symbol_attr(attr);
			break;

		case GSD_GSN:
			std::cout << psect_attr(attr);
			break;
		}
		std::cout << " " << deword(block + loop + 6);
		std::cout << std::endl;
	}
}

//!\brief Dump RLD data
//
void ObjectBlock::DumpRLD(
	int level)		//!< Level of detail to display
{
	if (level == 0)
	{
		return;
	}

	for (int loop = 0; loop < length - 6;)
	{
		unsigned char command = block[loop++];
		unsigned char displacement = block[loop++];
		std::cout << "      RLD command " << (int)command <<
			"  displacement = " <<
			std::oct << std::showbase <<
			(int)displacement << std::endl;

		switch (command & 077)
		{
			case 001:
			case 003:
			case 010:
				std::cout << "      Constant " <<
					std::oct << std::showbase <<
					deword(block + loop) <<
					std::endl;
				loop += 2;
				break;
			case 002:
			case 004:
				std::cout << "      Symbol " <<
					derad504b(block +loop) << std::endl;
				loop += 4;
				break;
			case 005:
			case 006:
				std::cout << "      Symbol " <<
					derad504b(block +loop) << std::endl;
				loop += 4;
				std::cout << "      Constant " <<
					std::oct << std::showbase <<
					deword(block + loop) <<
					std::endl;
				loop += 2;
				break;
			case 007:
			case 015:
			case 016:
				std::cout << "      Section " <<
					derad504b(block +loop) << std::endl;
				loop += 4;
				std::cout << "      Constant " <<
					std::oct << std::showbase <<
					deword(block + loop) <<
					std::endl;
				loop += 2;
				break;
			case 011:
				break;
			case 012:
			case 014:
				std::cout << "      Section " <<
					derad504b(block +loop) << std::endl;
				loop += 4;
				break;
			case 017:

			case 013:
			default:
				std::cout << "      *Unparsed command" << std::endl;
				break;
		}
	}

}

//**********************************************************************
// ObjectFile
//**********************************************************************

//!\brief Load ObjectFile from an RT-11 obcevt file given its name
//!
//!\returns ErrorCode
//!
int ObjectFile::ReadFile(
	const std::string &filename)	//!< Name of file to process
{
	file = filename;

	//
	// Try to open file
	//
	std::ifstream fin(filename.c_str(), std::ios_base::binary);
	if (fin)
	{
//		std::cout << "Open ok" << std::endl;
	}
	else
	{
		std::cout << "Unable to read " <<
			filename << std::endl;
		return ERROR_EOF;
	}

	//
	// Loop through all blocks of object file
	//
	ObjectBlock *working;
	do
	{
		working = &(*emplace(end()));
		working->ReadBlock(fin);
	}
	while (working->type != BLOCK_ENDMOD);

	return ERROR_OK;
}

//!\brief Dump out an entire ObjectFile
//
void ObjectFile::Dump(
	int level)	//!< Level of detail to display
{
	if (level == 0)
	{
		return;
	}

	std::cout << "ObjectFile: " << file << std::endl;

	for (auto loop = begin(); loop != end(); loop++)
	{
		(*loop).Dump(level);
	}
}

