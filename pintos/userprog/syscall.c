#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "threads/synch.h"

/* 공용 락 선언 */
struct lock filesys_lock;

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

int read (int fd, void *buffer, unsigned size);

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

/* buffer ~ buffer+size까지가 유저 영역이 맞는지 매핑이 되어있는지 확인하는 헬퍼 함수 */
bool
check_buffer(void *buffer, unsigned size)
{
	if (buffer == NULL) return false;
	if (size == 0) return true;
	/* uint8_t로 한 이유는 비트가 8비트가 1byte인데 이렇게
	   해야 size를 더했을때 정확한 end주소가 나옴 */
	uint8_t *start = buffer;
	uint8_t *end = start + size - 1;
	/* 하나라도 유저영역이 아니면 false */
	if (!is_user_vaddr(start) || !is_user_vaddr(end))
		return false;

	/* 버퍼가 걸친 모든 페이지가 매핑돼 있는지 */
	for (uint8_t *p = pg_round_down(start); p <= end; p += PGSIZE)
		/* 페이지에 매핑된게 있는지 검사 */
		if (pml4_get_page(thread_current()->pml4, p) == NULL)
			return false;
	/* 전부 통과시 true */
	return true;
}

int
read (int fd, void *buffer, unsigned size){
	if(!check_buffer(buffer, size)) return -1;
	/* 키보드에서 읽어옴 */
	if(fd==0){
		uint8_t *buf = buffer;
		for(unsigned i=0; i<=size; i++){
			buf[i] = input_getc();
		}
		return size;
	}else if(fd==1) return -1;
	else{
		
	}
}


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

/* 시스템 콜의 메인 인터페이스 */
void
syscall_handler (struct intr_frame *f UNUSED) {
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
		case SYS_CREATE:
			break;
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
