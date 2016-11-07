/*
 * =======================================================================
 *  This file is part of Bitonic-Sorter.
 *  Copyright (C) 2016 Marios Mitalidis
 *
 *  Bitonic-Sorter is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Bitonic-Sorter is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Bitonic-Sorter.  If not, see <http://www.gnu.org/licenses/>.
 * =======================================================================
 */

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <cilk/cilk.h>


// Constants & Variables (Test Related)
//===========================================================

const char* TEST_FLAG = "-test\0";
const int TEST_FLAG_LENGTH = 5; 

int TEST_MODE = 0;

// for time measurements
struct timeval startwtime, endwtime;
double seq_time; 


// Constants & Variables (Algorithm Related)
//===========================================================

int *a; //array to sort with bitonic sort
int *b; //array to sort with stdlib.h/qsort

const int ASCENDING  = 1;
const int DESCENDING = 0;

int N;                   //problem size
int P;                   //number of threads (user option)
int Nthreads;            //number of threads (maximum to be created)
int current_threads = 0; //current number of threads (actually created) 

int p;  //log2(number of threads, user option)
int q;  //log2(problem size)

struct args {

	int lo;
	int cnt;
	int dir;
}; // arguments to pass to recursive bitonic sort function

const int parallel_threshold = 1<<21;


// Function Declaration
//===========================================================

void parse_arguments        (int argc,char *argv[]);
void init                   (void);
void create_threads_and_exec(void);
int  cmpfunc                (const void*, const void*);
void test                   (void);
void clear                  (void);
void rec_bitonic_sort       (int,int,int);
void bitonic_merge          (int,int,int);
void compare                (int, int, int);
int  cmpfunc_asc            (const void*, const void*);
int  cmpfunc_des            (const void*, const void*);


// Main
//===========================================================

int main(int argc, char *argv[])
{
	parse_arguments        (argc,argv);
	init                   ();
	create_threads_and_exec();
	test                   ();
	clear                  ();

	return(0);
}


// Function Definition 
//===========================================================

// function : parse_arguments()
// description : Parse the user arguments and store the inputs
//               to the respective global variables.
//               (see doc. for parsing)
//---------------------------------------------------------------------

void parse_arguments(int argc, char *argv[])
{
	if (argc != 3 && argc != 4) {
		printf("Usage: %s %s p q\n\nwhere, %s is an optional flag (test mode)\n       P=2^p is the maximum number of parallel threads\n       N=2^q is the problem size\n",argv[0],TEST_FLAG,TEST_FLAG); 
		exit(1);
	}

	if (argc == 3) {
		p = atoi(argv[1]);
		q = atoi(argv[2]);
		TEST_MODE = 0;
	}
	else { // argc == 4

		if (strncmp(argv[1],TEST_FLAG,TEST_FLAG_LENGTH)) {
			printf("Illegal flag received: %s\n",argv[1]);
			exit(1);
		}

		p = atoi(argv[2]);
		q = atoi(argv[3]);
		TEST_MODE = 1;
	}

	P = 1 << p;
	N = 1 << q;

	Nthreads = P;
	if (P > (N/2)) {
		Nthreads = N/2;
		p = q - 1;
	}
	
}

// function : init()
// description : Allocate memory for the arrays (bitonic sort array and
//               qsort) and for pthreads. Also, initialize arrays.
//---------------------------------------------------------------------

void init(void)
{
	//allocate space for the array
	a = (int*) malloc(N * sizeof(int));
	if (a == NULL) {
		printf("Error allocating memory.\n");
		exit(4);
	}

	if (TEST_MODE) {
		b = (int*) malloc(N * sizeof(int));
		if (b == NULL) {
			printf("Error allocating memory.\n");
			exit(4);
		}
	}

	//initialize arrays
	srand( time(NULL) );
	int i;
	for (i = 0; i < N; i++) {
		a[i] = rand() % N;
	}

	if (TEST_MODE) {
		for(i = 0; i < N; i++) {
			b[i] = a[i];
		}
	}

}

// function : create_threads_and_exec()
// description : Create the pthreads and join them with the current
//               function. Execution of the bitonic computation starts here.
//               Also, measure the time to execute.
//---------------------------------------------------------------------

void create_threads_and_exec(void)
{
	// start measuring time
	gettimeofday(&startwtime,NULL);

	// sort the array
	rec_bitonic_sort(0,N,ASCENDING);

	// stop measuring time
	gettimeofday(&endwtime,NULL);

	// calculate time
	seq_time = (double) ( (endwtime.tv_usec - startwtime.tv_usec) / 1.0e6
              	+ endwtime.tv_sec - startwtime.tv_sec );

	// print time
	printf("%lf\n",seq_time);
	
}

// function : test()
// description : Check the result of the bitonic sort against the 
//               stdlib/qsort.
//---------------------------------------------------------------------

void test(void)
{
	if (TEST_MODE) {

		//sort secondary array
		qsort(b,N,sizeof(int),cmpfunc_asc);
		
		//compare the results
		int passed = 1;
		int i;
		for (i = 0; i < N; i++) {

			if (a[i] != b[i]) {
				passed = 0;
				break;
			}
		}

		if (passed) {
			printf("Test PASSED. Same results with stdlib/qsort.\n");
		}
		else {
			printf("Test NOT PASSED. Different results with stdlib/qsort.\n");

			printf("bitonic sort:\n");
			for (i = 0; i < N; i++)
				printf("%d ",a[i]);
			printf("\n");


			printf("qsort:\n");
			for (i = 0; i < N; i++)
				printf("%d ",b[i]);
			printf("\n");

		}
	}
}
			
// function : clear()
// description : Clear all allocated space from memory.
//---------------------------------------------------------------------

void clear(void)
{
	free(a);
	if (TEST_MODE) {
		free(b);
	}
}

// function : bitonic_merge()
// description : The bitonic merge algorithm.
//---------------------------------------------------------------------
	
void bitonic_merge(int lo, int cnt, int dir)
{
	if (cnt > 1) {

		int k = cnt / 2;
		int i;

		for (i = lo; i < lo + k; i++) {
			compare(i,i+k,dir);
		}
		
		bitonic_merge(lo,k,dir);
		bitonic_merge(lo+k,k,dir);
	}
}
		
// function : rec_bitonic_sort()
// description : The recursive bitonic sort algortithm executes from
//               each pthread.
//---------------------------------------------------------------------
	
void rec_bitonic_sort(int lo, int cnt, int dir)
{
	if (cnt > 1) {

		int k = cnt / 2;

		// Sorting Part 1
		//----------------------------


		// create new thread and split the work
		if (k > parallel_threshold) {

			cilk_spawn rec_bitonic_sort(lo,k,ASCENDING);
		}
		else {

			cilk_spawn qsort(a+lo, k, sizeof(int), cmpfunc_asc);
		}



		// Sorting Part 2
		//----------------------------

		// create new thread and split the work
		if (k > parallel_threshold) {

			cilk_spawn rec_bitonic_sort(lo+k,k,DESCENDING);
		}
		else {

			cilk_spawn qsort(a+lo+k, k, sizeof(int), cmpfunc_des);
		}


		// synchronize tasks 
		cilk_sync;

		// Merging Part
		//----------------------------
		
		bitonic_merge(lo,cnt,dir);
	}


}

// function : compare()
// description: Compare two positions and swap (if necessary) to get
//              the desired direction (i.e. ascending).
//---------------------------------------------------------------------

void compare(int i, int j, int dir)
{
	if ( dir == (a[i]>a[j]) ) {
		
		int t;
		t = a[i];
		a[i] = a[j];
		a[j] = t;
	}
}

// function : cmpfunc_asc()
// description: Compare two positions. Result to be used from qsort
//              ascending.
//---------------------------------------------------------------------

int cmpfunc_asc(const void* a, const void* b)
{
	return ( *(int*)a - *(int*)b );
}

// function : cmpfunc_des()
// description: Compare two positions. Result to be used from qsort
//              descending.
//---------------------------------------------------------------------

int cmpfunc_des(const void* a, const void* b)
{
	return ( - *(int*)a + *(int*)b );
}

