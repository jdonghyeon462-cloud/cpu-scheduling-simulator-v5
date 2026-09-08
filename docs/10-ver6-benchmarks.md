# 10. ver6 — 대규모 성능 실험과 최소 힙 SJF

[README](../README.md) · [벤치마크 코드](../benchmarks/benchmark.c) · [검증 결과](04-testing.md)

## 목표

V1~V5는 작은 입력에서 알고리즘의 시간과 실행 과정을 확인했습니다.
V6는 같은 workload를 결정적으로 여러 번 생성해 결과와 성능을 함께 비교합니다.
V2의 전수 탐색 SJF와 V6의 최소 힙 SJF가 같은 결과를 내는지도 확인합니다.

| 측정 대상 | 설명 |
| --- | --- |
| FCFS | 도착 순서 기반 기준선 |
| SJF_SCAN | V5까지의 후보 전수 탐색 구현 |
| SJF_HEAP | 도착 힙과 준비 작업 최소 힙 |
| RR q=1/2/4/8 | 시간 할당량별 실행 구간 비용 |

모든 방식은 같은 생성 입력과 checksum을 공유합니다.
통계가 같아야 하며, 경과 시간은 시스템 상태에 따라 달라지는 별도 지표입니다.

## 실행과 옵션

```powershell
pwsh -File scripts/benchmark.ps1 -Cases 1000 -Processes 100 -Pattern mixed -Csv reports/benchmark.csv
```

| 옵션 | 범위·기본값 | 의미 |
| --- | --- | --- |
| `-Cases` | 1~10,000, 기본 500 | 방식별 workload 반복 수 |
| `-Processes` | 1~100, 기본 100 | workload 하나의 작업 수 |
| `-Seed` | 0~4,294,967,295, 기본 42 | 결정적 난수 시드 |
| `-Pattern` | `mixed`, `burst`, `sparse` | 도착 시각 분포 |
| `-Csv` | 선택 | 새 CSV 저장 경로 |

`reports/`는 개인 측정 결과로 Git에서 제외합니다.
대표 결과는 [ver6-sample/benchmark.csv](../examples/ver6-sample/benchmark.csv)에 있습니다.
동일한 seed·pattern·cases·processes를 사용하면 입력 checksum과 평균 통계가 재현됩니다.

## workload 패턴

- `mixed`: 도착 0~199, 실행 1~50의 혼합 workload.
- `burst`: 모든 작업이 시각 0에 도착해 선택 자료구조를 집중 측정.
- `sparse`: 작업을 약 100 tick 간격으로 배치해 유휴 시간이 많은 경우.

생성기는 고정 LCG를 사용합니다. 타이머 결과는 wall clock에 가까워 OS 부하와 컴파일러의 영향을 받습니다.

## 최소 힙 SJF

V5는 매 선택마다 모든 미완료 입력을 훑었습니다. V6는 두 힙을 유지합니다.

```text
arrivals: 아직 도착하지 않은 작업을 도착 시각순으로 보관
ready:    현재 시각까지 도착한 작업을 실행 시간·도착·입력 순으로 보관
```

CPU가 비면 arrivals 루트 시각으로 이동하고 준비 작업을 ready에 넣습니다.
ready 루트를 꺼내 비선점 실행합니다. 동률의 마지막 기준은 원본 배열 위치라 ID 숫자와 무관합니다.

| 단계 | V5 전수 탐색 | V6 힙 |
| --- | --- | --- |
| 다음 도착 | O(n) 탐색 | 힙 루트 |
| 준비 작업 선택 | O(n) 탐색 | O(log n) |
| 전체 workload | O(n²) 선택 | O(n log n) 힙 연산 |
| 결과 명세 | 동일 | 동일 |

입력 상한 100에서는 실행 시간이 짧아 차이가 작을 수 있습니다.
벤치마크의 목적은 자료구조 선택의 비용을 관찰하는 것이며 특정 하드웨어 성능을 보장하지 않습니다.

## CSV와 대표 결과

CSV 헤더는 다음과 같습니다.

```text
algorithm,quantum,pattern,cases,processes,seed,batch_elapsed_ms,elapsed_ms_per_case,average_waiting,average_turnaround,average_response,average_makespan,total_execution_slices,input_checksum,timer
```

한 실행에서 FCFS, SJF_SCAN, SJF_HEAP, RR 네 시간 할당량으로 7행이 나옵니다.
`input_checksum`은 모든 행에서 같아야 하며, SJF_SCAN/HEAP의 평균 통계도 같아야 합니다.
대표 샘플은 Windows, GCC 15.2.0, `-O2`, 1,000 cases, 100 processes, seed 42로 세 번 측정했습니다.

| 패턴 | SJF_SCAN (ms) | SJF_HEAP (ms) | 평균 대기 |
| --- | ---: | ---: | ---: |
| mixed | 78.21 / 81.05 / 75.13 | 29.96 / 33.27 / 33.11 | 748.15019 |
| burst | 64.01 / 65.43 / 64.26 | 22.02 / 25.12 / 22.29 | 831.00230 |
| sparse | 41.65 / 40.06 / 44.15 | 10.28 / 9.49 / 12.74 | 0.00000 |

위 시간은 샘플 시점 값이며 다른 환경에서는 달라집니다.
`sparse`의 대기가 0인 것은 생성한 도착 간격이 작업 실행보다 충분히 긴 경우입니다.

## 검증 설계

`tests/test_sjf_heap.c`는 ID가 입력 순서와 다른 200개 결정적 입력을 생성해 V5 참조 구현과 V6 결과를 비교합니다.
동시 도착, 흩어진 도착, 원본 입력 보존을 포함합니다.
`tests/test_ver6.js`는 세 패턴의 checksum·통계 재현, 잘못된 옵션, CSV 충돌, V5 뷰어 필터를 확인합니다.
경과 시간은 음수가 아닌지만 검사하고 통계·checksum은 정확히 비교합니다.

총 검증은 기존 902개에 힙 200개와 V6 benchmark/filter 86개를 더한 1,188개입니다.

## 한계와 다음 단계

CPU 하나, CPU burst 하나, I/O 없음, 문맥 교환 비용 0이라는 모델입니다.
측정은 실제 CPU 사용률이나 운영체제 스케줄러 공정성을 측정하지 않습니다.
프로세스 상한은 100이며 메모리 부족을 강제로 주입하지 않았습니다.
V7에서는 대규모 입력의 메모리 사용량과 결과 필터를 확장할 수 있습니다.
