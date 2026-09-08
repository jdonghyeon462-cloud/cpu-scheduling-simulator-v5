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

typedef struct {
    int items[MAX_PROCESSES];
    int size;
    int by_burst;
    const Process *input;
} IndexHeap;

/* ID 값 대신 원본 배열 위치를 마지막 기준으로 사용합니다. */
static int precedes(const IndexHeap *heap, int a, int b)
{
    const Process *left = &heap->input[a], *right = &heap->input[b];
    if (heap->by_burst && left->burst_time != right->burst_time)
        return left->burst_time < right->burst_time;
    if (left->arrival_time != right->arrival_time)
        return left->arrival_time < right->arrival_time;
    return a < b;
}

static void heap_push(IndexHeap *heap, int item)
{
    int position = heap->size++;
    while (position > 0) {
        int parent = (position - 1) / 2;
        if (!precedes(heap, item, heap->items[parent])) break;
        heap->items[position] = heap->items[parent];
        position = parent;
    }
    heap->items[position] = item;
}

/* 호출자는 비어 있지 않은 힙에만 pop을 적용합니다. */
static int heap_pop(IndexHeap *heap)
{
    int first = heap->items[0];
    int last = heap->items[--heap->size];
    int position = 0;
    while (position * 2 + 1 < heap->size) {
        int child = position * 2 + 1;
        if (child + 1 < heap->size && precedes(heap, heap->items[child + 1], heap->items[child])) child++;
        if (!precedes(heap, heap->items[child], last)) break;
        heap->items[position] = heap->items[child];
        position = child;
    }
    if (heap->size > 0) heap->items[position] = last;
    return first;
}

static void simulate_sjf(const Process input[], int count, Process output[])
{
    IndexHeap arrivals = {.input = input, .by_burst = 0};
    IndexHeap ready = {.input = input, .by_burst = 1};
    long long time = 0;
    int finished = 0, i;
    for (i = 0; i < count; ++i) heap_push(&arrivals, i);
    while (finished < count) {
        int selected;
        if (ready.size == 0 && arrivals.size > 0 && time < input[arrivals.items[0]].arrival_time)
            time = input[arrivals.items[0]].arrival_time;
        while (arrivals.size > 0 && input[arrivals.items[0]].arrival_time <= time)
            heap_push(&ready, heap_pop(&arrivals));
        selected = heap_pop(&ready);
        output[finished] = input[selected];
        finish_process(&output[finished], time);
        time = output[finished++].completion_time;
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
