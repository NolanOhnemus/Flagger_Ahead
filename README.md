# Simulated Construction Zone Traffic Management

## Overview

This project demonstrates the implementation of a multi-threaded traffic simulation, inspired by real-world construction scenarios where a two-way road is temporarily reduced to a single lane. The system models synchronized traffic control using threads, mutexes, and semaphores to manage concurrent vehicle flow, simulating realistic driving patterns and coordination.

## Problem Context

In many construction scenarios, only one lane of a two-way road is available. Traffic must be alternately allowed from either side, coordinated by flaggers. The project replicates this situation in software, requiring precise timing, safe concurrency, and resource management. Each vehicle is represented by a thread, with additional synchronization logic ensuring vehicles pass safely through the shared construction zone without conflicts.

## Key Features

- **Multi-threaded Design**: Each car and the flagger logic run as independent threads.
- **Synchronized Traffic Flow**: Only one direction is active at a time, with a controlled switch based on timers and construction zone state.
- **Capacity Management**: Limits the number of cars in the construction zone at any time.
- **Dynamic Scheduling**: Cars may have different wait and crossing times based on simulation input.
- **Real-time Logging**: Each action (entering zone, direction change) is timestamped and logged.
- **Wait Time Tracking**: The program measures and outputs the actual wait time each vehicle experienced.

## Simulation Parameters

The simulation is driven by an input file passed via the command line:

```bash
./flagger sample_input.txt
```

### Input Format

The input file defines:
1. Initial car distribution (left and right side)
2. Crossing duration
3. Time interval before traffic direction switches
4. Construction zone capacity
5. Driving behavior for each car (number of trips and rest interval)

Example:
```
4 3 20 100 3
10 200
10 50
8 300
2 100
20 40
10 50
30 10
```

This specifies:
- 4 cars start on the left, 3 on the right
- Each crossing takes 20μs
- Direction switches every 100μs
- A maximum of 3 cars can be in the zone at once
- Each car has its own crossing count and rest time between trips

## Technical Implementation

- **Threading**: Implemented with POSIX threads (pthreads)
- **Synchronization Tools**: Mutexes and condition variables ensure safe access to shared resources
- **Control Flow**:
  - Initial direction: left to right
  - Traffic direction changes only after the construction zone is empty
  - Wait time is calculated per car (excluding rest/sleep intervals)

## Output and Logging

- Logs the car ID, crossing direction, and remaining trips at each crossing event
- Logs every traffic direction switch
- Prints total wait time per car at the end of the simulation

## Build Instructions

Navigate to the `src/` directory and run:

```bash
make
```

All necessary executables should be compiled using gdb.
