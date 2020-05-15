//!\file link11.h
//!
//!\brief This module contains the code used to read in object files.
//!
//! The format of the object files is described in the
//! "RT-11 Volume and File Formats Manual", chapter 2.
//!
//!\author Kevin Handy, Mar 2020
//!
#ifndef _link11_h_
#define _link11_h_

#include <iostream>
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
//!
//! Manage the reading of blocks of data from an object file.
//!
class ObjectBlock
{
public:
	int one;		//!< Two bytes, should be 1 if read is Ok
	int length;		//!< Block length.
	char* block;		//!< 8-bit byte block containing data
	int checksum;		//!< Checksum.

public:
	//!\brief Constructor
	//
	inline ObjectBlock()
	{
		one = 0;
		length = 0;
		block = 0;
		checksum = 0;
	}
	int ReadBlock(std::ifstream &in);
	void Dump(int detail);
};

#endif
