#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <queue>
#include <string>

#define PROCESS_NUM 10
#define QUANTUM 20  // time quantum (20ms) setting for RR scheduling
using namespace std;

// process(child) structer
typedef struct process {
    pid_t pid;
    float cpu_burst;
    float io_burst;
    bool interactive;
} PROCESS;

// PCB structer
typedef struct pcb {
    bool flag;
    float burst;
    bool interactive;
} PCB;

// MSG structer
typedef struct msg {
    long msgtype;  // Message type, must be > 0 with 'long' data type
    PCB pcb;       // Message data(PCB) to push in queue
} MSG;

int main() {
    // for process PID
    PROCESS child[PROCESS_NUM];
    int numofSwitching = 0;
    int status;
    /// if true -> interactive process, if not -> batch process
    bool interactive[PROCESS_NUM] = {false, false, false, true, true};

    printf("Parent Process ID: %d\n", getpid());

    for (int i = 0; i < PROCESS_NUM; i++) {
        pid_t forkProcess = fork();  // fork child process

        child[i].pid = forkProcess;
        child[i].cpu_burst = rand() % 30 + 10;  

        /// if interactive process
        if (interactive[i] == true) {
            child[i].interactive = true;
        }
        /// if batch process
        else {
            child[i].interactive = false;
        }

        // if Child Process
        if (forkProcess == 0) {
            int key_id;               // message id
            long msgtype = getpid();  // Message type as Child Process ID

            float cpu_burst = child[i].cpu_burst;

            // ftok to generate unique key
            key_t key = ftok("keyfile", 1234);
            MSG msg;                // Message
            msg.msgtype = msgtype;  // Message type as Child PID

            // creates a message queue and error handling
            key_id = msgget(key, IPC_CREAT | 0666);

            if (key_id == -1) {
                perror("msgget() error!");
                exit(1);  // unsuccessful termination
            }

            // Child Process Start!
            do {
                // Wait until receiving message from Parent Process
                if (msgrcv(key_id, &msg, sizeof(PCB), msgtype, 0) != -1) {
                    if (msg.pcb.interactive == true) {  // msg.pcb.interactive
                        // remaining CPU burst time is bigger than QUANTUM
                        if (cpu_burst > QUANTUM) {
                            cpu_burst -= QUANTUM;
                            sleep(QUANTUM);

                            msg.pcb.flag = true;  // CPU Burst Time remained
                            msg.pcb.burst = QUANTUM;

                            // send message to parent
                            msgsnd(key_id, &msg, sizeof(PCB), IPC_NOWAIT);
                        }

                        // remaining CPU burst time is smaller than QUANTUM
                        else {
                            cpu_burst = 0;
                            sleep(cpu_burst);

                            msg.pcb.flag = false;  // No CPU Burst Time remaining
                            msg.pcb.burst = cpu_burst;

                            msgsnd(key_id, &msg, sizeof(PCB), IPC_NOWAIT);
                        }
                    } else {
                        cpu_burst = 0;
                        sleep(cpu_burst);

                        msg.pcb.flag = false;  // No CPU Burst Time remaining
                        msg.pcb.burst = cpu_burst;

                        msgsnd(key_id, &msg, sizeof(PCB), IPC_NOWAIT);
                    }
                }
            } while (cpu_burst > 0);

            exit(0);  // child process successful termination
        }
    }

    // schedule log file open
    FILE *fp = fopen("Multi_level_schedule_dump.txt", "w");  // write mode (or append mode)

    // Create QUEUEs
    queue<long> RR_Queue;    /// QUEUE 1 as Round Robin (RR)
    queue<long> FCFS_Queue;  /// QUEUE 2 as First Come First Serve (FCFS)
    queue<long> runQueue;
    queue<long> Queue;
    float burst_time[PROCESS_NUM];
    float completion_time[PROCESS_NUM];
    float turnaround_time = 0;
    int priority[PROCESS_NUM];

    printf("\n[Run Status Process ENQUEUE]\n\n");
    fprintf(fp, "\n[Run Status Process ENQUEUE]\n\n");
    for (int i = 0; i < PROCESS_NUM; i++) {
        /// if interactive process
        if (interactive[i] == true) {
            RR_Queue.push(i);
            priority[i] = 1;
            runQueue.push(i);
        }
        /// if batch process
        else {
            FCFS_Queue.push(i);
            priority[i] = 2;
            runQueue.push(i);
        }

        printf("PID: \t%d\n", child[i].pid);
        printf("Arrival Order: \t%d\n", i + 1);  // Arrival time
        printf("Burst Time: \t%.2lf\n", child[i].cpu_burst);
        /// if interactive process
        if (interactive[i] == true) {
            printf("Priority: \t1\n\n");
        }
        /// if batch process
        else {
            printf("Priority: \t2\n\n");
        }
        fprintf(fp, "PID: \t%d\n", child[i].pid);
        fprintf(fp, "Arrival Order: \t%d\n", i + 1);
        fprintf(fp, "Burst Time: \t%.2lf\n\n", child[i].cpu_burst);
        /// if interactive process
        if (interactive[i] == true) {
            fprintf(fp, "Priority: \t1\n\n");
        }
        /// if batch process
        else {
            fprintf(fp, "Priority: \t2\n\n");
        }

        burst_time[i] = child[i].cpu_burst;
    }

    // ftok to generate unique key
    key_t key = ftok("keyfile", 1234);
    int key_id;
    MSG msg;

    // 메세지 생성
    key_id = msgget(key, IPC_CREAT | 0666);
    if (key_id == -1) {
        perror("msgget() error!\n");
        exit(1);  // unsuccessful termination
    }

    printf("\n[Process Execution Flow]\n");
    fprintf(fp, "\n[Process Execution Flow]\n");
    // Parent Process Start!
    do {
        /// runQueue 순회 출력
        printf("\nRun QUEUE: ");
        while (!runQueue.empty()) {
            long front = runQueue.front();
            printf(">>> %ld ", front + 1);
            runQueue.pop();
            Queue.push(front);
        }
        while (!Queue.empty()) {
            long front = Queue.front();
            Queue.pop();
            runQueue.push(front);
        }
        /// RR Queue 순회 출력
        printf("\nRR QUEUE: ");
        while (!RR_Queue.empty()) {
            long front = RR_Queue.front();
            printf(">>> %ld ", front + 1);
            RR_Queue.pop();
            Queue.push(front);
        }
        while (!Queue.empty()) {
            long front = Queue.front();
            Queue.pop();
            RR_Queue.push(front);
        }
        /// FCFS Queue 순회 출력
        printf("\nFCFS QUEUE: ");
        while (!FCFS_Queue.empty()) {
            long front = FCFS_Queue.front();
            printf(">>> %ld ", front + 1);
            FCFS_Queue.pop();
            Queue.push(front);
        }
        while (!Queue.empty()) {
            long front = Queue.front();
            Queue.pop();
            FCFS_Queue.push(front);
        }

        numofSwitching++;
        printf("\n>>> Context Switch #%d -> ", numofSwitching);  // Number of Context Switching
        fprintf(fp, "\n>>> Context Switch #%d -> ", numofSwitching);

        long run = runQueue.front();  // run status process

        /// if priority = 1, RR Queue DEQUEUE
        if (priority[run] == 1 && run == RR_Queue.front()) {
            runQueue.pop();  // runQueue DEQUEUE
            RR_Queue.pop();
        } else {
            runQueue.pop();  // runQueue DEQUEUE
            FCFS_Queue.pop();
        }

        printf("Running Process: P%ld - PID[%d] - Burst time[%.2lf]\n", run + 1, child[run].pid, child[run].cpu_burst);
        fprintf(fp, "Running Process: P%ld - PID[%d] - Burst time[%.2lf]\n", run + 1, child[run].pid, child[run].cpu_burst);

        msg.msgtype = child[run].pid;  // msgtype: Child PID

        /// if priority = 1, msg -> interactive is true
        if (priority[run] == 1) {
            msg.pcb.interactive = true;
        } else {
            msg.pcb.interactive = false;
        }

        // send message to child
        msgsnd(key_id, &msg, sizeof(PCB), IPC_NOWAIT);

        // if message received from child
        if (msgrcv(key_id, &msg, sizeof(PCB), child[run].pid, 0) != -1) {
            /// if interactive process
            if (child[run].interactive == true) {
                printf("Interactve process!\n");
                /// RR Logic
                // if burst time remaining, enqueue
                if (msg.pcb.flag == true) {
                    if (child[run].cpu_burst > 0) {
                        runQueue.push(run);
                        RR_Queue.push(run);
                        child[run].cpu_burst -= QUANTUM;
                        turnaround_time += QUANTUM;
                    }
                }
                // No burst time remaining
                else {
                    turnaround_time += child[run].cpu_burst;
                    child[run].cpu_burst = 0;
                    completion_time[run] = turnaround_time;
                }

                // if burst time is over
                if (child[run].cpu_burst <= 0) {
                    turnaround_time += (child[run].cpu_burst);  /// removed QUANTUM
                    child[run].cpu_burst = 0;
                    completion_time[run] = turnaround_time;
                }
            }

            /// if batch process
            else {
                printf("Batch process!\n");
                /// FCFS Logic
                /// if RR Queue is not empty
                if (!RR_Queue.empty()) {
                    FCFS_Queue.push(run);
                    runQueue.push(run);
                }
                /// if RR Queue is empty
                else {
                    // if burst time remaining, enqueue
                    if (msg.pcb.flag == true) {
                        if (child[run].cpu_burst > 0) {
                            turnaround_time += child[run].cpu_burst;
                        }
                    }
                    // No burst time remaining
                    else {
                        turnaround_time += child[run].cpu_burst;
                        child[run].cpu_burst = 0;
                        completion_time[run] = turnaround_time;
                    }
                }
            }
        }
    } while (!runQueue.empty());

    msgsnd(key_id, &msg, sizeof(PCB), IPC_NOWAIT);  // sending last message to child process
    wait(&status);                                  // wait for all the child process to terminate

    printf("\nRUN QUEUE Empty!\n");
    fprintf(fp, "\nRUN QUEUE Empty!\n");

    float sumofCompletiontime = 0.0;
    float sumofWaitingtime = 0.0;

    float minCompletiontime = 123456789.0;
    float maxCompletiontime = 0.0;
    float minWaitingtime = 123456789.0;
    float maxWaitingtime = 0.0;

    printf("\n\n[Multi-level Queue Scheduler]\n\n");
    printf("PID\t\tBurst Time\tCompletion Time\t\tWaiting Time\t\tPriority\n\n");
    fprintf(fp, "\n\n[Multi-level Queue Scheduler]\n\n");
    fprintf(fp, "PID\t\tBurst Time\tCompletion Time\t\tWaiting Time\tPriority\n\n");

    // Calculating Min, Max, Sum of Waiting and Completion time
    for (int i = 0; i < PROCESS_NUM; i++) {
        printf("%d\t\t%.2lf\t\t%.2lf\t\t\t%.2lf\t\t\t%d\n\n", child[i].pid, burst_time[i], completion_time[i], completion_time[i] - burst_time[i], priority[i]);
        fprintf(fp, "%d\t\t%.2lf\t\t%.2lf\t\t\t%.2lf\t\t\t%d\n\n", child[i].pid, burst_time[i], completion_time[i], completion_time[i] - burst_time[i], priority[i]);

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