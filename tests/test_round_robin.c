#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks;
#define CHECK(c) do { ++checks; if (!(c)) { \
    fprintf(stderr, "FAIL at line %d: %s\n", __LINE__, #c); \
    return EXIT_FAILURE; } } while (0)

int main(void)
{
    Process input[3] = {
        {.id = 30, .arrival_time = 0, .burst_time = 5},
        {.id = 10, .arrival_time = 1, .burst_time = 3},
        {.id = 20, .arrival_time = 2, .burst_time = 1}
    };
    Process snapshot[3];
    Process jobs[MAX_PROCESSES] = {0};
    Process fcfs[MAX_PROCESSES];
    Process bad = {.id = 1, .arrival_time = 0, .burst_time = 1};
    RoundRobinResult result = {0};
    ScheduleStats stats;
    const int order[] = {0, 1, 2, 0, 1, 0};
    const long long ends[] = {2, 4, 5, 7, 8, 9};
    int i;

    memcpy(snapshot, input, sizeof(input));
    CHECK(simulate_round_robin(input, 3, 2, &result) == RR_OK);
    CHECK(memcmp(snapshot, input, sizeof(input)) == 0);
    CHECK(result.slice_count == 6);
    for (i = 0; i < 6; ++i) {
        CHECK(result.slices[i].process_index == order[i] && result.slices[i].end == ends[i]);
    }
    CHECK(result.processes[0].waiting_time == 4 && result.processes[0].start_time == 0);
    stats = schedule_stats(result.processes, 3);
    CHECK(stats.completion_time == 9); /* 마지막 입력 P3는 시각 5에 이미 완료 */
    CHECK(stats.average_response == 1.0);
    CHECK(simulate_round_robin(input, 3, 2, &result) == RR_INVALID_INPUT);
    free_round_robin(&result);
    CHECK(result.slices == NULL && result.slice_count == 0);
    free_round_robin(&result);

    CHECK(simulate_round_robin(input, 3, MAX_TIME, &result) == RR_OK);
    CHECK(simulate_schedule(input, 3, ALGORITHM_FCFS, fcfs));
    for (i = 0; i < 3; ++i) {
        CHECK(fcfs[i].id == result.processes[i].id &&
              fcfs[i].start_time == result.processes[i].start_time &&
              fcfs[i].completion_time == result.processes[i].completion_time);
    }
    free_round_robin(&result);

    /* 100개가 동시에 준비되어 큐의 모든 칸을 쓰고 두 바퀴 순환합니다. */
    for (i = 0; i < MAX_PROCESSES; ++i) {
        jobs[i].id = MAX_PROCESSES - i; /* ID 크기와 무관한 입력 순서 */
        jobs[i].burst_time = 2;
    }
    CHECK(simulate_round_robin(jobs, MAX_PROCESSES, 1, &result) == RR_OK);
    CHECK(result.slice_count == 200);
    for (i = 0; i < 200; ++i) {
        if (result.slices[i].process_index != i % MAX_PROCESSES) {
            fprintf(stderr, "Queue wraparound order mismatch.\n");
            return EXIT_FAILURE;
        }
    }
    CHECK(result.processes[99].completion_time == 200);
    stats = schedule_stats(result.processes, MAX_PROCESSES);
    CHECK(stats.average_waiting == 148.5 && stats.average_response == 49.5);
    free_round_robin(&result);

    bad.burst_time = MAX_RR_SLICES;
    CHECK(simulate_round_robin(&bad, 1, 1, &result) == RR_OK);
    CHECK(result.slice_count == (size_t)MAX_RR_SLICES);
    CHECK(result.processes[0].waiting_time == 0);
    free_round_robin(&result);
    bad.burst_time++;
    CHECK(simulate_round_robin(&bad, 1, 1, &result) == RR_TOO_MANY_SLICES);
    CHECK(result.slices == NULL);

    for (i = 0; i < MAX_PROCESSES; ++i) {
        jobs[i].arrival_time = MAX_TIME;
        jobs[i].burst_time = MAX_TIME;
    }
    CHECK(simulate_round_robin(jobs, MAX_PROCESSES, MAX_TIME, &result) == RR_OK);
    stats = schedule_stats(result.processes, MAX_PROCESSES);
    CHECK(stats.completion_time == 101000000LL);
    CHECK(stats.average_waiting == 49500000.0 && stats.average_turnaround == 50500000.0);
    free_round_robin(&result);
    CHECK(simulate_round_robin(jobs, MAX_PROCESSES, 1, &result) == RR_TOO_MANY_SLICES);

    CHECK(simulate_round_robin(NULL, 1, 2, &result) == RR_INVALID_INPUT);
    CHECK(simulate_round_robin(input, 3, 2, NULL) == RR_INVALID_INPUT);
    CHECK(simulate_round_robin(input, 0, 2, &result) == RR_INVALID_INPUT);
    CHECK(simulate_round_robin(input, MAX_PROCESSES + 1, 2, &result) == RR_INVALID_INPUT);
    CHECK(simulate_round_robin(input, 3, 0, &result) == RR_INVALID_INPUT);
    CHECK(simulate_round_robin(input, 3, -1, &result) == RR_INVALID_INPUT);
    CHECK(simulate_round_robin(input, 3, MAX_TIME + 1, &result) == RR_INVALID_INPUT);
    bad.arrival_time = -1;
    CHECK(simulate_round_robin(&bad, 1, 2, &result) == RR_INVALID_INPUT);
    bad.arrival_time = MAX_TIME + 1;
    CHECK(simulate_round_robin(&bad, 1, 2, &result) == RR_INVALID_INPUT);
    bad.arrival_time = 0;
    bad.burst_time = 0;
    CHECK(simulate_round_robin(&bad, 1, 2, &result) == RR_INVALID_INPUT);
    bad.burst_time = MAX_TIME + 1;
    CHECK(simulate_round_robin(&bad, 1, 2, &result) == RR_INVALID_INPUT);
    printf("PASS: %d Round Robin C checks.\n", checks);
    return EXIT_SUCCESS;
}
