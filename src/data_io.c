#include "data_io.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 파일에서는 재입력하지 않습니다. 한 줄의 오류를 그 줄에서 끝냅니다. */
static int file_integer(FILE *file, int line_number, long long minimum,
                         long long maximum, long long *value,
                         char *error, size_t error_size)
{
    char line[128];
    size_t length = 0;
    int character;
    int invalid = 0;
    char *end;
    long long parsed;

    while ((character = fgetc(file)) != EOF && character != '\n') {
        if (character == 0 || length == sizeof(line) - 1) {
            invalid = 1;
        } else {
            line[length++] = (char)character;
        }
    }
    line[length] = '\0';
    if (ferror(file)) {
        snprintf(error, error_size, "Read error on line %d.", line_number);
        return 0;
    }
    errno = 0;
    parsed = strtoll(line, &end, 10);
    if (end == line || errno == ERANGE) {
        invalid = 1;
    }
    while (isspace((unsigned char)*end)) {
        ++end;
    }
    if (invalid || *end != '\0' || parsed < minimum || parsed > maximum) {
        snprintf(error, error_size, "Invalid integer on line %d (expected %lld..%lld).",
                 line_number, minimum, maximum);
        return 0;
    }
    *value = parsed;
    return 1;
}

int load_workload(const char *path, Process input[], int *count,
                  char *error, size_t error_size)
{
    Process parsed[MAX_PROCESSES] = {0};
    FILE *file = fopen(path, "rb");
    long long parsed_count;
    int character;
    int i;
    if (file == NULL) {
        snprintf(error, error_size, "Cannot open input '%s': %s", path, strerror(errno));
        return 0;
    }
    /* Windows 편집기가 넣는 UTF-8 BOM은 파일 시작에서만 허용합니다. */
    character = fgetc(file);
    if (character == 0xef) {
        int second = fgetc(file);
        int third = fgetc(file);
        if (second != 0xbb || third != 0xbf) {
            snprintf(error, error_size, "Invalid input encoding: expected plain text or UTF-8 BOM.");
            fclose(file);
            return 0;
        }
    } else if (character != EOF) {
        ungetc(character, file);
    }
    if (!file_integer(file, 1, 1, MAX_PROCESSES, &parsed_count, error, error_size)) {
        fclose(file);
        return 0;
    }
    for (i = 0; i < (int)parsed_count; ++i) {
        parsed[i].id = i + 1;
        if (!file_integer(file, 2 + i * 2, 0, MAX_TIME, &parsed[i].arrival_time, error, error_size) ||
            !file_integer(file, 3 + i * 2, 1, MAX_TIME, &parsed[i].burst_time, error, error_size)) {
            fclose(file);
            return 0;
        }
    }
    while ((character = fgetc(file)) != EOF) {
        if (!isspace((unsigned char)character)) {
            snprintf(error, error_size, "Unexpected data after %lld processes.", parsed_count);
            fclose(file);
            return 0;
        }
    }
    if (ferror(file)) {
        snprintf(error, error_size, "Read error after process data.");
        fclose(file);
        return 0;
    }
    if (fclose(file) != 0) {
        snprintf(error, error_size, "Cannot close input '%s'.", path);
        return 0;
    }
    for (i = 0; i < (int)parsed_count; ++i) {
        input[i] = parsed[i];
    }
    *count = (int)parsed_count;
    return 1;
}

static void write_input(FILE *file, const Process input[], int count)
{
    int i;
    fprintf(file, "%d\n", count);
    for (i = 0; i < count; ++i) {
        fprintf(file, "%lld\n%lld\n", input[i].arrival_time, input[i].burst_time);
    }
}

static void write_csv(FILE *file, const ExportRun runs[], int run_count, int count)
{
    int r;
    fputs("record_type,algorithm,quantum,pid,arrival,burst,first_start,completion,waiting,turnaround,response,average_waiting,average_turnaround,average_response,makespan,time_unit,policy,cpu_count,context_switch_cost,io_wait\n", file);
    for (r = 0; r < run_count; ++r) {
        const ExportRun *run = &runs[r];
        ScheduleStats stats = schedule_stats(run->processes, count);
        char quantum[32] = "";
        int i;
        if (run->quantum > 0) {
            snprintf(quantum, sizeof(quantum), "%lld", run->quantum);
        }
        for (i = 0; i < count; ++i) {
            const Process *p = &run->processes[i];
            fprintf(file, "PROCESS,%s,%s,P%d,%lld,%lld,%lld,%lld,%lld,%lld,%lld",
                    run->algorithm, quantum, p->id, p->arrival_time, p->burst_time,
                    p->start_time, p->completion_time, p->waiting_time,
                    p->turnaround_time, p->start_time - p->arrival_time);
            fprintf(file, ",,,,,ticks,%s,1,0,false\n", run->policy);
        }
        fprintf(file, "SUMMARY,%s,%s", run->algorithm, quantum);
        for (i = 0; i < 8; ++i) {
            fputc(',', file); /* pid부터 response까지 8개 열은 비웁니다. */
        }
        fprintf(file, ",%.17g,%.17g,%.17g,%lld,ticks,%s,1,0,false\n",
                stats.average_waiting, stats.average_turnaround,
                stats.average_response, stats.completion_time, run->policy);
    }
}

static void json_slice(FILE *file, int *first, int idle, int id,
                       long long start, long long end)
{
    if (!*first) {
        fputc(',', file);
    }
    *first = 0;
    fputs("\n        {\"process_id\":", file);
    if (idle) {
        fputs("null", file);
    } else {
        fprintf(file, "%d", id);
    }
    fprintf(file, ",\"start\":%lld,\"end\":%lld}", start, end);
}

static void write_json(FILE *file, const Process input[], int count,
                        const ExportRun runs[], int run_count)
{
    int i;
    int r;
    fputs("{\n  \"schema_version\":1,\n  \"simulator_version\":\"6.0.0\",\n"
          "  \"time_unit\":\"ticks\",\n"
          "  \"assumptions\":{\"cpu_count\":1,\"context_switch_cost\":0,\"io_wait\":false},\n"
          "  \"input\":[", file);
    for (i = 0; i < count; ++i) {
        fprintf(file, "%s\n    {\"id\":%d,\"arrival\":%lld,\"burst\":%lld}",
                i ? "," : "", input[i].id, input[i].arrival_time, input[i].burst_time);
    }
    fputs("\n  ],\n  \"runs\":[", file);
    for (r = 0; r < run_count; ++r) {
        const ExportRun *run = &runs[r];
        ScheduleStats stats = schedule_stats(run->processes, count);
        int first = 1;
        fprintf(file, "%s\n    {\n      \"algorithm\":\"%s\",\n      \"quantum\":",
                r ? "," : "", run->algorithm);
        if (run->quantum > 0) {
            fprintf(file, "%lld", run->quantum);
        } else {
            fputs("null", file);
        }
        fprintf(file, ",\n      \"policy\":\"%s\",\n      \"processes\":[", run->policy);
        for (i = 0; i < count; ++i) {
            const Process *p = &run->processes[i];
            fprintf(file, "%s\n        {\"id\":%d,\"arrival\":%lld,\"burst\":%lld,"
                    "\"first_start\":%lld,\"completion\":%lld,\"waiting\":%lld,"
                    "\"turnaround\":%lld,\"response\":%lld}",
                    i ? "," : "", p->id, p->arrival_time, p->burst_time, p->start_time,
                    p->completion_time, p->waiting_time, p->turnaround_time,
                    p->start_time - p->arrival_time);
        }
        fprintf(file, "\n      ],\n      \"summary\":{\"average_waiting\":%.17g,"
                "\"average_turnaround\":%.17g,\"average_response\":%.17g,\"makespan\":%lld},"
                "\n      \"timeline\":[", stats.average_waiting, stats.average_turnaround,
                stats.average_response, stats.completion_time);
        if (run->round_robin != NULL) {
            size_t s;
            for (s = 0; s < run->round_robin->slice_count; ++s) {
                const ExecutionSlice *slice = &run->round_robin->slices[s];
                int idle = slice->process_index == -1;
                int id = idle ? 0 : run->processes[slice->process_index].id;
                json_slice(file, &first, idle, id, slice->start, slice->end);
            }
        } else {
            long long previous_end = 0;
            for (i = 0; i < count; ++i) {
                const Process *p = &run->processes[i];
                if (previous_end < p->start_time) {
                    json_slice(file, &first, 1, 0, previous_end, p->start_time);
                }
                json_slice(file, &first, 0, p->id, p->start_time, p->completion_time);
                previous_end = p->completion_time;
            }
        }
        fputs("\n      ]\n    }", file);
    }
    fputs("\n  ]\n}\n", file);
}

int save_artifacts(const char *input_path, const char *csv_path, const char *json_path,
                   const Process input[], int count, const ExportRun runs[], int run_count,
                   char *error, size_t error_size)
{
    const char *paths[] = {input_path, csv_path, json_path};
    FILE *files[3] = {NULL, NULL, NULL};
    int created[3] = {0, 0, 0};
    int ok = 1;
    int i;

    /* C11의 x 모드는 기존 파일을 열지 않고 새 파일만 생성합니다. */
    for (i = 0; i < 3; ++i) {
        if (paths[i] == NULL) {
            continue;
        }
        files[i] = fopen(paths[i], "wx");
        if (files[i] == NULL) {
            snprintf(error, error_size, "Cannot create output '%s': %s. Use a new filename and an existing directory.",
                     paths[i], strerror(errno));
            ok = 0;
            break;
        }
        created[i] = 1;
    }
    if (ok) {
        if (files[0] != NULL) write_input(files[0], input, count);
        if (files[1] != NULL) write_csv(files[1], runs, run_count, count);
        if (files[2] != NULL) write_json(files[2], input, count, runs, run_count);
    }
    for (i = 0; i < 3; ++i) {
        if (files[i] != NULL) {
            int failed = ferror(files[i]);
            if (fclose(files[i]) != 0) failed = 1;
            if (failed) {
                snprintf(error, error_size, "Write/close failed for '%s'.", paths[i]);
                ok = 0;
            }
        }
    }
    if (!ok) {
        for (i = 0; i < 3; ++i) {
            if (created[i] && remove(paths[i]) != 0) {
                snprintf(error, error_size, "Export failed; could not remove incomplete output '%s'.", paths[i]);
            }
        }
    }
    return ok;
}
