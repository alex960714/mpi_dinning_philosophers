#include <stdio.h>
#include <windows.h>
#include "mpi.h"

#define LEFT (i+size-2)%(size-1)
#define RIGHT (i+1)%(size-1)

enum TState { THINKING, EATING, HUNGRY, FED };
TState *states = NULL;
int *priority;
int rank, size;

void Test(int i)
{
	if (states[i] == HUNGRY && states[LEFT] != EATING && states[RIGHT] != EATING)
	{
		if (priority[i] >= priority[LEFT] && priority[i] >= priority[RIGHT])
		{
			states[i] = EATING;
			priority[i] = 0;
		}
		else
		{
			/*if (priority[i] < priority[LEFT] && priority[LEFT] >= priority[RIGHT])
				GetForks(LEFT);
			else if (priority[i] < priority[RIGHT])
				GetForks(RIGHT);
			priority[i]++;*/
		}
	}
}

void GetForks(int i)
{
	states[i] = HUNGRY;
	Test(i);
	if (states[i] == EATING)
		MPI_Send(states + i, 1, MPI_INT, i, EATING, MPI_COMM_WORLD);
}

void PutForks(int i)
{
	states[i] = THINKING;
	if (states[LEFT] == HUNGRY)
		GetForks(LEFT);
	if (states[RIGHT] == HUNGRY)
		GetForks(RIGHT);
}

void eat(int index, double hun_time)
{
	printf("Philosopher %d starts to eat. He was hungry for %f seconds\n", index, hun_time);
	Sleep(rand() % 3000 + 1500);
	printf("Philosopher %d has finished to eat\n", index);
}

void think(int index)
{
	printf("Philosopher %d is thinking\n", index);
	Sleep(rand() % 3000 + 1500);
	printf("Philosopher %d is hungry\n", index);
}

int main(int argc, char **argv)
{
	int *buf_send, *buf_recv;
	int op_num;
	double st_time, en_time;
	MPI_Status status;

	if (argc < 2)
	{
		printf("So few arguments\n");
		system("pause");
		exit(0);
	}
	op_num = atoi(argv[1]);

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (size < 5)
	{
		printf("So few num of processes. There should be >=5 processes\n");
		MPI_Finalize();
		system("pause");
		exit(0);
	}

	if (!rank)
	{
		int fed_num = 0;
		states = new TState[size - 1];
		priority = new int[size - 1];
		for (int i = 0; i < size - 1; i++)
		{
			states[i] = THINKING;
			priority[i] = 0;
		}
		while (fed_num != size-1)
		{
			int i;
			buf_recv = new int[2];
			MPI_Recv(buf_recv, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			i = buf_recv[0];
			switch (buf_recv[1])
			{
			case HUNGRY:
				GetForks(i);
				if (states[i] != EATING)
					//MPI_Send(states + i, 1, MPI_INT, i, EATING, MPI_COMM_WORLD);
				//else
				{
					if (priority[i] < priority[LEFT] && priority[LEFT] >= priority[RIGHT])
					{
						GetForks(LEFT);
						//if (states[LEFT] == EATING)
							//MPI_Send(states + LEFT, 1, MPI_INT, i, EATING, MPI_COMM_WORLD);
					}
					else if (priority[i] < priority[RIGHT])
					{
						GetForks(RIGHT);
						//if (states[RIGHT] == EATING)
							//MPI_Send(states + RIGHT, 1, MPI_INT, i, EATING, MPI_COMM_WORLD);
					}
					priority[i]++;
				}
				break;
			case THINKING:
				PutForks(i);
				break;
			case FED:
				fed_num++;
			}
			for (int j = 0; j < size - 1; j++)
				if (priority[j])
					priority[j]++;
			delete[] buf_recv;
		}
		delete[] states;
		delete[] priority;
	}
	else
	{
		for (int j = 0; j < op_num; j++)
		{
			buf_send = new int[2];
			think(rank - 1);
			buf_send[0] = rank - 1;
			buf_send[1] = HUNGRY;
			st_time = MPI_Wtime();
			MPI_Send(buf_send, 2, MPI_INT, 0, HUNGRY, MPI_COMM_WORLD);
			MPI_Recv(buf_send + 1, 1, MPI_INT, 0, EATING, MPI_COMM_WORLD, &status);
			en_time = MPI_Wtime();
			eat(rank - 1, en_time - st_time);
			buf_send[1] = THINKING;
			MPI_Send(buf_send, 2, MPI_INT, 0, THINKING, MPI_COMM_WORLD);
			delete[] buf_send;
		}
		buf_send = new int[2];
		buf_send[0] = rank - 1;
		buf_send[1] = FED;
		MPI_Send(buf_send, 2, MPI_INT, 0, FED, MPI_COMM_WORLD);
		delete[] buf_send;
	}

	MPI_Finalize();

	return 0;
}