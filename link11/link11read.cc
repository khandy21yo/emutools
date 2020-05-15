//!\file link11read.cc
//!
//!\brief This module contains the code used to read in object files.
//!
//! The format of the object files is described in the
//! "RT-11 Volume and File Formats Manual", chapter 2.
//!
//!\author Kevin Handy, Mar 2020
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
int ObjectBlock::ReadBlock(std::ifstream &in)
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
	one = byte1 + byte2 << 8;
	if (one != 1)
	{
		return ERROR_BADDATA;
	}

	//
	// Block length
	//
	byte1 = in.get();
	byte2 = in.get();
	length = byte1 + byte2 << 8;

	//
	// Read data block
	//
	block = new char[length - 5];
	in.read(block, length - 5);

	//
	// Read checksum
	//
	checksum = in.get();
	
	return ERROR_OK;
}

//!\brief Dump ObjectBlocck data for debugging purposes
//!
void ObjectBlock::Dump(int detail)
{
	std::cout << "ObjectBlock" << std::endl;
	std::cout << "   One:      " << one << std::endl;
	std::cout << "   Length:   " << length << std::endl;
	std::cout << "   Checksum: " << checksum << std::endl;
}
