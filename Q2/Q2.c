#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

/* Colors */
#define RED  "\x1B[1;31m"
#define GRN  "\x1B[1;32m"
#define YEL  "\x1B[1;33m"
#define BLU  "\x1B[1;34m"
#define MAG  "\x1B[1;35m"
#define CYN  "\x1B[1;36m"
#define WHT  "\x1B[1;37m"

/* minimum function */
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define MAXL 100000

/* Global Variables 
    N = Number of Pharmaceutical Companies
    M = Number of Vaccination Zones
    O = Number of Students registered for vaccination
*/

int N,M,O;

typedef struct Company
{
    int id; // id of the company
    int status; // status of the company - whether it is producing vaccines or not
    int batches; // number of batches of vaccines produced by the company
    int p; // number of vaccines in each batch produced by the company
    int prob; // probability of success of the vaccine
    pthread_mutex_t company_mutex; // mutex to make sure that only one zone accesses at a given time
    pthread_t company_thread_id; // thread to produce vaccine
    pthread_cond_t condt_zone; // 
} Company;

typedef struct Zone
{
    int id; // id of the zone
    int p; // capacity of the zone
    int slots; // number of slots in the zone
    int strength; // number of students served by the zone
    int company_supply; // id of the company which supplies the zone
    pthread_mutex_t zone_mutex; // mutex to make sure that only one student access a zone at a given time
    pthread_t zone_thread_id; // thread to check and fill vaccines in zone
    pthread_cond_t condt_student; 
} Zone;

typedef struct Student
{
    int id; // id of the student
    int round; // number of round of the student
    pthread_t student_thread_id; // thread to wait for slot and allot it to student once it is available
} Student;


Company companies[MAXL]; // array of companiees
Zone zones[MAXL]; // array of Vaccination Zones
Student students[MAXL]; // array of Students


void vaccine_ready(Company *c)
{
    while(1>0)
    {
        if(c->batches != 0)
        {
            pthread_cond_wait(&(c->condt_zone), &(c->company_mutex));
        }
        else
        {
            break;
        }
    }

    printf("%sAll the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now.\n\n",MAG,c->id);
    pthread_mutex_unlock(&(c->company_mutex));
}

void ready_vaccination_zone(Zone *z)
{
    while(1>0)
    {
        if(z->slots == z->strength)
        {
            break;
        }
        else 
        {
            pthread_cond_wait(&(z->condt_student), &(z->zone_mutex));
        }
    }
    pthread_mutex_unlock(&(z->zone_mutex));
}

int student_vaccinated(int i, int id)
{
    int v;
    printf("%sStudent %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n\n",GRN,id, i+1);
    if(students[id-1].round > 3)
    {
        printf("%sStudent %d on Vaccination Zone %d has been vaccinated 3 times now. He/She will be sent back to home!\n\n",MAG,id, i+1);
        v=1;
    }
    else
    {
        printf("%sStudent %d on Vaccination Zone %d has been vaccinated which has success probability %d%%\n\n",WHT,id, i+1,companies[zones[i].company_supply].prob);
        int x = rand()%101;
        if(x<=companies[zones[i].company_supply].prob)
        {
            printf("%sStudent %d tested positive for antibody test. He/She can enter the college now!\n\n",WHT,id);
            v=1;
        }
        else
        {
            (students[id-1].round)++;
            printf("%sStudent %d tested negative for antibody test. Testing him/her again....\n\n",WHT,id);
            v=0;
            // student_vaccinated(i,id);
        }
    }
    pthread_cond_signal(&(zones[i].condt_student));
    pthread_mutex_unlock(&(zones[i].zone_mutex));
    return v;
}

void student_waiting_slot(int id)
{
    bool zone_ready =false;

    printf("%sStudent %d is waiting to be allocated a slot on a Vaccination Zone\n\n",YEL, id);

    while(zone_ready == false)
    {
        for(int j=0;j<M;j++)
        {
            pthread_mutex_lock(&(zones[j].zone_mutex));

            // critical section
            // check for slots who have strength left
            if(zones[j].slots > zones[j].strength)
            {
                zones[j].strength++;
                zone_ready = true;
                int k = student_vaccinated(j, id);
                if(k==0)
                {
                    // if(zones[j].slots >= zones[j].strength)
                    // {
                        zones[j].strength++;
                        k = student_vaccinated(j, id);

                        if(k==0)
                        {
                            // if(zones[j].slots >= zones[j].strength)
                            // {
                                zones[j].strength++;
                                k = student_vaccinated(j, id);
                                if(k==0)
                                    printf("%sStudent %d on Vaccination Zone %d has been vaccinated 3 times now. He/She will be sent back to home!\n\n",MAG,id, j+1);

                            // }
                        }
                    // }
                }
                break;
            }

            pthread_cond_signal(&(zones[j].condt_student));
            pthread_mutex_unlock(&(zones[j].zone_mutex));
        }
    }
}

void * company_thread(void* args)
{
    Company * c = (Company *)args;
    
    while(1>0)
    {
        int r = rand()%10 + 1;
        int w = 2 + rand()%4;
        int p = rand()%11 + 10;
        printf("%sPharmaceutical Company %d is preparing %d batches of vaccines which have success probability %d%%.\n\n",CYN, c->id, r, c->prob);
        sleep(w);
        pthread_mutex_lock(&(c->company_mutex));
        c->batches = r;
        c->p = p;
        printf("%sPharmaceutical Company %d has prepared %d batches of vaccines which have success probability %d%%.\n\n",BLU, c->id, c->batches, c->prob);     
        vaccine_ready(c);
    }

    return NULL;
}

void * zone_thread(void* args)
{

    Zone *z = (Zone*)args;
    
    while(1>0)
    {
        bool flag = false;
        for(int i=0;i<N;i++)
        {
            pthread_mutex_lock(&(companies[i].company_mutex));
            if(companies[i].batches != 0)
            {
                flag = true;

                companies[i].batches--;
                z->p = companies[i].p;
                z->company_supply = i;
                
                // printf("%sPharmaceutical Company %d is delivering vaccines to Vaccination zone %d which has success probability %d%%.\n\n", CYN,i+1,z->id,companies[z->company_supply].prob);
                // printf("%sPharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now.\n\n", BLU,i+1,z->id);

                printf("%sPharmaceutical Company %d is delivering vaccines to Vaccination zone %d which has success probability %d%%.\n\n%sPharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now.\n\n",CYN,i+1,z->id,companies[z->company_supply].prob, BLU,i+1,z->id);

                pthread_cond_signal(&(companies[i].condt_zone));
                // printf("\nB\n");
                pthread_mutex_unlock(&(companies[i].company_mutex));

                break;
            }
            pthread_cond_signal(&(companies[i].condt_zone));
            // printf("\nA\n");
            pthread_mutex_unlock(&(companies[i].company_mutex));
        }

        while(flag == true)
        {

            pthread_mutex_lock(&(z->zone_mutex));
            
            if(z->p == 0 && flag == true)
            {
                printf("%sVaccination Zone %d has run out of vaccines\n\n",MAG, z->id);
                pthread_mutex_unlock(&(z->zone_mutex));
                break;
            }



            z->strength = 0;
            int k = rand()%min(z->p,min(7,O))+1;
            z->slots = k;
            z->slots = min(z->slots,z->p);

            z->p = z->p - z->slots;
            printf("%sVaccination Zone %d is ready to vaccinate with %d slots.\n\nVaccination Zone %d entering Vaccination Phase.\n\n",YEL,z->id,z->slots,z->id);
            ready_vaccination_zone(z);
        }
        
    }

    return NULL;
}

void * student_thread(void* args)
{
    Student * s = (Student*)args;
    sleep(rand()%(max(O/2,1))); // to stimulate a delay in students arriving
    printf("%sStudent %d has arrived for his 1st round of Vaccination\n\n",YEL,s->id);
    student_waiting_slot(s->id);
    return NULL;
}

int main()
{
    srand(time(0));
    printf("\nEnter the number of Pharmaceutical companies: ");
    scanf("%d",&N);
    printf("\nEnter the number of Vaccination Zones: ");
    scanf("%d",&M);
    printf("\nEnter the number of Students: ");
    scanf("%d",&O);
    if((N == 0 || M == 0) && O > 0)
    {
        printf("Error in values\n\n");
        return 0;
    }
    int i;
    printf("\nEnter the probabilities (as integer percentage) of succcess for vaccines produced by Pharmaceutical companies.\n");
    for(i=0;i<N;i++)
    {
        int p;
        printf("\nEnter the probability of succcess for vaccine by Pharmaceutical company %d: ",i+1);
        scanf("%d",&p);
        companies[i].prob = p;
    }
    printf("\n\n");

    for(i=1;i<=M;i++)
    {
        zones[i-1].id = i;
    } 

    for(i=1;i<=O;i++)
    {
        students[i-1].id = i;
        students[i-1].round = 1;
    }

    for(i=1;i<=N;i++)
    {
        companies[i-1].id = i;
        pthread_mutex_init(&(companies[i-1].company_mutex), NULL);
    }


    for(i=1;i<=N;i++)
    {
        pthread_create(&(companies[i-1].company_thread_id), NULL, company_thread , &companies[i-1]);
    }

    for(i=1;i<=M;i++)
    {
        pthread_create(&(zones[i-1].zone_thread_id), NULL, zone_thread , &zones[i-1]);
    }

    for(i=1;i<=O;i++)
    {
        pthread_create(&(students[i-1].student_thread_id), NULL, student_thread , &students[i-1]);
    }

    for(i=1;i<=O;i++)
    {
        pthread_join(students[i-1].student_thread_id, 0); 
    }

    printf("%s\n\n\nSimulation over!\n\n\n\n\n",GRN);
    
    return 0; 
}