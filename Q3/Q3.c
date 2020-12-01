#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdbool.h>

/* Colors */
#define RED  "\x1B[1;31m"
#define GRN  "\x1B[1;32m"
#define YEL  "\x1B[1;33m"
#define BLU  "\x1B[1;34m"
#define MAG  "\x1B[1;35m"
#define CYN  "\x1B[1;36m"
#define WHT  "\x1B[1;37m"


typedef struct Musician
{
	int id; // id of the musician
	int arrivalTime; // arrival time of the musician
	int stageType; // type of the stage on which musician performs... 1-Acoustic  2-Electric  3-Both
	int maxWaitTime; // maximum waiting time of the musician
	int PerformanceTime; // time for which musician will perform
	char instrumentChar; // instrument character of the musician
	int stage_no; // number of the stage on which musician performs
	int status; // status of the musician... 1-Performing  0-Not Performing
	char name[25]; // Name of the musician
	sem_t collect; // semaphore
	pthread_t musician_thread_id; // thread for the musician
	pthread_mutex_t musician_mutex; // mutex for the musician
} Musician;

typedef struct Stage
{
	int id; // id of the stage
	int type; // type of the stage... 1-Acoustic  2-Electric
	int status; // status of the stage... 1-Occupied  0-Available
	int curr_mus; // ID of the musician currently performing on the stage
	bool singerflag; // flag to check if a singer is performing on the stage 
	pthread_mutex_t stage_mutex; // mutex for the stage
} Stage;

typedef struct Coordinator
{
	int id; // id of the coordinator
	int status; // status of the coordinator... 1-Busy  0-Availale
	int musician_no; // ID of the musician who is taking T-Shirt from this coordinator
	pthread_t coordinator_thread_id; // thread for coordinator
} Coordinator;

sem_t musician_get_tshirt;
pthread_mutex_t mutex;

/* Global Variables
k = total number of musicians and singers
a = total number of Acoustic stages
e = total number of electric stages
c = total number of coordinators
t = maximum waiting time
t1 = lower limit for performance time
t2 = upper limit for performance time
tot_stages = total number of stages(both Acoustic and electric)
*/

int k,a,e,c,t,t1,t2;
int tot_stages;
Stage stages[1000]; // array of stages
Musician musicians[1000]; // array of musicians
Coordinator coordinators[1000]; // array of coordinators

/*Code to allocate stage to performers*/
void * musician_thread(void* args)
{
	Musician * m = (Musician *)args;
	sleep(m->arrivalTime);
	if(m->instrumentChar !='s')
		printf("%sMusician %s has arrived\n\nMusician %s is waiting to perform\n\n",YEL, m->name,m->name);
	else
		printf("%sSinger %s has arrived\n\nSinger %s is waiting to perform\n\n",YEL, m->name,m->name);
	int stage_alloc = 0;
	bool wait_time_over = 0;

	clock_t tpp = clock();
	
	while(!stage_alloc)
	{
		clock_t time_taken = clock() - tpp;
		int times = (int)(time_taken/CLOCKS_PER_SEC)-7;

		if(times > m->maxWaitTime)
		{
			if(m->instrumentChar !='s')
				printf("%sMusician %s waited for %d seconds\n\n",RED,m->name,m->maxWaitTime);
			else
				printf("%sSinger %s waited for %d seconds\n\n",RED,m->name,m->maxWaitTime);
			wait_time_over = 1; 
			break;
		}

		if(m->stageType == 1)
		{
			for(int i=0;i<a;i++)
			{
				pthread_mutex_lock(&(stages[i].stage_mutex));
				if(stages[i].status == 0)
				{
					// printf("Musician %s alloted stage %d\n\n", &m->name, stages[i].type);
					stage_alloc = stages[i].status = 1;
					stages[i].curr_mus = m->id-1;
					m->stage_no = i+1;
					pthread_mutex_unlock(&(stages[i].stage_mutex));
					break;
				}
				pthread_mutex_unlock(&(stages[i].stage_mutex));
			}
		}
		else if(m->stageType == 2)
		{
			for(int i=a; i<tot_stages; i++)
			{
				pthread_mutex_lock(&(stages[i].stage_mutex));
				if(stages[i].status == 0)
				{
					// printf("Musician %s alloted stage %d\n\n", &m->name, stages[i].type);
					stage_alloc = 1;
					m->stage_no = i+1;
					stages[i].status = 1;
					stages[i].curr_mus = m->id-1;
					pthread_mutex_unlock(&(stages[i].stage_mutex));
					break;
				}
				pthread_mutex_unlock(&(stages[i].stage_mutex));
			}
		}
		else
		{
			for(int i=0; i<tot_stages; i++)
			{
				pthread_mutex_lock(&(stages[i].stage_mutex));
				if(m->instrumentChar == 's' && stages[i].singerflag == 0)
				{
					// printf("Musician %s alloted stage %d\n\n", &m->name, stages[i].type);
					stages[i].singerflag = 1;
					stages[i].status = 1;
					m->stage_no = i+1;
					stage_alloc = 1;
					
					pthread_mutex_unlock(&(stages[i].stage_mutex));
					break;
				}
				else if(stages[i].status == 0)
				{
					// printf("Musician %s alloted stage %d\n\n", &m->name, stages[i].type);
					stages[i].status = 1;
					m->stage_no = i+1;
					stage_alloc = 1;
					
					stages[i].curr_mus = m->id-1;
					pthread_mutex_unlock(&(stages[i].stage_mutex));
					break;
				}
				pthread_mutex_unlock(&(stages[i].stage_mutex));
			}
				
		}
	}
	
	if(wait_time_over)
	{
		printf("%sMusician %s %c left because of impatience!\n\n",MAG,m->name, m->instrumentChar);
		return NULL;
	}
	
	if(m->instrumentChar !='s')
	{
		if(stages[m->stage_no-1].type == 1)
			printf("%sMusician %s performing %c at Acoustic stage number %d for %d seconds\n\n",BLU,m->name, m->instrumentChar,stages[m->stage_no-1].id , m->PerformanceTime);

		else 
			printf("%sMusician %s performing %c at Electric stage number %d for %d seconds\n\n",BLU,m->name, m->instrumentChar,stages[m->stage_no-1].id, m->PerformanceTime);
	}
	else
	{
		int tp = stages[m->stage_no-1].curr_mus; ;
		if(musicians[tp].stage_no!=0)
		{
			if(musicians[tp].stageType == 1)
			{
				printf("%sSinger %s joined %s's performance at Acoustic stage number %d, performance extended by 2 seconds\n\n",CYN,m->name, musicians[tp].name, stages[m->stage_no-1].id);
				sleep(2);
			}
			else
			{
				printf("%sSinger %s joined %s's performance at Electric stage number %d, performance extended by 2 seconds\n\n",CYN,m->name, musicians[tp].name, stages[m->stage_no-1].id);
				sleep(2);
			}
		}
		else
		{
			if(stages[m->stage_no-1].type == 1)
				printf("%sSinger %s started solo performance at Acoustic stage number %d for %d seconds\n\n",GRN,m->name, stages[m->stage_no-1].id, m->PerformanceTime);

			else 
				printf("%sSinger %s started solo performance at Electric stage number %d for %d seconds\n\n",GRN,m->name, stages[m->stage_no-1].id, m->PerformanceTime);
		}
	}
	if(m->instrumentChar != 's')
		sleep(m->PerformanceTime);

	int no = m->stage_no-1;
	pthread_mutex_lock(&(stages[no].stage_mutex));
	if(m->instrumentChar != 's')
	{
		if(stages[no].status==1)
		{
			stages[no].status = 0;
		}
		else
		{
			stages[no].status = 1;
		}
	}
	m->status = 1;
	if(m->instrumentChar != 's')
	{
		if(stages[m->stage_no-1].type == 1)
			printf("%sMusician %s's performance at Acoustic Stage finished!\n\nMusician %s is waiting for T-Shirt\n\n",MAG,m->name,m->name);
		else
			printf("%sMusician %s's performance at Electric Stage finished!\n\nMusician %s is waiting for T-Shirt\n\n",MAG,m->name,m->name);
	}
	else
	{
		if(stages[m->stage_no-1].type == 1)
			printf("%sSinger %s's performance at Acoustic Stage finished!\n\nSinger %s is waiting for T-Shirt\n\n",YEL,m->name,m->name);
		else
			printf("%sSinger %s's performance at Electric Stage finished!\n\nSinger %s is waiting for T-Shirt\n\n",YEL,m->name,m->name);
	}


	sem_post(&musician_get_tshirt);
	pthread_mutex_unlock(&(stages[no].stage_mutex));
	
	sem_wait(&(m->collect));

	return NULL;
}

/*Code to Handle Giving T-shirt to Performers*/
void * coordinator_thread(void* args)
{
	int v;
	Coordinator * crd = (Coordinator *)args;
	while(1>0)
	{
		sem_wait(&musician_get_tshirt);
		crd->status = 1;
		for(int i=0;i<k;i++)
		{
			pthread_mutex_lock(&(musicians[i].musician_mutex));
			if(musicians[i].status != 0)
			{
				crd->musician_no = i+1;
				if(musicians[i].instrumentChar != 's')
					printf("%sMusician %s collecting T-shirt\n\n", WHT, musicians[i].name);
				else
					printf("%sSinger %s collecting T-shirt\n\n", WHT, musicians[i].name);

				musicians[i].status = 0;
				pthread_mutex_unlock(&(musicians[i].musician_mutex));
				break;
			}

			pthread_mutex_unlock(&(musicians[i].musician_mutex));
		}
		v++;
		if(crd->musician_no)
		{
			sleep(2);
			if(musicians[crd->musician_no-1].instrumentChar != 's')
				printf("%sMusician %s is exiting\n\n",BLU, musicians[crd->musician_no-1].name);
			else
				printf("%sSinger %s is exiting\n\n",BLU, musicians[crd->musician_no-1].name);
			sem_post(&(musicians[crd->musician_no - 1].collect));
			crd->musician_no = 0;
			crd->status = 0;
		}
	}
	
	return NULL;
}

int main()
{
	printf("Enter number of Musicians/Singers, Acoustic stages, Electric stages, Coordinators, Max Waiting Time, Lower Limit of Performance Time ,Upper Limit of Performance Time: \n");
	scanf("%d %d %d %d %d %d %d", &k,&a,&e,&c,&t,&t1,&t2);
	tot_stages = a+e;
	int i;
	for(i=0; i<a; i++)
	{
		stages[i].id = i+1;
		stages[i].type = 1;
		stages[i].curr_mus = 0;
		stages[i].singerflag = 0;
		stages[i].status = 0;
		pthread_mutex_init(&(stages[i].stage_mutex), NULL);
	}
	for(i=a; i<a+e; i++)
	{
		stages[i].id = i+1-a;
		stages[i].type = 2;
		stages[i].curr_mus = 0;
		stages[i].singerflag = 0;
		stages[i].status = 0;
		pthread_mutex_init(&(stages[i].stage_mutex), NULL);
	}

	for(i=0;i<k;i++)
	{
		char tp;
		int artime;
		scanf("%s %c %d",musicians[i].name, &tp, &artime);
		musicians[i].instrumentChar = tp;
		musicians[i].arrivalTime = artime;
	}
	printf("\n\n");

	srand(time(0));
	sem_init(&musician_get_tshirt, 0, 0); 

	for(i=0; i < k; i++)
	{
		musicians[i].id = i+1;
		musicians[i].stage_no = 0; 	
		if(musicians[i].instrumentChar == 'v')
			musicians[i].stageType = 1;
		else if(musicians[i].instrumentChar == 'b')
			musicians[i].stageType = 2;
		else
			musicians[i].stageType = 0;
		musicians[i].maxWaitTime = t;
		musicians[i].PerformanceTime = (rand()%(t2-t1+1))+t1;
		// musicians[i].PerformanceTime = 10; // checking for end case, that is, wait time ends just after the stage is empty
		sem_init(&(musicians[i].collect), 0, 0); 
		musicians[i].status = 0;
		pthread_mutex_init(&(musicians[i].musician_mutex), NULL);
	}

	for(i=0;i<c;i++)
	{
		coordinators[i].id = i+1;
		coordinators[i].musician_no = 0;
		coordinators[i].status = 0;
	}

	for(i=0;i<c;i++)
		pthread_create(&(coordinators[i].coordinator_thread_id), NULL, coordinator_thread , &coordinators[i]);

	for(i=0;i<k;i++)
		pthread_create(&(musicians[i].musician_thread_id), NULL, musician_thread , &musicians[i]);


	for(i=0;i<k;i++)
		pthread_join(musicians[i].musician_thread_id, 0);
	
	printf("%s\n\nFinished!!\n\n\n\n",RED);
	return 0;
}