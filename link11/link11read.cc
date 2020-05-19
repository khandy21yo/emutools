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
	byte1 = in.get();
	byte2 = in.get();
//std::cout << "Testget " << (int)byte1 << "," << (int)byte2 << std::endl;
	one = byte1 + (byte2 << 8);
	if (one != 1)
	{
		return ERROR_BADDATA;
	}

	//
	// Block length
	//
	byte1 = in.get();
	byte2 = in.get();
//std::cout << "Testget " << (int)byte1 << "," << (int)byte2 << std::endl;
	length = byte1 + (byte2 << 8);

	//
	// Type of block
	//
	type = in.get();
	byte2 = in.get();
	if (byte2 != 0)
	{
		std::cout << "Errer, byte2 = " << byte2 << std::endl;
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
	checksum = in.get();
	
	return ERROR_OK;
}

//!\brief Dump ObjectBlocck data for debugging purposes
//!
void ObjectBlock::Dump(
	int detail)	//!< Level of detail to display
{
	std::cout << "ObjectBlock" << std::endl;
	std::cout << "   One:      " << one << std::endl;
	std::cout << "   Length:   " << length << std::endl;
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
		DumpGSD(detail);
		break;

	}
	std::cout << "   Checksum: " << checksum << std::endl;
}

//!\brief Dump GSD data
//
void ObjectBlock::DumpGSD(
	int detail)		//!< Level of detail to display
{
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
			derad50(block[loop + 0]  + (block[loop + 1] << 8)) <<
			derad50(block[loop + 2]  + (block[loop + 3] << 8)) <<
			"  ";

		int attr = block[loop + 4];

		switch(block[loop + 5])
		{
		case GSD_PSECT:
			if (attr & 001)
				std::cout <<  "shr ";
			else
				std::cout <<  "prv ";
			if (attr & 002)
				std::cout <<  "ins ";
			else if (attr & 004)
				std::cout <<  "bss ";
			else
				std::cout <<  "dat ";
			if (attr & 020)
				std::cout <<  "ovr ";
			else
				std::cout <<  "cat ";
			if (attr & 040)
				std::cout <<  "rel ";
			else
				std::cout <<  "abs ";
			if (attr & 0100)
				std::cout <<  "gbl";
			else
				std::cout <<  "loc";
	
			break;

		case GSD_GSN:
			if (attr & 001)
				std::cout <<  "wek ";
			else
				std::cout <<  "str ";
			if (attr & 010)
				std::cout <<  "def ";
			else
				std::cout <<  "ref ";
			if (attr & 040)
				std::cout << "rel ";
			else
				std::cout << "abs ";
			break;
		}
		std::cout << " " << block[loop + 6] + (block[loop + 7] << 8);
		std::cout << std::endl;
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
//		std::cout << "Open failed" << std::endl;
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
	int detail)	//!< Level of detail to display
{
	std::cout << "ObjectFile: " << file << std::endl;

	for (auto loop = begin(); loop != end(); loop++)
	{
		(*loop).Dump(detail);
	}
}

