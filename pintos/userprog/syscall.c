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
#include "filesys/file.h"
#include "lib/kernel/stdio.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "devices/input.h"

/* 공용 락 선언 */
struct lock filesys_lock;

/* 전체 페이지 크기를 파일 포인터 크기 8바이트 나누면 전체 배열 사이즈가 나온다 */
const int fd_list_size = PGSIZE/8; 

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

int syscall_open(char *file);
int syscall_filesize(int fd);
int syscall_write(int fd, void *buffer, unsigned size);
int syscall_read (int fd, void *buffer, unsigned size);



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

static bool
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
	/* 유저 영역인지, 매핑되어있는지 확인 - syscall_exit() 호출로 프로세스 종료 */
	if (!check_buffer(buffer, size)) return -1; /* syscall_exit() 호출해야함 */
	/* fd=0 : 잘못된 접근(키보드) false 반환 */
	if(fd<=0 || fd>=fd_list_size) return -1;
	/* fd=1 : 콘솔에 출력 putbuf함수는 커널이 화면에 글자를 찍는 함수  */
	if(fd==1){
		putbuf(buffer, size);
		return size;
	}
	/* fd>=2 : 표에서 파일 꺼냄 */
	else if(fd>=2){
		if(!thread_current()->fd_list[fd])
			return -1;
		else{
			/* lock 처리 */
			lock_acquire(&filesys_lock);
			int write_byte = file_write(thread_current()->fd_list[fd], buffer, size);
			lock_release(&filesys_lock);
			return write_byte;
		}
	}
}

int
syscall_read (int fd, void *buffer, unsigned size){
	if(!check_buffer(buffer, size)) return -1; /* syscall_exit(-1)로 종료해야함 */
	/* 키보드에서 읽어옴 */
	if(fd==0){
		uint8_t *buf = buffer;
		/* 반복문 돌면서 하나씩 받아옴 size만큼 반복 */
		for(unsigned i=0; i<size; i++){
			buf[i] = input_getc();
		}
		return size;
	}
	/* fd=1 : 화면을 의미함으로 맞지 않은 fd임 */
	else if(fd==1 || fd < 0 || fd>=fd_list_size) 
		return -1;
	/* 파일에 접근해서 size만큼 읽어옴 */
	else{
		if(!thread_current()->fd_list[fd]) return -1;
		else{
			/* lock 처리 */
			lock_acquire(&filesys_lock);
			int byte_size = file_read(thread_current()->fd_list[fd], buffer, size);
			lock_release(&filesys_lock);
			return byte_size;
		}
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
			f->R.rax = syscall_open((char* )f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = syscall_filesize(f->R.rdi);
			break;		
		case SYS_READ:
			f->R.rax = syscall_read(f->R.rdi, (void *)f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			/* buffer는 포인터라서 uint64_t인 rsi가 들어가면 문제가 됨 캐스팅 해야함 */
			f->R.rax = syscall_write(f->R.rdi, (void*)f->R.rsi, f->R.rdx);
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
}

/* 
	fd_list 초기화 함수: 페이지를 할당받아 모든 페이지의 원소를 0으로 채워 초기화하는 동작
*/
void fd_list_init(struct thread* t){
	/* 페이지의 0을 채워 공간 할당 */
	t->fd_list = palloc_get_page(PAL_ZERO);
	if(!(t->fd_list)){
		thread_exit(); /* 프로세스 종료 */
	}
}

int syscall_open(char* file_name)
{
	struct thread *t = thread_current();

	/* ✔︎ 사용자 가상 주소 범위인지 확인 */
	if(!is_user_vaddr(file_name) || file_name == NULL || !(pml4_get_page(t->pml4, file_name))){
		thread_exit(); /* SYS_EXIT 함수가 구현되면 이 위치에 들어감 */
	}

	/* fd테이블이 비었다면 페이지 전체를 NULL로 초기화 */
	if(!t->fd_list){
		fd_list_init(t);
	}

	/* 문자열 시작 주소에서 끝 문자열까지만 접근했는지 판단 */
	int i=0;
	while(true){
		/* 
			✔︎ 문자열을 이동하며 각 문자열이 사용자 주소 공간인지, 페이지를 침범하지 않았는지 확인 
			→ 침범했다면 thread_exit();
		*/
		if(!(is_user_vaddr(file_name+i)) || !(pml4_get_page(t->pml4, file_name+i))){
			thread_exit();
		}
		/* 문자열의 끝 문자를 만났다면 break */
		if(file_name[i] == '\0'){
			break;
		}
		i++;
	}

	/* 
		fd=0,1을 제외하고 2부터 file객체를 push.
		filesys_open함수를 통해 열린 파일 객체를 2번 idx에 저장. 단, 2번이 있다면 4번.. for문을 돌면서 확인
	*/
	for(int i=2; i<fd_list_size; i++){
		/* NULL이면 현재 사용하지 않는 fd */
		if(t->fd_list[i] == NULL){
			/* 파일을 Open 후 반환된 파일 객체의 주소 저장(i번 FD 테이블에) */
			t->fd_list[i] = filesys_open(file_name);
			/* ✅ 성공: 열린 파일이 NULL X → 등록한 index(fd)반환 후 종료 */
			if(t->fd_list[i] != NULL){
				return i;
			/* ❌ 실패: 열린 파일이 NULL → -1 반환*/
			}else{
				return -1;
			}
		}
	}
	/* FD테이블이 꽉 찼을 때 -1(실패) 반환 */
	return -1;
}

/* fd로 열려 있는 파일의 크기를 바이트 단위로 반환한다. */
int syscall_filesize(int fd){
	/* 실행 중인 스레드 구조체 선언 */
	struct thread *t = thread_current();
	/* fd에 해당하는 file객체가 없다면 프로세스 종료? */
	if(!(t->fd_list[fd])){
		thread_exit();
	}
	return file_length(t->fd_list[fd]);

}