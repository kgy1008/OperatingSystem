#define _CRT_SECURE_NO_WARNINGS
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>  // for rand and exit func
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>  // for wait func
#include <time.h>      // for time func
#include <unistd.h>    // for fork func

#include <algorithm>  // for min/max func
#include <deque>
#include <iostream>
#include <queue>   // for C++ STL Queue
#include <string>  // for Message queue

#define PROCESS_NUM 10
#define QUANTUM 20                // time quantum default setting (20ms)
#define IO_BURST_PROBABILITY 0.5  // Set your desired probability here

using namespace std;

// Define I/O burst structure
typedef struct io_burst {
    float start_time;
    float duration;
} IO_BURST;

// process(child) structer
typedef struct process {
    pid_t pid;
    float cpu_burst;
    float io_burst;
    float random_number;
    std::vector<IO_BURST> io_bursts;  // Vector of I/O bursts
} PROCESS;

// PCB structer
typedef struct pcb {
    pid_t pid;
    bool flag;
    float burst;
    float io_burst;
} PCB;

// MSG structer
typedef struct msg {
    long msgtype;  // Message type, must be > 0 with 'long' data type
    PCB pcb;       // Message data(PCB) to push in queue
} MSG;

int main() {
    // array for process PID
    PROCESS child[PROCESS_NUM];
    pid_t parentPID = getpid();  // parent process PID
    pid_t pid;
    int status;
    int numofSwitching = 0;
    // schedule log file open
    FILE *fp = fopen("schedule_dump.txt", "w");  // write mode (or append mode)

    printf("Parent Process ID: %d\n", getpid());
    fprintf(fp, "Parent Process ID: %d\n", getpid());

    for (int i = 0; i < PROCESS_NUM; i++) {
        pid = fork();  // fork child process

        child[i].pid = pid;
        child[i].cpu_burst = rand() % 30 + 10;
        // Determine if I/O burst is created based on the probability
        child[i].random_number = ((float)rand()) / RAND_MAX;
        if (child[i].random_number <= IO_BURST_PROBABILITY) {
            child[i].io_burst = rand() % 20 + 5;
            int numIO = rand() % 5;  // Random number of I/O bursts for each process
            for (int j = 0; j < numIO; ++j) {
                IO_BURST io;
                io.start_time = static_cast<float>(rand() % static_cast<int>(child[i].cpu_burst));
                io.duration = static_cast<float>(rand() % static_cast<int>(child[i].cpu_burst - io.start_time));
                child[i].io_bursts.push_back(io);
            }
        } else {
            child[i].io_burst = 0;
        }

        if (pid == 0) {  // if Child Process!

            int key_id;               // message id
            long msgtype = getpid();  // Message type as Child Process ID
            float cpu_burst = child[i].cpu_burst;
            float IO_burst = child[i].io_burst;

            MSG msg;                // Message
            msg.msgtype = msgtype;  // Message type as Child PID

            // creates a message queue and error handling
            key_id = msgget((key_t)1234, IPC_CREAT | 0666);

            if (key_id == -1) {
                perror("msgget() error!");
                exit(1);  // unsuccessful termination
            }

            // Child Process Start!
            do {
                // Wait until receiving message from Parent Process
                if (msgrcv(key_id, &msg, sizeof(PCB), msgtype, 0) != -1) {
                    // remaining CPU burst time and I/O burst time are bigger than QUANTUM
                    if (cpu_burst > QUANTUM) {
                        cpu_burst -= QUANTUM;
                        sleep(QUANTUM);
                        msg.pcb.flag = true;  // CPU Burst Time and I/O Burst Time remained
                        msg.pcb.burst = QUANTUM;
                        msg.pcb.io_burst = IO_burst;
                        msg.pcb.pid = msgtype;
                        // send message to parent
                        msgsnd(key_id, &msg, sizeof(PCB), IPC_NOWAIT);
                    }
                    // remaining CPU burst time and I/O burst time are smaller than QUANTUM
                    else {
                        cpu_burst = 0;
                        sleep(cpu_burst);
                        msg.pcb.flag = false;  // No CPU Burst Time and I/O Burst Time remaining
                        msg.pcb.burst = cpu_burst;
                        msg.pcb.io_burst = IO_burst;
                        msg.pcb.pid = msgtype;
                        msgsnd(key_id, &msg, sizeof(PCB), IPC_NOWAIT);
                    }
                }
            } while (cpu_burst > 0);
            exit(0);  // child process successful termination
        }
    }

    // Create QUEUE
    std::deque<long> readyQueue;  // Run QUEUE
    std::deque<long> io_queue;    // I/O QUEUE

    float burst_time[PROCESS_NUM];
    float IO_burst_time[PROCESS_NUM];
    float completion_time[PROCESS_NUM];
    float turnaround_time = 0;  // turnaround time

    printf("\n[Run Status Process ENQUEUE]\n\n");
    fprintf(fp, "[Run Status Process ENQUEUE]\n\n");
    for (int i = 0; i < PROCESS_NUM; i++) {
        readyQueue.push_back(i);  // ENQUEUE

        printf("< Process #%d | PID: %d >\n", i + 1, child[i].pid);
        printf("CPU Burst Time:    %.2lf\n", child[i].cpu_burst);
        printf("I/O Burst Time:    %.2lf\n\n", child[i].io_burst);

        fprintf(fp, "< Process #%d | PID: %d >\n", i + 1, child[i].pid);
        fprintf(fp, "CPU Burst Time:    %.2lf\n", child[i].cpu_burst);
        fprintf(fp, "I/O Burst Time:    %.2lf\n\n", child[i].io_burst);

        burst_time[i] = child[i].cpu_burst;
        IO_burst_time[i] = child[i].io_burst;
    }

    int key_id;
    MSG msg;

    //  Create Message
    key_id = msgget((key_t)1234, IPC_CREAT | 0666);
    if (key_id == -1) {
        perror("msgget() error!\n");
        exit(1);  // unsuccessful termination
    }

    // Parent Process Start!
    printf("\n[Process Execution Flow]\n");
    fprintf(fp, "\n[Process Execution Flow]\n");

    do {
        numofSwitching++;
        printf("\n>>> Context Switch #%d\n ", numofSwitching);  // Number of Context Switching
        fprintf(fp, "\n>>> Context Switch #%d\n ", numofSwitching);

        printf("Ready Queue: ");
        fprintf(fp, "Ready Queue: ");
        for (long &i : readyQueue) {
            printf("P%ld ", i + 1);
            fprintf(fp, "P%ld ", i + 1);
        }

        long run = readyQueue.front();  // run status process
        readyQueue.pop_front();         // readyQueue DEQUEUE

        printf("\n Running Process: P%ld | PID[%d] | Remaining CPU Burst time[%.2lf] (-> %.0f) | Remaining I/O Burst time[%.2lf] (-> %.0f)\n", run + 1, child[run].pid, child[run].cpu_burst,
               (child[run].cpu_burst - QUANTUM >= 0) ? child[run].cpu_burst - QUANTUM : 0, child[run].io_burst, (child[run].io_burst - QUANTUM >= 0) ? child[run].io_burst - QUANTUM : 0);
        fprintf(fp, "\n Running Process: P%ld | PID[%d] | Remaining CPU Burst time[%.2lf] (-> %.0f) | Remaining I/O Burst time[%.2lf] (-> %.0f)\n", run + 1, child[run].pid, child[run].cpu_burst,
                (child[run].cpu_burst - QUANTUM >= 0) ? child[run].cpu_burst - QUANTUM : 0, child[run].io_burst, (child[run].io_burst - QUANTUM >= 0) ? child[run].io_burst - QUANTUM : 0);

        msg.msgtype = child[run].pid;  // msgtype: Child PID

        // send message to child
        msg.msgtype = child[run].pid;
        msgsnd(key_id, &msg, sizeof(PCB), IPC_NOWAIT);

        // if message received from child
        if (msgrcv(key_id, &msg, sizeof(PCB), child[run].pid, 0) != -1) {
            // if burst time remaining, enqueue
            if (msg.pcb.flag == true) {
                if (child[run].cpu_burst > 0) {
                    readyQueue.push_back(run);
                    child[run].cpu_burst -= QUANTUM;
                    turnaround_time += QUANTUM;
                }
            }
            // No burst time remaining
            else {
                turnaround_time += child[run].cpu_burst;
                child[run].cpu_burst = 0;
                completion_time[run] = turnaround_time;

                if (child[run].random_number <= IO_BURST_PROBABILITY) {
                    // Send Message to Parent Process to start I/O
                    printf(" ***** [%d] CPU burst reaches to zero, do I/O. *****\n", child[run].pid);
                    fprintf(fp, " ***** [%d] CPU burst reaches to zero, do I/O. *****\n", child[run].pid);
                    msg.msgtype = parentPID;
                    msg.pcb.io_burst = child[run].io_burst;
                    msg.pcb.pid = child[run].pid;
                    msgsnd(key_id, &msg, sizeof(PCB), IPC_NOWAIT);

                    // If Parent Process receives Message
                    if (msgrcv(key_id, &msg, sizeof(PCB), parentPID, 0) != -1) {
                        // If Current Child Process has I/O burst value
                        if (msg.pcb.io_burst > 0) {
                            io_queue.push_back(run);
                        }

                        for (auto it = io_queue.begin(); it != io_queue.end();) {
                            child[*it].io_burst -= QUANTUM;
                            printf(" I/O Queue: ");
                            fprintf(fp, " I/O Queue: ");

                            for (long &i : io_queue) {
                                printf("P%ld ", i + 1);
                                fprintf(fp, "P%ld ", i + 1);
                            }

                            if (child[*it].io_burst <= 0) {
                                child[*it].io_burst = 0;
                                printf("\n ***** [%d] I/O burst reaches to zero, end process. *****\n", child[*it].pid);
                                fprintf(fp, "\n ***** [%d] I/O burst reaches to zero, end process. *****\n", child[*it].pid);
                                it = io_queue.erase(it);
                            } else {
                                ++it;
                            }
                        }
                        printf("\n");
                        fprintf(fp, "\n");
                    }
                }
            }
            if (child[run].cpu_burst <= 0) {
                turnaround_time += child[run].cpu_burst;
                child[run].cpu_burst = 0;
                completion_time[run] = turnaround_time;
            }
        }
    } while (!readyQueue.empty());

    msgsnd(key_id, &msg, sizeof(PCB), IPC_NOWAIT);  // sending last message to child process
    wait(&status);                                  // wait for all the child process to terminate

    printf("\n RUN QUEUE Empty!\n");
    fprintf(fp, "\n RUN QUEUE Empty!\n");

    float sumofCompletiontime = 0.0;
    float sumofWaitingtime = 0.0;

    float minCompletiontime = 123456789.0;
    float maxCompletiontime = 0.0;
    float minWaitingtime = 123456789.0;
    float maxWaitingtime = 0.0;

    printf("\n\n[Round Robin Scheduler]\n\n");
    printf("PID\t\tBurst Time\tCompletion Time\t\tWaiting Time\n\n");
    fprintf(fp, "\n\n[Round Robin Scheduler]\n\n");
    fprintf(fp, "PID\t\tBurst Time\tCompletion Time\t\tWaiting Time\n\n");

    // Calculating Min, Max, Sum of Waiting and Completion time
    for (int i = 0; i < PROCESS_NUM; i++) {
        printf("%d\t\t%.2lf\t\t%.2lf\t\t\t%.2lf\n\n", child[i].pid, burst_time[i], completion_time[i], completion_time[i] - burst_time[i]);
        fprintf(fp, "%d\t\t%.2lf\t\t%.2lf\t\t\t%.2lf\n\n", child[i].pid, burst_time[i], completion_time[i], completion_time[i] - burst_time[i]);
        sumofCompletiontime += completion_time[i];
        sumofWaitingtime += (completion_time[i] - burst_time[i]);
        minCompletiontime = min(minCompletiontime, completion_time[i]);
        maxCompletiontime = max(maxCompletiontime, completion_time[i]);
        minWaitingtime = min(minWaitingtime, completion_time[i] - burst_time[i]);
        maxWaitingtime = max(maxWaitingtime, completion_time[i] - burst_time[i]);
    }

    // Min, Max, Average Completion time
    printf(">> Min Completion Time : %.2f\n", minCompletiontime);
    printf(">> MAX Completion Time : %.2f\n", maxCompletiontime);
    printf(">> Average Completion Time : %.2f\n\n", sumofCompletiontime / PROCESS_NUM);
    fprintf(fp, ">> Min Completion Time : %.2f\n", minCompletiontime);
    fprintf(fp, ">> MAX Completion Time : %.2f\n", maxCompletiontime);
    fprintf(fp, ">> Average Completion Time : %.2f\n\n", sumofCompletiontime / PROCESS_NUM);

    // Min, Max, Average Waiting time
    printf(">> Min Waiting Time : %.2f\n", minWaitingtime);
    printf(">> MAX Waiting Time : %.2f\n", maxWaitingtime);
    printf(">> Average Waiting Time : %.2f\n\n", sumofWaitingtime / PROCESS_NUM);
    fprintf(fp, ">> Min Waiting Time : %.2f\n", minWaitingtime);
    fprintf(fp, ">> MAX Waiting Time : %.2f\n", maxWaitingtime);
    fprintf(fp, ">> Average Waiting Time : %.2f\n\n", sumofWaitingtime / PROCESS_NUM);

    fclose(fp);
    return 0;
}