This subdirectory contains test files for link11.


To compile the hello world example, add paths to commands as necessary

macro11 -rt11 hello.mac -o hello.obj -l hello.lst
macro11 -rt11 putconch.mac -o putconch.obj -l putconch.lst
../link11 hello.obj putconch.obj -lda hello.lda -simh hello.simh

pdp11
   load hello.lda
   g

pdp11
   do hello.simh
   g

