#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "filesys/file.h"
#include "lib/kernel/stdio.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

int syscall_write(int fd, void *buffer, unsigned size);
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
syscall_write (int fd, void *buffer, unsigned size)
{	
	/* 유저 영역인지, 매핑되어있는지 확인 */
	if (!check_buffer(buffer, size)) return -1;
	/* fd=0 : 잘못된 접근(키보드) false 반환 */
	if(fd==0) return -1;
	/* fd=1 : 콘솔에 출력 putbuf함수는 커널이 화면에 글자를 찍는 함수  */
	if(fd==1){
		putbuf(buffer, size);
		return size;
	}
	/* fd>=2 : 표에서 파일 꺼냄 */
	else if(fd>=2){
		struct list_elem* cur_file = NULL;
		int index=0;
		/* [TODO] : file_table 순회하는 로직 필요 */
		for(cur_file=list_begin(thread_current()->fd_list); cur_file!=NULL; cur_file=list_next(cur_file)){
			if(fd == index) break;
			index++;
		}
		/* 순회 이후 cur_file이 NULL이면 리스트에 아무것도 채워지지 않았다는 뜻 */
		if(cur_file == NULL) return -1;
		struct file_descriptor* fd_file = list_entry(cur_file, struct file_descriptors, fd_elem);
		if(fd_file->file == NULL) return -1;
		off_t write_byte = file_write(fd_file->file, buffer, size);
		return (int)write_byte;
	}
}

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* syscall_entry가 유저 영역 스택을 커널 모드 스택으로 바꾸기 전까지는
	 * 인터럽트 서비스 루틴이 어떤 인터럽트도 처리하면 안 된다.
	 * 따라서 FLAG_FL을 마스킹했다. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
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
		case SYS_CREATE:
			break;
		case SYS_REMOVE:
			break;
		case SYS_OPEN:
			break;
		case SYS_FILESIZE:
			break;		
		/* 내 파트 */
		case SYS_READ:
			break;
		case SYS_WRITE:
			f->R.rax = syscall_write(f->R.rdi, f->R.rsi, f->R.rdx);
		/* 내 파트 */
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
