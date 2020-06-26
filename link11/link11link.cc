//!\file link11link.cc
//! \brief Actual work of linking
//!
//!\author Kevin Handy, Apr 2020
//!

#include "link11.h"


//**********************************************************************
// LinkPsect
//**********************************************************************

//!\brief Dump data structures
//!
//! Debugging routine to display data stored in class
//
void LinkPsect::Dump(
	int Level)	//!< Detail level
{
	std::cout <<
		"  psect module: " <<
		derad504b(module) << "  psect: " <<
		derad504b(name) << "  flag: " <<
		flag << "  length: " <<
		length << "  base: " <<
		base << std::endl;

}

//**********************************************************************
// Link
//**********************************************************************

//!\brief Dump out data in structure
//!
//! Debugging routine to display data stored in the class.
//!
void Link::Dump(
	int level)	//!< Level of detail
{
	std::cout << std::endl << "Link" << std::endl;

	//
	// Last module being munged
	//
	std::cout << " Current module: " <<
		derad504b(currentmodule) <<
		std::endl;

	//
	// Display start address
	//
	std::cout << "  Start" << std::endl;
	start.Dump(1);

	//
	// List all modules in list
	//
	psectlist.Dump(level);

	if (currentpsect)
	{
		std::cout << "  Current psect" << std::endl;
		currentpsect->Dump(level);
	}

	std::cout << "Globals" << std::endl;
	globalvars.Dump(level);
	reloclist.Dump(level);
}

//!\brief Pass100 - initialize psect tables
//!
//! Pass to scan through all blocks of data and to set up LinkPsect
//! structures.
//!!
//!\returns ErrorCode
//!
int Link::Pass100(
	ObjectBlock &block)	//!< One block of code from an object file
{
	switch(block.type)
	{
	case BLOCK_GSD:
		//
		// In this pass, we are only interested in global symbol
		// definitions, and psect definitions.
		//
		for (int loop = 0; loop < block.length - 6; loop += 8)
		{
			int attr = block.block[loop + 4];

//			std::cout <<
//				derad504b(block.block + loop) << "  ";

			switch(block.block[loop + 5])
			{
			case GSD_MODNAM:
//				std::cout << "module ";
				memcpy(currentmodule, block.block, 4);
				break;
			case GSD_CSECT:
//				std::cout << "csect  ";
				Pass100Psect(block.block[loop + 5],
					block.block + loop);
				break;
			case GSD_ISN:
				std::cout << "syngol ";
				break;
			case GSD_TA:
//				std::cout << "tran   ";
//std::cout << std::endl << "tran   " << deword(block.block + loop + 6) << std::endl;
				start.setname(block.block + loop);
				start.offset = deword(block.block + loop + 6);
				start.flags = GSN_DEF | GSN_REL;
				start.psect = currentpsect;
				break;
			case GSD_GSN:
//				std::cout << "global ";
				Pass100Gsn(block.block + loop);
				break;
			case GSD_PSECT:
//				std::cout << "psect  ";
				Pass100Psect(block.block[loop + 5],
					block.block + loop);
				break;
			case GSD_IDENT:
				std::cout << "ident  ";
				break;
			case GSD_VSECT:
//				std::cout << "vsect  ";
				Pass100Psect(block.block[loop + 5],
					block.block + loop);
				break;
			default:
				std::cout << "????   ";
				break;
			}

		}
		break;

	case BLOCK_ENDGSD:
		break;

	case BLOCK_TXT:
		Pass100Txt(block);
		break;

	case BLOCK_RLD:
		Pass100Rld(block);
		break;

	case BLOCK_ENDMOD:
		break;

	case BLOCK_ISD:
	case BLOCK_LIB:
	case BLOCK_ENDLIB:
		// not yet implemented at all
		std::cout << "Pass100: unimplemented type " <<
			block.type << std::endl;
		break;
	}

	return ERROR_OK;
}

//!\brief Handle setting up a psect/csect for Link::Pass100.
//!
//! Creates psect entries in the Link structure.
//!
//!\todo It should merge psects together with common names,
//! but this first implementation just adds all psects as
//! new.
//
int Link::Pass100Psect(
	int type,			//!< type of section (pset. csrct)
	const unsigned char *def)	//!< Pointer to definition in GSD
{
	LinkPsect *psect = 0;	//!< Pointer to this new psect

	psect = &(*psectlist.emplace(psectlist.end()));
	currentpsect = psect;

	memcpy(psect->module, currentmodule, 4);
	memcpy(psect->name, def, 4);
	psect->flag = def[4];
	psect->length = deword(def + 6);
	psect->length += psect->length & 1;	// Force to word boundry size

	if (psect->length != 0)
	{
		//
		// Allocate space for psect.
		// We may need to reallocate later if later psects fold
		// into this one.
		//
		psect->data = new unsigned char[psect->length];
	}

	return 0;
}

//!\brief Pas100 process a txt block
//
int Link::Pass100Txt(
	ObjectBlock &block)	//!< One block of code from an object file
{
	//
	// 1st 2 bytes contain load offset
	//
	unsigned int offset = deword(block.block);

	//
	// The rest of it is object code
	//
	memcpy(currentpsect->data + offset,
		block.block + 2,
		block.length - 8);

	return 0;
}

//!\brief Handle global symbols definitions
//
int Link::Pass100Gsn(
	const unsigned char *def)	//!< Pointer to global symbol definition
{
	int attr = def[4];

	//
	// Is this value already in the table
	//
	auto var = globalvars.Search(def);

	//
	// If it's already in the table,
	// do we keep the original or replace it with this one?
	//
	if (var != 0 &&
		(var->flags & GSN_DEF != 0) &&
		(attr & GSN_DEF!= 0))
	{
		//
		// Thid one is a ref, not a def,
		// and the original is a def,
		// so keep original
		//
		return 0;
	}

	//
	// If var is non-zero, then we want to update with the
	// new enter for better definition.
	// iIf it's zero, we didn't find symbol in table, create new entry
	//
	if (var == 0)
	{
		var = &(*globalvars.emplace(globalvars.end()));
	}

	//
	// Set up/update definition
	//
	var->setname(def);
	var->offset = deword(def + 6);
	var->flags = attr;
	var->psect = currentpsect;

	return 0;
}

//!\brief Table defining the size of RLD relocation records.
//!
//! Only code 01-017(8) are defined in the file formats manual.
//!
const static int rldsize[] =
{
//	0	1	2	3	4	5	6	7
	2,	4,	6,	4,	6,	8,	8,	8,

//	10	11	12	13	14	15	16	17
	4,	2,	6,	4,	6,	8,	8,	4
};

//!\brief Pas100 process a relocation block
//!
//! We handle as may relocations in this pass as we can,
//! and save the rest for the next pass.
//! The reason for delay s because of information that has
//! not yet been established.
//
int Link::Pass100Rld(
	ObjectBlock &object)	//!< One block of code from an object file
{
	Reloc *rld;		//!< Pointer to reloc record

	for (int loop = 0; loop < object.length - 6;)
	{
		unsigned int command = object.block[loop];
		unsigned int displacement = object.block[loop + 1];

		switch (command & 077)
		{
		case 007:
			//
			//	0   |   7
			//	section name
			//	constant
			//
			// We should probably do something here.
			// It shoud be setting currentpsect to the
			// chosen psect, but just letting the GSD
			// point at the last psect mentioned seems
			// to work for now.
			//
			// Also, we probably need to do something with
			// the constant.
			// I think it is an offset into the psect for
			// the relocations because they only have a 256
			// byte range.
			//
			break;

		case 001:
		case 003:
		case 010:
		case 002:
		case 004:
		case 005:
		case 006:
		case 015:
		case 016:
		case 011:
		case 012:
		case 014:
		case 017:
		case 013:
			//
			// We aren't ready to hsndle these yet, because the
			// addresses haven't been finalized.
			// Store them for later use.
			//
			rld = &(*reloclist.emplace(reloclist.end()));
			rld->psect = currentpsect;
			rld->data = new unsigned char[rldsize[command]];
			memcpy(rld->data, object.block + loop,
				rldsize[command]);
			break;

		default:
std::cout << "      Unparsed RLD command " << command <<
	"  displacement = " << displacement <<
       "  size = " << rldsize[command] << std::endl;
			break;
		}

		loop += rldsize[command];
	}

	return 0;
}

//!\brief Pass200 - Determine absolute addresses
//!
//! This pass assignes absolute addresses to psects, local and global
//! variables.
//!
int Link::Pass200(void)
{
	unsigned int base = 01000;	//!< Beginning address for program

	//
	// Assign absolute addresses to psects that need it
	//
	for (auto loop = psectlist.begin();
		loop != psectlist.end();
		loop++)
	{
		//
		// We only need to set up relative, not absolute, psects
		//
		if ((*loop).flag & 040)
		{
			(*loop).base = base;
			base += (*loop).length;
		}
	}

	//
	// Relocate start address
	//
	start.Reloc();

	//
	// Relocate globals
	//
	for (auto loop2 = globalvars.begin();
		loop2 != globalvars.end();
		loop2++)
	{
		(*loop2).Reloc();
	}

	//
	// Relocation pass
	//
	Pass200Rbl();

	return 0;
}

int Link::Pass200Rbl(void)
{
	unsigned char *sym;		//!< Pointer to radix50 symbol name
	Variable *symptr;		//!< Pointer to symbol definition
	unsigned int constant;		//!< Constant value


	for (auto loop = reloclist.begin();
		loop != reloclist.end();
		loop++)
	{
		unsigned int command = (*loop).data[0];
		unsigned int displacement = (*loop).data[1];

		switch (command & 077)
		{
		case 007:
			//
			//Shoudn't occur.
			//Should have been stripped off in Pass100R bl
			//
			break;

		case 002:
			//
			// Global relocation
			//
			sym = (*loop).data + 2;
			symptr = globalvars.Search(sym);
			enword((*loop).data + displacement, symptr->absolute);
			break;

		case 003:
			//
			// Internal Displaced Relocation
			//
			constant = deword((*loop).data + 2);
			enword((*loop).data + displacement,
				constant - ( (*loop).psect->base +
				displacement + 2) );
			break;

		case 004:
			//
			// Global Displaced Relocation
			//
			sym = (*loop).data + 2;
			symptr = globalvars.Search(sym);
			constant = symptr->absolute;
			enword((*loop).data + displacement,
				constant - ( (*loop).psect->base +
				displacement + 2) );
			break;

		//
		// Not yet progeammed in
		//
		case 001:
		case 010:
		case 005:
		case 006:
		case 015:
		case 016:
		case 011:
		case 012:
		case 014:
		case 017:
		case 013:
		default:
std::cout << "      Unparsed RLD command " << command <<
	"  displacement = " << displacement <<
       "  size = " << rldsize[command] << std::endl;
			break;
		}
	}

	return 0;
}

//!\brief Debug dump of reloc info
//
void Reloc::Dump(
	int Level)
{
	std::cout << "  reloc psect " <<
		derad504b(psect->name) << " ";

	unsigned char command = data[0];
	unsigned char displacement = data[1];
	std::cout << "command " << (int)command <<
		"  displacement " << (int)displacement << " ";

	switch (command & 077)
	{
		case 001:
		case 003:
		case 010:
			std::cout << "Constant " <<
				deword(data + 2) <<
				std::endl;
			break;
		case 002:
		case 004:
			std::cout << "Symbol " <<
				derad504b(data + 2) << std::endl;
			break;
		case 005:
		case 006:
			std::cout << "Symbol " <<
				derad504b(data + 2);
			std::cout << " Constant " <<
				deword(data + 6) <<
				std::endl;
			break;
		case 007:
		case 015:
		case 016:
			std::cout << "Section " <<
				derad504b(data + 2);
			std::cout << " Constant " <<
				deword(data + 6) <<
				std::endl;
			break;
		case 011:
			break;
		case 012:
		case 014:
			std::cout << "Section " <<
				derad504b(data + 2) << std::endl;
			break;
		case 017:

		case 013:
		default:
			std::cout << " *Unparsed command" << std::endl;
			break;
	}
}
