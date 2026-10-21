# Project 2: User Programs

## 테스트 결과

- **최종 점수:** 80.5 / 100 (80.5%)
- **전체 테스트:** 95개 중 85개 PASS / 10개 FAIL
- **테스트 상태:** 전체 통과 미완료
- **검증 브랜치:** 추후 입력
- **검증 커밋:** 추후 입력

### 영역별 결과

| 테스트 영역 | 전체 | PASS | FAIL |
|---|---:|---:|---:|
| User Programs (`tests/userprog`, no-vm 제외) | 63 | 54 | 9 |
| File System 기본 (`tests/filesys/base`) | 13 | 13 | 0 |
| 메모리 부족 (`tests/userprog/no-vm`) | 1 | 0 | 1 |
| Threads 회귀 테스트 (`tests/threads`) | 18 | 18 | 0 |
| **합계** | **95** | **85** | **10** |

### 영역별 점수 (통과 개수 / 원점수)

아래 점수는 영역별 원점수이며, 최종 점수는 다음 표의 영역별 반영 비율에 따라 환산함.

| 영역 | 세부 항목 | 통과 | 점수 |
|---|---|---:|---:|
| **threads** | alarm | 6/6 ✅ | 7/7 |
| | priority | 12/12 ✅ | 25/25 |
| **userprog 기능** | args | 5/5 ✅ | 5/5 |
| | create | 4/4 ✅ | 4/4 |
| | open | 3/3 ✅ | 3/3 |
| | read | 2/2 ✅ | 2/2 |
| | write | 2/2 ✅ | 2/2 |
| | close | 1/1 ✅ | 1/1 |
| | fork | 4/4 ✅ | 6/6 |
| | exec | 3/3 ✅ | 4/4 |
| | wait | 2/2 ✅ | 2/2 |
| | exit | 1/1 ✅ | 1/1 |
| | halt | 1/1 ✅ | 1/1 |
| | 재귀 실행 | 2/2 ✅ | 4/4 |
| | **rox** | **0/3 ❌** | **0/5** |
| | *소계* | *30/33* | *35/40* |
| **userprog 견고성** | fd 처리 | 7/7 ✅ | 8/8 |
| | 포인터 처리 | 5/5 ✅ | 5/5 |
| | 페이지 경계 버퍼 | 6/6 ✅ | 12/12 |
| | null/빈 문자열 | 3/3 ✅ | 3/3 |
| | fork/exec/wait 견고성 | 3/3 ✅ | 6/6 |
| | **예외 처리 (bad-*)** | **0/6 ❌** | **0/6** |
| | *소계* | *24/30* | *34/40* |
| **no-vm** | **multi-oom** | **0/1 ❌** | **0/3** |
| **filesys/base** | 작은 파일 | 5/5 ✅ | 6/6 |
| | 큰 파일 | 5/5 ✅ | 6/6 |
| | 동기화 | 3/3 ✅ | 5/5 |
| | *소계* | *13/13* | *17/17* |

### 최종 점수 반영 (전체 100%)

| 영역 | 획득 | 만점 |
|---|---:|---:|
| threads alarm | 2.0% | 2.0% |
| threads priority | 3.0% | 3.0% |
| userprog 기능 | 35.0% | 40.0% |
| userprog 견고성 | 25.5% | 30.0% |
| no-vm | 0.0% | 10.0% |
| filesys/base | 15.0% | 15.0% |
| **합계** | **80.5%** | **100%** |

### 잃은 점수 (19.5%)

| 실패 | 잃은 점수 |
|---|---:|
| multi-oom | 10.0% |
| rox 3개 | 5.0% |
| bad-* 6개 | 4.5% |
| **합계** | **19.5%** |

## 개별 테스트 결과

### User Programs

경로: `tests/userprog/`

| 테스트 | 결과 |
|---|---|
| args-none | ✅ PASS |
| args-single | ✅ PASS |
| args-multiple | ✅ PASS |
| args-many | ✅ PASS |
| args-dbl-space | ✅ PASS |
| halt | ✅ PASS |
| exit | ✅ PASS |
| create-normal | ✅ PASS |
| create-empty | ✅ PASS |
| create-null | ✅ PASS |
| create-bad-ptr | ✅ PASS |
| create-long | ✅ PASS |
| create-exists | ✅ PASS |
| create-bound | ✅ PASS |
| open-normal | ✅ PASS |
| open-missing | ✅ PASS |
| open-boundary | ✅ PASS |
| open-empty | ✅ PASS |
| open-null | ✅ PASS |
| open-bad-ptr | ✅ PASS |
| open-twice | ✅ PASS |
| close-normal | ✅ PASS |
| close-twice | ✅ PASS |
| close-bad-fd | ✅ PASS |
| read-normal | ✅ PASS |
| read-bad-ptr | ✅ PASS |
| read-boundary | ✅ PASS |
| read-zero | ✅ PASS |
| read-stdout | ✅ PASS |
| read-bad-fd | ✅ PASS |
| write-normal | ✅ PASS |
| write-bad-ptr | ✅ PASS |
| write-boundary | ✅ PASS |
| write-zero | ✅ PASS |
| write-stdin | ✅ PASS |
| write-bad-fd | ✅ PASS |
| fork-once | ✅ PASS |
| fork-multiple | ✅ PASS |
| fork-recursive | ✅ PASS |
| fork-read | ✅ PASS |
| fork-close | ✅ PASS |
| fork-boundary | ✅ PASS |
| exec-once | ✅ PASS |
| exec-arg | ✅ PASS |
| exec-boundary | ✅ PASS |
| exec-missing | ✅ PASS |
| exec-bad-ptr | ✅ PASS |
| exec-read | ✅ PASS |
| wait-simple | ✅ PASS |
| wait-twice | ✅ PASS |
| wait-killed | ✅ PASS |
| wait-bad-pid | ✅ PASS |
| multi-recurse | ✅ PASS |
| multi-child-fd | ✅ PASS |
| rox-simple | ❌ FAIL |
| rox-child | ❌ FAIL |
| rox-multichild | ❌ FAIL |
| bad-read | ❌ FAIL |
| bad-write | ❌ FAIL |
| bad-read2 | ❌ FAIL |
| bad-write2 | ❌ FAIL |
| bad-jump | ❌ FAIL |
| bad-jump2 | ❌ FAIL |

### File System 기본

경로: `tests/filesys/base/`

| 테스트 | 결과 |
|---|---|
| lg-create | ✅ PASS |
| lg-full | ✅ PASS |
| lg-random | ✅ PASS |
| lg-seq-block | ✅ PASS |
| lg-seq-random | ✅ PASS |
| sm-create | ✅ PASS |
| sm-full | ✅ PASS |
| sm-random | ✅ PASS |
| sm-seq-block | ✅ PASS |
| sm-seq-random | ✅ PASS |
| syn-read | ✅ PASS |
| syn-remove | ✅ PASS |
| syn-write | ✅ PASS |

### 메모리 부족

경로: `tests/userprog/no-vm/`

| 테스트 | 결과 |
|---|---|
| multi-oom | ❌ FAIL |

### Threads 회귀 테스트

경로: `tests/threads/`

| 테스트 | 결과 |
|---|---|
| alarm-single | ✅ PASS |
| alarm-multiple | ✅ PASS |
| alarm-simultaneous | ✅ PASS |
| alarm-priority | ✅ PASS |
| alarm-zero | ✅ PASS |
| alarm-negative | ✅ PASS |
| priority-change | ✅ PASS |
| priority-donate-one | ✅ PASS |
| priority-donate-multiple | ✅ PASS |
| priority-donate-multiple2 | ✅ PASS |
| priority-donate-nest | ✅ PASS |
| priority-donate-sema | ✅ PASS |
| priority-donate-lower | ✅ PASS |
| priority-fifo | ✅ PASS |
| priority-preempt | ✅ PASS |
| priority-sema | ✅ PASS |
| priority-condvar | ✅ PASS |
| priority-donate-chain | ✅ PASS |

## 테스트 환경

- Project: KAIST Pintos
- 테스트 실행 환경: Docker

## 비고

- Project 2 개발 중 전체 테스트 실행 결과.
- 제공된 영역별 테스트 결과와 배점 기준을 반영함.
- 최종 환산 점수는 영역별 반영 비율을 적용한 값이며, 단순 테스트 통과율과 구분함.
- 전체 집계에는 Threads 회귀 테스트 18개가 포함됨.
