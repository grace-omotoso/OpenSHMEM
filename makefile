all:	test test1
test: 	test.c comms.c rte.c comms.h rte.h
	mpicc test.c comms.c rte.c -o test

test1:	test1.c comms.c rte.c comms.h rte.h
	mpicc test1.c comms.c rte.c -o test1
