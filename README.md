# Labs-With-FreeRTOS
This repository includes many labs designed to help users to get
started with the FreeRTOS Kernel on curiosity 2.0 pic32 mz ef board. The repo 
is updating whenever the new  lab or the code snippet for testing is completed.

# Hardware use:
Curiority 2.0 pic32 mz ef board
# Software / Tool use:
* MPLAB X IDE v6.25.
* MPLAB Code Configurator (MCC) v5.6.2.
* XC32 Compiler v4.60
  
# Tutorials
## Tasks and Scheduling
* [Task Creation]
* [Static Allocation]
* [Task Priorities]
* [Task Parameters]
* [Scheduling Algorithm - Prioritized Preemptive Scheduling with Time Slicing](docs/tutorial_6/README.md)

## Queue
* [Using Queues for Inter-Task Communication]
* [Using Queues for Communication between Interrupts and Tasks]

## Semaphore and Mutex
* [Semaphore]
* [Mutex]

## Software Timer
* [Using software timer for interval timing]
* [Using software timer for handling debounce when key press]

## Priority Inversion and Priority Inheritance
* [Using static tasks to simulate the implementation of Priority Inversion and Priority Inheritance]

## Event Group
* [Using static tasks to learn implementation of event group on push button and DMA transfer]

## Event Group Synchronization
* [Using LED blinking to implement event group synchronization]

## Task Notification and Task Handle in Static Task
* [Task Notification]
* [Extra work: Refactor source code of the labs]

## Stream Buffer
* [Applied Stream Buffer on data transfer from UART6 input to UART1 output]
* [Unit Implementation and testing on UART1 and external USB-TTL 5V adaptor - applying polling technique]
* [Integration Test: UART1 as input and UART6 as output and then vice versa - applying polling technique]
* [Sytem Testing: the version of code is written for maximum portability across future modifications]
* [This Lab is also building a custom timer API layer on top of FreeRTOS. It is the first step of building system frameworks]
  
(continue)
