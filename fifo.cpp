/*
This approach uses \textbf{monitors} to handle the synchronization of the queue.
It has two key methods: get_customer and add_customer. These methods should be the only interaction between the producer thread
- the thread reading from the input file adding customers to the queue - and the consumer threads - the 4 threads representing the 4 tables of the dining center.


The queue is implemented using C++'s std::deque data structure. It is almost like an std::vector except it has functionality for placing/deleting elements
at both the start and end of the queue.
*/

#include <iostream>
#include <pthread.h>
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <deque>
#include <vector>
#include <fstream>
#include <atomic>

// for time tracking instead of sleep()
#include <chrono>

using namespace std;

// global variable for the file in order to use it in the producer thread.
FILE *file;
int burst;
int qsize = 0;
std::atomic<bool> done(false);
pthread_mutex_t lock;
pthread_cond_t has_customer;

struct Customer
{
    int id; // process ID
    int eating_time_left;
    std::chrono::steady_clock::time_point arrival_time; // time for when the student arrive
    std::chrono::steady_clock::time_point sit_time;     // sit at a table
    std::chrono::steady_clock::time_point leave_time;   // leave the table
};

struct QueueMonitor
{
    // Students in the queue
    std::deque<Customer> customers;

    Customer get_customer()
    {
        pthread_mutex_lock(&lock); // Lock for exclusive access to the queue

        // Wait for customers to be available
        while (customers.empty() && !done)
        {
            // printf("Thread %ld is waiting for customers...\n", pthread_self());
            pthread_cond_wait(&has_customer, &lock); // Wait for signal from producer
            // printf("Thread %ld was signaled by producer...\n", pthread_self()); // Debug after waking up
        }

        // Check if the producer has finished and there are no customers left
        if (customers.empty() && done)
        {
            printf("Thread %ld exiting, no more customers in queue.\n", pthread_self());
            pthread_mutex_unlock(&lock); // Unlock before exiting
            pthread_exit(NULL);          // Exit the consumer thread
        }

        // Fetch the customer from the queue
        Customer c = customers.front();
        customers.pop_front(); // Remove the customer from the queue
        qsize--;

        // printf("Thread %ld fetched customer ID: %d, queue size: %lu\n", pthread_self(), c.id, customers.size());

        pthread_mutex_unlock(&lock); // Unlock after fetching the customer

        return c;
    }

    void add_customer(Customer c)
    {
        pthread_mutex_lock(&lock); // Ensure exclusive access to queue

        // Capture arrival time
        c.arrival_time = std::chrono::steady_clock::now();

        // Add the customer to the queue
        // printf("Trying to add customer ID: %d with eating time: %d\n", c.id, c.eating_time_left);
        customers.push_back(c); // Add customer to deque
        // printf("Customer ID: %d added, new queue size: %lu\n", c.id, customers.size());

        qsize++; // Update queue size

        // Signal waiting consumers that a customer is available
        pthread_cond_signal(&has_customer); // wake up on consumers

        pthread_mutex_unlock(&lock); // Unlock after adding the customer
    }
};

struct QueueMonitor queue;

/*Function for the producer thread. There should be 1 of these. It is in charge of adding customers to the end of the queue as they arrive.
    It has two key methods: get_customer and add_customer. These methods should be the only interaction between the producer thread
*/
void *producer_function(void *arg)
{
    int id1 = 1; // starting customer ID
    int eating_time_left;
    int arrival_time;

    // printf("Producer started processing...\n");

    // Reading burst time (ignored for now)
    fscanf(file, "%i", &burst);

    // Add customers to the queue as they arrive
    while (fscanf(file, "%d %d", &arrival_time, &eating_time_left) == 2)
    {
        // printf("File reading successful, read arrival time: %d, eating time: %d\n", arrival_time, eating_time_left);

        // Simulate when the customer arrives
        sleep(arrival_time);

        // Create a new customer
        Customer customer;
        customer.id = id1;
        customer.eating_time_left = eating_time_left;

        // Add the customer to the queue
        queue.add_customer(customer);
        printf("Added customer <student ID: %d>, queue size: %lu\n", customer.id, queue.customers.size());

        id1++; // Increment the customer ID
    }

    // signal the consumers that the producer is done
    pthread_mutex_lock(&lock);
    done = true; // Indicate producer is done
    pthread_cond_broadcast(&has_customer);
    pthread_mutex_unlock(&lock);

    // Signal all waiting consumers
    // printf("Producer signaling that production is done, waking up consumers...\n");
    fclose(file);       // Close the input file
    pthread_exit(NULL); // Exit the producer thread
}

/*Function for the consumer/table thread. There should be 4 of these. It is in charge of taking customers from the queue when the table is free

    Note that for the round-robin implementation, a student's eating_time_left may be larger than their time quantum.
    If this is the case, this thread should update eating_time_left, and add the student back to the end of the queue!
    Start the dining process and do busy waiting ( a while loop maybe) until time
    is up. In the FIFO scheduling algorithm which is non-preemptive, the student
    will sit at the table and eat until s/he is done, then the student will get out of
    the dining centre.

*/
void *table_function(void *arg)
{
    int table_id = *((int *)arg); // Get the table ID passed as an argument
    // printf("Consumer thread for table %d started...\n", table_id);

    while (true)
    {
        // Fetch customer from queue
        // printf("Consumer thread %d trying to fetch customer...\n", table_id);

        // No need to lock here, queue.get_customer handles it
        Customer c = queue.get_customer();

        // Exit condition if no more customers and producer is done
        if (c.id == 0 && done && qsize == 0)
        {
            printf("Consumer %d exiting, no more customers.\n", table_id);
            break;
        }

        // Capture the time the student is seated at a table
        c.sit_time = std::chrono::steady_clock::now();

        // Output student seating and Simulate eating
        printf("Sit <student ID: %d> at Table %d\n", c.id, table_id);
        printf("Customer <ID: %d> is eating for %d seconds at Table %d\n", c.id, c.eating_time_left, table_id);
        sleep(c.eating_time_left);

        // Calculate turnaround and wait times
        auto turnaround_time = std::chrono::duration_cast<std::chrono::seconds>(c.leave_time - c.arrival_time).count();
        auto wait_time = std::chrono::duration_cast<std::chrono::seconds>(c.sit_time - c.arrival_time).count();

        // Output leaving details
        printf("Leave <student ID: %d> Turnaround <%ld> Wait <%ld> at Table %d\n", c.id, turnaround_time, wait_time, table_id);

        // Update queue size safely is taken care by get_customer

        // Exit condition after updating the queue size
        if (done && qsize == 0)
        {
            printf("No more customers, consumer %d exiting...\n", table_id);
            break;
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    // check the the number of arguments are correct
    if (argc != 2)
    {
        printf("Usage: %s <input_file> is not provided\n", argv[0]);
        return 0; // Return 0 to indicate an error
    }

    // Open the input file for reading queue size
    file = fopen(argv[1], "r+");
    if (!file)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Read the file to determine queue size
    int arrival_time, eating_time_left;
    while (fscanf(file, "%i %i", &arrival_time, &eating_time_left) == 2)
    {
        qsize++;
    }
    fclose(file);

    // Reopen the file for actual processing
    file = fopen(argv[1], "r");
    if (!file)
    {
        perror("Error reopening file");
        exit(EXIT_FAILURE);
    }

    // Initialize the mutex and conditonal variable
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        perror("Error initializing mutex");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&has_customer, NULL) != 0)
    {
        perror("Error initializing cond variable");
        exit(EXIT_FAILURE);
    }

    // Create the producer thread
    pthread_t producer_thread;
    if (pthread_create(&producer_thread, NULL, producer_function, NULL) != 0)
    {
        perror("Error creating producer thread");
        exit(EXIT_FAILURE);
    }

    pthread_t table1, table2, table3, table4;
    int table_ids[] = {1, 2, 3, 4};

    if (pthread_create(&table1, NULL, table_function, (void *)&table_ids[0]) != 0 ||
        pthread_create(&table2, NULL, table_function, (void *)&table_ids[1]) != 0 ||
        pthread_create(&table3, NULL, table_function, (void *)&table_ids[2]) != 0 ||
        pthread_create(&table4, NULL, table_function, (void *)&table_ids[3]) != 0)
    {
        perror("Error creating consumer threads");
        exit(EXIT_FAILURE);
    }

    // Join all threads to ensure they finish properly
    printf("Waiting for threads to finish...\n");
    pthread_join(producer_thread, NULL);
    pthread_join(table1, NULL);
    pthread_join(table2, NULL);
    pthread_join(table3, NULL);
    pthread_join(table4, NULL);

    printf("Cleaning up resources...\n");
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&has_customer);

    printf("Program completed successfully.\n");

    return 0;
}