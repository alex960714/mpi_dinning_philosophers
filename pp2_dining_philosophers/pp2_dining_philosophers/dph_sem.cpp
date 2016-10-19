#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

HANDLE ss, s[5];
enum TState { THINKING, EATING, HUNGRY };
TState states[5] = { THINKING,THINKING,THINKING,THINKING,THINKING };

int left(int index) { return (index + 4) % 5; }
int right(int index) { return (index + 1) % 5; }

void Test(int index)
{
	WaitForSingleObject(ss, INFINITE);
	if (states[index] == HUNGRY && states[left(index)] != EATING && states[right(index)] != EATING)
	{
		states[index] = EATING;
		ReleaseSemaphore(s[index], 1, NULL);
	}
	ReleaseSemaphore(ss, 1, NULL);
}

void GetForks(int index)
{
	states[index] = HUNGRY;
	Test(index);
	WaitForSingleObject(s[index], INFINITE);
}

void PutForks(int index)
{
	states[index] = THINKING;
	Test(left(index));
	Test(right(index));
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

DWORD WINAPI philosopher_run(LPVOID lp)
{
	int i;
	double time_st, time_en;
	int index = (int)lp;
	for (i = 0; i < 5; i++)
	{
		think(index);
		time_st = (double)GetTickCount()/1000;
		GetForks(index);
		time_en = (double)GetTickCount() / 1000;
		eat(index,time_en-time_st);
		PutForks(index);
	}
	printf("Philosopher %d is fed\n", index);
	return 0;
}

int main(int argc, char **argv)
{
	int i;
	HANDLE philosophers[5], philosopherID[5];
	ss = CreateSemaphore(NULL, 1, 1, NULL);
	for (i = 0; i < 5; i++)
	{
		s[i] = CreateSemaphore(NULL, 0, 1, NULL);
	}

	for (i = 0; i < 5; i++)
	{
		philosophers[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)philosopher_run, (void*)i, 0, (LPDWORD)&philosopherID[i]);
	}

	WaitForMultipleObjects(5, philosophers, TRUE, INFINITE);

	CloseHandle(ss);
	for (i = 0; i < 5; i++)
	{
		CloseHandle(philosophers[i]);
		CloseHandle(s[i]);
	}
	return 0;
}