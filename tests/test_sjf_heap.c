#include "scheduler.h"
#include "reference_sjf.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t next_value(uint32_t *state)
{
    *state = *state * UINT32_C(1664525) + UINT32_C(1013904223);
    return *state;
}

static int same(const Process *a, const Process *b)
{
    return a->id == b->id && a->arrival_time == b->arrival_time &&
        a->burst_time == b->burst_time && a->start_time == b->start_time &&
        a->completion_time == b->completion_time && a->waiting_time == b->waiting_time &&
        a->turnaround_time == b->turnaround_time;
}

int main(void)
{
    uint32_t random = 20260908;
    int c;
    for (c = 0; c < 200; ++c) {
        Process input[MAX_PROCESSES] = {0}, original[MAX_PROCESSES], heap[MAX_PROCESSES], scan[MAX_PROCESSES];
        int count = c < 10 ? MAX_PROCESSES : (int)(next_value(&random) % MAX_PROCESSES) + 1;
        int i;
        for (i = 0; i < count; ++i) {
            input[i].id = count - i; /* 입력 순서와 다른 ID로 동률 검사 */
            input[i].arrival_time = c == 0 ? 0 : c == 1 ? (count - i) * 100LL : next_value(&random) % 200;
            input[i].burst_time = c < 2 ? 10 : 1 + next_value(&random) % 70;
        }
        memcpy(original, input, sizeof(input));
        if (!reference_sjf(input, count, scan) || !simulate_schedule(input, count, ALGORITHM_SJF, heap) ||
            memcmp(original, input, sizeof(input)) != 0) return 1;
        for (i = 0; i < count; ++i) {
            if (!same(&heap[i], &scan[i])) {
                fprintf(stderr, "SJF mismatch in case %d, result %d.\n", c, i);
                return 1;
            }
        }
    }
    puts("PASS: 200 SJF heap/reference workload comparisons.");
    return 0;
}
