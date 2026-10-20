#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "filesys/filesys.h" /* 파일 생성·삭제 등 파일 시스템 함수의 선언을 가져온다. */
#include "threads/synch.h"
#include "threads/mmu.h" /* 페이지 매핑 확인 */
#include "threads/vaddr.h" /* 유저 주소 영역 확인 */

/* 공용 락 선언 */
struct lock filesys_lock;

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* 시스템 콜.
 *
 * 예전에는 시스템 콜 서비스를 인터럽트 핸들러가 처리했다
 * (예: 리눅스의 int 0x80). 하지만 x86-64에서는 제조사가 시스템 콜을
 * 요청하는 더 효율적인 경로인 `syscall` 명령어를 제공한다.
 *
 * syscall 명령어는 MSR(Model Specific Register, 모델별 레지스터)의
 * 값을 읽어서 동작한다. 자세한 내용은 매뉴얼을 참고할 것. */

#define MSR_STAR 0xc0000081         /* 세그먼트 셀렉터 MSR */
#define MSR_LSTAR 0xc0000082        /* Long mode에서 SYSCALL이 점프할 목적지 */
#define MSR_SYSCALL_MASK 0xc0000084 /* eflags용 마스크 */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* 파일시스템 공용 락 초기화 */
	lock_init(&filesys_lock);
	/* syscall_entry가 유저 영역 스택을 커널 모드 스택으로 바꾸기 전까지는
	 * 인터럽트 서비스 루틴이 어떤 인터럽트도 처리하면 안 된다.
	 * 따라서 FLAG_FL을 마스킹했다. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* Project 2 : 파일 이름을 종료 문자까지 안전하게 읽을 수 있는지 검사한다. */
static bool
valid_user_string(const char *str) {
	if (str == NULL)
		return false; /* 문자열 주소가 NULL이면 실패합니다. */

	const char *p = str; /* 첫 번째 문자부터 검사하기 위해 현재 위치를 p에 저장합니다. */

	for (;;) {
		if (!is_user_vaddr(p))
			return false; /* 현재 주소가 유저 영역 밖이면 실패합니다. 커널 메모리를 파일 이름으로 읽지 못하게 합니다. */

		if (pml4_get_page(thread_current()->pml4, p) == NULL)
			return false; /* 유저 영역이더라도 실제 메모리가 매핑되지 않은 주소라면 실패합니다. */

		if (*p == '\0')
			return true; /* 앞의 주소 검사를 통과한 뒤 문자를 읽습니다. 문자열 끝이면 검사 성공입니다. */

		p++; /* 다음 문자로 이동해서 반복합니다. 따라서 문자열이 페이지 경계를 넘어가도 다음 주소를 검사합니다. */
	}
}

/* 시스템 콜의 메인 인터페이스 */
void
syscall_handler (struct intr_frame *f) {
	// TODO: 여기에 구현을 작성할 것.
	/* 
		Syscall Dispatcher
		case 0~13개에 해당되는 syscall 기능을 switch문으로 구현
	*/
	switch (f->R.rax)
	{
		case SYS_HALT: 
			break;
		case SYS_EXIT: 
			break;
		case SYS_FORK: 
			break;
		case SYS_EXEC: 
			break;
		case SYS_WAIT: 
			break;
		case SYS_CREATE: {

			const char *file = (const char *) f->R.rdi; /* rdi에 들어 있는 값을 파일 이름 문자열의 주소로 해석해서, 포인터 변수 file에 저장 */

			if (!valid_user_string(file)) {
				/* TODO : 공통 EXIT 구현 후 종료 상태 -1로 종료하도록 변경한다. */
				f->R.rax = false;
				return;
			}
			lock_acquire(&filesys_lock); /* 다른 파일 시스템 작업이 끝날 때까지 기다린 뒤 락을 획득합니다. */

			bool result = filesys_create(file, (unsigned) f->R.rsi); /* rdi의 파일 이름과 rsi의 초기 크기로 생성하고, 성공 여부를 반환값 레지스터 rax에 저장한다. */

			lock_release(&filesys_lock); /* 다른 작업이 진행할 수 있도록 락을 해제합니다. */

			f->R.rax = result; /* 생성 결과를 유저에게 전달한 반환값으로 저장합니다. */
			break;
		}
		case SYS_REMOVE:
			break;
		case SYS_OPEN:
			break;
		case SYS_FILESIZE:
			break;		
		case SYS_READ:
			break;
		case SYS_WRITE:
			break;
		case SYS_SEEK:
			break;
		case SYS_TELL:
			break;
		case SYS_CLOSE:
			break;
		default:
			break;
	}	
	
	printf ("system call!\n");
	thread_exit ();
}
