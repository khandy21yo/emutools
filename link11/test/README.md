This subdirectory contains test files for link11.


To build the test coe in this subdirectory, edit maketest to adjust
the PATH for the location of macro11 and link11, and execute it.


The hello program is a simple "Hello World" program that runs on
the bare hardware of a PDP-11. Only the colsole device is needed,
and 4k+ of RAM. (Stack is starting at  010000). The source code is
in hello.mac and putconch.mac.

To execute the hello program in simh, you can use the following
methods depending on which binary you wich to use.

lda (absolute binary loader format)

	pdp11
	load hello.lda
	g


simh script format

	pdp11
	do hello.simh
	g

or
	pdp11 hello.simh
	g

