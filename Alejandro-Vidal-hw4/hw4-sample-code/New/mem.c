/********************************************************
##########################################################
## COP4610 – Principles of Operating Systems – Fall 2017
## 
## Students: 	Juan Valladares - 2676611
##				Alejandro Vidal - 5913959
##
## Project: Memory Allocator. In this project, you will be 
##			implementing a memory allocator for the heap of 
##			a user-level process
##
## I Certify that this program code has been written by us
## and no part of it has been taken from any sources.
##########################################################
*********************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "mem.h"

//Structure to Contain each block of memory allocated
typedef struct mem_block_hd{
	struct mem_block_hd *next;		//Next Block
	struct mem_block_hd *prev;		//Previous Block
	int size;						//Size of the block
	int is_free;					//Determine if this block is free
} block_header;


//Structure to Contain globals variables of the library
typedef struct globals_variables{
	block_header* full_space;		//Full block of memory after initialization
	block_header* next_free;		// nxt_free is the ptr for next free block available.
	block_header* temp;				//my_globals->Temp block used to find the my_globals->next_free value depending on the policy
	block_header* base_ptr_to_free;	// Set the ptr block to free after receiving a ptr within this block

	int used_policy;
	
}globals;

globals* my_globals; //pointer to the global variables struct
int is_init = 0;		//To know if the memory was already initialized 


//***Helper Method***

//Method used to set the pointer my_globals->next_free depending on the policy before allocating the memory requested
int Set_Next_Free(int size){

	//my_globals->temp = my_globals->next_free; 	//Used to move through the hole s
	
	switch(my_globals->used_policy) {

	   case MEM_POLICY_FIRSTFIT:{
	   		my_globals->temp = my_globals->next_free;
	   		//This loop goes through the blocks finding the first block big enough to allocate the memory
	      	do{
	      		if(my_globals->temp->is_free == 1){		//if this block is free
	      			if(size  <= my_globals->temp->size + sizeof(block_header)){	//If this block is big enough
	      				my_globals->next_free = my_globals->temp;
	      				return 0;
	      			}
	      		}
	      		my_globals->temp = my_globals->temp->next;				//otherwise move to the next block				

				if (!my_globals->temp)						//End of the list, start from head.
					my_globals->temp = my_globals->full_space;

	      	}while (my_globals->temp != my_globals->next_free);			//The loop finishes if we go around the structure and finish in the original pointer (like a circular table)
	      	return -1;							//FAil to find a block big enough
	    break; 
	   }	      
	   		
		
	   case MEM_POLICY_BESTFIT:{
	   		int at_least_one; 
	   		at_least_one = 0;		//Boolean to check if we found at least one block big enough to allocate	   		

	   		my_globals->temp = my_globals->full_space;		

			do{						//This loop goes through all the blocks finding the smallest block big enough to allocate the memory
	      		if(my_globals->temp->is_free == 1){		//if this block is free
	      			if(size  <= my_globals->temp->size + sizeof(block_header)){	//If this block is big enough
	      				at_least_one = 1;
	      				if(my_globals->next_free->size > my_globals->temp->size){
	      					my_globals->next_free = my_globals->temp;	      				
	      				}
	      			}
	      		}
	      		my_globals->temp = my_globals->temp->next;				//otherwise move to the next block				

				
	      	}while (my_globals->temp);			//The loop finishes if we go around the structure and finish in the original pointer (like a circular table)
	      	
	      	if(at_least_one == 1)				//Block Found
	      		return 0;

	      	return -1;							//Fail to find a block big enough
	      
	     break;  
	   }
	   		
	   
	   case MEM_POLICY_WORSTFIT:{

	   		my_globals->temp = my_globals->full_space;	

	   		int at_least_one; 
	   		at_least_one = 0;		//Boolean to check if we found at least one block big enough to allocate

	        //This loop goes through the blocks finding the largest block to allocate the memory
	      	do{
	      		if(my_globals->temp->is_free == 1){		//if this block is free
	      			if(size  <= my_globals->temp->size + sizeof(block_header)){	//If this block is big enough
	      				at_least_one = 1;
	      				if(my_globals->next_free->size < my_globals->temp->size){
	      					my_globals->next_free = my_globals->temp;	      				
	      				}
	      			}
	      		}
	      		my_globals->temp = my_globals->temp->next;				//otherwise move to the next block				

				
	      	}while (my_globals->temp);			//The loop finishes if we go around the structure and finish in the original pointer (like a circular table)
	      	
	      	if(at_least_one == 1)				//Block Found
	      		return 0;

	      	return -1;
	     break;    

	     }
	}//end switch	   		

}//end method



int Mem_Init(int size, int policy){
	
    
	//Check if the memory was already initialized
	if (is_init == 1) {
		fprintf(stderr, "Error!!! Memory already Initialized.\n");
		return -1;
	}

	//Check that the size is grater than 0 bytes
	if(size <= 0){
		fprintf(stderr,"Error!!! Requested block size is not positive\n");
		return -1;
	}

	int fd = 0;			//used to open the device
	int page_size = 0;	//Page size
	void* space_ptr;	//Final space requested
	
	page_size = getpagesize();
	
	if(size % page_size != 0)
        size = size + (page_size-(size % page_size));		//Raound up the size to next full page size

	fd = open ("/dev/zero", O_RDWR);		// open the /dev/zero device

	space_ptr = mmap(NULL,size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);	//Requesting the memory space

	if(space_ptr == MAP_FAILED){
		perror("mmap");
		return -1;
	}
	
	my_globals = space_ptr;			//Space to allocate the global variables at the begining of the full memory

	//Initializing the Memory structure
	my_globals->full_space = (block_header*)((char*)(my_globals) + sizeof(globals));	//Full space for our blocks will start after the globals space
	
	my_globals->full_space->next = NULL;
	my_globals->full_space->prev = NULL;
	my_globals->full_space->size = size - sizeof(block_header) - sizeof(globals);	//The size available is (the full size - size of the structure)
	my_globals->full_space->is_free = 1; 
	my_globals->next_free = my_globals->full_space;			//Next free block available is initialy the full block space 
	my_globals->used_policy = policy;			//Defining our policy used to allocated blocks
	
	//****Portion of code that can be used to debug our code****
	/*printf("\nInside mem_init*************\n");
	printf("Size of the block header = %d\n", sizeof(block_header));
	printf("Size of the globals block= %d\n", sizeof(globals));
	printf("Total Address start at= %p\n", my_globals);
    printf("Full address start at= %p and has a size of %d\n", my_globals->full_space, my_globals->full_space->size);*/	
		
	is_init = 1;					//Mark the memory as initialized
	close(fd);						//Close the divice
	return 0;
}


void* Mem_Alloc(int size){

	if(is_init==1){

		if(size % 8 != 0)
				size = size + (8-(size % 8));	//Raund up size to the next byte

		int test = Set_Next_Free(size);  //Setting the my_globals->next_free pointer depending on the policy used
		
		if( test == 0){	//If we found a block big enought 
		
	    	block_header* current = my_globals->next_free;	//This will be the pointer used to allocate the memory (=my_globals->next_free)
	    	current->is_free = 0;
	    	
	    	
	    	block_header *next = (block_header*)((char*)(current) + size);		//Pointer to the next free block
			next->size = current->size -size;									//Calculating the size left after the allocation
			next->next = current->next;									    	//New next free block
			next->prev = current;												//Previous will be the actual allocated block
			next->is_free = 1;													
			
			//Updating the state of the allocated block
			current->next = next;										
			current->size = size - sizeof(block_header);
			

						
			// next search start from current->next
			my_globals->next_free = next;

			//****Portion of code that can be used to debug our code****	
			/*printf("\n************Inside Allocating********\n");
			printf("Allocating pointer = %p and its previous is = %p\n", current, current->prev);		
			printf("\nCurrent address start at= %p and has a size of %d\n", current, current->size);
			printf("Next Free start at address= %p and has a size of %d\n", next, next->size);
			printf("Returning Pointer = %p\n", (void*)(current));*/
						
			return (void*)current; 	//Return Successful*/
			
		}else{
			return NULL;	//there is not enough free space within memory region
		}
	}
	return NULL;		
}

int Mem_Free(void* ptr){
	
	if (ptr == NULL) {
		printf("Error! tried to free NULL\n");
        return -1;
	}

	int test = Mem_IsValid(ptr);				//Check if the prt address is valid 
	
	if(test ==1){
		int object_size = Mem_GetSize(ptr);		//Get the size of the block to free
		
		//****Portion of code that can be used to debug our code****
		//printf("\n************Inside Mem_free********\n");
		//printf("\nthe sise to free is %d\n", object_size);						
		//printf("Base Pointer to free = %p\n", my_globals->base_ptr_to_free);


		block_header* current = (block_header*)(my_globals->base_ptr_to_free);	//Block that is going to be freed 
	
		current-> is_free = 1;					//Mark this block as free

		block_header* tnext = current->next;	//temp pointer for the next loops
		block_header* tprev = current->prev;	//temp pointer for the next loops

		
		while(tnext){			//Check forward if the next blocks are free to unified the size on one block
			if(tnext->is_free){
				current->size += ((tnext->size) + sizeof(block_header));		//Add up the size
				current->next = tnext->next;
				if(tnext->next)
					tnext->next->prev = current;									//Advance the pointer
					tnext = tnext->next;
			}else{
				break;
			}
		}

		
		while(tprev){					//Check backward if the previous blocks are free to unified the size on one block
			if(tprev->is_free){
				int totalsize =0;
				totalsize = current->size + ((tprev->size) + sizeof(block_header));		//Add up the size	
				current = tprev;			
				current->prev = tprev->prev;
				current->size = totalsize;
				if(tprev->prev)
					tprev->prev->next = current;
					tprev = tprev->prev;												
			}else{
				break;
			}
		}

		return 0;	//Successfully free the memory
	}
	return -1;			//Fail to free the memory

}

int Mem_IsValid(void* ptr){
	block_header* pointer=(block_header*)ptr;
	block_header* temporary = my_globals->full_space;		
	
	do{		//Loop through all the Memory
		if(temporary<=pointer && (block_header*)((char*)temporary + sizeof(block_header)+ temporary->size)>pointer && temporary->is_free==0){
			return 1;
		}
		temporary = temporary->next;
	}while (temporary);	
	
	return 0;
}

int Mem_GetSize(void* ptr){
	block_header* pointer=(block_header*)ptr;
	block_header* temporary = my_globals->full_space;		
	
	do{		//Loop through all the Memory
		if(temporary<=pointer && (block_header*)((char*)temporary+ sizeof(block_header)+ temporary->size)>pointer && temporary->is_free==0){
			my_globals->base_ptr_to_free = temporary;
			return temporary->size;
			
		}
		temporary = temporary->next;
	}while (temporary);	
	
	return -1;
}

float Mem_GetFragmentation(){	
	block_header* temporary = my_globals->full_space;			//Pointer used to traverse the memory structure 
	
	float total=0;					//Total free memory
	float fragmentation=0;			//Fragmentation factor
	int biggest=0;					//Biggest block of free memory
	
	do{		//Loop through all the Memory
		if(temporary->is_free==1){				//If block is free
			total+= temporary->size;
			if(biggest< temporary->size)
				biggest= temporary->size;
		}
	
		temporary = temporary->next;
	}while (temporary);						//Until the end of the list

	if (total == 0){
		return 1;
	}
	
	fragmentation = biggest/total;
	return fragmentation;
}


