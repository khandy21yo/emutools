//!\file link11.h
//!
//!\brief This module contains the code used to read in object files.
//!
//! The format of the object files is described in the
//! "RT-11 Volume and File Formats Manual", chapter 2.
//!
//!\author Kevin Handy, Apr 2020
//!
#ifndef _link11_h_
#define _link11_h_

#include <iostream>
#include <fstream>
#include <list>
#include <cstring>

//!\brief enum for block types
//!
enum BlockTypes
{
	BLOCK_GSD = 1,		//!< GSD Global Symbol Directory
	BLOCK_ENDGSD = 2,	//!< End of GSD
	BLOCK_TXT = 3,		//!< Holds the actual binary of the program
	BLOCK_RLD = 4,		//!< RLD Relocation Directory
	BLOCK_ISD = 5,		//< ISD Internal Symbol Directory (not used by RT11)
	BLOCK_ENDMOD = 6,	//!< ENDMOD end of object module
	BLOCK_LIB = 7,		//!< LIB start of library
	BLOCK_ENDLIB = 8	//!< ENDLIB end of library
};

//!\brief enum describibg GSD types
//!
enum GSDTypes
{
	GSD_MODNAM = 0,		//!< Module Name
	GSD_CSECT = 1,		//!< c-sect
	GSD_ISN = 2,		//!< ISN Internal symbol name
	GSD_TA = 3,		//!< Transfer address
	GSD_GSN = 4,		//!< Global symbol name
	GSD_PSECT = 5,		//!< p-sect (prograam section)
	GSD_IDENT = 6,		//!< Program version Identification
	GSD_VSECT = 7		//!< vsect (Mapped array declaration)
};

//!\brief enum defining error code
//
enum ErrorCode
{
	ERROR_OK = 0,		//!< No error
	ERROR_EOF = 1,		//!< End of file reached
	ERROR_BADDATA		//!< Bad data read from object file
};


//**********************************************************************
// link11read.cc
//**********************************************************************

//! \brief Class to handle blocks of data from an object file
//!
//! Manage the reading of blocks of data from an object file.
//!
class ObjectBlock
{
public:
	int one;		//!< Two bytes, should be 1 if read is Ok
	int length;		//!< Block length.
	int type;		//!< Type of block
	unsigned char* block;	//!< 8-bit byte block containing data
	int checksum;		//!< Checksum.

public:
	//!\brief Constructor
	//
	inline ObjectBlock()
	{
		one = 0;
		length = 0;
		type = 0;
		block = 0;
		checksum = 0;
	}
	//!\brief destructor
	//
	inline ~ObjectBlock()
	{
		if (block)
		{
			delete[] block;
		}
	}
	int ReadBlock(std::ifstream &in);
	void Dump(int detail);
	void DumpGSD(int detail);
	void DumpRLD(int detail);
};


//!\brief class to hold an entire object file
//!
//! Stores all of the blocks read from one object file.
//!
class ObjectFile : public std::list<ObjectBlock>
{
public:
	std::string file;		//!< Name of file

public:
	int ReadFile(const std::string &filename);
	void Dump(int detail);
};

//**********************************************************************
// link11link.cc
//**********************************************************************
//
class LinkPsect;	// Forwars reference.
			// Defined later, but we need to be ablle to
			// point at it before it is defined. (recursive)


//!\brief Variables
//!
//! Contains definition of a single variable
//!
class Variable
{
public:
	unsigned char name[4];		//!< Name of var in radix50
	LinkPsect *psect;		//!< psect the var belongs to
	int offset;			//!< offset address.
					//! Base address is in *psect.
	int absolute;			//!< Absolute address.

};

class VariableList : public std::list<Variable>
{
public:
};

//
//!\brief program section class
//!
class LinkPsect
{
public:
	unsigned char module[4];	//!< Name of module in radix50
	unsigned char name[4];		//!< Name of psect in radix50
	unsigned int flag;		//!< psect flags
	unsigned int length;		//!< Sise of data allocated
	unsigned int base;		//!< Base address.
	unsigned char *data;		//!< Code for this psect
	VariableList localvars;		//!< Local variables

public:
	//! \brief Constructor
	//
	LinkPsect()
	{
		flag = 0;
		length = 0;
		base = 0;
		data = 0;
	}
	//!\brief Destructor
	~LinkPsect()
	{
		if (data)
		{
			delete[] data;
		}
	}
	void Dump(int level);
};

//!\brief list of program sections
//!
class LinkPsectList : public std::list<LinkPsect>
{
public:
	void Dump(int level)
	{
		for (auto loop = begin(); loop != end(); loop++)
		{
			(*loop).Dump(level);
		}
	}
};


//!\brief Link class
//!
//! Does the heavy lifting of the link process
//!
class Link
{
public:
	LinkPsectList psectlist;	//!< All Program sections
	VariableList globalvars;	//!< Global variables

	LinkPsect *currentpsect;	//!< Current psect being worked on
	unsigned char currentmodule[4];	//!< Current module being worked on

public:
	//!/brief Constructor
	Link()
	{
		currentpsect = 0;
		memset(currentmodule, 0, 6);
	}
	void Dump(int level);
	int Pass100(ObjectBlock &block);

	//!\brief Process all blocks in one file for Pass100.
	//!
	//!\returns an ErrorCode
	//!
	inline int Pass100(
		ObjectFile &block)	//!< File to be processed
	{
		int error = ERROR_OK;
		for (auto loop = block.begin();
			loop != block.end() && error == ERROR_OK;
			loop++)
		{
			error = Pass100(*loop);
		}
		return error;
	}
	int Pass100Psect(int type, const unsigned char *def);
	int Pass100Txt(ObjectBlock &block);
};

//**********************************************************************
// link11util.cc
//**********************************************************************

std::string derad50(int x);

//!\brief convert 4 bytes to 6 radix50 characters.
//!
//! Common occurrence, so make it easier to code.
//!
//!\returns a 6 character string.
//
inline std::string derad504(
	unsigned char a,	//!< 1st vyte
	unsigned char b,	//!< 2nd byte
	unsigned char c,	//!< 3rd byte
	unsigned char d)	//!< 4th byte
{
	return derad50(a + (b << 8)) + derad50(c + (d << 8));
}

//!\brief convert 4 bytes to 6 radix50 characters.
//!
//! Common occurrence, so make it easier to code.
//!
//!]returns a 6 character string.
//
inline std::string derad504b(
	const unsigned char* a)	//!< Pointer to 4 bytes of radix50 characters
{
	return derad504(a[0], a[1], a[2], a[3]);
}

//!\brief Read one word (two bytes) from the object file
//!
//! Reads one word of data (2 bytes) from an object wil,
//! and handles the endianess of the data.
//!
//!\returns one word (16 bytes) integer.
//!
inline unsigned int deword(
	const unsigned char *a)		//!< Pointer to two bytes of data
{
	return a[0] + (a[1] << 8);
}

//!\brief Read one word (two bytes) from the object file
//!
//! Reads one word of data (2 bytes) from an object wil,
//! and handles the endianess of the data.
//!
//!\returns one word (16 bytes) integer.
//!
inline unsigned int deword(
	const unsigned char a,		//!< 1st byte
	const unsigned char b)		//!< 2nd byte
{
	return a + (b << 8);
}

#endif
