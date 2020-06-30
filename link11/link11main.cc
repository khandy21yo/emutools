//!\file link11main.cc
//!
//!\brief Main driver code for link11
//!
//!\mainpage
//! main driver code for link11.
//! Handles user command line processing,
//! and main driver for the lionking process.
//!
//! Passes:
//! 1. Load object file data.
//! 2. Scan for all program sections and variables.
//! 3. Set absolute addresses for all program sections.
//! 4. Handle relocations.
//! 5. Write our binary code.
//! 
//!\author Kevin Handy, Apr 2020
//

#include "link11.h"

//
// Globals
//
int debug = 0;		//!< Debug printout flag

//!\brief Main
//!
int main(int argc, char **argv)
{
	Link passes;		// Main linker class
	int loop;		// Generic loop variable
	std::list<std::string> objname;	// List of object files
       					// to process
	std::string ldaname;	// Name of .lda output file
	std::string simhname;	// Name of .simh output file

	if (argc <= 1)
	{
		std::cout << "link11 <filenames> <options>" << std::endl;
		std::cout << "   <filenames> is a list of pdp11 rt11 object files" <<
			std::endl;
		std::cout << "<options" << std::endl;
		std::cout << "   -lda <filename>" << std::endl;
		std::cout << "   -simh <filename>" << std::endl;
		std::cout << "   -debug" << std::endl;
		std::cout << std::endl;

		exit(0);
	}

	for (loop = 1; loop < argc; loop++)
	{
		std::string ag = argv[loop];

		if (ag == "-lda")
		{
			ldaname = argv[loop + 1];
			loop++;
		}
		else if (ag == "-simh")
		{
			simhname = argv[loop + 1];
			loop++;
		}
		else if (ag == "-debug")
		{
			debug = 1;
		}
		else
		{
			objname.push_back(ag);
		}
	}

	for (auto loopo = objname.begin(); loopo != objname.end(); loopo++)
	{
		ObjectFile of;
		of.ReadFile(*loopo);
		of.Dump(debug);

		passes.Pass100(of);
	}

	passes.Pass200();

	if (ldaname != "")
	{
		passes.WriteAbs(ldaname);
	}
	if (simhname != "")
	{
		passes.WriteSimh(simhname);
	}


	passes.Dump(debug);
}

