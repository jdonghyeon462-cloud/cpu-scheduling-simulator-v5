# CPU Scheduling Simulator ver5

**C11로 구현한 FCFS·비선점 SJF·Round Robin CPU 스케줄링 시뮬레이터와 한국어 코드 분석 자료입니다.**

현재 완성본은 **ver5 (`v5.0.0`)**입니다. 기존 저장소 주소와 폴더 이름의 `ver1`은 유지하며,
[V1 소스·README](https://github.com/jdonghyeon462-cloud/cpu-scheduling-simulator-v1/tree/main)는
`v1.0.0` 태그에서 그대로 확인할 수 있습니다.
[V2 소스·README](https://github.com/jdonghyeon462-cloud/cpu-scheduling-simulator-v2/tree/main)도 보존했습니다.
[V3 소스·README](https://github.com/jdonghyeon462-cloud/cpu-scheduling-simulator-v3/tree/main)도 태그에서 확인할 수 있습니다.
[V4 소스·README](https://github.com/jdonghyeon462-cloud/cpu-scheduling-simulator-v4/tree/main)도 태그에서 확인할 수 있습니다.

프로세스의 도착 시간과 CPU 실행 시간을 입력하면 실행 순서, CPU 유휴 구간,
각 프로세스의 대기·반환 시간과 평균 응답 시간을 출력합니다.
실제 운영체제의 스케줄러를 제어하지 않고 가상의 시간을 계산합니다.
한 번 입력한 데이터를 세 알고리즘에 각각 적용해 결과를 비교할 수 있습니다.
V4에서는 입력 파일을 직접 읽고, 입력·CSV 비교표·전체 JSON 실행 기록을 저장할 수 있습니다.
V5에서는 이 JSON을 브라우저에서 재생하며 작업 상태를 시간에 따라 확인할 수 있습니다.

## 빠른 안내

- [C 소스 코드](src/main.c)
- [파일 입출력 코드](src/data_io.c) · [V4 분석 및 파일 형식](docs/08-ver4-file-io.md)
- [저장된 입력 샘플](examples/ver4-sample/input.txt) · [CSV 샘플](examples/ver4-sample/results.csv) · [JSON 샘플](examples/ver4-sample/results.json)
- [V5 뷰어](viewer/index.html) · [뷰어 모델 테스트](tests/test_viewer.js)
- [공통 자료구조](src/scheduler.h) · [FCFS·SJF·RR 계산 코드](src/scheduler.c)
- [V3 상세 분석: 원형 큐·시간 할당량·응답 시간](docs/07-ver3-round-robin.md)
- [V2 상세 분석: 도착 조건·동률·비교 실험](docs/06-ver2-sjf.md)
- [설계와 FCFS 원리](docs/01-design.md)
- [함수별 코드 분석](docs/02-code-walkthrough.md)
- [예제 실험과 결과 해석](docs/03-experiments.md)
- [테스트 방법과 검증 범위](docs/04-testing.md)
- [버전별 개발 계획](docs/05-roadmap.md)
- [변경 기록](CHANGELOG.md)

## 폴더 구조

```text
cpu-scheduling-simulator-ver1/
├── README.md                   # 프로젝트 소개와 실행 방법
├── CHANGELOG.md                # 버전별 변경 기록
├── .gitignore                  # 실행 파일과 임시 파일 제외
├── .gitattributes              # 텍스트 줄바꿈 규칙
├── src/
│   ├── main.c                  # 입력·모드 선택·결과 출력
│   ├── scheduler.h             # 공통 자료구조와 함수 선언
│   ├── scheduler.c             # FCFS·SJF·RR 계산, 원형 큐와 통계
│   ├── data_io.h               # 입력·내보내기 함수 선언
│   └── data_io.c               # 엄격한 파일 읽기와 CSV·JSON 저장
├── viewer/
│   ├── index.html               # 로컬 JSON 실행 기록 뷰어
│   ├── style.css                # 반응형 화면 스타일
│   ├── model.js                 # JSON 검증·상태·구간 계산
│   ├── app.js                   # 재생·파일 선택·화면 갱신
│   └── sample-data.js            # V5 기본 예제
├── docs/
│   ├── 01-design.md            # 문제 정의, 자료구조, 알고리즘
│   ├── 02-code-walkthrough.md  # 함수와 C 문법 분석
│   ├── 03-experiments.md       # 입력별 계산 과정과 FCFS 한계
│   ├── 04-testing.md           # 검증 방법과 실제 결과
│   ├── 05-roadmap.md           # 버전별 완료 내용과 확장 계획
│   ├── 06-ver2-sjf.md          # V2 구현 분석과 비교 실험
│   ├── 07-ver3-round-robin.md  # V3 원형 큐·선점·실행 기록 분석
│   ├── 08-ver4-file-io.md      # V4 파일 형식·재현·실패 처리 분석
│   └── 09-ver5-viewer.md       # V5 JSON 뷰어·재생·상태 분석
├── examples/
│   ├── basic.txt              # 기본 계산
│   ├── idle.txt               # CPU 유휴 구간
│   ├── ties-unsorted.txt      # 동시 도착과 정렬
│   ├── long-first.txt         # 긴 작업이 먼저 실행되는 예제
│   ├── short-first.txt        # 짧은 작업이 먼저 실행되는 예제
│   ├── sjf-comparison.txt     # 같은 입력의 FCFS·SJF 비교
│   ├── sjf-future.txt         # 아직 도착하지 않은 작업 제외
│   ├── sjf-ties.txt           # 실행 시간·도착·입력 순서 동률
│   ├── sjf-tradeoff.txt       # 평균과 개별 작업의 대기 차이
│   ├── rr-basic.txt           # Round Robin 기본 예제
│   ├── rr-boundary.txt        # 시간 할당량 끝 시각의 도착
│   ├── rr-idle.txt            # 분할 실행과 초기·중간 IDLE
│   ├── rr-response.txt        # 대기 시간과 응답 시간의 차이
│   ├── ver4-sample/           # 프로그램으로 생성한 대표 저장 결과
│       ├── input.txt
│       ├── results.csv
│       └── results.json
│   └── ver5-sample/           # V5 뷰어용 프로그램 출력
│       ├── input.txt
│       └── results.json
├── scripts/
│   ├── build.ps1              # 프로젝트 경로를 기준으로 빌드
│   └── run.ps1                # 빌드 후 실행, 예제 입력 지원
├── tests/
│   ├── test_fcfs.ps1          # V1 동작의 회귀 테스트
│   ├── test_ver2.ps1          # CLI·메뉴·SJF 참조 모델 비교
│   ├── test_scheduler.c       # 원본 보존·동률·입력 범위 검사
│   ├── test_ver3.ps1          # RR CLI·참조 모델·자원 한도 검사
│   ├── test_round_robin.c     # 큐 최대 용량·순환·메모리 해제 검사
│   ├── test_ver4.ps1          # 파일 왕복·형식·충돌·전체 기록 검사
│   └── test_viewer.js         # JSON 모델·상태·시간 이동 검사
├── build/                     # 로컬 빌드·테스트 산출물, Git 추적 제외
└── reports/                   # 개인 실험 결과, Git 추적 제외
```

GitHub에는 `build/`, `reports/`와 실행 파일을 올리지 않습니다.
빈 빌드 폴더도 Git에는 저장되지 않으며 빌드 스크립트가 자동으로 만듭니다.
문서 링크는 저장소 내부의 상대 경로를 사용하므로 다른 위치에 복제해도 연결됩니다.

## 개발 환경

- 프로그램: C11, C 표준 라이브러리만 사용.
- 확인 환경: Windows, GCC 15.2.0 (MinGW-w64).
- 보조 스크립트와 자동 테스트: PowerShell 7 이상 (`pwsh`).
- 스크립트 없이 C 컴파일러만으로도 빌드하고 실행할 수 있습니다.
- 소스와 문서는 UTF-8이며, 콘솔 출력은 영어입니다.

## 빌드 및 실행

아래 명령은 **이 README가 있는 프로젝트 루트**에서 실행합니다.

### Windows: C 컴파일러만 사용하는 방법

GCC를 설치하고 PATH에 등록한 뒤 PowerShell에서 실행합니다.

```powershell
New-Item -ItemType Directory -Force build | Out-Null
gcc -std=c11 -Wall -Wextra -Wpedantic src/main.c src/scheduler.c src/data_io.c -o build/simulator.exe
.\build\simulator.exe
```

예제 입력을 넣으려면:

```powershell
Get-Content .\examples\basic.txt | .\build\simulator.exe
Get-Content .\examples\sjf-comparison.txt | .\build\simulator.exe --algorithm compare
Get-Content .\examples\rr-basic.txt | .\build\simulator.exe --algorithm rr --quantum 2
```

### Windows: 보조 스크립트 사용

PowerShell 7 이상을 설치했다면:

```powershell
pwsh -File .\scripts\build.ps1
pwsh -File .\scripts\run.ps1
pwsh -File .\scripts\run.ps1 -Example basic
pwsh -File .\scripts\run.ps1 -Algorithm sjf -Example sjf-comparison
pwsh -File .\scripts\run.ps1 -Algorithm compare -Example sjf-comparison
pwsh -File .\scripts\run.ps1 -Algorithm rr -Quantum 2 -Example rr-basic
pwsh -File .\scripts\run.ps1 -Algorithm compare -Quantum 1 -Example rr-basic
pwsh -File .\scripts\run.ps1 -Algorithm compare -Quantum 2 -Example rr-basic -OutputDirectory reports/run01
pwsh -File .\scripts\run.ps1 -Algorithm compare -InputFile reports/run01/input.txt -OutputDirectory reports/run02

# V5 로컬 뷰어
node .\scripts\serve-viewer.js 8787
# 브라우저에서 http://127.0.0.1:8787/ 열기
```

`run.ps1`은 매번 빌드하므로 수정 전 실행 파일을 실수로 실행하지 않습니다.
`-Example`에는 위 폴더 구조에 나열된 예제 파일명을 확장자 없이 지정합니다.
`-Algorithm`은 `fcfs`(기본값), `sjf`, `rr`, `compare`를 지원합니다.
`-Quantum`은 `rr` 또는 `compare`에서만 지정하며 기본값은 2입니다.
`-InputFile`과 `-Example`은 함께 지정할 수 없습니다.
`-OutputDirectory`를 지정하면 폴더를 만들고 `input.txt`, `results.csv`, `results.json`을 저장합니다.
두 스크립트는 자신의 위치로 프로젝트 루트를 찾으므로, 다른 작업 폴더에서
스크립트의 전체 경로를 지정해도 올바른 소스와 예제를 사용합니다.

### Linux / macOS

```sh
mkdir -p build
cc -std=c11 -Wall -Wextra -Wpedantic src/main.c src/scheduler.c src/data_io.c -o build/simulator
./build/simulator
./build/simulator < examples/basic.txt
./build/simulator --algorithm compare < examples/sjf-comparison.txt
./build/simulator --algorithm rr --quantum 2 < examples/rr-basic.txt
```

위 환경에서는 표준 C 코드를 빌드하도록 안내하지만 실제 실행 검증은 Windows에서 수행했습니다.

## 실행 모드

| 실행 옵션 | 동작 |
| --- | --- |
| 옵션 없음 / `--algorithm fcfs` | V1과 같은 FCFS 입력 및 계산 |
| `--algorithm sjf` | 비선점 SJF 실행 |
| `--algorithm rr` | Round Robin 실행, 기본 시간 할당량 2 |
| `--algorithm compare` | 동일 입력으로 FCFS·SJF·RR 및 비교표 출력 |
| `--quantum N` | RR 시간 할당량, 1~1,000,000; rr/compare에서만 허용 |
| `--menu` | 숫자로 선택: 1=FCFS, 2=SJF, 3=비교, 4=RR |
| `--help` | 입력을 기다리지 않고 사용법 출력 |

## V5 실행 기록 뷰어

V4에서 만든 JSON을 브라우저에서 열어 시간 축을 움직이며 세 알고리즘의 실행 과정을 확인합니다.
뷰어는 파일을 서버로 업로드하지 않고 브라우저 메모리에서만 읽습니다.

```powershell
node .\scripts\serve-viewer.js 8787
```

브라우저에서 `http://127.0.0.1:8787/`을 열면 V5 기본 예제가 표시됩니다.
`JSON 파일 열기`로 `results.json`을 선택하거나 `기본 예제 불러오기`를 누르세요.
알고리즘 탭, 시간 슬라이더, 처음·이전 구간·재생·다음 구간 버튼, 재생 속도를 제공합니다.
현재 CPU, 대기·완료·미도착 개수, 작업별 실행·남은 시간도 시각에 맞춰 갱신합니다.
알 수 없는 알고리즘, 잘못된 시간 계산, 겹치는 구간, 실행량 불일치, 16MB 초과 파일은 오류로 표시합니다.

```powershell
node .\tests\test_viewer.js
```

### V4 파일 옵션

| 옵션 | 동작 |
| --- | --- |
| `--input FILE` | 예제와 같은 텍스트 형식을 직접 읽기 |
| `--save-input FILE` | 검증된 원본 입력을 같은 형식으로 저장 |
| `--csv FILE` | 선택한 알고리즘의 작업별 결과와 평균을 CSV로 저장 |
| `--json FILE` | 입력·실험 조건·작업 결과·전체 타임라인을 JSON으로 저장 |

파일 옵션은 일반 알고리즘·시간 할당량 옵션과 조합합니다. `--menu`와 `--help`는 단독 사용합니다.
출력 파일은 **새 파일만 생성**합니다. 같은 파일명이 존재하면 종료 코드 1로 안내하며 덮어쓰지 않습니다.
출력 경로의 부모 폴더는 미리 만들거나 `run.ps1 -OutputDirectory`를 사용하세요.
실험을 반복할 때는 `reports/run01`, `reports/run02`처럼 다른 폴더를 사용합니다.

```powershell
New-Item -ItemType Directory -Force reports/run03 | Out-Null
.\build\simulator.exe --algorithm compare --quantum 2 --input examples/rr-basic.txt --save-input reports/run03/input.txt --csv reports/run03/results.csv --json reports/run03/results.json
```

프로그램을 직접 실행할 때의 상대 파일 경로는 **현재 터미널 폴더** 기준입니다.
`run.ps1`의 상대 `-InputFile`·`-OutputDirectory`는 **프로젝트 루트** 기준이며 절대 경로도 허용합니다.
공백이 있는 경로는 따옴표로 감쌉니다.

예: `./build/simulator.exe --menu`로 실행하고 3을 입력하면 비교 모드가 됩니다.
메뉴의 3 또는 4를 선택하면 프로세스 개수 전에 시간 할당량을 추가로 입력받습니다.
알고리즘 옵션은 소문자입니다. 잘못된 옵션은 종료 코드 1로 종료합니다.
일반 실행의 표준 입력은 V1과 같으며 시간 할당량은 명령줄 옵션으로 지정합니다.
`--menu`에서만 모드 선택과 필요한 시간 할당량을 먼저 입력합니다.

## 입력 형식

**한 줄에 정수 하나씩** 입력합니다.

1. 프로세스 개수: 1~100.
2. P1의 도착 시간: 0~1,000,000.
3. P1의 실행 시간: 1~1,000,000.
4. P2부터 마지막 프로세스까지 도착 시간과 실행 시간 반복.

프로세스 ID는 입력 순서대로 P1, P2, ...가 부여됩니다.
잘못된 값은 안내 후 다시 입력받습니다. 필요한 입력이 끝나기 전에 EOF가 발생하면 종료 코드 1을 반환합니다.
V4는 `--input`으로 예제 파일을 직접 읽으며 기존 파이프·키보드 입력도 지원합니다.
파일의 각 줄에는 정수 하나가 필요하고, 중간 빈 줄·부족한 입력·추가 데이터·범위 밖 값은 오류입니다.
LF/CRLF, 마지막 줄의 줄바꿈 생략, UTF-8 BOM과 데이터 뒤의 공백은 허용합니다.
파일 오류는 줄 번호와 함께 종료하며, 키보드 입력처럼 다시 입력받지 않습니다.

## 실행 예시

입력 파일 [basic.txt](examples/basic.txt)는 아래 작업을 나타냅니다.

| 프로세스 | 도착 시간 | 실행 시간 |
| --- | ---: | ---: |
| P1 | 0 | 5 |
| P2 | 1 | 3 |
| P3 | 2 | 1 |

```text
=== FCFS Timeline ===
Intervals use [start, end). Time unit: abstract ticks.
[0, 5) P1
[5, 8) P2
[8, 9) P3

=== Results (execution order) ===
PID         Arrival      Burst      Start Completion    Waiting   Turnaround
P1                0          5          0          5          0            5
P2                1          3          5          8          4            7
P3                2          1          8          9          6            7

Average waiting time: 3.33
Average turnaround time: 6.33
Average response time: 3.33
```

`[0, 5)`는 시각 0에 시작해 시각 5에 종료하는 구간입니다.
`IDLE`은 실행할 프로세스가 없는 시간입니다. 단위는 가상 tick입니다.

## 알고리즘과 범위

FCFS는 먼저 도착한 프로세스부터 끝까지 실행하며, 도착 시간이 같으면 입력 순서입니다.
비선점 SJF는 **현재 시각까지 도착한 미완료 작업 중** 실행 시간이 가장 짧은 작업을 선택합니다.
SJF의 동률 규칙은 실행 시간 → 도착 시간 → 입력 순서입니다.
두 방식 모두 실행 중인 작업을 중간에 교체하지 않습니다.
Round Robin은 최대 시간 할당량만큼 실행한 뒤, 미완료라면 준비 큐의 뒤로 이동합니다.
실행 구간 끝 시각까지 도착한 작업을 먼저 큐에 넣고 현재 작업을 재삽입합니다.

```text
시작 = max(이전 작업 종료, 도착)
종료 = 시작 + 실행 시간
대기 = 시작 - 도착
반환 = 종료 - 도착

Round Robin:
반환 = 최종 종료 - 도착
총 대기 = 반환 - 총 실행 시간
응답 = 최초 시작 - 도착
```

단일 CPU, 작업별 하나의 CPU burst, I/O 대기 없음, 문맥 교환 비용 0을 가정합니다.
RR은 이 burst를 여러 실행 구간으로 나눕니다.
FCFS는 안정적인 삽입 정렬 O(n²), 이후 계산 O(n)입니다.
SJF는 작업 선택마다 남은 후보를 훑는 O(n²) 구현입니다.
RR은 도착 순서를 정렬하고 원형 큐에서 작업을 순환시킵니다.
실행 구간 수를 s라 하면 O(n² + s)이며 실제 시간 애니메이션은 구현하지 않았습니다.

### RR 실행 기록의 크기

메모리와 계산량을 제한하기 위해 RR 실행 구간은 최대 **100,000개**입니다.
`Σ ceil(실행 시간 / 시간 할당량)`이 이 값을 넘으면 실행 전에 종료 코드 1로 안내합니다.
시간 할당량을 키우거나 작업량을 줄이면 됩니다. FCFS·SJF 단독 실행에는 이 제한이 없습니다.
콘솔에는 RR 타임라인의 앞 **200개 구간**까지 표시하고, 생략 여부와 전체 구간 수를 알립니다.
개별 결과와 평균은 생략된 구간까지 전부 계산한 값입니다. 내부 실행 기록 배열에는 전체 구간을 저장합니다.
V4의 JSON 내보내기에도 이 전체 기록을 저장합니다. CSV는 작업별 결과·요약 표이며 타임라인은 포함하지 않습니다.

## V2 비교 예시

[`sjf-comparison.txt`](examples/sjf-comparison.txt)는 P1=(0, 5), P2=(1, 4), P3=(2, 1)입니다.

| 알고리즘 | 실행 순서 | 평균 대기 | 평균 반환 | 전체 완료 시각 |
| --- | --- | ---: | ---: | ---: |
| FCFS | P1 → P2 → P3 | 3.67 | 7.00 | 10 |
| SJF | P1 → P3 → P2 | 2.67 | 6.00 | 10 |

SJF는 P1이 끝나는 시각 5에 준비된 P2와 P3 중 짧은 P3를 먼저 고릅니다.
P3의 대기는 7에서 3으로 줄고, P2의 대기는 4에서 5로 늘어납니다.
평균 개선과 개별 작업의 대기 변화는 함께 봐야 합니다.
[상세 분석](docs/06-ver2-sjf.md)에는 손계산과 다른 경계 사례를 정리했습니다.

## V3 시간 할당량 비교

[`rr-basic.txt`](examples/rr-basic.txt)에 RR의 시간 할당량만 바꿔 적용한 결과입니다.

| 시간 할당량 | 실행 구간 수 | 평균 대기 | 평균 반환 | 평균 응답 |
| --- | ---: | ---: | ---: | ---: |
| 1 | 9 | 2.67 | 5.67 | 0.33 |
| 2 | 6 | 3.33 | 6.33 | 1.00 |
| 10 | 3 | 3.33 | 6.33 | 3.33 |

시간 할당량이 충분히 크면 이 모델의 RR은 FCFS와 같은 실행 순서가 됩니다.
작은 시간 할당량은 실행 기회를 더 자주 나누지만, 평균 대기까지 항상 개선하는 것은 아닙니다.
실제 문맥 교환 비용은 이 모델에 포함하지 않았습니다.
[V3 상세 분석](docs/07-ver3-round-robin.md)에 큐 변화와 손계산을 정리했습니다.

## 자동 테스트

PowerShell 7 이상과 GCC가 필요합니다.

```powershell
pwsh -File .\tests\test_fcfs.ps1
pwsh -File .\tests\test_ver2.ps1
pwsh -File .\tests\test_ver3.ps1
pwsh -File .\tests\test_ver4.ps1
node .\tests\test_viewer.js
```

소스를 다시 빌드한 후 고정 예제, 입력 검증, 최대 입력, 독립적인 tick 단위 참조 모델을 확인합니다.
Windows에서 기존 702개와 V4 파일 입출력 176개, V5 뷰어 모델 24개로 **총 902개 검사**를 통과했습니다.
검증 항목과 결과는 [테스트 문서](docs/04-testing.md)에 기록했습니다.

## 버전 관리

V1부터 V5까지 `v1.0.0`·`v2.0.0`·`v3.0.0`·`v4.0.0`·`v5.0.0` 태그로 구분합니다.
저장소 주소와 폴더 구조를 유지한 채 변경 이력을 쌓습니다.
각 태그에는 당시 소스·README·문서·테스트가 함께 남습니다.
다음 단계는 [ver6 성능·대규모 실험](docs/05-roadmap.md)입니다.
