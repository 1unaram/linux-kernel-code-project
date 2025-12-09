# Linux Kernel PID 시스템 개선 프로젝트 (Skiplist 및 RB-Skiplist 기반)

<br>

## Project Summary

이 프로젝트는 기존 리눅스 커널에서 해시 테이블(정확히는 radix tree) 기반으로 관리하던 PID 관리 구조를 Skip List와 Red-Black Skip List(RB-Skiplist) 기반 구조로 교체하여, 프로세스 관리 시스템의 성능 특성과 동작을 비교·분석하기 위한 성능 비교 연구 프로젝트이다. PID의 할당과 해제가 매우 빈번하게 일어나고, PID 개수가 많아질수록 트리의 높이가 증가하여 캐시 지역성이 떨어지고 캐시 미스 발생 확률이 높아지는 문제를 완화하는 것이 목표이다.

리눅스 커널에서는 Red Black Tree, Radix Tree, List, Hash Table 등 다양한 자료구조를 여러 서브시스템에 사용한다. PID 시스템의 경우 radix tree 구조를 이용하여 PID의 삽입, 조회, 삭제 등을 관리하는데, 노드가 퍼져 있거나 PID 개수가 많아질 경우 비효율이 발생할 수 있다. 이러한 단점을 보완하기 위해 Skip List 및 RB-Skiplist 자료구조를 도입하여 PID 관리 구조를 변경하고, microbenchmark와 system benchmark를 통해 성능을 비교하였다.

<br>

## Team Information

-   과목: 2025-2 Linux System and Its Application Term-Project
-   팀: Team 5

| 이름   | 역할                          | 비고              |
| ------ | ----------------------------- | ----------------- |
| 김하람 | PM / Kernel 개발              | 소프트웨어학부 21 |
| 김도연 | Kernel 개발                   | 소프트웨어학부 21 |
| 이승화 | 발표 준비 및 자료 분석        | 소프트웨어학부 21 |
| 황민   | 발표 및 자료 분석 / 성능 개선 | 소프트웨어학부 23 |

<br>

## Repository Structure

본 저장소는 **3개의 브랜치**로 구성되어 있습니다:

-   **`master`**: 프로젝트 문서, 핵심 수정 파일, 성능 테스트 결과
-   **`skiplist`**: Skip List가 적용된 **전체 Linux 5.4.214 커널 소스코드**
-   **`rb-skiplist`**: RB-Skip List가 적용된 **전체 Linux 5.4.214 커널 소스코드**

### 브랜치별 사용 방법

```bash
# Skip List 커널 소스코드 받기
git clone -b skiplist https://github.com/1unaram/linux-kernel-code-project.git skiplist-kernel
cd skiplist-kernel

# RB-Skip List 커널 소스코드 받기
git clone -b rb-skiplist https://github.com/1unaram/linux-kernel-code-project.git rb-skiplist-kernel
cd rb-skiplist-kernel

# 바로 빌드 가능
make menuconfig
make -j$(nproc)
```

## Directory Structure (master 브랜치)

```
linux-kernel-code-project/
├── README.md                          # 프로젝트 설명 문서
├── presentation-file.pdf              # 발표 자료
├── perf_pid_test.sh                   # PID 성능 테스트 자동화 스크립트
├── .gitignore                         # Git 무시 파일 목록
│
├── skiplist/                          # Skip List 기반 PID 관리 구현 (핵심 파일만)
│   ├── Kconfig                        # 커널 설정 옵션
│   ├── Makefile                       # 빌드 설정 파일
│   ├── pid.c                          # PID 할당/해제 핵심 로직
│   ├── pid.h                          # PID 구조체 및 함수 선언
│   ├── pid_namespace.c                # PID 네임스페이스 관리
│   ├── pid_namespace.h                # PID 네임스페이스 헤더
│   ├── pid_skiplist.c                 # Skip List 자료구조 구현
│   ├── pid_skiplist.h                 # Skip List 헤더
│   ├── sched.c                        # 스케줄러 관련 코드
│   └── loadavg.c                      # 시스템 로드 평균 계산
│
├── rb-skiplist/                       # RB-Skip List 기반 PID 관리 구현 (핵심 파일만)
│   ├── Kconfig                        # 커널 설정 옵션 (CONFIG_PID_RB_SKIPLIST)
│   ├── Makefile                       # 빌드 설정 파일
│   ├── pid.c                          # PID 할당/해제 핵심 로직
│   ├── pid.h                          # PID 구조체 및 함수 선언
│   ├── pid_namespace.c                # PID 네임스페이스 관리
│   ├── pid_namespace.h                # PID 네임스페이스 헤더
│   ├── pid_rb_skiplist.c              # RB-Skip List 자료구조 구현
│   ├── pid_rb_skiplist.h              # RB-Skip List 헤더
│   ├── sched.c                        # 스케줄러 관련 코드
│   └── loadavg.c                      # 시스템 로드 평균 계산
│
└── performance-test-result/           # 성능 테스트 결과
    ├── perf/                          # perf 도구를 이용한 성능 분석
    │   ├── original/                  # 오리지널 커널 perf 결과
    │   ├── skiplist/                  # Skip List 커널 perf 결과
    │   └── rb-skiplist/               # RB-Skiplist 커널 perf 결과
    │
    └── UnixBench/                     # UnixBench 시스템 벤치마크 결과
        ├── original-1st.txt/html      # 오리지널 커널 1차 테스트
        ├── original-2nd.txt/html      # 오리지널 커널 2차 테스트
        ├── skiplist-1st.txt/html      # Skip List 커널 1차 테스트
        ├── skiplist-2nd.txt/html      # Skip List 커널 2차 테스트
        ├── skiplist-3rd.txt/html      # Skip List 커널 3차 테스트
        ├── rb-skiplist-1st.txt/html   # RB-Skiplist 커널 1차 테스트
        └── rb-skiplist-2nd.txt/html   # RB-Skiplist 커널 2차 테스트
```

<br>

## 주요 구현 내용

### 1. Skiplist 기반 PID 관리 (skiplist/)

-   확률적 균형 트리 구조를 이용하여 PID를 관리한다.
-   탐색, 삽입, 삭제 연산에 대해 평균적으로 \(O(\log n)\) 시간 복잡도를 갖도록 설계하였다.
-   기존 radix tree 기반 PID 관리 코드(pid.c, pid_namespace.c 등)를 Skiplist 기반 로직으로 대체하였다.
-   컴파일 옵션: `CONFIG_PID_SKIPLIST` 사용

### 2. Red-Black Skiplist 기반 PID 관리 (rb-skiplist/)

-   Skiplist 구조에 Red-Black Tree의 균형 특성을 결합한 형태로 설계하였다.
-   일반 Skiplist보다 더 안정적인 성능(최악 시간 복잡도 측면)을 목표로 한다.
-   PID 관리, 네임스페이스, 스케줄러 관련 로직을 RB-Skiplist 중심으로 재구성하였다.
-   컴파일 옵션: `CONFIG_PID_RB_SKIPLIST` 사용

### 3. 성능 테스트

-   perf 도구를 활용하여 fork, clone, context switch, wait 등의 시스템 콜 성능을 측정하였다.
-   UnixBench를 이용해 전체 시스템 성능을 벤치마크하여, 오리지널 커널, Skip List 커널, RB-Skiplist 커널 간의 성능 차이를 비교하였다.

예시 microbenchmark(단순 fork + wait 기반) 결과(평균 대기 시간):

| 자료구조 타입 | 평균 대기 시간 (ms) |
| ------------- | ------------------- |
| original      | 733.43 ms           |
| skiplist      | 750.95 ms           |
| rb-skiplist   | 773.29 ms           |

<img width="940" height="536" alt="image" src="https://github.com/user-attachments/assets/7a81f4ef-15f9-4192-872f-148132833b8c" />

단순 fork 기준 microbenchmark에서는 자료구조 복잡도 증가로 인해 Skiplist 및 RB-Skiplist 모두 오리지널 커널 대비 대기 시간이 증가하였다.
반면 UnixBench 기반 system benchmark에서는 단일 코어 환경에서 Skiplist 및 RB-Skiplist 모두 약 10~20% 수준의 점수 상승이 관측되었으며, 멀티 코어 환경에서는 Skiplist의 점수가 전반적으로 하락하는 경향을 보인 반면, RB-Skiplist는 일부 테스트에서 소폭(약 1.6%) 점수 상승을 보이기도 했다.

<br>

## 파일 설명

### 소스 코드

-   **`skiplist/`**: Skip List 기반 PID 관리 구현

    -   `pid_skiplist.c/h`: Skip List 자료구조 핵심 구현 (삽입, 삭제, 조회)
    -   `pid.c`: Skip List를 이용한 PID 할당 및 해제 로직
    -   `pid_namespace.c`: PID 네임스페이스 관리 (Skip List 기반)
    -   `Kconfig`: 커널 빌드 시 Skip List 옵션 설정

-   **`rb-skiplist/`**: Red-Black Skip List 기반 PID 관리 구현
    -   `pid_rb_skiplist.c/h`: RB-Skip List 자료구조 핵심 구현
    -   `pid.c`: RB-Skip List를 이용한 PID 할당 및 해제 로직
    -   `pid_namespace.c`: PID 네임스페이스 관리 (RB-Skip List 기반)
    -   `Kconfig`: 커널 빌드 시 RB-Skip List 옵션 설정 (`CONFIG_PID_RB_SKIPLIST`)

### 성능 테스트 결과

-   **`performance-test-result/perf/`**: perf 도구를 이용한 시스템 콜 단위 성능 분석

    -   `original/`: 오리지널 커널(radix tree 기반) 성능 측정 결과
    -   `skiplist/`: Skip List 커널 성능 측정 결과
    -   `rb-skiplist/`: RB-Skip List 커널 성능 측정 결과

-   **`performance-test-result/UnixBench/`**: 전체 시스템 벤치마크
    -   각 커널 버전별 UnixBench 점수 및 HTML 형식 상세 리포트
    -   1차, 2차, 3차 테스트 결과 포함

<br>

## 사용 방법

### 1. 커널 컴파일 및 적용

#### 사전 요구사항

-   gcc, make, binutils 등 커널 빌드 도구
-   충분한 디스크 공간 (최소 20GB 이상 권장)
-   Git

#### 방법 1: 전체 커널 소스코드 사용 (권장)

**`skiplist` 또는 `rb-skiplist` 브랜치에는 이미 자료구조가 적용된 전체 Linux 5.4.214 커널 소스코드가 포함되어 있습니다.**

```bash
# Skip List 커널 빌드
git clone -b skiplist https://github.com/1unaram/linux-kernel-code-project.git skiplist-kernel
cd skiplist-kernel

# 또는 RB-Skip List 커널 빌드
git clone -b rb-skiplist https://github.com/1unaram/linux-kernel-code-project.git rb-skiplist-kernel
cd rb-skiplist-kernel

# 패키지 설치
sudo apt install build-essential libncurses-dev flex bison libssl-dev libelf-dev dwarves git -y

# 커널 설정 (이미 적용된 설정 확인/수정)
make menuconfig

# 빌드 및 설치
make -j$(nproc)
sudo make modules_install
sudo make install

# 부트로더 업데이트
sudo vim /etc/default/grub # TIMEOUT_STYLE=hidden 주석 처리, timeout 10으로 변경
sudo update-grub

# 재부팅
reboot
```

#### 방법 2: 수동으로 파일 적용

1. **커널 소스 준비**

    ```bash
    wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.4.214.tar.xz
    tar -xvf linux-5.4.214.tar.xz
    cd linux-5.4.214
    ```

2. **소스 파일 적용**

    - master 브랜치의 `skiplist/` 또는 `rb-skiplist/` 디렉터리에서 수정된 파일들을 커널 소스 트리의 해당 위치로 복사
    - 주요 적용 위치:

        - include/linux/pid_namespace.h
        - include/linux/pid.h
        - kernel/pid_namespace.c
        - kernel/pid.c
        - arc/powerpc/platforms/cell/spufs/sched.c
        - fs/proc/loadavg.c
        - init/Kconfig

    - 새로 추가:
        - include/linux/pid_skiplist.h (또는 pid_rb_skiplist.h)
        - lib/pid_skiplist.c (또는 pid_rb_skiplist.c)

3. **패키지 설치**

    ```bash
    sudo apt install build-essential libncurses-dev flex bison libssl-dev libelf-dev dwarves git -y
    ```

4. **커널 설정**

    ```bash
    make menuconfig
    ```

    - General setup → PID Data Structure 옵션에서 선택:
        - `CONFIG_PID_SKIPLIST`: Skip List 사용
        - `CONFIG_PID_RB_SKIPLIST`: RB-Skip List 사용
        - 기본값: Radix Tree (오리지널)

    <img width="566" height="414" alt="image" src="https://github.com/user-attachments/assets/ceefc569-74da-4961-85ea-ed05a57640be" />

5. **커널 컴파일**

    ```bash
    make -j$(nproc)              # 병렬 빌드
    make modules_install         # 모듈 설치
    make install                 # 커널 이미지 설치
    ```

6. **부트로더 업데이트**

    ```bash
    sudo vim /etc/default/grub # TIMEOUT_STYLE=hidden 주석 처리, timeout 시간 10으로 변경
    sudo update-grub
    ```

7. **재부팅 및 확인**

    ```bash
    reboot
    uname -r                     # 커널 버전 확인
    ```

    ```bash
    dmesg | grep "PID SKIPLIST"  # 적용 확인
    [    0.745715] [pid_idr_init] PID SKIPLIST is successfully init!
    ```

<br>

### 2. 성능 테스트 수행

#### Microbenchmark (perf 도구)

1. **perf 도구 설치**

```bash
cd <kernel-source-code>/tools/perf
make
sudo make install
perf --version
sudo sysctl -w kernel.perf_event_paranoid=-1 # 권한 설정
```

2. **자동화 스크립트 사용**

본 프로젝트에서는 종합적인 PID 성능 테스트를 위한 스크립트를 제공합니다:

```bash
# perf_pid_test.sh 실행 권한 부여
chmod +x perf_pid_test.sh

# 테스트 실행
./perf_pid_test.sh
```

스크립트는 다음 5가지 테스트를 자동으로 수행합니다:

-   **Test 1**: Process Creation (fork)
-   **Test 2**: Clone (with namespace)
-   **Test 3**: Context Switching
-   **Test 4**: Detailed CPU Profiling
-   **Test 5**: Wait Performance

결과는 `perf_results_<kernel-version>_<timestamp>/` 디렉터리에 저장됩니다.

#### System Benchmark (UnixBench)

```bash
# UnixBench 설치 및 실행
git clone https://github.com/kdlucas/byte-unixbench
cd byte-unixbench/UnixBench
./Run

# 결과 파일은 results/ 디렉터리에 생성됨
```

<br>

### 3. 성능 결과 분석

#### Perf 결과 확인

`performance-test-result/perf/` 디렉터리에서 각 구현별 성능 비교:

-   `original/`: 오리지널 커널 (radix tree 기반)
-   `skiplist/`: Skip List 커널
-   `rb-skiplist/`: RB-Skip List 커널

#### 주요 성능 지표 및 의미

| **지표**         | **의미**             | **PID 성능과의 관계**           |
| ---------------- | -------------------- | ------------------------------- |
| cycles           | CPU 사이클 수        | ↓ PID 할당/조회가 빠를수록 감소 |
| instructions     | 실행된 명령어 수     | PID 자료구조 코드 효율성        |
| task-clock       | 실제 CPU 사용 시간   | ↓ 전체 오버헤드 감소 지표       |
| context-switches | 컨텍스트 스위칭 횟수 | PID 조회 빈도와 직접 연관       |
| page-faults      | 페이지 폴트 발생     | 메모리 접근 패턴 (캐시 효율성)  |
| cache-misses     | 캐시 미스 횟수       | 캐시 지역성 및 메모리 접근 패턴 |

<br>

#### UnixBench 결과 확인

`performance-test-result/UnixBench/` 디렉터리의 결과 파일:

-   `.txt` 파일: 텍스트 형식 점수 및 통계
-   `.html` 파일: 시각화된 상세 리포트

비교 항목:

-   System Benchmarks Index Score (전체 점수)
-   Process Creation, Context Switching, System Call 등 개별 테스트 점수

<br>

## 기술적 세부사항

### Skiplist 구현 특징

-   레벨당 50% 확률로 상위 레벨 링크 생성
-   최대 레벨: 32
-   RCU(Read-Copy-Update) 기반 동시성 제어로 읽기 성능 최적화
-   평균 시간 복잡도: O(log n)

### RB-Skip List 구현 특징

-   Skip List의 최상위 레벨에 Red-Black Tree 추가
-   두 단계 검색: RB-Tree로 범위 좁힌 후 Skip List로 정확한 위치 탐색
-   최악의 경우에도 O(log n) 성능 보장
-   캐시 지역성 개선을 위한 노드 구조 최적화

### 주요 수정 파일

-   `kernel/pid.c`: PID 할당/해제 핵심 로직
-   `kernel/pid_namespace.c`: 네임스페이스별 PID 관리
-   `include/linux/pid.h`: PID 구조체 정의
-   `include/linux/pid_namespace.h`: 네임스페이스 구조체 정의
-   `kernel/sched.c`: 스케줄러 통합
-   `fs/proc/loadavg.c`: /proc/loadavg 시스템 정보

## 성능 분석 결과 요약

| 테스트 유형       | Original    | Skip List         | RB-Skip List      |
| ----------------- | ----------- | ----------------- | ----------------- |
| Microbench (fork) | 733.43 ms   | 750.95 ms (+2.4%) | 773.29 ms (+5.4%) |
| UnixBench Single  | 기준 (100%) | ~110-120%         | ~110-120%         |
| UnixBench Multi   | 기준 (100%) | ~95-98%           | ~100-101.6%       |

**결론:**

-   단순 fork 작업에서는 자료구조 복잡도로 인해 약간의 성능 저하
-   복잡한 시스템 워크로드(UnixBench)에서는 싱글 코어 환경에서 10-20% 성능 향상
-   멀티 코어 환경에서 RB-Skip List가 Skip List 대비 더 안정적인 성능

<br>

## Git 브랜치 정보

### 브랜치 구조

본 저장소는 **3개의 브랜치**로 구성되어 있으며, 각 브랜치는 다음과 같은 목적을 가집니다:

| 브랜치          | 설명                                 | 용도                                       |
| --------------- | ------------------------------------ | ------------------------------------------ |
| **master**      | 문서, 핵심 수정 파일, 테스트 결과    | 프로젝트 개요 및 비교를 위한 핵심 파일     |
| **skiplist**    | **Skip List 적용 전체 커널 소스**    | 즉시 빌드 가능한 완전한 Linux 5.4.214 커널 |
| **rb-skiplist** | **RB-Skip List 적용 전체 커널 소스** | 즉시 빌드 가능한 완전한 Linux 5.4.214 커널 |

### 브랜치별 특징

**master 브랜치:**

-   프로젝트 문서 및 발표 자료
-   핵심 수정 파일만 포함 (저장소 용량 최소화)
-   성능 테스트 결과 및 분석 자료
-   자동화된 성능 테스트 스크립트 (`perf_pid_test.sh`)

**skiplist 브랜치:**

-   **Linux 5.4.214 LTS 전체 소스코드** (~1GB)
-   Skip List 자료구조가 적용된 상태
-   `CONFIG_PID_SKIPLIST` 컴파일 옵션 포함
-   **바로 빌드 및 설치 가능**

**rb-skiplist 브랜치:**

-   **Linux 5.4.214 LTS 전체 소스코드** (~1GB)
-   Red-Black Skip List 자료구조가 적용된 상태
-   `CONFIG_PID_RB_SKIPLIST` 컴파일 옵션 포함
-   **바로 빌드 및 설치 가능**

### 브랜치 사용 방법

```bash
# 전체 커널 소스코드가 포함된 브랜치 클론
git clone -b skiplist https://github.com/1unaram/linux-kernel-code-project.git skiplist-kernel
git clone -b rb-skiplist https://github.com/1unaram/linux-kernel-code-project.git rb-skiplist-kernel

# 또는 로컬에서 브랜치 전환
git checkout skiplist      # Skip List 커널로 전환
git checkout rb-skiplist   # RB-Skip List 커널로 전환
git checkout master        # master 브랜치로 돌아가기
```

### 브랜치를 이용한 빠른 테스트

**skiplist 또는 rb-skiplist 브랜치를 사용하면 별도의 파일 복사 없이 바로 커널을 빌드할 수 있습니다:**

```bash
# 1. 원하는 브랜치 클론
git clone -b skiplist https://github.com/1unaram/linux-kernel-code-project.git skiplist-kernel
cd skiplist-kernel

# 2. 바로 빌드
make menuconfig  # PID 자료구조 옵션 확인
make -j$(nproc)
sudo make modules_install
sudo make install

# 3. 재부팅 후 테스트
reboot
```

<br>

## 참고 자료

-   RB-SkipList: 탐색 성능 향상을 위한 SkipList와 RB-Tree의 통합 기법 (2024.7, 신호진 외 4명)

### 추가 자료

-   발표 자료: `presentation-file.pdf`
-   Linux 5.4.214 LTS: https://cdn.kernel.org/pub/linux/kernel/v5.x/
-   GitHub Repository: https://github.com/1unaram/linux-kernel-code-project
    -   master: 문서 및 핵심 파일
    -   skiplist: Skip List 적용 전체 커널
    -   rb-skiplist: RB-Skip List 적용 전체 커널

<br>

## License

이 프로젝트는 Linux Kernel 코드를 기반으로 하며, GPL-2.0 라이선스를 따릅니다.
