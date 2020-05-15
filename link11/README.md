RT-11 Object File Linker

(INCOMPLETE)

This is a project in process and doesn't do anything interesting yet.


This prigram attempts to link TY-11 Object files, as created by the
macro11 program, into a binary file loadable by simh.


NOTES:

this program makes heavy use of in-memory data, and is written in C++,
so running this inside of a PDP11 emulater is highly unlikely.

I've tried to make the code as clear as possible, so it should be
possible for normal humans to understand the code, and make use of any
parts of it that they desire.

Having tried to reserect the l121 from the Unix Archive, I discovered
yjat the code was a bizarre mixture of RT11 object format and something
 else, with rather strange things occurring throughout the code with
numeric values referenced that did not match up to the PDP11 Object
file format at all. I finally decided that it would be less effort to
write one from scratch instead of trying to fix l11.

