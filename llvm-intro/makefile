
all: truec truel

#
# Clean up all generated files
#
clean:
	rm truec truec.ll
	rm truel

#
# truec is a very simple program that simply returns EXIT_SUCCESS as the
# success status. This is essentially the standard Unix "true" program.
#
# This shows how to convert C code into LLVM code (truec.ll), and then
# how to convert that into an executable.
#
# The use of 'clang' to compile the .ll file into an executable one could
# be replaced with a call to 'llc', but the parameters would be different.
#
truec : truec.c
	clang -c -emit-llvm -S -O3 truec.c
	clang -O3 -o truec truec.ll

truel: truel.ll
	clang -O3 truel.ll -o truel

