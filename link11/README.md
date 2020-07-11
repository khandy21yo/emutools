RT-11 Object File Linker

(INCOMPLETE)


This prigram attempts to link RT-11 Object files, as created by the
macro11 program, into a binary file loadable by simh.

It is currnntly missing many important parts, and is likely to be
full of bugs, but it successfully creates the 'hello' program in
the /test subdirectory.

It is written in, I think, current standard C++, and does not use
any nonstandard external libraries. It uses cmake to build.
The old Unix Archive linker source (l11) is included, but it is so
broken that it is not usable.

Missing features include

	- Error messages for many operations
	- object libraries
	- psects larger than 256 bytes
	- relocations other than 01-06.
	- common psects (fortran /COMMON/ blocks for example)
	- and many more.

NOTES:

this program makes heavy use of in-memory data, and is written in C++,
so running this inside of a PDP11 emulater is highly unlikely.

I've tried to make the code as clear as possible, so it should be
possible for normal humans to understand the code, and make use of any
parts of it that they desire.

Having tried to reserect the l11 from the Unix Archive, I discovered
yjat the code was a bizarre mixture of RT11 object format and something
 else, with rather strange things occurring throughout the code with
numeric values referenced that did not match up to the PDP11 Object
file format at all. I finally decided that it would be less effort to
write one from scratch instead of trying to fix l11.

The initial code will be just enough to link the example program in
the test subdirectory into a runnable application. After that is 
accomplished, only then will I attempt to accomplish more features.



To compile the program

	cmake .
	make

building in debug mode

	cmake -DCMAKE_BUILD_TYPE=Debug .
	make

Command line

	link11 <options> <object files>

	<options>
		-lda <filename>
			Create a .lda, absolute binary, file.
			This can be loaded using the simh command
			'load <filename>'.
		-simh <filename>
			Create a .simh command file.
			This can be loaded using the simh command
			'do <filename>'.
		-rt11
			All object files are in RT11 format.
			This is the default.
		-rsx
			All object files are in RSX format.
		-debug
			Output data structures during the run.
			Not useful to anyone but developers of link11.

		-map <filename>
			Create a map listing.

	<object files>
		A list of the object files to be linked.

	If yo9u don't specify an output file using -lda ot -simh,
	then nothing will be created.

Example
	../link11 hello.obj putconch.obj -lda hello.lda -simh hello.simh

To compile the /test applications, read the README.md file in the /test
subdirectory.

