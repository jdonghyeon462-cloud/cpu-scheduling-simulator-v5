# 08. ver4 — 파일 저장과 재현 가능한 비교 실험

[README](../README.md) · [테스트](04-testing.md) · [다음 버전](05-roadmap.md)

## 1. V3에서 무엇이 달라졌는가?

V3까지는 계산 결과를 콘솔에서 확인했습니다.
V4는 **입력을 파일로 보관하고 같은 실험을 다시 실행하는 흐름**을 추가합니다.
알고리즘 계산은 그대로 유지하고, 파일 로더와 내보내기를 별도 모듈로 구현했습니다.

| 기능 | V3 | V4 |
| --- | --- | --- |
| 입력 | 키보드·파이프 | 기존 방식 + 파일 직접 읽기 |
| 입력 보관 | 사용자가 따로 작성 | 검증된 입력을 텍스트로 저장 |
| 비교 결과 | 화면 표 | 화면 + CSV 작업·요약 행 |
| 실행 기록 | 콘솔은 최대 200개 RR 구간 | JSON에 내부 전체 구간 저장 |
| 실험 조건 | 화면·문서 | CSV·JSON에도 시간 할당량·규칙·가정 기록 |
| 결과 폴더 | 빌드 중심 | 실험마다 reports 하위 폴더 사용 |

## 2. 한 번 실행해 보기

프로젝트 루트에서 실행합니다. 출력 폴더는 실행 스크립트가 만듭니다.

```powershell
pwsh -File scripts/run.ps1 -Algorithm compare -Quantum 2 -Example rr-basic -OutputDirectory reports/run01
```

```text
reports/run01/
├── input.txt       # 원본 순서의 도착·실행 시간
├── results.csv     # 작업별 결과와 알고리즘별 평균
└── results.json    # 원본 입력·조건·결과·전체 타임라인
```

같은 입력을 다시 실행합니다.

```powershell
pwsh -File scripts/run.ps1 -Algorithm compare -Quantum 2 -InputFile reports/run01/input.txt -OutputDirectory reports/run02
```

두 JSON은 입력·알고리즘·시간 할당량이 같으면 같은 내용을 갖습니다.
파일 경로나 현재 시각은 결과 안에 넣지 않아 같은 실험의 비교를 방해하지 않습니다.
세 파일의 실제 예시는 [샘플 폴더](../examples/ver4-sample/input.txt),
[CSV](../examples/ver4-sample/results.csv), [JSON](../examples/ver4-sample/results.json)에서 확인할 수 있습니다.

## 3. 입력 형식과 검증

입력은 V1부터 사용한 한 줄에 정수 하나의 형식입니다.

```text
3
0
5
1
3
2
1
```

프로세스 개수 3과 P1=(0,5), P2=(1,3), P3=(2,1)을 뜻합니다.
ID는 파일에 저장하지 않고 읽은 순서대로 다시 부여합니다.
`--save-input`은 알고리즘이 정렬한 결과가 아니라 원본 순서를 기록합니다.

`load_workload`는 임시 배열에 모두 읽고 검증한 뒤에만 호출자의 배열과 개수를 갱신합니다.
따라서 중간에 잘못된 값이 있어도 절반만 읽은 데이터를 계산에 사용하지 않습니다.

| 사례 | 처리 |
| --- | --- |
| LF / CRLF | 둘 다 허용 |
| 파일 시작의 UTF-8 BOM | 제거 후 읽기 |
| 마지막 줄바꿈 없음 | 마지막 정수가 올바르면 허용 |
| 정수 앞뒤 공백 | 허용 |
| 모든 데이터 뒤의 공백·빈 줄 | 허용 |
| 필요한 값 사이의 빈 줄 | 줄 번호와 함께 실패 |
| 개수·시간 범위 밖 숫자 | 실패 |
| 소수·문자·정수 오버플로 | 실패 |
| NUL 바이트·너무 긴 줄 | 실패 |
| 필요한 입력 누락·추가 데이터 | 실패 |

텍스트 숫자 형식용 로더이며 JSON이나 CSV 입력을 읽는 기능은 아닙니다.
허용 범위는 개수 1~100, 도착 0~1,000,000, 실행 1~1,000,000입니다.
파일에서는 잘못된 줄을 다음 값으로 건너뛰지 않고 즉시 종료합니다.
키보드 입력의 재입력 동작과 구분한 이유는 손상된 실험 파일을 조용히 다른 입력으로 해석하지 않기 위해서입니다.

## 4. 파일 코드의 책임

| 파일 | 책임 |
| --- | --- |
| [main.c](../src/main.c) | 파일 옵션 해석, 입력 경로 선택, 계산·저장 호출 |
| [scheduler.c](../src/scheduler.c) | FCFS·SJF·RR 계산과 통계 |
| [data_io.h](../src/data_io.h) | 파일 함수와 ExportRun 선언 |
| [data_io.c](../src/data_io.c) | 입력 파싱, 텍스트·CSV·JSON 저장, 실패 처리 |

`ExportRun`은 알고리즘 이름·규칙·시간 할당량과 이미 계산된 결과의 포인터를 묶습니다.
내보내기 과정에서 다시 스케줄링하지 않습니다.
FCFS·SJF는 실행 순서의 프로세스 배열에서 구간과 IDLE을 복원하고,
RR은 V3의 `RoundRobinResult.slices` 전체를 순회합니다.

계산 모듈은 파일 이름이나 CSV 열을 알 필요가 없습니다.
V5의 화면도 같은 JSON을 읽을 수 있어 계산과 표현을 분리할 수 있습니다.

## 5. CSV 형식

CSV는 헤더 한 줄과 두 종류의 레코드로 구성됩니다.

- `PROCESS`: 프로세스별 결과 한 행.
- `SUMMARY`: 알고리즘별 평균과 전체 완료 시각 한 행.

비교 모드에서 프로세스가 n개라면 데이터 행은 `3 × (n+1)`개입니다.
기본 예제는 12개이며, 단독 알고리즘은 n+1개입니다.

| 열 | 의미 |
| --- | --- |
| record_type | PROCESS 또는 SUMMARY |
| algorithm | FCFS, SJF, RR |
| quantum | RR의 시간 할당량; 다른 방식은 빈 칸 |
| pid | P1처럼 표시한 ID; SUMMARY에서는 빈 칸 |
| arrival, burst | 도착 시각, 필요한 CPU 시간 |
| first_start, completion | 최초 시작, 최종 종료 |
| waiting, turnaround, response | 총 대기, 반환, 최초 응답 |
| average_waiting, average_turnaround, average_response | SUMMARY 행의 세 평균 |
| makespan | SUMMARY 행의 전체 완료 시각, 시각 0 기준 |
| time_unit | ticks |
| policy | 해당 알고리즘의 선택·큐 처리 규칙 |
| cpu_count, context_switch_cost, io_wait | 1, 0, false |

PROCESS 행에서는 평균·makespan 열을 비우고, SUMMARY 행에서는 개별 작업 열을 비웁니다.
타임라인은 CSV에 넣지 않았습니다. 구간 분석에는 JSON을 사용합니다.

문자열 열에는 프로그램에서 정한 고정 값만 들어가며 입력 파일명은 포함하지 않습니다.
CSV의 긴 소수는 계산 오류가 아닙니다. 화면은 소수 두 자리로 표시하지만
내보내기는 `%.17g`를 사용해 이후 계산에 필요한 실수 정밀도를 보존합니다.

## 6. JSON 형식

루트는 다음 필드를 갖습니다.

| 필드 | 내용 |
| --- | --- |
| schema_version | 파일 구조 버전: 현재 1 |
| simulator_version | 프로그램 버전: 현재 4.0.0 |
| time_unit | ticks |
| assumptions | 단일 CPU, 문맥 교환 비용 0, I/O 없음 |
| input | 입력 순서의 id·arrival·burst |
| runs | 선택한 알고리즘들의 계산 결과 |

각 `runs` 원소는 `algorithm`, `quantum`, `policy`, `processes`, `summary`, `timeline`을 가집니다.
FCFS·SJF의 quantum은 null입니다.

```json
{"process_id": 1, "start": 0, "end": 2}
```

이 구간은 P1이 [0,2)에 실행되었다는 의미입니다.
`process_id: null`은 CPU 유휴 구간입니다.
RR 타임라인이 화면에서 200개로 잘려도 JSON에는 허용된 전체 구간을 저장합니다.
실행 구간 최대 100,000개라는 V3의 계산 상한은 유지합니다.

`processes`는 FCFS·SJF에서는 실행 순서, RR에서는 입력 순서입니다.
알고리즘 사이의 같은 작업은 배열 위치가 아닌 `id`로 연결해야 합니다.
`timeline`은 모든 알고리즘에서 시간 순서입니다.

선택 규칙 문자열은 다음과 같습니다.

| 알고리즘 | policy |
| --- | --- |
| FCFS | arrival_then_input |
| SJF | burst_then_arrival_then_input |
| RR | arrivals_before_requeue |

실행 시간이나 구현을 바꾼 실험을 비교할 때는 입력뿐 아니라 quantum·policy·assumptions도 확인해야 합니다.

## 7. 경로와 덮어쓰기 처리

직접 실행하는 프로그램의 상대 경로는 현재 작업 폴더를 기준으로 해석합니다.
실행 스크립트의 상대 `-InputFile`과 `-OutputDirectory`는 스크립트가 있는 프로젝트 루트 기준입니다.
절대 경로도 사용할 수 있으며 공백이 있는 경로는 따옴표로 감쌉니다.

표준 C의 `fopen(path, "wx")`로 새 파일만 만들기 때문에 기존 파일을 덮어쓰지 않습니다.
입력 파일을 출력 경로로 지정하거나 두 출력에 같은 경로를 주면 실패합니다.
새 실험에는 새 하위 폴더를 선택합니다.

세 출력 경로를 모두 열 수 있는지 먼저 확인한 후 내용을 씁니다.
중간 경로 열기 또는 쓰기·닫기가 실패하면 이번 호출에서 만든 파일의 정리를 시도합니다.
기존에 있던 파일은 정리 대상으로 삼지 않습니다.
정리에 실패한 파일이 있으면 오류 메시지에 그 경로를 표시합니다.

이 동작은 여러 파일의 운영체제 수준 원자적 저장을 보장하지 않습니다.
프로그램 강제 종료나 장치 장애가 발생하면 불완전 파일이 남을 수 있습니다.
계산 결과가 화면에 나왔어도 저장이 실패하면 종료 코드는 1이며,
`Saved ...` 메시지는 모든 요청 파일의 쓰기·닫기가 성공한 뒤에만 출력합니다.

## 8. GitHub에 보관하는 것

소스·문서·테스트와 작은 대표 샘플을 저장소에 보관합니다.
개인의 반복 실험 파일은 `reports/`, 빌드·테스트 파일은 `build/`에서 관리하고 Git에서 제외합니다.
`examples/ver4-sample/`은 프로그램으로 생성한 대표 결과라 예외적으로 버전 관리합니다.

대표 샘플을 새 폴더에 재생성하려면:

```powershell
pwsh -File scripts/run.ps1 -Algorithm compare -Quantum 2 -InputFile examples/ver4-sample/input.txt -OutputDirectory reports/recreated-sample
```

V1~V3 태그는 이전 소스와 문서를 그대로 보존합니다. V4는 `v4.0.0` 태그로 구분합니다.

## 9. 확인 결과와 남은 범위

새 파일 기능 검사 176개와 기존 알고리즘 검사 702개로 총 **878개**를 통과했습니다.
파일 저장 후 재실행 결과 일치, CSV/JSON 교차 비교, 경로 충돌 및 신규 파일 정리를 확인했습니다.
구체적인 사례와 테스트 환경은 [테스트 문서](04-testing.md)에 있습니다.

현재는 JSON/CSV 가져오기, 파일 덮어쓰기 옵션, 네트워크 저장소, 그래픽 뷰어를 구현하지 않았습니다.
다음 V5에서는 내보낸 JSON으로 실행 과정을 시각화할 수 있습니다.
