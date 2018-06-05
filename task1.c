//################################################################################################
//
// Program     : Real-time task models in Linux
// Source file : task1.c
// Authors     : Sarvesh Patil & Vishwakumar Doshi
// Date        : 5 February 2018
//
//################################################################################################

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

#define NUM_TASKS 50                   // Number of threads
#define NUM_COMMANDS 50                // Max number of commands in each thread


//################################################
//
// Global variables & functions declaration area
//
//################################################


// MATRIX ENTRY CONVENTIONS
//==========================
// -10 => Periodic task
// -20 => Aperiodic task
// Multiplies mutex by 100*(no. of that mutex lock/unlock) & adds 1 if lock/2 if unlock
// e.g L2 mutex lock will be represented by (100*2+1) 101


const char *mDevice = "/dev/input/event2";  //Storing path of mouse event. Change this event number if your machine shows different number for mouse event
struct input_event ie;

pthread_cond_t left,right,start;            // Condition variable for start sync, left and right click aperiodic tasks events
pthread_mutexattr_t AT[10];                 // Mutex attribute array
pthread_mutex_t M[10],startm,leftm,rightm;  // Mutex array, and mutexes in conjunction with condition variables

int mat[NUM_TASKS][NUM_COMMANDS];           // Matrix declaration

int tret, stp_flag = 0,cret,pi_flag;       // thread return , condition variable and mutex function return value, 
int trace_fd = -1;
int marker_fd = -1;


//###################
//
// Compute Function
//
//###################
void compute(int x)                       // Utilises CPU to perform computations
{
    int i, j=0;
    for (i = 0; i < x; i++)
    {
        j = j + i;
    }
}

//#######################
//
// Thread Functions area
//
//########################

void *periodic_task(void *arg)         // Re-entrant periodic thread funtion || arg contains info about which line(row) to read from matrix
{
    int k = 3,j;
    struct timespec time,period;
    period.tv_sec = mat[*(int*)arg][2]/1000;
    period.tv_nsec = (mat[*(int*)arg][2]%1000) * 1000000ul;
    char buff[256],bufftemp[256];
    
    pthread_mutex_lock(&startm);
    cret = pthread_cond_wait(&start,&startm);            // All threads waiting on start condition variable to start simultaneously
    if(cret!=0)
        printf("Periodic condition variable error %d\n",cret);        
    pthread_mutex_unlock(&startm);

    clock_gettime(CLOCK_MONOTONIC,&time);    
    
    while(stp_flag == 0)                                // stop flag is 1 after total period in main expires
    {
        
        time.tv_sec+=period.tv_sec;
        time.tv_nsec+=period.tv_nsec;
        if(time.tv_nsec > 1000000000ul)
        {
            time.tv_sec++;
            time.tv_nsec = 0;
        }

        while(mat[*(int*)arg][k] != -1)                // -1 indicates end of line in matrix (our convention)
        {
    
            if(mat[*(int*)arg][k] < -99)               // Recognising mutex command
            {
                if((mat[*(int*)arg][k] % 100) == -1)   // 1s digit is -1 for lock and 100s digit tells which mutex to lock/unlock
                {
                    pthread_mutexattr_init(&AT[mat[*(int*)arg][k]/-100 - 1]);      // Attribute initialised to default 

                    if(pi_flag == 0)                                           // priority inheritance flag 
                        pthread_mutexattr_setprotocol(&AT[mat[*(int*)arg][k]/-100 - 1],PTHREAD_PRIO_INHERIT); //protocol is set to inherit only when flag is 0

                    pthread_mutex_init(&M[mat[*(int*)arg][k]/-100 - 1],&AT[mat[*(int*)arg][k]/-100 - 1]); // initialises with given attribute

                    cret = pthread_mutex_lock(&M[mat[*(int*)arg][k]/-100 - 1]);                       // locks mutex
                    if(cret !=0)
                        printf("Error Locking mutex %d\n", mat[*(int*)arg][k]/-100);

                    sprintf(buff,"\t");

                    for(j = 0;j < 3*(*(int*)arg);j++)
                        strcat(buff,"\t");
                    sprintf(bufftemp,"Locking mutex %d\n", mat[*(int*)arg][k]/-100);
                    strcat(buff,bufftemp);
                    write(marker_fd, &buff,30);

                    printf("Locking mutex %d\n",mat[*(int*)arg][k]/-100);
                }

                else if((mat[*(int*)arg][k])% 100 == -2)      // 1s digit is -2 for unlock
                {
                    cret = pthread_mutex_unlock(&M[mat[*(int*)arg][k]/-100 - 1]);  // 100s digit tells which mutex to unlock
                    if(cret !=0)
                        printf("Error Unlocking mutex %d\n", mat[*(int*)arg][k]/-100);
                    printf("Unlocking mutex %d\n",mat[*(int*)arg][k]/-100);

                    sprintf(buff,"\t");

                    for(j = 0;j < 3*(*(int*)arg);j++)
                        strcat(buff,"\t");
                    sprintf(bufftemp,"Unlocking mutex %d\n", mat[*(int*)arg][k]/-100);
                    strcat(buff,bufftemp);
                    write(marker_fd, &buff,30);

                    pthread_mutex_destroy(&M[mat[*(int*)arg][k]/-100 - 1]);
                }
            }

            else
            {
                compute(mat[*(int*)arg][k]);                           // else positive value tells number of loop iterations
                for(j = 0;j < 3*(*(int*)arg);j++)
                printf("\t");
                printf("Computing for %d\n",mat[*(int*)arg][k]);
            }

            k++;
        }
        clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&time,0); //Takes time provided in time object as absolue time and thread goes to sleep
        k=3;                              // starts reading matrix again from index 3 as previous index 0,1,2 are priority, period etc one time used values
    }
    printf("Exiting periodic\n");
    pthread_exit(NULL);
}



void *aperiodic_task(void *arg)              // Aperiodic thread function 
{  
    int k = 3,j;
    int lr = mat[*(int*)arg][2];             // lr is 0 for left click triggered threads and 1 for right click triggered ones
    char buff[256],bufftemp[256];
    pthread_mutex_lock(&startm);
    cret = pthread_cond_wait(&start,&startm);
    if(cret!=0)
        printf("Aperiodic start condition variable error %d\n",cret);
    pthread_mutex_unlock(&startm);

    while(stp_flag == 0)
    {
        if(lr == 0)
        {
            pthread_mutex_lock(&leftm);
            cret = pthread_cond_wait(&left,&leftm);           // Waits on left click condition variable
            if(cret!=0)
                printf("Left Aperiodic condition variable error %d\n",cret);
            pthread_mutex_unlock(&leftm);
            if(stp_flag==1)                                  // all Condition variables are broadcasted during termination
                break;
        }

        else if(lr == 1)
        {
            pthread_mutex_lock(&rightm);                 
            cret = pthread_cond_wait(&right,&rightm);       // waits on right click condition variable
            if(cret!=0)
                printf("Right Aperiodic condition variable error %d\n",cret);
            pthread_mutex_unlock(&rightm);
            if(stp_flag==1)
                break;
        }

        while(mat[*(int*)arg][k] != -1)                    // Keeps reading matrix line till -1 is reached
        {
            if(mat[*(int*)arg][k] < -99)
            {
                if((mat[*(int*)arg][k] % 100) == -1)
                {
                    pthread_mutexattr_init(&AT[mat[*(int*)arg][k]/-100 - 1]);

                    if(pi_flag == 0)
                        pthread_mutexattr_setprotocol(&AT[mat[*(int*)arg][k]/-100 - 1],PTHREAD_PRIO_INHERIT);

                    pthread_mutex_init(&M[mat[*(int*)arg][k]/-100 - 1],&AT[mat[*(int*)arg][k]/-100 - 1]);
                    cret = pthread_mutex_lock(&M[mat[*(int*)arg][k]/-100 - 1]);
                    if(cret !=0)
                        printf("Error Locking mutex %d\n", mat[*(int*)arg][k]/-100);

                    sprintf(buff,"\t");

                    for(j = 0;j < 3*(*(int*)arg);j++)
                        strcat(buff,"\t");
                    sprintf(bufftemp,"Locking mutex %d\n", mat[*(int*)arg][k]/-100);
                    strcat(buff,bufftemp);
                    write(marker_fd, &buff,30);

                    printf("Locking mutex %d\n",mat[*(int*)arg][k]/-100);
                }

                else if((mat[*(int*)arg][k])% 100 == -2)
                {
                    cret = pthread_mutex_unlock(&M[mat[*(int*)arg][k]/-100 - 1]);
                    if(cret !=0)
                        printf("Error Unlocking mutex %d\n", mat[*(int*)arg][k]/-100);
                    
                    printf("Unlocking mutex %d\n",mat[*(int*)arg][k]/-100);

                    sprintf(buff,"\t");

                    for(j = 0;j < 3*(*(int*)arg);j++)
                        strcat(buff,"\t");
                    sprintf(bufftemp,"Unlocking mutex %d\n", mat[*(int*)arg][k]/-100);
                    strcat(buff,bufftemp);
                    write(marker_fd, &buff,30);

                    pthread_mutex_destroy(&M[mat[*(int*)arg][k]/-100 - 1]);
                }
            }

            else
            {
                compute(mat[*(int*)arg][k]);
                for(j = 0;j < 3*(*(int*)arg);j++)
                    printf("\t");
                printf("Computing for %d\n",mat[*(int*)arg][k]);
            }

            k++;

        }
        k=3;
    }
    printf("Exiting aperiodic\n");
    pthread_exit(NULL);
}

//##############
//
// Mouse thread
//
//##############

void* mouse_detect(void* arg)  // Thread function to detect mouse click event and broadcast corresponding condition variables
{
    int fd,bytes;
    fd = open(mDevice,O_RDWR);     // Opens event file located in /dev/input/eventX

        if(fd == -1) //Because open returns -1 on occurence of error
        {
            printf("ERROR Opening %s\n", mDevice);
        }

    while(stp_flag == 0)
    {
        bytes = read(fd, &ie, sizeof(struct input_event)); 

            if(bytes > 0)
            {
                if((ie.code == BTN_LEFT) && (ie.value == 0))
                    pthread_cond_broadcast(&left);                     // broadcasting of condition variable based on left or right click
                else if((ie.code == BTN_RIGHT) && (ie.value == 0))
                    pthread_cond_broadcast(&right);
            }
     }

     return NULL;
}


//#############################
//
// Main
//
//#############################

int main(void)
{   
    
    // ftrace computation for Galileo
    char *debugfs;
    char path[256];
        

        debugfs = "/sys/kernel/debug";
        if (debugfs) {
            strcpy(path, debugfs);  /* BEWARE buffer overflow */
            strcat(path,"/tracing/tracing_on");
            trace_fd = open(path, O_WRONLY);
            if (trace_fd >= 0)
                write(trace_fd, "1", 1);

            strcpy(path, debugfs);
            strcat(path,"/tracing/trace_marker");
            marker_fd = open(path, O_WRONLY);
        }

    // Main code area

    struct timespec mainstop;           // timespec variable for calculating time for terminating main program
    char buffer[1024] ;   
    char *record,*line;   
    int i=0,j=0;                        // Matrix index variable
    
    // Input menu
    printf("Scenario selection options\n===========================\n");
    printf("Priority Inheritence enabled mutex? \nEnter 0 if Yes\nEnter 1 if No\n---------------\nChoice : ");
    scanf("%d",&pi_flag);

    // File operations
    //=================
    FILE *fstream = fopen("input.txt","r");   // opening input file
    if(fstream == NULL)   
    {      
       printf("\nFile opening failed ");      
       return -1 ;   
    }

    line = fgets(buffer,sizeof(buffer),fstream);   // Reading line and storing string in line variable
    record = strtok(line," ");                     // Separating tokens based on space delimeter

        while(record != NULL)     
        {     
            mat[i][j] = atoi(record);              //Storing in matrix after converting to int     
            record = strtok(NULL," ");  
            j++;   
        }
    mat[i][j] = -1;                                // -1 to indicate end of line
    i++;
    j=0;

    int num_tasks = mat[0][0];                     // Number of threads
    int period = mat[0][1];                        // Total period of main program
    printf("Number of tasks : %d\n", num_tasks);
    printf("Period of tasks : %d\n\n", period);
    printf("Input file\n-----------------\n");


    
    while((line=fgets(buffer,sizeof(buffer),fstream))!=NULL)   
    { 
        record = strtok(line," "); 

        if(record[0] == 'P')                   // Places -10 on 0th position if periodic thread
        {
            mat[i][j] = -10;
            j++;
        }

        else if(record[0] == 'A')              // Places -20 on 0th position if aperiodic thread
        {
            mat[i][j] = -20;
            j++;
        }

        record = strtok(NULL," ");     
        
        while(record != NULL)     
        {     
            if(record[0] == 'L')              
                mat[i][j] = atoi(&record[1]) * -100 - 1; // 100s digit tells which mutex to lock/unlock and 1s digit tells whether to lock/unlock

            else if(record[0] == 'U')
                mat[i][j] = atoi(&record[1]) * -100 - 2;

            else
            {   
                mat[i][j] = atoi(record); 
            }
        record = strtok(NULL," ");     
            
        j++;
        }
        mat[i][j] = -1;
        i++;
        j=0;   
    }
    
    for(i=0;i<=num_tasks;i++)                   // prints matrix
    {
        for(j=0;j<20;j++)
        {
            if(mat[i][j] ==-1)
                break;
            else
                printf("%d\t",mat[i][j]);  
        }
        printf("\n");
    }

    //==================
    // Thread Creation
    //==================
    
    pthread_t tid[num_tasks];                  // thread id array
    pthread_t mouse_det;                       // thread id of mouse click detection thread

    pthread_attr_t attr;                       // thread attributes
    pthread_attr_t mouse_attr;        
        

    struct sched_param param; //Declaring structure object param of type sched_param
    int cur_line[num_tasks];  // passing line number to thread functions

    cret = pthread_cond_init(&start,NULL);            // initialising condition variables
    if(cret!=0)
         printf("Start condn init error %d\n",cret);

    cret = pthread_cond_init(&left,NULL);
    if(cret!=0)
         printf("Left condn init error %d\n",cret);

    cret = pthread_cond_init(&right,NULL);
    if(cret!=0)
         printf("Right condn init error %d\n",cret);
    
    
    pthread_mutex_init(&startm,NULL);            // Initialising condition variable mutexes(thread mutex are different)
    pthread_mutex_init(&rightm,NULL);
    pthread_mutex_init(&leftm,NULL);

    
    //Setting thread affinity to core 0
    //----------------------------------
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0,&cpuset);

    // Setting pthread attributes
    //----------------------------
    pthread_attr_init(&mouse_attr);
    pthread_attr_setinheritsched(&mouse_attr,PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&mouse_attr, SCHED_FIFO);               // setting scheduling priority
    param.sched_priority = 99;                                          // Setting highest priority for mouse detection thread
    pthread_attr_setschedparam(&mouse_attr,&param); 

    tret=pthread_create(&mouse_det,&mouse_attr,mouse_detect,NULL);
    if(tret)
            {
                printf("Error..Retruned with code %d\n", tret );
                exit(-1);
            }
            
       
    pthread_setaffinity_np(mouse_det,sizeof(cpu_set_t),&cpuset);
    
    for(i=0;i<num_tasks;i++)
    {
        pthread_attr_init(&attr);
        pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        param.sched_priority = mat[i+1][1];                           // Setting priority of rest of threads acc to input file specification
        pthread_attr_setschedparam(&attr,&param); 
        cur_line[i] = i+1;                                            // storing line number to pass as argument to thread functions
       
        if(mat[i+1][0] ==-10)                                         // create periodic thread if 0th element is -10
        {
            tret=pthread_create(&tid[i],&attr,periodic_task,(void *)&cur_line[i]);
            pthread_setaffinity_np(tid[i],sizeof(cpu_set_t),&cpuset);
            if(tret)
            {
                printf("Error..Retruned with code %d\n", tret );
                exit(-1);
            }
            printf("Create periodic thread %d\n",i);
        }

        else if(mat[i+1][0] == -20)                                   // create aperiodic thread if 0th element is -20
        {
            tret=pthread_create(&tid[i],&attr,aperiodic_task,(void *)&cur_line[i]);
            pthread_setaffinity_np(tid[i],sizeof(cpu_set_t),&cpuset);
            if(tret)
            {
                printf("Error..Retruned with code %d\n", tret );
                exit(-1);
            }
            printf("Create aperiodic_task %d\n",i);
        } 
    }


// Conditional variable & setting timer
//======================================

usleep(1000);                                      // allowing all threads to be created and waitinng on condition variable before broadcasting
pthread_cond_broadcast(&start);                    // all threads start running

clock_gettime(CLOCK_MONOTONIC,&mainstop);          // gets current time

mainstop.tv_sec += mat[0][1]/1000;                 // adds time to sleep
mainstop.tv_nsec += (mat[0][1]%1000)*1000000ul;

if(mainstop.tv_nsec > 1000000000ul)
{
    mainstop.tv_nsec = 0;
    mainstop.tv_sec++;
}

clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&mainstop,0);  // sleeps for total program period
printf("========================\nTerminating..\n========================\n");
stp_flag = 1;                                                // signals termination for all thread loops

pthread_cond_broadcast(&right);                              // releases all broadcast variables
pthread_cond_broadcast(&left);


// Initiating Termination 
//========================
pthread_cond_destroy(&start);
pthread_cond_destroy(&left);
pthread_cond_destroy(&right);

for(i = 0; i< num_tasks; i++)                                // Waits for all threads to join
    pthread_join(tid[i],NULL);   

printf("Terminated..\n");    
write(trace_fd, "0", 1);

return 0 ; 
}

//##################
// End of task1.c
//##################