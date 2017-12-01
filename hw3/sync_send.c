/*
 * Time: 2017/11/23
 * Author: Yun Chen Lo
 * Purpose: Part II
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define INF 999

void saveSolution(int *map, int vertex, const char* outfile);

int main(int argc, char** argv) {

	assert(argc == 4);
    const char* infile = argv[1];
    const char* outfile = argv[2];
	int num_proc = strtol(argv[3], 0, 10);

	/* MPI Declare */
    int nprocs;
    int myrank;

	/* Initialize MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    double total_IO_time = 0.0;
    double total_comm_time = 0.0;
    double total_isend_time = 0.0;
    double start, end;
    double send_time = 0.0, recv_time = 0.0, reduce_time = 0.0;
    double neibor_time = 0.0;
    double total_time = 0.0;
    int vertex;
    int *map;
    int num_iter=0;

    total_IO_time -= MPI_Wtime();
    total_time -= MPI_Wtime();
    if(myrank == 0){
	    /* Read in Adjacent Array*/
	    FILE * pFile;
		int in, counter=0;
		int i, j;
		int *dist;

		pFile = fopen ( infile , "r" );

		fscanf (pFile, "%d", &in);
		vertex = in;
	  	map = (int *)malloc((vertex*vertex)*sizeof(int));
	  	for(i=0; i<vertex; i++){
	  		for(j=0; j<vertex; j++){
	  			if(i!=j)
	  				map[vertex*i + j] = INF;
	  			else
	  				map[vertex*i + j] = 0;
	  		}
	  	}

	  	while (!feof (pFile))
	    {  
			fscanf (pFile, "%d", &in); 
			counter ++;
			if(counter > 1){
				if((counter-2) % 3 == 0){
					i=in;
				}
				else if ((counter-2) % 3 == 1 ){
					j=in;
				}
				else if((counter-2) % 3 == 2){
					map[vertex*i + j] = in;
					map[vertex*j + i] = in;
				}
	      	}
	    }
	    MPI_Bcast(&vertex, 1, MPI_INT, 0, MPI_COMM_WORLD);
	    MPI_Bcast(&map[0], vertex*vertex, MPI_INT, 0, MPI_COMM_WORLD);
	}
	else{
		MPI_Bcast(&vertex, 1, MPI_INT, 0, MPI_COMM_WORLD);
		map = (int *)malloc((vertex*vertex)*sizeof(int));
		MPI_Bcast(&map[0], vertex*vertex, MPI_INT, 0, MPI_COMM_WORLD);
	}
	total_IO_time += MPI_Wtime();

    for(int v=0; v < vertex; v++){
    	/* Root Vertex */ 
    	if(myrank == v){
    		/* Initilization */
    		MPI_Request req1;
    		MPI_Status status;
    		int *weight = (int*)malloc(vertex * sizeof(int));
    		int dist;
    		//int *dist = (int*)malloc(vertex * sizeof(int));
    		int num_neighbor=0;
    		int flag;
    		int done_recv=0, done, new_dist;
    		neibor_time -= MPI_Wtime();
    		printf("root start\n");
    		/* Dynamic Table */ 
    		/*
    		if(v>0){
    			MPI_Bcast(&map[0], vertex*vertex, MPI_INT, v-1, MPI_COMM_WORLD);
    		}
    		*/
			neibor_time += MPI_Wtime();

    		for(int i =0; i<vertex; i++){
    			weight[i] = map[v * vertex + i];
    			if(weight[i]!=INF)
    				num_neighbor++;
    		}
	    	
			/* Read several times & send */
			
	    	while(done_recv != vertex){
	    		done = 1;
	    		send_time -= MPI_Wtime();
	    		for (int i = 0; i < vertex; i++){
					if (weight[i] < INF && i != v){ 
						MPI_Send(&weight[i], 1, MPI_INT, i, v, MPI_COMM_WORLD);
					}
				}
	    		MPI_Barrier(MPI_COMM_WORLD);
	    		send_time += MPI_Wtime();
	    		recv_time -= MPI_Wtime();
	    		for(int i =0; i<num_neighbor-1; i++){
		    		MPI_Recv(&new_dist, 1, MPI_INT, MPI_ANY_SOURCE, v, MPI_COMM_WORLD, &status);

	    		}
	    		recv_time += MPI_Wtime();
	    		reduce_time -= MPI_Wtime();
	    		MPI_Allreduce(&done, &done_recv, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	    		reduce_time += MPI_Wtime();
	    		num_iter ++;
	    	}
	    	total_comm_time -= MPI_Wtime();
	    	int *dist_result = (int*)malloc(vertex * sizeof(int));
	    	dist = 0;
			MPI_Gather(&dist, 1, MPI_INT, dist_result, 1, MPI_INT, v, MPI_COMM_WORLD);
			for(int i=0; i<vertex; i++){
				if(i!=myrank){
					map[v*vertex + i] = dist_result[i];
					map[i*vertex + v] = dist_result[i];
				}
			}

    		total_comm_time += MPI_Wtime();
    		MPI_Barrier(MPI_COMM_WORLD);
    	}
    	else{ // other vertex
    		int dist, new_dist, dist_source, d;
    		int *weight = (int*)malloc(vertex * sizeof(int));
    		int done_recv=0, done;
    		int flag;
       		MPI_Status status;
    		MPI_Request req1;
    		dist = INF;
    		neibor_time -= MPI_Wtime();
    		//printf("start\n");
    		/* dynamic table */
    		/*
    		if(v >0){
    			MPI_Bcast(&map[0], vertex*vertex, MPI_INT, v-1, MPI_COMM_WORLD);
    		}
    		*/
    		neibor_time += MPI_Wtime();
    		//total_comm_time += MPI_Wtime();
    		int num_neighbor =0;
    		for(int i =0; i<vertex; i++){
    			weight[i] = map[myrank * vertex + i];
    			if(weight[i]!= INF)
    				num_neighbor++;
       		}
       		
       		
    		/* Read several times & send */
	    	while(done_recv != vertex){
	    		done = 1;
	    		int active =0;
	    		flag =1;
	    		
	    		/* Send State */
	    		send_time -= MPI_Wtime();
	    		for( int j=0; j<vertex; j++){
	    			if( j != myrank && weight[j] != INF){
	    				d = weight[j] + dist;
	    				MPI_Send(&d, 1, MPI_INT, j, v, MPI_COMM_WORLD);
	    			}
	    		}
	    		MPI_Barrier(MPI_COMM_WORLD);
	    		send_time += MPI_Wtime();
	    		recv_time -= MPI_Wtime();
	    		/* Read State */
	    		for(int i=0; i<num_neighbor-1; i++){
	    			MPI_Recv(&new_dist, 1, MPI_INT, MPI_ANY_SOURCE, v, MPI_COMM_WORLD, &status);
	    			if(dist > new_dist){
	    				dist = new_dist;
	    				done = 0;
	    			}
	    		}
	    		recv_time += MPI_Wtime();
	    		
	    		reduce_time -= MPI_Wtime();
	    		MPI_Allreduce(&done, &done_recv, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	    		reduce_time += MPI_Wtime();
	    	}
	    	total_comm_time -= MPI_Wtime();
	    	float *dist_result = NULL;
	    	printf("send back\n");
	    	MPI_Gather(&dist, 1, MPI_INT, dist_result, 1, MPI_INT, v, MPI_COMM_WORLD);
	    	printf("done send\n");
	    	//MPI_Send(&dist, 1, MPI_INT, v, vertex+v, MPI_COMM_WORLD);
	    	total_comm_time += MPI_Wtime();
	    	MPI_Barrier(MPI_COMM_WORLD);
    	}
    }
    total_comm_time -= MPI_Wtime();
    if(myrank == vertex-1){
    	saveSolution(map, vertex ,outfile);
    }
    total_comm_time += MPI_Wtime();

    total_time += MPI_Wtime();
    
    /*
    printf("total time = %lf\n", total_time);
    printf("total_isend_time time = %lf\n", total_isend_time);
    printf("IO time = %lf\n", total_IO_time);
    printf("comm time = %lf\n", total_comm_time);
    */
    printf("total time = %lf\n", total_time);
    printf("total:%lf, send_time:%lf, recv_time:%lf, reduce_time:%lf, neigh:%lf\n", total_time, send_time, recv_time, reduce_time, neibor_time);
    
    //printf("num of iteration = %d\n", num_iter);
	
    /* cleanup */    
    MPI_Finalize();
}

void saveSolution(int *map, int vertex, const char* outfile){
	int i, j;
	FILE *out;
	out=fopen(outfile, "w");
	for (i = 0; i <  vertex; i++){
      for (j = 0; j < vertex; j++){
         fprintf(out, "%d ", map[vertex*i + j]);
      }
      fprintf(out, "\n");
    }
    fclose(out);

}