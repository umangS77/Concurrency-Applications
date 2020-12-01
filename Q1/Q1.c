#define _POSIX_C_SOURCE 199309L
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>

/* Colors */
#define RED "\x1B[1;31m"
#define GRN "\x1B[1;32m"
#define YEL "\x1B[1;33m"
#define BLU "\x1B[1;34m"
#define MAG "\x1B[1;35m"
#define CYN "\x1B[1;36m"
#define WHT "\x1B[1;37m"


struct arg{
	int l;
	int h;
	int *arr;
};

int * shareMem(size_t size){
     key_t mem_key = IPC_PRIVATE;
     int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
     return (int*)shmat(shm_id, NULL, 0);
}

void merge(int* arr, int l, int mid,  int h);
void concurrent_mergesort(int *arr, int l, int h);
void *threaded_mergesort( void *args);
void normal_mergesort(int *brr, int l, int h);


int main()
{

	struct timespec ts;
	int n;

	printf("Enter the number of elements: ");
	scanf("%d",&n);


	int *arr = shareMem(sizeof(int)*(n));

	int brr[n];

	printf("Enter the elements: \n");

	for(int i=0;i<n;i++)
	{
		scanf("%d", &arr[i]);
	}
	printf("\n");
	for(int i=0;i<n;i++)
	{
		brr[i] = arr[i];
	}

	/* START portion for concurrent merge sort using processes */

	printf("%sRunning concurrent mergesort using processes: \n",YEL);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	long double st = ts.tv_nsec/(1e9)+ts.tv_sec;

	concurrent_mergesort(arr,0, n-1);

	for(int i=0; i<n; i++)
	{
	  printf("%d ",arr[i]);
	}
	printf("\n");

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("%sTime taken using processes = %Lf\n\n",YEL, en - st);
	long double t1 = en-st;

	/* FINISH portion for concurrent merge sort using processes */

	/* START portion for concurrent merge sort using threads */

	pthread_t tid;
	struct arg a;
	a.l = 0;
	a.h = n-1;
	a.arr = brr;

	printf("%sRunning concurrent mergesort using threads: \n",CYN);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	st = ts.tv_nsec/(1e9)+ts.tv_sec;

	pthread_create(&tid, NULL, threaded_mergesort, &a);
	pthread_join(tid, NULL);

	for(int i=0; i<n; i++)
	{
	  printf("%d ",brr[i]);
	}
	printf("\n");

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("%sTime taken using threads = %Lf\n\n", CYN, en - st);
	long double t2 = en-st;

	/* FINISH portion for concurrent merge sort using threads */

     /* START portion for normal merge sort */

	printf("%sRunning normal mergesort: \n",MAG);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	st = ts.tv_nsec/(1e9)+ts.tv_sec;

	normal_mergesort(brr, 0, n-1);

	for(int i=0; i<n; i++)
	{
	  printf("%d ",brr[i]);
	}
	printf("\n");

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("%sTime taken by normal mergesort= %Lf\n\n", RED, en - st);
	long double t3 = en - st;

     /* FINISH portion for normal merge sort */

	printf("%sNormal mergesort ran:\n\t[ %Lf ] times faster than concurrent mergesort using processes.\n\t[ %Lf ] times faster than concurrent mergesort using threads.\n\n\n", WHT, t1/t3, t2/t3);
	shmdt(arr);

	return 0;
}


/* START code to merge left and half subarray */
void merge(int *arr, int l, int mid,  int h)
{
	int l1 = mid-l+1;
	int ar1[l1];
	int l2 = h-mid;
	int ar2[l2];
	int i,j,p=l;

	for(i=l,j=0;i<=mid;i++,j++)
	{
		ar1[j]=arr[i];
	}
	for(i=mid+1,j=0;i<=h;i++,j++)
	{
		ar2[j]=arr[i];
	}

	i=j=0;
	while(i<l1 && j<l2)
	{
		if(ar1[i]<ar2[j])
		{
			arr[p]=ar1[i];
			p++;
			i++;
		}
		else
		{
			arr[p]=ar2[j];
			p++;
			j++;
		}
	}

	for(;i<l1;i++,p++)
		arr[p]=ar1[i];
	for(;j<l2;j++,p++)
		arr[p]=ar2[j];

}

/* FINISH code to merge left and half subarray */


/* START code for merge sort using processes */

void concurrent_mergesort(int *arr, int l, int h)
{
	// printf("l=%d, h=%d\n",l,h);
	if(h-l < 5)
	{
		for(int i=l;i<h;i++)
		{
			int low_index = i;
			for(int j=i+1; j<=h;j++)
			{
				if(arr[j] < arr[low_index])
				{
					low_index = j;
				}
			}
			int temp = arr[i];
			arr[i] = arr[low_index];
			arr[low_index] = temp;
		}
	}
	else if(l<h)
	{
		int mid = l + (h-l)/2;
		pid_t pid = fork();
		if(pid < 0)
		{
			printf("Fork Failed!!\n");
			exit(1);
		}
		if(pid == 0)
		{
			concurrent_mergesort(arr,l,mid);
			exit(0);
		}
		else
		{
			wait(NULL);
			concurrent_mergesort(arr,mid+1,h);
		}
		merge(arr,l,mid,h);
	}
}

/* FINISH code for merge sort using processes */


/* START code for merge sort using threads*/

void *threaded_mergesort( void *a)
{
	struct arg *args = (struct arg*) a;
	int l = args->l;
	int h = args->h;
	int *arr = args->arr;

	if(l>h)
		return NULL;

	if(h-l < 5)
	{
		for(int i=l;i<h;i++)
		{
			int low_index = i;
			for(int j=i+1; j<=h;j++)
			{
				if(arr[j] < arr[low_index])
				{
					low_index = j;
				}
			}
			int temp = arr[i];
			arr[i] = arr[low_index];
			arr[low_index] = temp;
		}
	}

	else if(l<h)
	{
		int mid = l + (h-l)/2;
		pthread_t tid1,tid2;

		struct arg arg1;
		arg1.l = l;
		arg1.h = mid;
		arg1.arr = arr;

		struct arg arg2;
		arg2.l = mid+1;
		arg2.h = h;
		arg2.arr = arr;

		pthread_create(&tid1, NULL, threaded_mergesort, &arg1);
		pthread_create(&tid2, NULL, threaded_mergesort, &arg2);

		pthread_join(tid1,NULL);
		pthread_join(tid2,NULL);

		merge(arr,l,mid,h);

	}

}


/* FINISH code for merge sort using threads */


/* START code for normal merge sort */

void normal_mergesort(int *brr, int l, int h)
{
	// printf("l=%d, h=%d\n",l,h);
	if(h-l < 5)
	{
		for(int i=l;i<h;i++)
		{
			int low_index = i;
			for(int j=i+1; j<=h;j++)
			{
				if(brr[j] < brr[low_index])
				{
					low_index = j;
				}
			}
			int temp = brr[i];
			brr[i] = brr[low_index];
			brr[low_index] = temp;
		}
	}
	else if(l<h)
	{
		int mid = l + (h-l)/2;
		normal_mergesort(brr,l,mid);
		normal_mergesort(brr,mid+1,h);
		merge(brr,l,mid,h);
	}
}

/* FINISH code for mornal merge sort */
