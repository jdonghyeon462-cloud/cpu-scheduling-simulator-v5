#ifndef REFERENCE_SJF_H
#define REFERENCE_SJF_H
#include "scheduler.h"
/* V5의 전수 탐색 SJF. 벤치마크와 결과 대조에서만 사용합니다. */
int reference_sjf(const Process input[], int count, Process output[]);
#endif
