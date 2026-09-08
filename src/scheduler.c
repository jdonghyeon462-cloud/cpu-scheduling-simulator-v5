#include "scheduler.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>

static void finish_process(Process *process, long long start)
{
    process->start_time = start;
    process->completion_time = start + process->burst_time;
    process->waiting_time = start - process->arrival_time;
    process->turnaround_time = process->completion_time - process->arrival_time;
}

/* 삽입 정렬: 같은 시각에 도착한 작업은 원래 입력 순서를 유지합니다. */
static void simulate_fcfs(Process processes[], int count)
{
    long long current_time = 0;
    int i;

    for (i = 1; i < count; ++i) {
        Process current = processes[i];
        int j = i - 1;
        while (j >= 0 && processes[j].arrival_time > current.arrival_time) {
            processes[j + 1] = processes[j];
            --j;
        }
        processes[j + 1] = current;
    }

    for (i = 0; i < count; ++i) {
        if (current_time < processes[i].arrival_time) {
            current_time = processes[i].arrival_time;
        }
        finish_process(&processes[i], current_time);
        current_time = processes[i].completion_time;
    }
}

static void simulate_sjf(const Process input[], int count, Process output[])
{
    int completed[MAX_PROCESSES] = {0};
    int finished_count = 0;
    long long current_time = 0;

    while (finished_count < count) {
        int selected = -1;
        long long next_arrival = LLONG_MAX;
        int i;

        for (i = 0; i < count; ++i) {
            if (completed[i]) {
                continue;
            }
            if (input[i].arrival_time > current_time) {
                if (input[i].arrival_time < next_arrival) {
                    next_arrival = input[i].arrival_time;
                }
                continue;
            }

            /* 도착한 작업만 후보입니다. 실행 시간과 도착이 모두 같으면
             * 먼저 순회한 원소를 유지하므로 ID 값과 무관하게 입력 순서입니다. */
            if (selected == -1 ||
                input[i].burst_time < input[selected].burst_time ||
                (input[i].burst_time == input[selected].burst_time &&
                 input[i].arrival_time < input[selected].arrival_time)) {
                selected = i;
            }
        }

        if (selected == -1) {
            /* 남은 작업이 있지만 준비된 작업이 없을 때만 시간을 건너뜁니다. */
            current_time = next_arrival;
            continue;
        }

        output[finished_count] = input[selected];
        finish_process(&output[finished_count], current_time);
        current_time = output[finished_count].completion_time;
        completed[selected] = 1;
        ++finished_count;
    }
}

int simulate_schedule(const Process input[], int count, Algorithm algorithm,
                      Process output[])
{
    int i;

    if (input == NULL || output == NULL || count < 1 || count > MAX_PROCESSES ||
        (algorithm != ALGORITHM_FCFS && algorithm != ALGORITHM_SJF)) {
        return 0;
    }
    for (i = 0; i < count; ++i) {
        if (input[i].arrival_time < 0 || input[i].arrival_time > MAX_TIME ||
            input[i].burst_time < 1 || input[i].burst_time > MAX_TIME) {
            return 0;
        }
    }

    if (algorithm == ALGORITHM_FCFS) {
        for (i = 0; i < count; ++i) {
            output[i] = input[i];
        }
        simulate_fcfs(output, count);
    } else {
        simulate_sjf(input, count, output);
    }
    return 1;
}

ScheduleStats schedule_stats(const Process output[], int count)
{
    ScheduleStats stats = {0};
    long long total_waiting = 0;
    long long total_turnaround = 0;
    long long total_response = 0;
    int i;

    if (output == NULL || count < 1 || count > MAX_PROCESSES) {
        return stats;
    }
    for (i = 0; i < count; ++i) {
        total_waiting += output[i].waiting_time;
        total_turnaround += output[i].turnaround_time;
        total_response += output[i].start_time - output[i].arrival_time;
        if (output[i].completion_time > stats.completion_time) {
            stats.completion_time = output[i].completion_time;
        }
    }
    stats.average_waiting = (double)total_waiting / count;
    stats.average_turnaround = (double)total_turnaround / count;
    stats.average_response = (double)total_response / count;
    return stats;
}

typedef struct {
    int items[MAX_PROCESSES];
    int head;
    int size;
} ReadyQueue;

static int enqueue(ReadyQueue *queue, int index)
{
    int tail;
    if (queue->size == MAX_PROCESSES) {
        return 0;
    }
    tail = (queue->head + queue->size) % MAX_PROCESSES;
    queue->items[tail] = index;
    ++queue->size;
    return 1;
}

/* 호출하는 쪽에서 size > 0을 확인합니다. */
static int dequeue(ReadyQueue *queue)
{
    int index = queue->items[queue->head];
    queue->head = (queue->head + 1) % MAX_PROCESSES;
    --queue->size;
    return index;
}

RoundRobinStatus simulate_round_robin(const Process input[], int count,
                                     long long quantum, RoundRobinResult *result)
{
    ReadyQueue queue = {{0}, 0, 0};
    long long remaining[MAX_PROCESSES];
    int order[MAX_PROCESSES];
    int next = 0;
    long long time = 0;
    long long execution_count = 0;
    size_t used = 0;
    ExecutionSlice *slices;
    int i;

    if (input == NULL || result == NULL || count < 1 || count > MAX_PROCESSES ||
        quantum < 1 || quantum > MAX_TIME || result->slices != NULL) {
        return RR_INVALID_INPUT;
    }
    for (i = 0; i < count; ++i) {
        if (input[i].arrival_time < 0 || input[i].arrival_time > MAX_TIME ||
            input[i].burst_time < 1 || input[i].burst_time > MAX_TIME) {
            return RR_INVALID_INPUT;
        }
        execution_count += (input[i].burst_time + quantum - 1) / quantum;
    }
    if (execution_count > MAX_RR_SLICES) {
        return RR_TOO_MANY_SLICES;
    }
    /* 실행 횟수는 미리 계산할 수 있고 IDLE은 최대 count개입니다. */
    slices = malloc(((size_t)execution_count + (size_t)count) * sizeof(*slices));
    if (slices == NULL) {
        return RR_NO_MEMORY;
    }
    for (i = 0; i < count; ++i) {
        int j = i;
        order[i] = i;
        while (j > 0 && input[order[j - 1]].arrival_time > input[i].arrival_time) {
            order[j] = order[j - 1];
            --j;
        }
        order[j] = i;
        remaining[i] = input[i].burst_time;
        result->processes[i] = input[i];
        result->processes[i].start_time = -1;
    }

    while (next < count || queue.size > 0) {
        int index;
        long long duration;
        Process *process;

        if (queue.size == 0 && next < count && time < input[order[next]].arrival_time) {
            long long arrival = input[order[next]].arrival_time;
            slices[used++] = (ExecutionSlice){-1, time, arrival};
            time = arrival;
        }
        while (next < count && input[order[next]].arrival_time <= time) {
            if (!enqueue(&queue, order[next++])) {
                free(slices);
                return RR_INVALID_INPUT;
            }
        }
        index = dequeue(&queue);
        process = &result->processes[index];
        if (process->start_time == -1) {
            process->start_time = time;
        }
        duration = remaining[index] < quantum ? remaining[index] : quantum;
        slices[used++] = (ExecutionSlice){index, time, time + duration};
        time += duration;
        remaining[index] -= duration;

        /* 이번 구간 중간 및 끝 시각의 도착을 먼저 반영합니다. */
        while (next < count && input[order[next]].arrival_time <= time) {
            if (!enqueue(&queue, order[next++])) {
                free(slices);
                return RR_INVALID_INPUT;
            }
        }
        if (remaining[index] > 0) {
            if (!enqueue(&queue, index)) {
                free(slices);
                return RR_INVALID_INPUT;
            }
        } else {
            process->completion_time = time;
            process->turnaround_time = time - process->arrival_time;
            process->waiting_time = process->turnaround_time - process->burst_time;
        }
    }
    result->slices = slices;
    result->slice_count = used;
    return RR_OK;
}

void free_round_robin(RoundRobinResult *result)
{
    if (result != NULL) {
        free(result->slices);
        *result = (RoundRobinResult){0};
    }
}
