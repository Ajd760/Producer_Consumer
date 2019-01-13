/**************************************************************************************************************
Written by: Alan Doose
CS 433 HW 4
Nov 17 2018

Purpose: This program simulates the producer consumer problem with multithreading using the Unix pthread API. 
           To run the program, compile with -lpthread library linked. 
           When running the program, it expects 3 arguments to be passed on the command line:
            1. Length of time in seconds for the main thread to sleep while the producer/consumer threads operate
            2. Number of Producer threads to create
            3. Number of Consumer threads to create

Assumptions: It is assumed that this program will be compiled in a Unix environment and it is dependent upon
               The header files listed below. It is also dependent upon my buffer.h header file which should be
               included in the same directory as this main.cpp, otherwise you need to point the compiler to it
             It is also assumed that the user will enter the proper parameters on the command line, otherwise 
               the program will notify that it did not receive the proper amount/type of parameters and abort.
*************************************************************************************************************/

#include <stdlib.h>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "buffer.h"

//Prototypes
void *producer(void *param);
void *consumer(void *param);
int insert_item(buffer_item item);
int remove_item(buffer_item* item);
void printBuffer();


/********* CRITICAL SECTION GLOBALS ****************/
buffer_item buffer[BUFFER_SIZE]; //Globally accesible shared buffer (BUFFER_SIZE value and buffer_item type are defined in buffer.h)
int g_first = 0; //global indexes of the buffer
int g_last = -1;
int g_totalInserted = 0; //global counters to keep track of how many items are added and removed from the shared buffer
int g_totalRemoved = 0;
int g_count = 0; //counter for qty of item in buffer (used for double checking critical section and for printing buffer)

pthread_mutex_t lock; //The mutex lock
sem_t empty; //counting semaphores to indicate empty and full buffer to sleep threads
sem_t full;
/************************************************/

//The main function checks the parameters passed on the command line and creates the proper amount of
//  producer and consumer threads before going to sleep for a length of time specified on the command line.
//  Upon waking up, the main thread prints some statistics and ends the program
int main(int argc, char* argv[])
{
  srand(time(0)); //random for random sleep times in producer/consumer threads

  //This program requires exactly 3 paramters to be passed on the command line
  if(argc != 4)
    {
      std::cout << "Too few arguments, run this program as follows: './doose int1 int2 int3', where: " << std::endl; 
      std::cout << "int1: how long the main thread should sleep (in seconds) before terminating" << std::endl;
      std::cout << "int2: the number of producer threads to create" << std::endl;
      std::cout << "int3: the number of consumer threads to create" << std::endl;
    }
  else
    {
      //Get the command line parameters
      int sleepTime = atoi(argv[1]);
      int producerThreadCount = atoi(argv[2]);
      int consumerThreadCount = atoi(argv[3]);

      pthread_t producers[producerThreadCount]; //tid's of the producers
      pthread_t consumers[consumerThreadCount]; //tid's of the consumers

      std::cout << "Producer Consumer Problem Simulation\n"
		<< "Written by Alan Doose for CS433\n" 
		<< "Starting simulation with sleep time " << sleepTime << ", " 
		<< producerThreadCount << " producer threads and " << consumerThreadCount << " consumer threads\n\n";

      //Initialize buffer
      for(int i = 0; i < BUFFER_SIZE; i++) //set all slots in buffer to 0
	buffer[i] = 0;

      /***** Debug - testing buffer wraparound and printing ***

      g_first = g_last = 3; //start filling buffer from 2nd to last slot and make sure it wraps around
      for(int i = 1; i <= 4; i++)
	{
	  buffer[g_last] = i;
	  g_last = (g_last + 1) % BUFFER_SIZE;
	}
      printBuffer();
      ***********************/


      //Initialize semaphores and mutex lock
      if(pthread_mutex_init(&lock, NULL) != 0) //pthread_mutex_init() returns 0 on successful initialization
	{
	  std::cout << "Error initializing mutex lock, aborting...\n";
	  return -1;
	}
      if(sem_init(&empty, 0, 0) == -1) //empty semaphore init to 0, because the buffer is empty at start
	{
	  std::cout << "Error initializing empty semaphore, aborting...\n";
          return -1;
	}
      if(sem_init(&full, 0, BUFFER_SIZE) == -1) //full semaphore init to BUFFER_SIZE since that is the max # of items the buffer can hold
	{
	  std::cout << "Error initializing full semaphore, aborting...\n";
          return -1;
	}
      


      //Create producer threadds
      std::cout << "Creating " << producerThreadCount  << " producer threads..." << std::endl;
      for(int i = 0; i < producerThreadCount; i++)
	{
	  std::cout << "Creating producer thread " << i+1 << std::endl;
	  pthread_create(&producers[i], NULL, producer, NULL);
	}


      //Create consumer threads
      std::cout << "Creating " << consumerThreadCount  << " consumer threads..." << std::endl;
      for(int i = 0; i < consumerThreadCount; i++)
        {
	  std::cout << "Creating consumer thread " << i+1 << std::endl;
	  pthread_create(&consumers[i], NULL, consumer, NULL);
        }


      //Sleep main thread for time specified on command line
      std::cout << "\nPutting main thread to sleep for " << sleepTime << " seconds...\n\n";
      sleep(sleepTime);

      //End simulation, print some statistics
      std::cout << "...Program ending... " << std::endl;
      std::cout << "Total number of items added to buffer: " << g_totalInserted << std::endl;
      std::cout << "Total number of items removed from buffer: " << g_totalRemoved << std::endl;
      printBuffer();
      std::cout << "Goodbye\n\n";
      
      
      //cleanup
      pthread_mutex_destroy(&lock);       
      sem_destroy(&empty);
      sem_destroy(&full);
    }
  
  return 0;
}

//Inserts an item into the shared buffer
// Synchrnization if achieved with a counting semaphore and standard mutex lock
// @param buffer_item item - the item to add to the buffer
// @return value - 0 indicates success, -1 indicated error
int insert_item(buffer_item item)
{
  //check full or not
  sem_wait(&full);

  //try the lock
  pthread_mutex_lock(&lock);

  ///////////critical section
  if(g_count == BUFFER_SIZE) //Double check error condition of trying to add an item to a full buffer (never encountered in testing)
    return -1;

  g_last = (g_last + 1) % BUFFER_SIZE; //increment to next index
  buffer[g_last] = item; //add the item to the buffer
  g_totalInserted++; //increment statistic
  g_count++; //increment global counter

  //print updated contents of buffer
  std::cout << "Inserted " << item << " into the buffer. ";
  printBuffer();
  //////////End critical section

  //release the lock
  pthread_mutex_unlock(&lock);

  //increment empty and notify any sleeping process that buffer not empty (if it was before)
  sem_post(&empty);

  return 0;
}

//Removes an item from the shared buffer
// Synchrnization if achieved with a counting semaphore and standard mutex lock
// @param buffer_item* item - pointer to a container to hold the removed item for use by the caller
// @return value - 0 indicates success, -1 indicated error
int remove_item(buffer_item* item)
{
  //check empty or not
  sem_wait(&empty);

  //try the lock
  pthread_mutex_lock(&lock);

  ///////////critical section
  if(g_count == 0) //Double check error condition of trying to remove an item from an empty buffer (never encountered in testing)
    return -1;

  *item = buffer[g_first]; //remove item from the buffer
  g_first = (g_first + 1) % BUFFER_SIZE; //move first slot to next element
  g_totalRemoved++; //increment statistic
  g_count--;  //decrement global counter

  //print updated contents of buffer
  std::cout << "Removed " << *item << " from the buffer. ";
  printBuffer();
  //////////End critical section

  //release the lock
  pthread_mutex_unlock(&lock);

  //increment full and notify any sleeping process that buffer not full (if it was before)
  sem_post(&full);

  return 0; //return 0 indicated success
}

//This is the entry function for producer threads
//  Producer threads sleep for a random period of time (1-5 seconds), upon awakening they attempt to add a buffer_item
//    to the shared buffer by calling the synchronized insert_item(buffer_item item) function
// @param void *param is not used but left in to match pthread standard for passing parameters on thread creation
void *producer(void *param)
{
  buffer_item item;

  while(true)
    {
      unsigned int randSleepTime = rand() % 5 + 1; //random sleep 1-5 seconds
      sleep(randSleepTime);
      item = rand(); //random number betweemn 1 and RAND_MAX (library defined) to insert into buffer

      if(insert_item(item) == 0) //success
        {
	  //Successfully added item to buffer, nothing to report (updated buffer contents are printed in the critical section)
	}
      else
        {
	  std::cout << "Error inserting item into buffer\n";
	}

    }
}

//This is the entry function for consumer threads
//  Consumer threads sleep for a random period of time (1-5 seconds), upon awakening they attempt to remove a buffer_item
//    from the shared buffer by calling the synchronized remove_item(buffer_item *item) function
// @param void *param is not used but left in to match pthread standard for passing parameters on thread creation
void *consumer(void *param)
{
  buffer_item item;

  while(true)
    {
      unsigned int randSleepTime = rand() % 5 + 1; //random sleep 1-5 seconds
      sleep(randSleepTime);

      if(remove_item(&item) == 0) //success
        {
	  //Successfully removed item from buffer, nothing to report (updated buffer contents are printed in the critical section)
	}
      else
        {
	  std::cout << "Error removing item from buffer\n";
	}

    }

}

//Prints out the contents of the buffer in the form: "[1, 2, 3, ... , 6]" where the leftmost item is at the front of the buffer/queue
void printBuffer()
{
  int j = g_count; //counter to decrement while iterating through the buffer

  std::cout << "The current content of the buffer is [";
  for(int i = g_first; j > 0; i = (i+1) % BUFFER_SIZE) //print the buffer with wraparound since this buffer is implemented with a wraparound array
    {
      std::cout << buffer[i]; //print the item

      if(i != g_last) //if this is not the last item to print add comma and space for formatting
	std::cout << ", ";

      j--; //decrement counter
    }
  std::cout << "]\n"; //end of buffer
}
