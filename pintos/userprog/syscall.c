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
#include "threads/palloc.h"
#include "devices/input.h"
#include "threads/init.h"
#include "userprog/process.h"
#include "include/userprog/process.h"

/* 공용 락 선언 */
struct lock filesys_lock;

/* 전체 페이지 크기를 파일 포인터 크기 8바이트 나누면 전체 배열 사이즈가 나온다 */
const int fd_list_size = PGSIZE/8; 

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

bool syscall_create(const char *file, unsigned initial_size);
bool syscall_remove(const char *file);
int syscall_open(char *file);
int syscall_filesize(int fd);
int syscall_write(int fd, void *buffer, unsigned size);
int syscall_read (int fd, void *buffer, unsigned size);
void syscall_exit(int status);
void syscall_seek(int fd, unsigned position);
void syscall_close(int fd);
void syscall_halt(void);
tid_t syscall_fork (const char *thread_name, struct intr_frame *f);

/* 파일 이름을 종료 문자까지 안전하게 읽을 수 있는지 검사한다 */
static bool valid_user_string(const char *str);

/* 🧝🏼KyleKim wait기능 구현 */
int syscall_wait(int pid);

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


/* fork 시스템 콜의 입구.
   thread_name : 자식 스레드 이름(유저 포인터), f : 시스템 콜 시점의 유저 레지스터
   리턴 : 부모는 자식 tid, 실패하면 TID_ERROR (자식은 __do_fork에서 rax=0으로 따로 설정됨) */
tid_t
syscall_fork (const char *thread_name, struct intr_frame *f)
{
	/* 잘못된 유저 포인터(NULL, 커널 주소, 매핑 안 됨)는 -1 리턴이 아니라 프로세스 종료 */
	if(!valid_user_string(thread_name))
		syscall_exit(-1);
	/* f를 넘기는 이유: 자식이 부모의 레지스터 상태를 그대로 복사해 가야 하기 때문 */
	return process_fork(thread_name, f);
}

void 
syscall_exit(int status){
	/* 스레드의 exit_status 필드에 return으로 나온 값 담음 */
	thread_current()->exit_status = status;
	thread_exit(); /* process_exit 호출 → 메시지 출력 후 소멸 */
}

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
	if (!check_buffer(buffer, size)) syscall_exit(-1);
	/* fd=0 : 잘못된 접근(키보드) -1 반환 */
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
	if(!check_buffer(buffer, size)) syscall_exit(-1);
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

bool
syscall_create(const char *file, unsigned initial_size) {
    if (!valid_user_string(file)) {
        syscall_exit(-1);
        return false; /* 종료 함수가 반환하지 않으면 도달하지 않음 */
    }

    lock_acquire(&filesys_lock);
    bool result = filesys_create(file, initial_size);
    lock_release(&filesys_lock);

    return result;
}

bool
syscall_remove(const char *file) {
    if (!valid_user_string(file)) {
        syscall_exit(-1);
        return false;
    }

    lock_acquire(&filesys_lock);
    bool result = filesys_remove(file);
    lock_release(&filesys_lock);

    return result;
}

void
syscall_seek(int fd, unsigned position) {
    struct thread *t = thread_current();

    /* 표준 입출력과 배열 범위 밖의 fd는 처리하지 않는다. */
    if (fd < 2 || fd >= fd_list_size)
        return;

    /* 파일 테이블이 없거나 해당 fd가 열려 있지 않으면 반환한다. */
    if (t->fd_list == NULL || t->fd_list[fd] == NULL)
        return;

    lock_acquire(&filesys_lock);
    file_seek(t->fd_list[fd], position);
    lock_release(&filesys_lock);
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
			syscall_halt();
			break;
		case SYS_EXIT: 
			syscall_exit(f->R.rdi);
			break;
		case SYS_FORK:
			/* rdi = 첫 번째 인자(자식 이름). 부모가 받을 리턴값(자식 tid)은 rax에 저장 */
			f->R.rax = syscall_fork((char *)f->R.rdi, f);
			break;
		case SYS_EXEC: 
			break;
		case SYS_WAIT:
			f->R.rax = syscall_wait(f->R.rdi);
			break;
		case SYS_CREATE: 
			f->R.rax = syscall_create((const char *) f->R.rdi, (unsigned) f->R.rsi);
			break;
		case SYS_REMOVE: 
			f->R.rax = syscall_remove((const char *) f->R.rdi);
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
			syscall_seek((int) f->R.rdi, (unsigned) f->R.rsi);
			break;
		case SYS_TELL:
			break;
		case SYS_CLOSE:
			syscall_close(f->R.rdi);
			break;
		default:
			break;
	}	
	// printf ("system call!\n");
}

/* 
	fd_list 초기화 함수: 페이지를 할당받아 모든 페이지의 원소를 0으로 채워 초기화하는 동작
*/
bool fd_list_init(struct thread* t){
	/* 페이지의 0을 채워 공간 할당 */
	t->fd_list = palloc_get_page(PAL_ZERO);
	/* 메모리 부족이면 false 리턴. 호출한 쪽(open/exec/fork)이 각자 상황에 맞게 실패 처리함
	   (여기서 thread_exit을 하면 fork 중 부모가 sema_down에서 영원히 잠들 수 있어서 리턴만 함) */
	if(!(t->fd_list)){
		return false;
	}
	return true;
}

int syscall_open(char* file_name)
{
	struct thread *t = thread_current();

	/* ✔︎ 사용자 가상 주소 범위인지 file_name이 비어있지 않은지 페이지에 해당되는지 확인 */
	if(!is_user_vaddr(file_name) || file_name == NULL || !(pml4_get_page(t->pml4, file_name))){
		syscall_exit(-1);
	}

	/* fd테이블이 비었다면 페이지 전체를 NULL로 초기화 */
	if(!t->fd_list){
		if(!fd_list_init(t))
			return -1;
	}

	/* 문자열 시작 주소에서 끝 문자열까지만 접근했는지 판단 */
	int i=0;
	while(true){
		/* 
			✔︎ 문자열을 이동하며 각 문자열이 사용자 주소 공간인지, 페이지를 침범하지 않았는지 확인 
			→ 침범했다면 thread_exit();
		*/
		if(!(is_user_vaddr(file_name+i)) || !(pml4_get_page(t->pml4, file_name+i))){
			syscall_exit(-1);
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
			/* 공용 락 등록 */
			lock_acquire(&filesys_lock);
			/* 파일을 Open 후 반환된 파일 객체의 주소 저장(i번 FD 테이블에) */
			t->fd_list[i] = filesys_open(file_name);
			/* 공용 락 해제 */
			lock_release(&filesys_lock);
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

	/* 
		1) fd가 0이나 1에 접근 시 -1 반환 
		2) file descriptor 테이블이 없다면 실패 반환
		3) fd에 해당하는 file 객체가 없다면 실패 반환
		4) fd가 배열 크기가 같거나 넘을 경우 실패 반환
	*/
	if(fd <= 1 || fd >= fd_list_size || !(t->fd_list) || !(t->fd_list[fd])){
		return -1;
	}
	/* 공용 락 등록 */
	lock_acquire(&filesys_lock);

	/* 반환값(result) 저장 */
	int result = file_length(t->fd_list[fd]);

	/* 공용 락 해제 */
	lock_release(&filesys_lock);

	return result; 

}

/* 프로세스가 정상적으로 종료되거나 강제 종료되면, 열려 있는 모든 파일 디스크립터가 자동으로 닫힌다 */
void syscall_close(int fd){
	struct thread* t = thread_current();
	/* 
		1) fd가 0이나 1에 접근 시 함수 종료
		2) file descriptor 테이블이 없다면 실패 반환
		3) fd에 해당하는 file 객체가 없다면 실패 반환
		4) fd가 배열 크기가 같거나 넘을 경우 실패 반환
	*/
	if(fd <= 1 || fd >= fd_list_size || !(t->fd_list) || !(t->fd_list[fd])){
		return;
	}

	/* 공용 락 등록 */
	lock_acquire(&filesys_lock);

	/* fd에 해당되는 file객체를 해제 → file_close()함수 사용 */
	file_close(t->fd_list[fd]);
	
	/* file 객체를 바라보는 포인터도 NULL 초기화 */
	t->fd_list[fd] = NULL;

	/* 공용 락 해제 */
	lock_release(&filesys_lock);
}

/* power_off() 함수를 호출하여 Pintos 운영체제를 종료하는 함수 */
void syscall_halt(void){
	power_off();
}

/*
 * 자식 프로세스 pid가 종료될 때까지 기다리고 자식의 종료 상태를 반환하는 함수.
 *
 * 기본 동작
 *   1) 자식이 실행 중이면 종료될 때까지 기다린 뒤,
 *      자식이 exit()에 전달한 상태 값을 반환한다.
 *   2) 자식이 exit()를 호출하지 않고 커널에 의해 종료되었다면 -1을 반환한다.
 *   3) 자식이 이미 종료되었어도 부모가 종료 상태를 받을 수 있어야 한다.
 *
 * 즉시 -1을 반환하는 경우
 *   4) pid가 호출한 프로세스의 직접적인 자식이 아닌 경우.
 *      - 성공한 fork()의 반환값으로 받은 pid만 직접적인 자식에 해당한다.
 *      - 손자 프로세스는 기다릴 수 없으며, 고아 프로세스의 부모를 재지정하지 않는다.
 *   5) 같은 자식에 대해 이미 wait()를 호출한 경우.
 *      - 각 자식에 대한 wait()는 최대 한 번만 허용한다.
 *
 * 자원 및 수명 관리
 *   6) 부모는 여러 자식을 원하는 순서로 기다리거나, 기다리지 않고 종료할 수 있다.
 *   7) 부모의 wait() 호출 여부와 부모·자식의 종료 순서에 관계없이
 *      각 프로세스의 모든 자원(struct thread 포함)은 해제되어야 한다.
 *      단, 부모가 받아야 할 자식의 종료 상태는 필요한 동안 보존되어야 한다.
 *   8) 최초 프로세스가 종료되기 전에는 Pintos가 종료되지 않아야 한다.
*/
int syscall_wait(int pid){
	/* TODO: 자식 식별: 전달받은 pid가 어떤 자식인지 확인할 수 있다. */
	/* TODO: 종료 상태 보관: 자식의 종료 상태 81을 부모가 받을 때까지 보존한다. */
	/* TODO: 종료 대기: 자식이 실행 중이면 부모가 기다리고, 자식이 종료하면 대기를 끝낸다. */
	/* TODO: 먼저 종료된 자식 처리: 자식이 먼저 종료했더라도 부모가 종료 상태를 받을 수 있다. */
	/* TODO: 중복 호출 처리: 같은 자식에 대한 두 번째 wait()는 기다리지 않고 -1을 반환한다. */
	/* TODO: 정보의 수명 보장: 부모가 종료 상태를 읽기 전에 필요한 정보가 해제되지 않아야 한다. */
	return process_wait(pid);
}