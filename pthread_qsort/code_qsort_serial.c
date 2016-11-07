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


// Constants & Variables (Algorithm Related)
//===========================================================

int *a; //array to sort with stdlib/qsort

int N;  //problem size
int q;  //log2(problem size)

// for time measurements
struct timeval startwtime, endwtime;
double seq_time; 


// Function Declaration
//===========================================================

void  parse_arguments        (int argc,char *argv[]);
void  init                   (void);
int   cmpfunc                (const void*, const void*);
void  exec                   (void);
void  clear                  (void);


// Main
//===========================================================

int main(int argc, char *argv[])
{
	parse_arguments(argc,argv);
	init();
	exec();
	clear();

	return(0);
}


// Function Definition 
//===========================================================

void parse_arguments(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s q\n\nwhere,  N=2^q is the problem size\n",argv[0]); 
		exit(1);
	}

	q = atoi(argv[1]);
	N = 1 << q;
}


void init(void)
{
	//allocate space for the array
	a = (int*) malloc(N * sizeof(int));
	if (a == NULL) {
		printf("Error allocating memory.\n");
		exit(4);
	}

	//initialize arrays
	srand( time(NULL) );
	int i;
	for (i = 0; i < N; i++) {
		a[i] = rand() % N;
	}
}


//code from:
//www.tutorialspoint.com/c_standard_library/c_function_qsort.htm
int cmpfunc(const void* a, const void* b)
{
	return ( *(int*)a - *(int*)b );
}

void exec(void)
{
	// start measuring time
	gettimeofday(&startwtime,NULL);

	//sort array
	qsort(a,N,sizeof(int),cmpfunc);

	// stop measuring time
	gettimeofday(&endwtime,NULL);

	// calculate time
	seq_time = (double) ( (endwtime.tv_usec - startwtime.tv_usec) / 1.0e6
                   + endwtime.tv_sec - startwtime.tv_sec );

	// print time
	printf("%lf\n",seq_time);
		
}
			

void clear(void)
{
	//free array space
	free(a);
}
