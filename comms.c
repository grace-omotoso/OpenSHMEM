/*******************************************OpenSHMEM*******************************
 * *	GitHub.com: 	https://github.com/andela-bomotoso/OpenSHMEM
 * *	Author: 		Bukola Grace Omotoso
 * *	Email:			bgo2e@mtmail.mtsu.edu
 * *	Description: 	A partial implementation of the OpenSHMEM 1.4 Specification
 * *	Date:			3rd March 2020
 * ************************************************************************************/

//#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <rte.h>
//using namespace std;
void** shm_buffer;
int shmid;


void comms_init()	{

	rte_init();
        int pe = rte_my_pe();
        int npes = rte_n_pes();
	/*ftok to generate unique key*/
        key_t key = ftok("shmfile",pe);
        /*shmget returns an identifier in shmid*/
        shmid = shmget(key, 1024, 0666|IPC_CREAT);
	 /*shmat to attach shared memory*/
        shm_buffer = shmat(shmid, (void**)0,0);
	
}
/*put char buffer into the shared memory*/
void comms_put(char* dest, char* source, size_t nelems, int pe){
         printf("Hi1\n");	
	int offset = (size_t)dest - (size_t)shm_buffer[pe];
	printf("%d\n", offset);
	 printf("Hi2\n");
	/*Copy the source into shmem buffer*/
	 memcpy(shm_buffer[pe],   (void*)source,  sizeof(source));
         printf("Hi3\n");
	/*Copy source into dest*/
	memcpy((void*)dest, (void*)source,  nelems);
		
	printf("Data written in memory internally: %s\n", dest);
}


/*fetch char buffer from the shared memory*/
void comms_get(char* dest, char* source, size_t nelems, int pe){
	int offset = (size_t)source - (size_t)shm_buffer[pe];
	/*copy source into dest*/
	memcpy((void*)dest,  (void*)source+offset,  nelems);
	
	printf("Data read from memory internally: %s\n", dest);
}

void comms_finalize()	{

        /*detach from shared memory*/
          shmdt(shm_buffer);

	/*destroy the shared memory*/
	shmctl(shmid, IPC_RMID, NULL);

}


