#ifndef DATA_IO_H
#define DATA_IO_H

#include "scheduler.h"

typedef struct {
    const char *algorithm; /* 프로그램에서 정한 고정 문자열 */
    const char *policy;
    long long quantum;    /* FCFS/SJF는 0, JSON에서는 null로 기록 */
    const Process *processes;
    const RoundRobinResult *round_robin; /* FCFS/SJF는 NULL */
} ExportRun;

/* 성공 시에만 input과 count를 갱신합니다. input 용량은 MAX_PROCESSES입니다. */
int load_workload(const char *path, Process input[], int *count,
                  char *error, size_t error_size);

/* 검증·계산이 끝난 입력과 결과를 저장합니다. NULL 경로는 생략합니다.
 * 기존 파일은 덮어쓰지 않습니다. 성공 1, 실패 0과 오류 설명을 반환합니다.
 * 한 호출에서 생성한 출력은 실패 시 정리를 시도하며, 기존 파일은 건드리지 않습니다. */
int save_artifacts(const char *input_path, const char *csv_path, const char *json_path,
                   const Process input[], int count, const ExportRun runs[], int run_count,
                   char *error, size_t error_size);

#endif
