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
	std::string mapname;	// Name of map file

	//
	//Usage message
	//
	if (argc <= 1)
	{
		std::cout << "link11 <filenames> <options>" << std::endl;
		std::cout << "   <filenames> is a list of pdp11 rt11 object files" <<
			std::endl;
		std::cout << "<options" << std::endl;
		std::cout << "   -lda <filename>" << std::endl;
		std::cout << "   -simh <filename>" << std::endl;
		std::cout << "   -map  <filename>" << std::endl;
		std::cout << "   -debug" << std::endl;
		std::cout << std::endl;

		exit(0);
	}

	//
	// Parse command line options
	//
	for (loop = 1; loop < argc; loop++)
	{
		std::string ag = argv[loop];

		if (ag == "-lda")
		{
			if (ldaname != "")
			{
				std::cerr << "-lda specified more than once" <<
					std::endl;
				exit(1);
			}
			ldaname = argv[loop + 1];
			loop++;
		}
		else if (ag == "-simh")
		{
			if (simhname != "")
			{
				std::cerr << "-simh specified more than once" <<
					std::endl;
				exit(1);
			}
			simhname = argv[loop + 1];
			loop++;
		}
		else if (ag == "-map")
		{
			if (mapname != "")
			{
				std::cerr << "-map specified more than once" <<
					std::endl;
				exit(1);
			}
			mapname = argv[loop + 1];
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

	if (objname.size() == 0)
	{
		std::cerr << "No object files specified" << std::endl;
		exit(1);
	}

	//
	// Read all object files
	// Append psects to master psect list
	//
	for (auto loopo = objname.begin(); loopo != objname.end(); loopo++)
	{
		ObjectFile of;
		of.ReadFile(*loopo);
		of.Dump(debug);

		passes.Pass100(of);
	}

	//
	// Relocate
	//
	passes.Pass200();

	passes.Dump(debug);

	//
	// Generate output
	//
	if (ldaname != "")
	{
		passes.WriteAbs(ldaname);
	}
	if (simhname != "")
	{
		passes.WriteSimh(simhname);
	}
	if (mapname != "")
	{
		passes.WriteMap(mapname);
	}

}

