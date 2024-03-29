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
#include <stdbool.h>

void** shm_buffer;
void* buffer_head;
int shmid;
void* mybuffer;
int me;
int npes;
int* pSync;
void* comms_malloc();
struct ThreadData {
    pthread_t thread_id;
    int* dest;
    int* source;
    int  pe;
    int  nelems;
};

void *memcpy_put(void *arguments){
	struct ThreadData *data = (struct ThreadData*) arguments;
	int* dest =  data -> dest;
	int* source = data -> source;
	int pe = data -> pe;
	int nelems = data -> nelems;
	shm_buffer[pe] = mybuffer = shmat(shmid, (void**)0,0);
        int offset = (size_t)dest - (size_t)mybuffer;
        memcpy(shm_buffer[pe]+offset,  (void*)source,  sizeof(int)*nelems);
	pSync[me] = 1;
}


void *memcpy_get(void *arguments){
        struct ThreadData *data = (struct ThreadData*) arguments;
        int* dest =  data -> dest;
        int* source = data -> source;
        int pe = data -> pe;
        int nelems = data -> nelems;
        shm_buffer[pe] = mybuffer = shmat(shmid, (void**)0,0);
        int offset = (size_t)source - (size_t)mybuffer;
        /*copy source into dest*/
        memcpy((void*)dest,  (void*)shm_buffer[pe]+offset, sizeof(int)*nelems);
	pSync[me] = 1;
}

void comms_init()	{
	rte_init();
        me = rte_my_pe();
        npes = rte_n_pes();
	pSync = comms_malloc(npes*sizeof(int*));
	int i = 0;
	shm_buffer = malloc(npes*sizeof(void*));
	/*ftok to generate unique key*/
        key_t key = ftok("shmfile",me);
        /*shmget returns an identifier in shmid*/
        shmid = shmget(key, 1024, 0666|IPC_CREAT);
	 /*shmat to attach shared memory*/
        shm_buffer[me] = mybuffer = shmat(shmid, (void**)0,0);
	buffer_head = mybuffer;
}


/*put int buffer into shared memory*/
void comms_int_put(int* dest, int* source, size_t nelems, int pe){
	shm_buffer[pe] = mybuffer = shmat(shmid, (void**)0,0);
        int offset = (size_t)dest - (size_t)mybuffer;

        /*Copy the source into shmem buffer*/
        memcpy(shm_buffer[pe]+offset,   (void*)source,  sizeof(int)*nelems);
}

/*fetch int buffer from the shared memory*/
void comms_int_get(int* dest, int* source, size_t nelems, int pe){
	shm_buffer[pe] = mybuffer = shmat(shmid, (void**)0,0);
        int offset = (size_t)source - (size_t)mybuffer;
        /*copy source into dest*/
        memcpy((void*)dest,  (void*)shm_buffer[pe]+offset, sizeof(int)*nelems);
}


 /*put int buffer into shared memory asynchronously*/    
void comms_int_put_nbi(int *dest, const int *source, size_t nelems, int pe){
    struct ThreadData data;
    pthread_t thread_id; 
    data.thread_id = thread_id;
    data.dest = dest;
    data.source = source;
    data.nelems = nelems;
    data.pe = pe;
    pthread_create(&thread_id, NULL, memcpy_put, (void*)&data); 
    pthread_detach(thread_id, NULL); 
}


 /*fetch int buffer from the shared memory asynchronously*/
void comms_int_get_nbi(int *dest, const int *source, size_t nelems, int pe){
    pSync[me] = 0;
    struct ThreadData data;
    pthread_t thread_id;
    data.thread_id = thread_id;
    data.dest = dest;
    data.source = source;
    data.nelems = nelems;
    data.pe = pe;
    pthread_create(&thread_id, NULL, memcpy_get, (void*)&data);
    pthread_detach(thread_id, NULL);
}

void comms_putmem(void *dest, const void *source, size_t nelems, int pe){
	shm_buffer[pe] = mybuffer = shmat(shmid, (void**)0,0);
        int offset = (size_t)dest - (size_t)mybuffer;

        /*Copy the source memory location into shmem buffer*/
        memcpy((void*)shm_buffer[pe]+offset,  source,  sizeof(int)*nelems);
}

void comms_getmem(void *dest, const void *source, size_t nelems, int pe){
        shm_buffer[pe] = mybuffer = shmat(shmid, (void**)0,0);
        int offset = (size_t)source - (size_t)mybuffer;
        /*copy source into dest*/
        memcpy(dest,  (void*)shm_buffer[pe]+offset, sizeof(int)*nelems);
}

bool isAllPE(int* pSync, int npes){
        int i = 0;
        for (i = 0; i < npes; i++){
                if(pSync[i] == 0)
                        return false;
        }
        return true;
}

void barrier_PE0(){
	int i = 0;
	for(i = 1; i < npes; i++){
		while(pSync[i] == 0){

		}
		pSync[i] = 0;
	}
	int pe = 0;
	for(pe = 0; i < npes; i++){
		pSync[pe] = 1;
	}
}

void barrier_OtherPE(){
	pSync[me] = 0;
	comms_int_put( pSync[me], pSync[me], 1, 0);
	while(pSync[me] == 0){

	}
	pSync[me] = 0;
}

void barrier_all(){
          //This PE updates the psync value on PE 0
	  pSync[me] = 1;
          comms_int_put( pSync[me], pSync[me], 1, 0);
          //check that other PEs have a value of pSync 1 on PE 0
          int i = 0;
          while(true){
                for (i = 0; i < npes; i++){
                    comms_int_get(pSync[i],pSync[i], 1, 0);
                }
        if(isAllPE(pSync, npes))
                break;
        }
}


void* comms_malloc(size_t bytes){
	//barrier_all();	
	void* addr = buffer_head;
	buffer_head += bytes;
	return addr;
}

void comms_finalize()	{

        /*detach from shared memory*/
          shmdt(shm_buffer);

	/*destroy the shared memory*/
	shmctl(shmid, IPC_RMID, NULL);
	rte_finalize();
}


