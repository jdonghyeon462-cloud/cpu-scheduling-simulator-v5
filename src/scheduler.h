#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>

#define MAX_PROCESSES 100
#define MAX_TIME 1000000LL
#define MAX_RR_SLICES 100000LL

typedef struct {
    int id;
    long long arrival_time;
    long long burst_time;
    long long start_time;
    long long completion_time;
    long long waiting_time;
    long long turnaround_time;
} Process;

typedef enum {
    ALGORITHM_FCFS,
    ALGORITHM_SJF
} Algorithm;

typedef struct {
    double average_waiting;
    double average_turnaround;
    long long completion_time;
    double average_response;
} ScheduleStats;

/* process_index=-1은 IDLE, 그 외 값은 원본 배열의 위치입니다. */
typedef struct {
    int process_index;
    long long start;
    long long end;
} ExecutionSlice;

typedef struct {
    Process processes[MAX_PROCESSES]; /* 입력 순서, start_time은 최초 실행 */
    ExecutionSlice *slices;
    size_t slice_count;
} RoundRobinResult;

typedef enum {
    RR_OK,
    RR_INVALID_INPUT,
    RR_TOO_MANY_SLICES,
    RR_NO_MEMORY
} RoundRobinStatus;

/* result는 {0}으로 초기화합니다. 성공 후 free_round_robin으로 해제합니다.
 * 재사용 전에도 해제해야 합니다. 입력과 결과는 별도 저장 공간입니다.
 * 실행 구간 끝까지 도착한 작업을 먼저 넣고 미완료 작업을 재삽입합니다. */
RoundRobinStatus simulate_round_robin(const Process input[], int count,
                                     long long quantum, RoundRobinResult *result);
void free_round_robin(RoundRobinResult *result);

/* input과 output은 각각 count개 이상의 원소를 가진 별도 배열이어야 합니다.
 * input은 변경하지 않으며 output은 실행 순서입니다.
 * 동률은 FCFS: 도착 -> 입력 순서, SJF: 실행 시간 -> 도착 -> 입력 순서.
 * 입력 범위 또는 알고리즘이 잘못되면 0, 성공하면 1을 반환합니다. */
int simulate_schedule(const Process input[], int count, Algorithm algorithm,
                      Process output[]);

/* 성공적으로 계산한 FCFS/SJF/RR 결과에 사용합니다. 순서와 무관합니다. */
ScheduleStats schedule_stats(const Process output[], int count);

#endif
