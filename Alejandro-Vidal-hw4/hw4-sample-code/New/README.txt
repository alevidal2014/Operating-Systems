README

	1. Have all the files in the same folder, your test file, and the files that we compressed and sent. 

Then we nee to compile our shared library
	2. In your shell in the directory in which you uncompressed the files type 'make', to make the library file (libmem.so) to which your test will refer to.

To run your test of mem.c you must:
	3. Then compile your test program with the following command: gcc [Test File Name] -lmem [libmem.so] -L. -o [however your wish to call your program]
	4. Then you set the environment with the following command: setenv LD_LIBRARY_PATH ${LD_LIBRARY_PATH}:.

5. Finally your can run the program to use our memory allocator.

