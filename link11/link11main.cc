//!\file link11main.cc
//!
//\brief Main driver code for link11
//!
//! main driver code for link11.
//! Handles user command line processing,
//! and main driver for the lionking process.
//!
//! Passes:
//! 1. Load object file data.
//!
//!\author Kevin Handy, Apr 2020
//

#include "link11.h"


//!\brief Main
//!
int main(int argc, char **argv)
{
	ObjectFile of;
	std::string filename = "test/hello.obj";

	of.ReadFile(filename);
	of.Dump(1);

	Link passes;
	passes.PassTxt(of);
}

