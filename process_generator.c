#include "headers.h"
#include "datastructures/generatorQueue.c"
#include <string.h>
void clearResources(int);

struct msgbuff
{
  long mtype;
  struct process p;
};

int msgqid;

// upper bound to the number of processes (recieved from the process_generator which in fact is the number of processes in the input file) 
int total_num_pro;


int main(int argc, char *argv[])
{

  signal(SIGINT, clearResources);
  msgqid = msgget(1, IPC_CREAT | 0666);
  if (msgqid == -1)
  {
    perror("Error in create");
    exit(-1);
  }
  printf("msgqid = %d\n", msgqid);

  struct queue *q;
  q = malloc(sizeof(struct queue));
  initialize(q);
  // 1. Read the input files.
  FILE *inFile = fopen(argv[1], "r");
  if (inFile == NULL)
  {
    printf("Error! Could not open file\n");
    exit(-1);
  }
  struct stat sb;
  if (stat(argv[1], &sb) == -1)
  {
    perror("stat");
    exit(EXIT_FAILURE);
  }
  char *buff = malloc(sb.st_size);

  while (fscanf(inFile, "%s", buff) != EOF)
  {
    if (buff[0] == '#')
    {
      fscanf(inFile, "%[^\n] ", buff);
    }
    else
    {
      total_num_pro++;
      struct process *p1 = malloc(sizeof(struct process));
      p1->id = atoi(buff);
      fscanf(inFile, "%s", buff);
      p1->arrival = atoi(buff);
      fscanf(inFile, "%s", buff);
      p1->runtime = atoi(buff);
      fscanf(inFile, "%s", buff);
      p1->priority = atoi(buff);
      fscanf(inFile, "%s", buff);
      p1->memsize = atoi(buff);
      enqueue(q, p1);
    }
  }

  fclose(inFile);
//  display(q->front);

  // 2. Read the chosen scheduling algorithm and its parameters, if there are any from the argument list.
  
  int num_parms=argc+2;
  char **parms;
  parms = malloc(num_parms * sizeof(char *)); //all arguments passed to the funcion
  for(int i=0;i<num_parms;i++)
    parms[i] = malloc(50 * sizeof(char));
  
  int i=0;
  parms[i++] = "./scheduler.out";
  parms[i++] = argv[3];
  if(argv[3][0]=='5')
    parms[i++] = argv[5]; 
  sprintf(parms[i++],"%d",total_num_pro);
  parms[i++] = NULL;

  // 3. Initiate and create the scheduler and clock processes.
  int *PID;
  PID = (int *)malloc(2 * sizeof(int));
  for (int i = 0; i < 2; i++)
  {

    PID[i] = fork();

    if (PID[i] == -1)
    {
      printf("Error In Forking\n");
    }

    else if (PID[i] == 0)
    {

      if (i == 1)
      {
//        printf("I'm schedular, My process ID is %d, my parent ID : %d \n", getpid(), getppid());
        execv("./scheduler.out", parms);
      }
      else
      {
//        printf("I'm clock, My process ID is %d, my parent ID : %d \n", getpid(), getppid());
        execl("./clk.out", "./clk.out", NULL);
      }
    }
  }

  // 4. Use this function after creating the clock process to initialize clock.
  initClk();
  // 5. Create a data structure for processes and provide it with its parameters.

  //done before

  // 6. Send the information to the scheduler at the appropriate time.
  int x = getClk();
//  printf("Current Time is %d\n", x);

  while (!(isempty(q)))
  {
    int temp = getClk();
    if (temp != x)
    {
      x = temp;
//      printf("Current Time is %d\n", x);
//      display(q->front);
    }

    int t = q->front->proc->arrival;

    if(t <= x)
    {
      //now starting to send the process to the scheduler
      int send_val;
      struct msgbuff message;
//      printf("Dequeuing ...\n");
      struct process* temp = dequeue(q);
      printf("\ngenerator: sending id=%d memsize=%d\n",temp->id,temp->memsize);
      message.p.id=temp->id;message.p.arrival=temp->arrival;
      message.p.runtime=temp->runtime;message.p.priority=temp->priority;
      message.p.memsize=temp->memsize;
      message.mtype=1;
      send_val = msgsnd(msgqid, &message, sizeof(message.p), !IPC_NOWAIT);
      if (send_val == -1)
      {
        perror("Errror in send");
      }
    }
  }

  // send message indicates that all has arrived (message with id =-1) 

  struct msgbuff message;
  message.p.id=-1;
  message.mtype=1;
  printf("\ngenerator: sending id=%d\n",message.p.id);
  msgsnd(msgqid, &message, sizeof(message.p), !IPC_NOWAIT);

  while(true);

  // 7. Clear clock resources
  destroyClk(false);
  return 0;
}

void clearResources(int signum)
{
  //TODO Clears all resources in case of interruption
  msgctl(msgqid,IPC_RMID,(struct msqid_ds*)0);
  printf("\ngenerator: terminating\n");
  destroyClk(true);
  exit(0);
}
