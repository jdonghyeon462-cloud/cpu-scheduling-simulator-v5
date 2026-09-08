#include "scheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks = 0;

#define CHECK(condition) do { \
    ++checks; \
    if (!(condition)) { \
        fprintf(stderr, "FAIL at line %d: %s\n", __LINE__, #condition); \
        return EXIT_FAILURE; \
    } \
} while (0)

int main(void)
{
    Process input[4] = {
        {.id = 40, .arrival_time = 0, .burst_time = 5},
        {.id = 20, .arrival_time = 2, .burst_time = 2},
        {.id = 30, .arrival_time = 1, .burst_time = 2},
        {.id = 10, .arrival_time = 1, .burst_time = 2}
    };
    Process snapshot[4];
    Process fcfs[MAX_PROCESSES];
    Process sjf[MAX_PROCESSES];
    Process again[MAX_PROCESSES];
    Process maximum[MAX_PROCESSES] = {0};
    Process invalid = {.id = 1, .arrival_time = 0, .burst_time = 1};
    ScheduleStats stats;
    int i;

    memcpy(snapshot, input, sizeof(input));
    CHECK(simulate_schedule(input, 4, ALGORITHM_FCFS, fcfs));
    CHECK(memcmp(input, snapshot, sizeof(input)) == 0);
    CHECK(simulate_schedule(input, 4, ALGORITHM_SJF, sjf));
    CHECK(memcmp(input, snapshot, sizeof(input)) == 0);
    CHECK(sjf[0].id == 40 && sjf[1].id == 30 &&
          sjf[2].id == 10 && sjf[3].id == 20);
    CHECK(fcfs[0].id == 40 && fcfs[1].id == 30 &&
          fcfs[2].id == 10 && fcfs[3].id == 20);

    /* SJF 다음에 FCFS를 다시 실행해도 입력 순서가 오염되지 않아야 합니다. */
    CHECK(simulate_schedule(input, 4, ALGORITHM_FCFS, again));
    for (i = 0; i < 4; ++i) {
        CHECK(again[i].id == fcfs[i].id &&
              again[i].start_time == fcfs[i].start_time);
        CHECK(sjf[i].start_time >= sjf[i].arrival_time);
        CHECK(sjf[i].completion_time == sjf[i].start_time + sjf[i].burst_time);
        CHECK(sjf[i].turnaround_time == sjf[i].waiting_time + sjf[i].burst_time);
    }

    CHECK(!simulate_schedule(NULL, 1, ALGORITHM_FCFS, fcfs));
    CHECK(!simulate_schedule(input, 1, ALGORITHM_SJF, NULL));
    CHECK(!simulate_schedule(input, 0, ALGORITHM_FCFS, fcfs));
    CHECK(!simulate_schedule(input, MAX_PROCESSES + 1, ALGORITHM_FCFS, fcfs));
    CHECK(!simulate_schedule(input, 1, (Algorithm)99, fcfs));
    invalid.arrival_time = -1;
    CHECK(!simulate_schedule(&invalid, 1, ALGORITHM_SJF, sjf));
    invalid.arrival_time = MAX_TIME + 1;
    CHECK(!simulate_schedule(&invalid, 1, ALGORITHM_SJF, sjf));
    invalid.arrival_time = 0;
    invalid.burst_time = 0;
    CHECK(!simulate_schedule(&invalid, 1, ALGORITHM_FCFS, fcfs));
    invalid.burst_time = MAX_TIME + 1;
    CHECK(!simulate_schedule(&invalid, 1, ALGORITHM_FCFS, fcfs));

    for (i = 0; i < MAX_PROCESSES; ++i) {
        maximum[i].id = i + 1;
        maximum[i].arrival_time = MAX_TIME;
        maximum[i].burst_time = MAX_TIME;
    }
    CHECK(simulate_schedule(maximum, MAX_PROCESSES, ALGORITHM_SJF, sjf));
    CHECK(sjf[MAX_PROCESSES - 1].id == MAX_PROCESSES);
    stats = schedule_stats(sjf, MAX_PROCESSES);
    CHECK(stats.average_waiting == 49500000.0);
    CHECK(stats.average_turnaround == 50500000.0);
    CHECK(stats.completion_time == 101000000LL);

    printf("PASS: %d C scheduler checks.\n", checks);
    return EXIT_SUCCESS;
}
