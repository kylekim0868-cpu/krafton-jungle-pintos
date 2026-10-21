# Project 2: User Programs

## 테스트 결과

- **최종 점수:** 미확인 — 배점 기준 확인 후 입력
- **전체 테스트:** 95개 중 67개 PASS / 28개 FAIL
- **테스트 상태:** 전체 통과 미완료
- **검증 브랜치:** 추후 입력
- **검증 커밋:** 추후 입력

### 영역별 결과

| 테스트 영역 | 전체 | PASS | FAIL |
|---|---:|---:|---:|
| User Programs (`tests/userprog`, no-vm 제외) | 63 | 38 | 25 |
| File System 기본 (`tests/filesys/base`) | 13 | 11 | 2 |
| 메모리 부족 (`tests/userprog/no-vm`) | 1 | 0 | 1 |
| Threads 회귀 테스트 (`tests/threads`) | 18 | 18 | 0 |
| **합계** | **95** | **67** | **28** |

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
| fork-once | ❌ FAIL |
| fork-multiple | ❌ FAIL |
| fork-recursive | ❌ FAIL |
| fork-read | ❌ FAIL |
| fork-close | ❌ FAIL |
| fork-boundary | ❌ FAIL |
| exec-once | ❌ FAIL |
| exec-arg | ❌ FAIL |
| exec-boundary | ❌ FAIL |
| exec-missing | ❌ FAIL |
| exec-bad-ptr | ✅ PASS |
| exec-read | ❌ FAIL |
| wait-simple | ❌ FAIL |
| wait-twice | ❌ FAIL |
| wait-killed | ❌ FAIL |
| wait-bad-pid | ✅ PASS |
| multi-recurse | ❌ FAIL |
| multi-child-fd | ❌ FAIL |
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
| syn-read | ❌ FAIL |
| syn-remove | ✅ PASS |
| syn-write | ❌ FAIL |

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
- 테스트 실행 환경: 추후 입력
- 테스트 실행 명령어: 추후 입력
- 검증 날짜: 추후 입력

## 비고

- Project 2 개발 중 전체 테스트 실행 결과.
- 제공된 실행 결과를 기준으로 작성했으며, 개별 배점은 반영하지 않음.
- 전체 집계에는 Threads 회귀 테스트 18개가 포함됨.
- 실패한 테스트의 원인은 별도 분석이 필요함.