#include <stdio.h>
#include <windows.h>
#include <time.h>
#include "mpi.h"

#define LEFT (i+size-2)%(size-1)
#define RIGHT (i+1)%(size-1)

enum TState { THINKING, EATING, HUNGRY, FED };
TState *states = NULL;
int *priority;
int rank, size;
double *wait_time = NULL;

MPI_Status status;

void eat(int index, double hun_time)
{
	printf("Philosopher %d starts to eat. He was hungry for %f seconds\n", index, hun_time);
	Sleep(rand() % 1000);
	printf("Philosopher %d has finished to eat\n", index);
}

void think(int index)
{
	printf("Philosopher %d is thinking\n", index);
	Sleep(rand() % 1000);
	printf("Philosopher %d is hungry\n", index);
}

void Test(int i)
{
	if (states[i] == HUNGRY && states[LEFT] != EATING && states[RIGHT] != EATING)
	{
			states[i] = EATING;
	}
}

void GetForks(int i)
{
	states[i] = HUNGRY;
	Test(i);
	if (states[i] == EATING)
	{
		MPI_Send(states + i, 1, MPI_INT, i, EATING, MPI_COMM_WORLD);
		//MPI_Recv(wait_time + i, 1, MPI_DOUBLE, i, EATING, MPI_COMM_WORLD, &status);
		//eat(i , wait_time[i]);
	}
}

void PutForks(int i)
{
	states[i] = THINKING;
	//think(i);
	if (states[LEFT] == HUNGRY)
		GetForks(LEFT);
	if (states[RIGHT] == HUNGRY)
		GetForks(RIGHT);
}

int main(int argc, char **argv)
{
	int *buf_send, *buf_recv;
	int op_num;
	int fed_num;
	double st_time, en_time;
	
	
	if (argc < 2)
	{
		printf("So few arguments\n");
		//system("pause");
		exit(0);
	}
	op_num = atoi(argv[1]);

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	srand(time(NULL));
	if (size < 5)
	{
		printf("So few num of processes. There should be >=5 processes\n");
		MPI_Finalize();
		//system("pause");
		exit(0);
	}

	if (rank == size - 1)
	{
		fed_num = 0;
		states = new TState[size - 1];
		wait_time = new double[size - 1];
		for (int i = 0; i < size - 1; i++)
		{
			states[i] = THINKING;
			//think(i);
		}
		st_time = MPI_Wtime();
	}

	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == size - 1)
	{
		while (fed_num != size - 1)
		{
			int i;
			buf_recv = new int[2];
			MPI_Recv(buf_recv, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			i = buf_recv[0];
			switch (buf_recv[1])
			{
			case HUNGRY:
				GetForks(i);
				break;
			case THINKING:
				PutForks(i);
				break;
			case FED:
				fed_num++;
				printf("Philosopher %d is fed\n", i);
			}
			delete[] buf_recv;
		}
	}
	else 
	{
		for (int j = 0; j < op_num; j++)
		{
			buf_send = new int[2];
			think(rank);
			buf_send[0] = rank;
			buf_send[1] = HUNGRY;
			st_time = MPI_Wtime();
			MPI_Send(buf_send, 2, MPI_INT, size-1, HUNGRY, MPI_COMM_WORLD);
			MPI_Recv(buf_send + 1, 1, MPI_INT, size-1, EATING, MPI_COMM_WORLD, &status);
			en_time = MPI_Wtime();
			//MPI_Send(&en_time, 1, MPI_DOUBLE, size - 1, EATING, MPI_COMM_WORLD);
			eat(rank, en_time - st_time);
			buf_send[1] = THINKING;
			MPI_Send(buf_send, 2, MPI_INT, size-1, THINKING, MPI_COMM_WORLD);
			delete[] buf_send;
		}
		buf_send = new int[2];
		buf_send[0] = rank;
		buf_send[1] = FED;
		MPI_Send(buf_send, 2, MPI_INT, size-1, FED, MPI_COMM_WORLD);
		
		delete[] buf_send;
	}

	MPI_Barrier(MPI_COMM_WORLD);
	if (rank == size - 1)
	{
		en_time = MPI_Wtime();
		printf("MPI version time: %f\n", en_time - st_time);
		delete[] states;
	}

	MPI_Finalize();

	return 0;
}