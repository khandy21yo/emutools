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
//! 
//! Development states.
//! 1. Get it to read data from object files correctly.
//! initilly hard coded to the files "hello.obj" and "putconch.obj".
//! 2. Dump the loaded data to bbe sure it is reading correctly.
//! 3. Handle the indivifual passes.
//! 4. Geerate a functional binary file.
//! 5. Create a command line parser to allow  for other code to be linked.
//!
//!\author Kevin Handy, Apr 2020
//

#include "link11.h"


//!\brief Main
//!
int main(int argc, char **argv)
{
	Link passes;

	{
		ObjectFile of;
		std::string filename = "test/hello.obj";
		of.ReadFile(filename);
		of.Dump(1);

		passes.Pass100(of);
	}

	{
		ObjectFile of2;
		std::string filename2 = "test/putconch.obj";
		of2.ReadFile(filename2);
		of2.Dump(1);

		passes.Pass100(of2);
	}

	passes.Pass200();
	passes.WriteAbs("test/hello.lda");
	passes.WriteSimh("test/hello.simh");

	passes.Dump(1);
}

