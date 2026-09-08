#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheduler.h"
#include "data_io.h"

typedef enum { MODE_FCFS = 1, MODE_SJF, MODE_COMPARE, MODE_RR } RunMode;
#define MAX_PRINTED_SLICES 200

typedef struct {
    const char *input;
    const char *save_input;
    const char *csv;
    const char *json;
} FileOptions;

/* 한 줄에 정수 하나를 입력받습니다. 잘못된 값은 다시 입력받습니다. */
static int read_integer(const char *prompt, long long minimum,
                        long long maximum, long long *result)
{
    char line[128];

    for (;;) {
        char *end;
        long long value;

        printf("%s", prompt);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            fprintf(stderr, "\nInput ended before all values were entered.\n");
            return 0;
        }

        /* 긴 입력의 나머지가 다음 질문의 답으로 사용되지 않게 비웁니다. */
        if (strchr(line, '\n') == NULL && strlen(line) == sizeof(line) - 1) {
            int character;
            while ((character = getchar()) != '\n' && character != EOF) {
                /* 남은 입력을 버립니다. */
            }
            printf("Invalid input: line is too long.\n");
            continue;
        }

        errno = 0;
        value = strtoll(line, &end, 10);

        if (end == line || errno == ERANGE) {
            printf("Invalid input: enter an integer from %lld to %lld.\n",
                   minimum, maximum);
            continue;
        }

        while (isspace((unsigned char)*end)) {
            ++end;
        }

        if (*end != '\0' || value < minimum || value > maximum) {
            printf("Invalid input: enter an integer from %lld to %lld.\n",
                   minimum, maximum);
            continue;
        }

        *result = value;
        return 1;
    }
}

static void print_usage(const char *program)
{
    printf("Usage: %s [--algorithm fcfs|sjf|rr|compare] [--quantum N]\n", program);
    printf("       %s --menu | --help\n", program);
    printf("File options: --input FILE --save-input FILE --csv FILE --json FILE\n");
    printf("Exports create NEW files only. Parent directories must already exist.\n");
    printf("Default: fcfs. Input: count, then arrival and burst per process.\n");
    printf("RR/compare default quantum: 2. Quantum range: 1..%lld.\n", MAX_TIME);
    printf("Enter one integer per line. Menu: 1=FCFS, 2=SJF, 3=compare, 4=RR.\n");
}

/* 1: 실행, 0: 도움말 출력 후 종료, -1: 잘못된 옵션/입력. */
static int parse_mode(int argc, char *argv[], RunMode *mode, long long *quantum,
                       FileOptions *files)
{
    int algorithm_seen = 0;
    int quantum_seen = 0;
    int i;
    *mode = MODE_FCFS;
    *quantum = 2;
    if (argc == 1) {
        return 1;
    }
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--menu") == 0) {
        long long selection;
        printf("1. FCFS\n2. SJF (non-preemptive)\n3. Compare FCFS, SJF and RR\n4. Round Robin\n");
        if (!read_integer("Select mode (1-4): ", 1, 4, &selection)) {
            return -1;
        }
        *mode = (RunMode)selection;
        if ((*mode == MODE_RR || *mode == MODE_COMPARE) &&
            !read_integer("Time quantum: ", 1, MAX_TIME, quantum)) {
            return -1;
        }
        return 1;
    }
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--algorithm") == 0 && !algorithm_seen && i + 1 < argc) {
            const char *name = argv[++i];
            algorithm_seen = 1;
            if (strcmp(name, "fcfs") == 0) {
                *mode = MODE_FCFS;
            } else if (strcmp(name, "sjf") == 0) {
                *mode = MODE_SJF;
            } else if (strcmp(name, "rr") == 0) {
                *mode = MODE_RR;
            } else if (strcmp(name, "compare") == 0) {
                *mode = MODE_COMPARE;
            } else {
                break;
            }
        } else if (strcmp(argv[i], "--quantum") == 0 && !quantum_seen && i + 1 < argc) {
            char *end;
            const char *value = argv[++i];
            quantum_seen = 1;
            errno = 0;
            *quantum = strtoll(value, &end, 10);
            if (end == value || *end != '\0' || errno == ERANGE ||
                *quantum < 1 || *quantum > MAX_TIME) {
                break;
            }
        } else if (i + 1 < argc &&
                   (strcmp(argv[i], "--input") == 0 || strcmp(argv[i], "--save-input") == 0 ||
                    strcmp(argv[i], "--csv") == 0 || strcmp(argv[i], "--json") == 0)) {
            const char **destination;
            if (strcmp(argv[i], "--input") == 0) destination = &files->input;
            else if (strcmp(argv[i], "--save-input") == 0) destination = &files->save_input;
            else if (strcmp(argv[i], "--csv") == 0) destination = &files->csv;
            else destination = &files->json;
            if (*destination != NULL || argv[i + 1][0] == '\0') {
                break;
            }
            *destination = argv[++i];
        } else {
            break;
        }
    }
    if (i == argc && (!quantum_seen || *mode == MODE_RR || *mode == MODE_COMPARE)) {
        return 1;
    }
    fprintf(stderr, "Invalid command-line arguments.\n");
    print_usage(argv[0]);
    return -1;
}

static void print_results(const char *name, const Process processes[], int count)
{
    long long current_time = 0;
    ScheduleStats stats = schedule_stats(processes, count);
    int i;

    printf("\n=== %s Timeline ===\n", name);
    printf("Intervals use [start, end). Time unit: abstract ticks.\n");

    for (i = 0; i < count; ++i) {
        const Process *process = &processes[i];

        if (current_time < process->start_time) {
            printf("[%lld, %lld) IDLE\n", current_time, process->start_time);
        }
        printf("[%lld, %lld) P%d\n", process->start_time,
               process->completion_time, process->id);
        current_time = process->completion_time;
    }

    printf("\n=== Results (execution order) ===\n");
    printf("%-8s %10s %10s %10s %10s %10s %12s\n",
           "PID", "Arrival", "Burst", "Start", "Completion", "Waiting",
           "Turnaround");

    for (i = 0; i < count; ++i) {
        const Process *process = &processes[i];
        char label[16];

        snprintf(label, sizeof(label), "P%d", process->id);
        printf("%-8s %10lld %10lld %10lld %10lld %10lld %12lld\n",
               label, process->arrival_time, process->burst_time,
               process->start_time, process->completion_time,
               process->waiting_time, process->turnaround_time);
    }

    printf("\nAverage waiting time: %.2f\n", stats.average_waiting);
    printf("Average turnaround time: %.2f\n", stats.average_turnaround);
    printf("Average response time: %.2f\n", stats.average_response);
}

static void print_rr(const RoundRobinResult *result, int count, long long quantum)
{
    ScheduleStats stats = schedule_stats(result->processes, count);
    size_t i;
    printf("\n=== RR Timeline ===\nTime quantum: %lld\n", quantum);
    for (i = 0; i < result->slice_count && i < MAX_PRINTED_SLICES; ++i) {
        const ExecutionSlice *slice = &result->slices[i];
        if (slice->process_index == -1) {
            printf("[%lld, %lld) IDLE\n", slice->start, slice->end);
        } else {
            printf("[%lld, %lld) P%d\n", slice->start, slice->end,
                   result->processes[slice->process_index].id);
        }
    }
    if (i < result->slice_count) {
        printf("Timeline truncated: showing %d of %zu intervals. Statistics use all intervals.\n",
               MAX_PRINTED_SLICES, result->slice_count);
    }
    printf("\n=== RR Results (input order) ===\n");
    printf("%-8s %10s %10s %10s %10s %10s %12s %10s\n",
           "PID", "Arrival", "Burst", "Start", "Completion", "Waiting", "Turnaround", "Response");
    for (i = 0; i < (size_t)count; ++i) {
        const Process *p = &result->processes[i];
        char label[16];
        snprintf(label, sizeof(label), "P%d", p->id);
        printf("%-8s %10lld %10lld %10lld %10lld %10lld %12lld %10lld\n",
               label, p->arrival_time, p->burst_time, p->start_time,
               p->completion_time, p->waiting_time, p->turnaround_time,
               p->start_time - p->arrival_time);
    }
    printf("\nAverage waiting time: %.2f\n", stats.average_waiting);
    printf("Average turnaround time: %.2f\n", stats.average_turnaround);
    printf("Average response time: %.2f\n", stats.average_response);
}

static void print_comparison(const Process fcfs[], const Process sjf[],
                             const Process rr[], int count)
{
    ScheduleStats first = schedule_stats(fcfs, count);
    ScheduleStats second = schedule_stats(sjf, count);
    ScheduleStats third = schedule_stats(rr, count);

    printf("\n=== Comparison (same input) ===\n");
    printf("%-10s %16s %18s %16s %16s\n", "Algorithm", "Avg waiting",
           "Avg turnaround", "Completion", "Avg response");
    printf("%-10s %16.2f %18.2f %16lld %16.2f\n", "FCFS", first.average_waiting,
           first.average_turnaround, first.completion_time, first.average_response);
    printf("%-10s %16.2f %18.2f %16lld %16.2f\n", "SJF", second.average_waiting,
           second.average_turnaround, second.completion_time, second.average_response);
    printf("%-10s %16.2f %18.2f %16lld %16.2f\n", "RR", third.average_waiting,
           third.average_turnaround, third.completion_time, third.average_response);
    printf("Waiting difference (FCFS - SJF): %.2f\n",
           first.average_waiting - second.average_waiting);
    printf("Positive: SJF waits less on this input. Negative: FCFS waits less.\n");
}

int main(int argc, char *argv[])
{
    Process processes[MAX_PROCESSES] = {0};
    Process fcfs[MAX_PROCESSES];
    Process sjf[MAX_PROCESSES];
    RoundRobinResult rr = {0};
    ExportRun runs[3];
    int run_count = 0;
    FileOptions files = {0};
    char file_error[512];
    long long quantum;
    RunMode mode;
    int option_status = parse_mode(argc, argv, &mode, &quantum, &files);
    long long input_count;
    int count;
    int i;

    if (option_status <= 0) {
        return option_status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    printf("CPU Scheduling Simulator ver5\n");
    printf("Mode: %s\n", mode == MODE_FCFS ? "FCFS" :
           mode == MODE_SJF ? "SJF (non-preemptive)" :
           mode == MODE_RR ? "Round Robin" : "Compare FCFS, SJF and RR");
    if (mode == MODE_RR || mode == MODE_COMPARE) {
        printf("Time quantum: %lld\n", quantum);
    }
    printf("Arrival: 0..%lld | Burst: 1..%lld\n\n", MAX_TIME, MAX_TIME);

    if (files.input != NULL) {
        if (!load_workload(files.input, processes, &count, file_error, sizeof(file_error))) {
            fprintf(stderr, "%s\n", file_error);
            return EXIT_FAILURE;
        }
        printf("Loaded %d processes from %s\n", count, files.input);
    } else {
        if (!read_integer("Number of processes (1-100): ", 1,
                          MAX_PROCESSES, &input_count)) {
            return EXIT_FAILURE;
        }
        count = (int)input_count;

        for (i = 0; i < count; ++i) {
            processes[i].id = i + 1;
            printf("\nP%d\n", processes[i].id);

            if (!read_integer("  Arrival time: ", 0, MAX_TIME,
                              &processes[i].arrival_time) ||
                !read_integer("  Burst time: ", 1, MAX_TIME,
                              &processes[i].burst_time)) {
                return EXIT_FAILURE;
            }
        }
    }

    /* RR 자원/입력 검증에 실패하면 비교 결과를 일부만 출력하지 않습니다. */
    if (mode == MODE_RR || mode == MODE_COMPARE) {
        RoundRobinStatus status = simulate_round_robin(processes, count, quantum, &rr);
        if (status != RR_OK) {
            if (status == RR_TOO_MANY_SLICES) {
                fprintf(stderr, "RR requires more than %lld execution slices. Increase --quantum or reduce workload.\n", MAX_RR_SLICES);
            } else if (status == RR_NO_MEMORY) {
                fprintf(stderr, "Unable to allocate RR timeline.\n");
            } else {
                fprintf(stderr, "Invalid RR simulation input.\n");
            }
            return EXIT_FAILURE;
        }
    }
    /* 같은 원본에서 별도의 결과를 계산하여 알고리즘 간 영향을 막습니다. */
    if (mode == MODE_FCFS || mode == MODE_COMPARE) {
        if (!simulate_schedule(processes, count, ALGORITHM_FCFS, fcfs)) {
            fprintf(stderr, "FCFS simulation failed.\n");
            free_round_robin(&rr);
            return EXIT_FAILURE;
        }
        print_results("FCFS", fcfs, count);
        runs[run_count++] = (ExportRun){"FCFS", "arrival_then_input", 0, fcfs, NULL};
    }
    if (mode == MODE_SJF || mode == MODE_COMPARE) {
        if (!simulate_schedule(processes, count, ALGORITHM_SJF, sjf)) {
            fprintf(stderr, "SJF simulation failed.\n");
            free_round_robin(&rr);
            return EXIT_FAILURE;
        }
        print_results("SJF", sjf, count);
        runs[run_count++] = (ExportRun){"SJF", "burst_then_arrival_then_input", 0, sjf, NULL};
    }
    if (mode == MODE_RR || mode == MODE_COMPARE) {
        print_rr(&rr, count, quantum);
        runs[run_count++] = (ExportRun){"RR", "arrivals_before_requeue", quantum, rr.processes, &rr};
    }
    if (mode == MODE_COMPARE) {
        print_comparison(fcfs, sjf, rr.processes, count);
    }
    if (files.save_input != NULL || files.csv != NULL || files.json != NULL) {
        if (!save_artifacts(files.save_input, files.csv, files.json, processes, count,
                            runs, run_count, file_error, sizeof(file_error))) {
            fprintf(stderr, "%s\n", file_error);
            free_round_robin(&rr);
            return EXIT_FAILURE;
        }
        if (files.save_input != NULL) printf("Saved input: %s\n", files.save_input);
        if (files.csv != NULL) printf("Saved CSV: %s\n", files.csv);
        if (files.json != NULL) printf("Saved JSON: %s\n", files.json);
    }
    free_round_robin(&rr);
    return EXIT_SUCCESS;
}
