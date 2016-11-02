#include <stdio.h>
#include <windows.h>
#include <time.h>
#include "mpi.h"
#include <fstream>
#include <iostream>
using namespace std;

enum TState { THINKING, EATING, HUNGRY, FED };
TState *states = NULL;
int *priority;
int ProcRank, ProcNum;
double *wait_time = NULL;
double st_time, en_time;
ofstream os;

MPI_Status status;

int left(int i) { return (i + ProcNum-2) % (ProcNum-1); }
int right(int i) { return (i + 1) % (ProcNum-1); }

void print_states()
{
	os<<" States: [";
	for (int i = 0; i < ProcNum - 1; i++)
	{
		switch (states[i])
		{
		case THINKING: os<<"THINKING ";
			break;
		case EATING: os<<"EATING ";
			break;
		case HUNGRY: os<<"HUNGRY ";
			break;
		case FED: os<<"FED ";
		}
	}
	os << "]" << endl;
}

void eat(int index, double hun_time)
{
	//printf("Philosopher %d starts to eat. He was hungry for %f seconds\n", index, hun_time);
	Sleep(rand() % 1000);
	//printf("Philosopher %d has finished to eat\n", index);
}

void think(int index)
{
	//printf("Philosopher %d is thinking\n", index);
	//os << "Philosopher "<<index<<" is thinking" << endl;
	Sleep(rand() % 1000);
	//printf("Philosopher %d is hungry\n", index);
	//os << "Philosopher "<<index<<" is hungry" << endl;
}

void Test(int i)
{
	if (states[i] == HUNGRY && states[left(i)] != EATING && states[right(i)] != EATING)
	{
			states[i] = EATING;
	}
}

void GetForks(int i)
{
	states[i] = HUNGRY;
	//os << "Philosopher " << i << " is hungry" << endl;
	Test(i);
	if (states[i] == EATING)
	{
		MPI_Send(states + i, 1, MPI_INT, i, EATING, MPI_COMM_WORLD);
		en_time = MPI_Wtime();
		os << "Philosopher " << i << " starts to eat. Time: " << en_time - st_time << endl;
		print_states();
		//MPI_Recv(wait_time + i, 1, MPI_DOUBLE, i, EATING, MPI_COMM_WORLD, &status);
		//eat(i , wait_time[i]);
	}
}

void PutForks(int i)
{
	states[i] = THINKING;
	//think(i);
	if (states[left(i)] == HUNGRY)
		GetForks(left(i));
	if (states[right(i)] == HUNGRY)
		GetForks(right(i));
}



int main(int argc, char **argv)
{
	int *buf_send, *buf_recv;
	int op_num;
	int fed_num;
	
	
	if (argc < 2)
	{
		printf("So few arguments\n");
		//system("pause");
		exit(0);
	}
	op_num = atoi(argv[1]);

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

	srand(time(NULL));
	if (ProcNum < 5)
	{
		printf("So few num of processes. There should be >=5 processes\n");
		MPI_Finalize();
		//system("pause");
		exit(0);
	}

	if (ProcRank == ProcNum - 1)
	{
		fed_num = 0;
		states = new TState[ProcNum - 1];
		wait_time = new double[ProcNum - 1];
		for (int i = 0; i < ProcNum - 1; i++)
		{
			states[i] = THINKING;
			//think(i);
		}
		st_time = MPI_Wtime();
	}

	MPI_Barrier(MPI_COMM_WORLD);
	
	if (ProcRank == ProcNum - 1)
	{
		os.open("events.txt");
		while (fed_num != ProcNum - 1)
		{
			int i;
			buf_recv = new int[2];
			MPI_Recv(buf_recv, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			en_time = MPI_Wtime();
			i = buf_recv[0];
			switch (buf_recv[1])
			{
			case HUNGRY:
				states[i] = HUNGRY;
				os << "Philosopher " << i << " is hungry. Time: "<< en_time-st_time << endl;
				print_states();
				GetForks(i);
				break;
			case THINKING:
				states[i] = THINKING;
				en_time = MPI_Wtime();
				os << "Philosopher " << i << " is thinking. Time: "<<en_time-st_time << endl;
				print_states();
				PutForks(i);
				break;
			case FED:
				states[i] = FED;
				en_time = MPI_Wtime();
				os << "Philosopher " << i << " is fed. Time: "<<en_time-st_time << endl;
				print_states();
				fed_num++;
				//printf("Philosopher %d is fed\n", i);
			}
			delete[] buf_recv;
		}
	}
	else 
	{
		for (int j = 0; j < op_num; j++)
		{
			buf_send = new int[2];
			think(ProcRank);
			buf_send[0] = ProcRank;
			buf_send[1] = HUNGRY;
			st_time = MPI_Wtime();
			MPI_Send(buf_send, 2, MPI_INT, ProcNum-1, HUNGRY, MPI_COMM_WORLD);
			MPI_Recv(buf_send + 1, 1, MPI_INT, ProcNum-1, EATING, MPI_COMM_WORLD, &status);
			en_time = MPI_Wtime();
			//MPI_Send(&en_time, 1, MPI_DOUBLE, size - 1, EATING, MPI_COMM_WORLD);
			eat(ProcRank, en_time - st_time);
			buf_send[1] = THINKING;
			MPI_Send(buf_send, 2, MPI_INT, ProcNum-1, THINKING, MPI_COMM_WORLD);
			delete[] buf_send;
		}
		buf_send = new int[2];
		buf_send[0] = ProcRank;
		buf_send[1] = FED;
		MPI_Send(buf_send, 2, MPI_INT, ProcNum-1, FED, MPI_COMM_WORLD);
		
		delete[] buf_send;
	}

	MPI_Barrier(MPI_COMM_WORLD);
	if (ProcRank == ProcNum - 1)
	{
		en_time = MPI_Wtime();
		os<<"MPI version time: "<<en_time - st_time<<endl;
		delete[] states;
		os.close();
	}
	
	MPI_Finalize();

	return 0;
}