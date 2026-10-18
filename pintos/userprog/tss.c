#include "userprog/tss.h"
#include <debug.h>
#include <stddef.h>
#include "userprog/gdt.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "intrinsic.h"

/* TSS (Task-State Segment, 태스크 상태 세그먼트).
 *
 *  TSS는 x86-64 전용 구조체로, 프로세서에 내장된 멀티태스킹 지원 기능인
 *  "태스크"를 정의하는 데 쓰인다. 하지만 이식성, 속도, 유연성 등의
 *  이유로 대부분의 x86-64 OS는 TSS를 거의 완전히 무시한다.
 *  우리도 예외가 아니다.
 *
 *  다만 TSS로만 할 수 있는 일이 하나 있다. 바로 유저 모드에서
 *  발생한 인터럽트를 처리할 때의 스택 전환이다.
 *  유저 모드(ring 3)에서 인터럽트가 발생하면, 프로세서는 현재 TSS의
 *  rsp0 멤버를 보고 인터럽트 처리에 사용할 스택을 결정한다.
 *  따라서 TSS를 만들고 최소한 이 필드들은 초기화해야 하며,
 *  이 파일이 바로 그 일을 한다.
 *
 *  인터럽트가 interrupt gate 또는 trap gate로 처리될 때(우리가 처리하는
 *  모든 인터럽트가 해당된다), x86-64 프로세서는 다음과 같이 동작한다.
 *
 *    - 인터럽트된 코드가 인터럽트 핸들러와 같은 ring에 있다면 스택
 *      전환은 일어나지 않는다. 커널 안에서 실행 중일 때 발생한
 *      인터럽트가 이 경우다. 이때 TSS의 내용은 상관없다.
 *
 *    - 인터럽트된 코드가 핸들러와 다른 ring에 있다면, 프로세서는
 *      새 ring을 위해 TSS에 지정된 스택으로 전환한다. 유저 영역에서
 *      발생한 인터럽트가 이 경우다. 이미 사용 중인 스택을 쓰면 내용이
 *      깨질 수 있으므로, 사용 중이 아닌 스택으로 전환하는 것이
 *      중요하다. 유저 영역에서 실행 중이라면 현재 프로세스의 커널 스택은
 *      사용 중이 아니라는 것을 알 수 있으므로, 항상 그 스택을 쓸 수 있다.
 *      그래서 스케줄러가 스레드를 바꿀 때 TSS의 스택 포인터도
 *      새 스레드의 커널 스택을 가리키도록 바꾼다.
 *      (이 호출은 thread.c의 schedule에 있다.) */

/* 커널 TSS. */
struct task_state *tss;

/* 커널 TSS를 초기화한다. */
void
tss_init (void) {
	/* 우리의 TSS는 call gate나 task gate에서 사용되지 않으므로
	 * 일부 필드만 참조된다. 그래서 그 필드들만 초기화한다. */
	tss = palloc_get_page (PAL_ASSERT | PAL_ZERO);
	tss_update (thread_current ());
}

/* 커널 TSS를 반환한다. */
struct task_state *
tss_get (void) {
	ASSERT (tss != NULL);
	return tss;
}

/* TSS의 ring 0 스택 포인터가 스레드 스택의 끝을 가리키도록 설정한다. */
void
tss_update (struct thread *next) {
	ASSERT (tss != NULL);
	tss->rsp0 = (uint64_t) next + PGSIZE;
}
