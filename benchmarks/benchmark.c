#include "reference_sjf.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint32_t random_value(uint32_t *state)
{
    *state = *state * UINT32_C(1664525) + UINT32_C(1013904223);
    return *state;
}

static double now_seconds(void)
{
    struct timespec timestamp;
    if (timespec_get(&timestamp, TIME_UTC) != TIME_UTC) return -1;
    return (double)timestamp.tv_sec + (double)timestamp.tv_nsec / 1000000000.0;
}

static int number(const char *text, unsigned long max, unsigned long *value)
{
    char *end;
    if (*text < '0' || *text > '9') return 0;
    errno = 0;
    *value = strtoul(text, &end, 10);
    return !errno && *end == '\0' && *value <= max;
}

int main(int argc, char *argv[])
{
    unsigned long cases = 500, count = 100, seed = 42;
    const char *pattern = "mixed", *csv_path = NULL;
    const char *names[] = {"FCFS", "SJF_SCAN", "SJF_HEAP", "RR", "RR", "RR", "RR"};
    const int quantums[] = {0, 0, 0, 1, 2, 4, 8};
    unsigned seen = 0;
    FILE *csv = stdout;
    int i, mode;
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("benchmark [--cases 1..10000] [--processes 1..100] [--seed 0..4294967295] [--pattern mixed|burst|sparse] [--csv NEW_FILE]");
        return 0;
    }
    for (i = 1; i < argc; i += 2) {
        unsigned bit = 0;
        if (i + 1 >= argc) break;
        if (strcmp(argv[i], "--cases") == 0) {
            bit = 1; if (!number(argv[i + 1], 10000, &cases) || !cases) break;
        } else if (strcmp(argv[i], "--processes") == 0) {
            bit = 2; if (!number(argv[i + 1], MAX_PROCESSES, &count) || !count) break;
        } else if (strcmp(argv[i], "--seed") == 0) {
            bit = 4; if (!number(argv[i + 1], UINT32_MAX, &seed)) break;
        } else if (strcmp(argv[i], "--pattern") == 0) {
            bit = 8; pattern = argv[i + 1];
            if (strcmp(pattern, "mixed") && strcmp(pattern, "burst") && strcmp(pattern, "sparse")) break;
        } else if (strcmp(argv[i], "--csv") == 0) {
            bit = 16; csv_path = argv[i + 1]; if (!*csv_path) break;
        } else break;
        if (seen & bit) break;
        seen |= bit;
    }
    if (i < argc) { fputs("Invalid benchmark arguments. Use --help.\n", stderr); return 1; }
    if (csv_path) {
        csv = fopen(csv_path, "wx");
        if (!csv) { fputs("Cannot create CSV. Choose a new file in an existing directory.\n", stderr); return 1; }
    }
    fputs("algorithm,quantum,pattern,cases,processes,seed,batch_elapsed_ms,elapsed_ms_per_case,average_waiting,average_turnaround,average_response,average_makespan,total_execution_slices,input_checksum,timer\n", csv);
    for (mode = 0; mode < 7; ++mode) {
        uint32_t rng = (uint32_t)seed, hash = UINT32_C(2166136261);
        unsigned long c;
        double waiting = 0, turnaround = 0, response = 0, makespan = 0;
        long long slices = 0;
        double begin = now_seconds(), end;
        for (c = 0; c < cases; ++c) {
            Process input[MAX_PROCESSES] = {0}, output[MAX_PROCESSES];
            RoundRobinResult rr = {0};
            ScheduleStats stats;
            for (i = 0; i < (int)count; ++i) {
                uint32_t a = random_value(&rng), b = random_value(&rng);
                input[i].id = i + 1;
                input[i].arrival_time = strcmp(pattern, "burst") == 0 ? 0 :
                    strcmp(pattern, "sparse") == 0 ? i * 100LL + a % 20 : a % 200;
                input[i].burst_time = 1 + b % 50;
                hash = (hash ^ (uint32_t)input[i].arrival_time) * UINT32_C(16777619);
                hash = (hash ^ (uint32_t)input[i].burst_time) * UINT32_C(16777619);
            }
            if (mode >= 3) {
                size_t s;
                if (simulate_round_robin(input, (int)count, quantums[mode], &rr) != RR_OK) goto failure;
                stats = schedule_stats(rr.processes, (int)count);
                for (s = 0; s < rr.slice_count; ++s) if (rr.slices[s].process_index >= 0) slices++;
                free_round_robin(&rr);
            } else {
                int ok = mode == 1 ? reference_sjf(input, (int)count, output) :
                    simulate_schedule(input, (int)count, mode == 0 ? ALGORITHM_FCFS : ALGORITHM_SJF, output);
                if (!ok) goto failure;
                stats = schedule_stats(output, (int)count);
                slices += (long long)count;
            }
            waiting += stats.average_waiting; turnaround += stats.average_turnaround;
            response += stats.average_response; makespan += stats.completion_time;
        }
        end = now_seconds();
        if (begin < 0 || end < 0 || end < begin) goto failure;
        {
            double milliseconds = 1000.0 * (end - begin);
            fprintf(csv, "%s,%d,%s,%lu,%lu,%lu,%.6f,%.9f,%.9f,%.9f,%.9f,%.9f,%lld,%" PRIu32 ",timespec_get_TIME_UTC\n",
                    names[mode], quantums[mode], pattern, cases, count, seed, milliseconds, milliseconds / cases,
                    waiting / cases, turnaround / cases, response / cases, makespan / cases,
                    slices, hash);
        }
    }
    {
        int failed = ferror(csv);
        if (csv_path ? fclose(csv) != 0 : fflush(csv) != 0) failed = 1;
        if (!failed) return 0;
    }
    if (csv_path) remove(csv_path);
    fputs("Benchmark CSV write failed.\n", stderr);
    return 1;
failure:
    if (csv_path) { fclose(csv); remove(csv_path); }
    fputs("Benchmark simulation or timer failed.\n", stderr);
    return 1;
}
