#include "reference_sjf.h"
#include <limits.h>
#include <stddef.h>

/* V5의 후보 전수 탐색 방식을 독립 참조 구현으로 보존합니다. */
int reference_sjf(const Process input[], int count, Process output[])
{
    int completed[MAX_PROCESSES] = {0};
    int finished = 0, i;
    long long time = 0;
    if (!input || !output || count < 1 || count > MAX_PROCESSES) return 0;
    for (i = 0; i < count; ++i)
        if (input[i].arrival_time < 0 || input[i].arrival_time > MAX_TIME ||
            input[i].burst_time < 1 || input[i].burst_time > MAX_TIME) return 0;
    while (finished < count) {
        int selected = -1;
        long long next = LLONG_MAX;
        for (i = 0; i < count; ++i) {
            if (completed[i]) continue;
            if (input[i].arrival_time > time) {
                if (input[i].arrival_time < next) next = input[i].arrival_time;
                continue;
            }
            if (selected == -1 || input[i].burst_time < input[selected].burst_time ||
                (input[i].burst_time == input[selected].burst_time && input[i].arrival_time < input[selected].arrival_time)) selected = i;
        }
        if (selected == -1) { time = next; continue; }
        output[finished] = input[selected];
        output[finished].start_time = time;
        time += input[selected].burst_time;
        output[finished].completion_time = time;
        output[finished].waiting_time = output[finished].start_time - input[selected].arrival_time;
        output[finished].turnaround_time = time - input[selected].arrival_time;
        completed[selected] = 1;
        finished++;
    }
    return 1;
}
