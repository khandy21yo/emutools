//!\file link11read.cc
//
//!\brief This module contains the code used to read in object files.
// 
// The format of the object files is described in the
// "RT-11 Volume and File Formats Manual", chapter 2.
//
//!\author Kevin Handy, Mar 2020
//

#include <fstream>

//!\brief enum defining error code
//
enum ErrorCodes
{
	ERROR_OK = 0,		//!< No error
	ERROR_EOF = 1,		//!< End of file reached
	ERROR_BADDATA		//!< Bad data read from object file
};


//! \brief Class to handle blocks of data from an object file
//
// Manage the reading of blocks of data from an object file.
//
class ObjectBlock
{
public:
	int one;		// Two bytes, should be 1 if read is Ok
	int length;		// Block length.
	std::string block;	// 8-bit byte block containing data
	int checksum;		// Checksum.

public:
	int ReadBlock(std::ifstream &in);
};


//!\brief Read one block from an object file
//
// Pulls one block of data from an RT11 object file.
// It does not try to interpret the data,
// it just reads in the raw blocks.
//
int ObjectBlock::ReadBlock(std::ifstream &in)
{
	char byte1;	// 1st character read
	char byte2;	// 2nd character read

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

	byte1 = in.get();
	byte2 = in.get();
	length = byte1 + byte2 << 8;

	return ERROR_OK;
}
