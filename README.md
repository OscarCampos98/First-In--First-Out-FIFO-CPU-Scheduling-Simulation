# First-In--First-Out-FIFO-CPU-Scheduling-Simulation

# Overview
This program simulates a First-In-First-Out (FIFO) scheduling algorithm applied to a dining center scenario, where students
arrive at different times and each needs a specific amount of time to eat. There are a fixed number of tables (four in this case),
and the program assigns students to tables in the order they arrive.

The program uses multithreading to simulate tables as separate threads, each processing one student at a time.
The program is designed to run on Linux environments, specifically tested using WSL Ubuntu.

# Description
The program reads a file containing arrival times and eating durations for students. It then schedules students using the
FIFO scheduling algorithm, which ensures that students are seated and served in the order they arrive. 
The simulation logs when students arrive, are seated, and leave, along with additional details such as
turnaround time and wait time.

The FIFO algorithm guarantees that:
    - The first student to arrive is the first to be seated.
    - Once seated, a student is allowed to finish eating without interruptions.
The simulation also logs thread activities such as customer assignment to tables, the queue size at different points,
and customer departures.

# Implementation Details

Multithreading
The program employs multithreading using pthread to simulate multiple tables in the dining center.
Four threads are created to represent four tables. These threads serve customers based on their arrival order.
The main thread (producer) reads the input file and adds students to the queue.
The consumer threads (tables) fetch students from the queue and simulate them eating.

Synchronization
Mutex locks (pthread_mutex_lock) ensure that only one thread at a time can access or modify the student queue.
Condition variables (pthread_cond_wait, pthread_cond_signal) are used to coordinate between the producer
(which adds students) and the consumers (which process students). This ensures that threads fetch customers 
only when the queue is not empty.

Time Management
The program uses std::chrono::steady_clock to track when students arrive, sit, and leave. This provides high-precision timing.
Each student’s turnaround time (time from arrival to departure) and wait time (time spent waiting for a table) are
calculated and logged.

# Running the Program

To compile the program, make sure you are in a Linux environment or using WSL Ubuntu. Run the following command:
g++ fifo.cpp -o fifo -pthread

Running the Program
./fifo <input_file.txt>
Where <input_file.txt> is the path to the test file containing student arrival times and eating durations.

# Input File Format
Each test file contains:

  - The quantum value on the first line.
  - Subsequent lines with two integers:
  - Arrival time (in seconds): The time when the student arrives.
  - Eating time (in seconds): The total time the student needs to eat.

Example Input:

    3
    0 3
    0 3
    0 3
    0 3
    4 6

# Output Format
The program logs various events during the simulation, including:

  - Student arrival: When a student is added to the queue.
  - Student seating: When a student is seated at a table.
  - Student departure: When a student finishes eating and leaves the dining center.
  - Queue updates: When students are added or fetched from the queue.

Example Output:

    Added customer <student ID: 1>, queue size: 1
    Sit <student ID: 1> at Table 1
    Customer <ID: 1> is eating for 3 seconds at Table 1
    Added customer <student ID: 2>, queue size: 1
    Sit <student ID: 2> at Table 2
    Customer <ID: 2> is eating for 3 seconds at Table 2
    ...
    Leave <student ID: 1> Turnaround <-20784> Wait <0> at Table 1
    Leave <student ID: 2> Turnaround <-20784> Wait <0> at Table 2

# Test Files
The folder of test files contains various scenarios with the same format:
    - A single quantum value followed by arrival and eating times.
These files allow you to test different cases for student arrival sequences and varying eating durations.

# Platform
This program has been tested on WSL Ubuntu, a Windows Subsystem for Linux (WSL) environment. It should work on other Linux-based environments as well.

# Potential Enhancements
Adding colored output for easier readability (optional).
Implementing more complex scheduling algorithms such as round-robin or priority scheduling.
Visualizing the table assignments using a graphical interface.
