#README

 /********************************************************************/  
 Class: COP 4610 Operating Systems Principle - Fall 2017

 Homework Assignmnet #5 - Implementing a File System

 Authors:  	Juan C Valladares  	Panther ID: <2676611>
 			Alejandro Vidal  	Panther ID: <5913959>
 
 Certification:
 Taking as a starting point the partial implementation provided by Prof. Liu.
 We hereby certify that this work is our own and none of it is the work of
 any other person. 
 /********************************************************************/ 


To compile the library that is useful with the disk, you must:

	1. Place all requires files in the same folder in order to compile them.
		(This files include: LibDisk.h, LibDisk.c, LibFS.h, LibFS.c, Makefile.LibDisk, Makefile.LibFS)

	2. Compile the disk and the File System libraries using the commands:
		>make -f Makefile.LibDisk
		>make -f Makefile.LibFS

	3. It is also advisable to use the following files in order to test your library:
		( Makefile, simple-test.c, slow-cat.c, slow-ls.c, slow-touch.c, slow-mkdir.c, slow-rm.c, slow-rmdir.cm, slow-import.c, slow-export.c)

	4. Compile all the tester files by using the command:
		>make

	5.Execute the file you wich in order to test our File System. 
		Example:
				>./simple-test.exe [diskName]

	