/**
 * University of Pittsburgh
 * Department of Computer Science
 * CS1645: Introduction to HPC Systems
 * Student:
 * Instructor: Bryan Mills, University of Pittsburgh
 * MPI particle-interaction code.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define TAG 7
#define CONSTANT 777

// Particle-interaction constants
#define A 10250000.0
#define B 726515000.5
#define MASS 0.1
#define DELTA 1

// Random initialization constants
#define POSITION 0
#define VELOCITY 1

// Structure for shared properties of a particle (to be included in messages)
struct Particle{
	float x;
	float y;
	float mass;
	float fx;
	float fy;
};

// Headers for auxiliar functions
float random_value(int type);
void print_particles(struct Particle *particles, int n);
void interact(struct Particle *source, struct Particle *destination);
void compute_interaction(struct Particle *source, struct Particle *destination, int limit);
void compute_self_interaction(struct Particle *set, int size);
void merge(struct Particle *first, struct Particle *second, int limit);


//Because C is stupid
int realMod (int a, int b) //reference http://stackoverflow.com/a/4003287/910303
{
   int ret = a % b;
   if(ret < 0)
     ret+=b;
   return ret;
}

// Main function
main(int argc, char** argv){
	int myRank;// Rank of process
	int p;// Number of processes
	int n;// Number of total particles
	int previous;// Previous rank in the ring
	int next;// Next rank in the ring
	int tag = TAG;// Tag for message
	int number;// Number of local particles
	struct Particle *globals;// Array of all particles in the system
	struct Particle *locals;// Array of local particles
	struct Particle *remotes;// Array of foreign particles
	char *file_name;// File name
	MPI_Status status;// Return status for receive
	int j, rounds, initiator, sender;
	double start_time, end_time;

	// checking the number of parameters
	if(argc < 2){
		printf("ERROR: Not enough parameters\n");
		printf("Usage: %s <number of particles> [<file>]\n", argv[0]);
		exit(1);
	}

	// getting number of particles
	n = atoi(argv[1]);

	// initializing MPI structures and checking p is odd
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);
	if(p % 2 == 0){
		p = p - 1;
		if(myRank == p){
			MPI_Finalize();
			return 0;
		}
	}
	srand(myRank+myRank*CONSTANT);

	// acquiring memory for particle arrays
	number = n / p;
	if (n % p != 0 && myRank == 0){ //only print 1x
		printf("#Warning: n is not evenly divisible!\n");
	}
	locals = (struct Particle *) malloc(number * sizeof(struct Particle));
	remotes = (struct Particle *) malloc(number * sizeof(struct Particle));
	int data_count_in_floats=(number * (sizeof (struct Particle)) / sizeof(float));
	// checking for file information
	if(argc == 3){
		if(myRank == 0){
			globals = (struct Particle *) malloc(n * sizeof(struct Particle));

			// YOUR CODE GOES HERE (reading particles from file)
			printf("#About to read %s\n", argv[2]);
			read_file(globals,n,argv[2]);
			printf("#allegedly read file\n");

		}
		// To send/recv (or scatter/gather) you will need to learn how to
		// transfer structs of floats, treat it as a contiguous block of
		// floats. Here is an example:
		// MPI_Send(locals,
		//          number * (sizeof (struct Particle)) / sizeof(float),
		//          MPI_FLOAT,
		//          next_rank,
		//          tag,
		//          MPI_COMM_WORLD)
		// MPI_Recv(remotes,
		//          number * (sizeof (struct Particle)) / sizeof(float),
		//          MPI_FLOAT,
		//          previous_rank,
		//          tag,
		//          MPI_COMM_WORLD,
		//          &status);
		// hint: because your nodes need to both send and receive you
		// might consider asyncronous send/recv.

		// YOUR CODE GOES HERE (distributing particles among processors)
		//printf("#%d About to scatter\n", myRank);
		MPI_Scatter(globals,
						data_count_in_floats,
						MPI_FLOAT,
						locals,
						data_count_in_floats,
						MPI_FLOAT,
						0,
						MPI_COMM_WORLD
					);
		//printf("#%d Done Scattering\n", myRank);

	} else {
		// random initialization of local particle array
		for(j = 0; j < number; j++){
			locals[j].x = random_value(POSITION);
			locals[j].y = random_value(POSITION);
			locals[j].fx = 0.0;
			locals[j].fy = 0.0;
			locals[j].mass = MASS;
		}
	}

	//At this point, we have locals setup on every task.
	memcpy(remotes,locals,(number * (sizeof (struct Particle)) / sizeof(float)));



	// starting timer
	if(myRank == 0){
		start_time = MPI_Wtime();
	}

	// YOUR CODE GOES HERE (ring algorithm)
	int i;
	for (i = 0; i < (p-1)/2 ; i++ ){
		int sendTo = realMod((myRank+1),p);
		int recvFrom = realMod((myRank-1), p);
		//printf("#%d Sending To %d\n",myRank, sendTo);
	//	printf("#%d Recv From %d\n",myRank, recvFrom);

		MPI_Sendrecv_replace(
			remotes,
			data_count_in_floats,
			MPI_FLOAT,
			sendTo,
			tag,
			recvFrom,
			tag,
			MPI_COMM_WORLD,
			&status
		);
		//printf("#%d SendRecv Complete!\n", myRank);

		compute_interaction(locals,remotes,number);
		//printf("#%d compute_interaction complete!\n", myRank);
	}

	int stepsToGoToOrig = p - (p-1)/2;
	int origRankOfTheseRemotes = realMod((myRank + stepsToGoToOrig), p);
	int whoHasMyOrig = realMod((myRank - stepsToGoToOrig),p);
	/*printf("#%d DONE WITH LOOP\n",myRank);
	printf("#%d My remotes go back to %d\n",myRank, origRankOfTheseRemotes);
	printf("#%d Recving remotes from: %d\n", myRank, whoHasMyOrig);*/
	MPI_Sendrecv_replace(
		remotes,
		data_count_in_floats,
		MPI_FLOAT,
		origRankOfTheseRemotes,
		tag,
		whoHasMyOrig,
		tag,
		MPI_COMM_WORLD,
		&status
	);
	//printf("#%d RecvOrig complete\n", myRank);
	merge(locals,remotes,number);
	compute_self_interaction(locals,number);

	// stopping timer
	if(myRank == 0){
		end_time = MPI_Wtime();
		printf("Duration: %f seconds\n", (end_time-start_time));
	}

	// printing information on particles
	if(argc == 3){

		// YOUR CODE GOES HERE (collect particles at rank 0)
		MPI_Gather(
			locals,
			data_count_in_floats,
			MPI_FLOAT,
			globals,
			data_count_in_floats,
			MPI_FLOAT,
			0,
			MPI_COMM_WORLD
		);

		if(myRank == 0) {
			print_particles(globals,n);
		}
	}


	// finalizing MPI structures
	MPI_Finalize();
}

// Function for random value generation
float random_value(int type){
	float value;
	switch(type){
	case POSITION:
		value = (float)rand() / (float)RAND_MAX * 100.0;
		break;
	case VELOCITY:
		value = (float)rand() / (float)RAND_MAX * 10.0;
		break;
	default:
		value = 1.1;
	}
	return value;
}

// Function for printing out the particle array
void print_particles(struct Particle *particles, int n){
	int j;
	printf("Index\tx\ty\tmass\tfx\tfy\n");
	for(j = 0; j < n; j++){
		printf("%d\t%f\t%f\t%f\t%f\t%f\n",j,particles[j].x,particles[j].y,particles[j].mass,particles[j].fx,particles[j].fy);
	}
}

// Function for computing interaction among two particles
// There is an extra test for interaction of identical particles, in which case there is no effect over the destination
void interact(struct Particle *first, struct Particle *second){
	float rx,ry,r,fx,fy,f;

	// computing base values
	rx = first->x - second->x;
	ry = first->y - second->y;
	r = sqrt(rx*rx + ry*ry);

	if(r == 0.0)
		return;

	f = A / pow(r,6) - B / pow(r,12);
	fx = f * rx / r;
	fy = f * ry / r;

	// updating sources's structure
	first->fx = first->fx + fx;
	first->fy = first->fy + fy;

	// updating destination's structure
	second->fx = second->fx - fx;
	second->fy = second->fy - fy;

}

// Function for computing interaction between two sets of particles
void compute_interaction(struct Particle *first, struct Particle *second, int limit){
	int j,k;

	for(j = 0; j < limit; j++){
		for(k = 0; k < limit; k++){
			interact(&first[j],&second[k]);
		}
	}
}

// Function for computing interaction between two sets of particles
void compute_self_interaction(struct Particle *set, int size){
	int j,k;

	for(j = 0; j < size; j++){
		for(k = j+1; k < size; k++){
			interact(&set[j],&set[k]);
		}
	}
}

// Function to merge two particle arrays
// Permanent changes reside only in first array
void merge(struct Particle *first, struct Particle *second, int limit){
	int j;

	for(j = 0; j < limit; j++){
		first[j].fx += second[j].fx;
		first[j].fy += second[j].fy;
	}
}

// Reads particle information from a text file
int read_file(struct Particle *set, int size, char *file_name){
	FILE *ifp, *ofp;
	char *mode = "r";
	ifp = fopen(file_name, mode);

	if (ifp == NULL) {
		fprintf(stderr, "Can't open input file!\n");
		return 1;
	}

	// reading particle values
	int i;
	for(i=0; i<size; i++){
		fscanf(ifp, "%f\t%f\t%f", &set[i].x, &set[i].y, &set[i].mass);
		set[i].fx = 0.0;
		set[i].fy = 0.0;
	}

	// closing file
	fclose(ifp);

	return 0;
}
