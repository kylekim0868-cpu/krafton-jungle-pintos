#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "intrinsic.h"
#include "userprog/syscall.h"
#include "devices/timer.h"
#include "threads/thread.h"
#ifdef VM
#include "vm/vm.h"
#endif

static void process_cleanup (void);
static bool load (const char *file_name, struct intr_frame *if_);
static void initd (void *f_name);
static void __do_fork (void *);

/* initd와 그 밖의 모든 프로세스가 공통으로 쓰는 프로세스 초기화 함수. */
static void
process_init (void) {
	struct thread *current = thread_current ();
}

/* FILE_NAME에서 읽어 들인 첫 번째 유저 프로그램 "initd"를 시작한다.
 * 새 스레드는 process_create_initd()가 리턴하기 전에 스케줄될 수
 * 있고, 심지어 종료될 수도 있다. 성공하면 initd의 스레드 id를 리턴하고,
 * 스레드를 만들 수 없으면 TID_ERROR를 리턴한다.
 * 이 함수는 반드시 한 번만 호출해야 한다. */
tid_t
process_create_initd (const char *file_name) {
	char *fn_copy;
	tid_t tid;

	/* FILE_NAME의 복사본을 만든다.
	 * 그러지 않으면 호출한 쪽과 load() 사이에 race가 생긴다. */
	fn_copy = palloc_get_page (0);
	if (fn_copy == NULL)
		return TID_ERROR;
	strlcpy (fn_copy, file_name, PGSIZE);

	/* 오류 발생 이유 : 현재 thread_create에 이름자리에 첫번째 token만 잘라서 들어가야하는데
	   잘린게 들어가는게 아닌 16바이트 길이만큼 잘린 문자가 이름에 들어감 때문에 출력이 달라서 Fail 발생
	   temp_buf의 크기가 16인 이유는 애초에 이름으로 자르는 크기가 16이기때문에 굳이 더 크게 만들 필요가 없음 */
	char* save_ptr, temp_buf[16], *token;
	/* temp_buf에 file_name을 temp_buf길이만큼 복사함 */
	strlcpy(temp_buf, file_name, sizeof temp_buf);
	/* 첫번째 인자(이름)만 잘라서 token에 저장 */
	token = strtok_r(temp_buf, " ", &save_ptr);

	/* FILE_NAME을 실행할 새 스레드를 만든다. */
	tid = thread_create (token, PRI_DEFAULT, initd, fn_copy);
	if (tid == TID_ERROR)
		palloc_free_page (fn_copy);
	return tid;
}

/* 첫 번째 유저 프로세스를 띄우는 스레드 함수. */
static void
initd (void *f_name) {
#ifdef VM
	supplemental_page_table_init (&thread_current ()->spt);
#endif

	process_init ();

	if (process_exec (f_name) < 0)
		PANIC("Fail to launch initd\n");
	NOT_REACHED ();
}

/* 현재 프로세스를 `name`이라는 이름으로 복제한다. 새 프로세스의
 * 스레드 id를 리턴하고, 스레드를 만들 수 없으면 TID_ERROR를 리턴한다. */
tid_t
process_fork (const char *name, struct intr_frame *if_ UNUSED) {
	/* 현재 스레드를 새 스레드로 복제한다. */
	return thread_create (name,
			PRI_DEFAULT, __do_fork, thread_current ());
}

#ifndef VM
/* 이 함수를 pml4_for_each에 넘겨서 부모의 주소 공간을 복제한다.
 * Project 2 전용이다. */
static bool
duplicate_pte (uint64_t *pte, void *va, void *aux) {
	struct thread *current = thread_current ();
	struct thread *parent = (struct thread *) aux;
	void *parent_page;
	void *newpage;
	bool writable;

	/* 1. TODO: parent_page가 커널 페이지라면 바로 리턴한다. */

	/* 2. 부모의 pml4(page map level 4)에서 VA에 해당하는 페이지를 찾는다. */
	parent_page = pml4_get_page (parent->pml4, va);

	/* 3. TODO: 자식을 위해 PAL_USER 페이지를 새로 할당하고
	 *    TODO: 그 결과를 NEWPAGE에 담는다. */

	/* 4. TODO: 부모의 페이지를 새 페이지에 복사하고,
	 *    TODO: 부모의 페이지가 쓰기 가능한지 확인해서
	 *    TODO: 그 결과에 따라 WRITABLE을 설정한다. */

	/* 5. 새 페이지를 자식의 페이지 테이블에 주소 VA로, WRITABLE 권한과 함께
	 *    추가한다. */
	if (!pml4_set_page (current->pml4, va, newpage, writable)) {
		/* 6. TODO: 페이지 삽입에 실패하면 에러 처리를 한다. */
	}
	return true;
}
#endif

/* 부모의 실행 문맥(execution context)을 복사하는 스레드 함수.
 * 힌트) parent->tf에는 프로세스의 유저 영역 문맥이 들어 있지 않다.
 *       즉, process_fork의 두 번째 인자를 이 함수로 직접 넘겨 주어야
 *       한다. */
static void
__do_fork (void *aux) {
	struct intr_frame if_;
	struct thread *parent = (struct thread *) aux;
	struct thread *current = thread_current ();
	/* TODO: parent_if(즉, process_fork()의 if_)를 어떻게든 전달할 것. */
	struct intr_frame *parent_if;
	bool succ = true;

	/* 1. CPU 문맥을 로컬 스택으로 읽어 온다. */
	memcpy (&if_, parent_if, sizeof (struct intr_frame));

	/* 2. 페이지 테이블(PT)을 복제한다. */
	current->pml4 = pml4_create();
	if (current->pml4 == NULL)
		goto error;

	process_activate (current);
#ifdef VM
	supplemental_page_table_init (&current->spt);
	if (!supplemental_page_table_copy (&current->spt, &parent->spt))
		goto error;
#else
	if (!pml4_for_each (parent->pml4, duplicate_pte, parent))
		goto error;
#endif

	/* TODO: 여기에 코드를 작성할 것.
	 * TODO: 힌트) 파일 객체를 복제하려면 include/filesys/file.h의
	 * TODO:       `file_duplicate`를 사용한다. 부모는 이 함수가 부모의
	 * TODO:       자원을 성공적으로 복제할 때까지 fork()에서 리턴하면
	 * TODO:       안 된다는 점에 유의할 것. */

	process_init ();

	/* 마지막으로, 새로 만든 프로세스로 전환한다. */
	if (succ)
		do_iret (&if_);
error:
	thread_exit ();
}

/* 현재 실행 문맥을 f_name의 프로그램으로 바꾼다(기존 프로세스 이미지를
 * 새 프로그램으로 교체한다). 실패하면 -1을 리턴한다.
 * 성공하면 do_iret으로 유저 모드에 진입하므로 리턴하지 않는다. */
int
process_exec (void *f_name) {
	char *file_name = f_name;
	bool success;

	/* 스레드 구조체 안의 intr_frame은 쓸 수 없다.
	 * 현재 스레드가 재스케줄될 때 실행 정보가 그 멤버에
	 * 저장되기 때문이다. 그래서 로컬 변수 _if를 쓴다. */
	struct intr_frame _if;
	_if.ds = _if.es = _if.ss = SEL_UDSEG; /* 데이터·스택 관련 세그먼트 값들을 유저 모드용 값으로 설정합니다. */
	_if.cs = SEL_UCSEG;
	_if.eflags = FLAG_IF | FLAG_MBS;

	/* 먼저 현재 문맥(기존 메모리 등)을 없앤다. */
	process_cleanup ();

	/* 현재 스레드에 fd_list가 생성이 안됬을때만 초기화
	   무조건 새 페이지를 할당하면 기존에 열어둔 파일들을 잃어버리고  예전 페이지는 해제되지 않고 남음 (메모리 누수 발생) */
	if(thread_current()->fd_list == NULL)
		fd_list_init(thread_current());

	/* 그런 다음 바이너리를 로드한다. */
	success = load (file_name, &_if); /* 새로 ELF로드해서 메모리에 적재 */

	/* 로드에 실패하면 종료한다. */
	palloc_free_page (file_name); /* free 해주는 */
	if (!success)
		return -1;

	/* 전환된 프로세스를 시작한다. */
	do_iret (&_if);
	NOT_REACHED (); /* do_iret이 여기로 돌아오면 깨짐 */
}


/* 스레드 TID가 죽을 때까지 기다렸다가 그 종료 코드(exit status)를
 * 리턴한다. 커널에 의해 종료됐다면(즉, 예외로 인해 죽임을 당했다면)
 * -1을 리턴한다. TID가 유효하지 않거나, 호출한 프로세스의 자식이
 * 아니거나, 해당 TID에 대해 process_wait()가 이미 성공적으로
 * 호출된 적이 있다면, 기다리지 않고 즉시 -1을 리턴한다.
 *
 * 이 함수는 문제 2-2에서 구현한다. 지금은 아무 일도 하지 않는다. */
int
process_wait (tid_t child_tid UNUSED) {
	/* XXX: 힌트) process_wait(initd)가 리턴하면 pintos가 종료된다.
	 * XXX:       process_wait를 구현하기 전까지는 여기에 무한 루프를
	 * XXX:       넣어 두기를 권장한다. */
	timer_sleep(100);
	return -1;
}

/* 프로세스를 종료한다. 이 함수는 thread_exit ()에서 호출된다. */
void
process_exit (void) {
	struct thread *curr = thread_current ();
	/* TODO: 여기에 코드를 작성할 것.
	 * TODO: 프로세스 종료 메시지를 구현할 것
	 * TODO: (project2/process_termination.html 참고).
	 * TODO: 프로세스 자원 정리도 여기서 구현하기를 권장한다. */

	if (curr->pml4 != NULL)
		printf("%s: exit(%d)\n", curr->name, curr->exit_status);
	process_cleanup ();
}

/* 현재 프로세스의 자원을 해제한다. */
static void
process_cleanup (void) {
	struct thread *curr = thread_current ();

#ifdef VM
	supplemental_page_table_kill (&curr->spt);
#endif

	uint64_t *pml4;
	/* 현재 프로세스의 페이지 디렉터리를 파괴하고, 커널 전용 페이지
	 * 디렉터리로 되돌아간다. */
	pml4 = curr->pml4;
	if (pml4 != NULL) {
		/* 여기서는 올바른 순서가 매우 중요하다. 페이지 디렉터리를 바꾸기
		 * 전에 cur->pagedir를 먼저 NULL로 설정해야 한다. 그래야 타이머
		 * 인터럽트가 프로세스의 페이지 디렉터리로 다시 전환하지 못한다.
		 * 또한 프로세스의 페이지 디렉터리를 파괴하기 전에 기본(커널)
		 * 페이지 디렉터리를 먼저 활성화해야 한다. 그러지 않으면 현재
		 * 활성화된 페이지 디렉터리가 이미 해제되어 지워진 것이 된다. */
		curr->pml4 = NULL;
		pml4_activate (NULL);
		pml4_destroy (pml4);
	}
}

/* 다음 스레드에서 유저 코드를 실행할 수 있도록 CPU를 설정한다.
 * 이 함수는 컨텍스트 스위치 때마다 호출된다. */
void
process_activate (struct thread *next) {
	/* 스레드의 페이지 테이블을 활성화한다. */
	pml4_activate (next->pml4);

	/* 인터럽트 처리에 사용할 스레드의 커널 스택을 설정한다. */
	tss_update (next);
}

/* 우리는 ELF 바이너리를 로드한다. 아래 정의들은 ELF 명세 [ELF1]에서
 * 거의 그대로 가져온 것이다. */

/* ELF 타입들. [ELF1] 1-2 참고. */
#define EI_NIDENT 16

#define PT_NULL    0            /* 무시한다. */
#define PT_LOAD    1            /* 로드할 수 있는 세그먼트. */
#define PT_DYNAMIC 2            /* 동적 링킹 정보. */
#define PT_INTERP  3            /* 동적 로더의 이름. */
#define PT_NOTE    4            /* 보조 정보. */
#define PT_SHLIB   5            /* 예약됨. */
#define PT_PHDR    6            /* 프로그램 헤더 테이블. */
#define PT_STACK   0x6474e551   /* 스택 세그먼트. */

#define PF_X 1          /* 실행 가능. */
#define PF_W 2          /* 쓰기 가능. */
#define PF_R 4          /* 읽기 가능. */

/* 실행 파일 헤더. [ELF1] 1-4 ~ 1-8 참고.
 * ELF 바이너리의 맨 앞에 위치한다. */
struct ELF64_hdr {
	unsigned char e_ident[EI_NIDENT];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

struct ELF64_PHDR {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};

/* 약어 */
#define ELF ELF64_hdr
#define Phdr ELF64_PHDR

static bool setup_stack (struct intr_frame *if_);
static bool validate_segment (const struct Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes,
		bool writable);

/* FILE_NAME의 ELF 실행 파일을 현재 스레드로 로드한다.
 * 실행 파일의 시작 주소(entry point)를 *RIP에, 초기 스택 포인터를
 * *RSP에 저장한다.
 * 성공하면 true, 그렇지 않으면 false를 리턴한다.
 *
 * (부연) Project 2의 인자 전달(argument passing)은 이 함수에서
 * 구현한다. FILE_NAME에는 "프로그램이름 인자1 인자2 ..." 전체
 * 문자열이 들어오므로, 실행 파일을 열기 전에 첫 단어만 분리해야 한다. */
static bool
load (const char *file_name, struct intr_frame *if_) {
	struct thread *t = thread_current ();
	struct ELF ehdr;
	struct file *file = NULL;
	off_t file_ofs;
	bool success = false;
	int i;

	/* 페이지 디렉터리를 할당하고 활성화한다. */
	t->pml4 = pml4_create ();
	if (t->pml4 == NULL)
		goto done;
	process_activate (thread_current ());
	
	/* 여기에서 파싱을 마쳐야함 */
	/* strtok_r은 문자열을 구분자로 나뉜 토큰들로 조갬.
	   처음 호출할 때 S는 토큰으로 나눈 문자열이여야함, 이후 호출에서는 NULL 포인터여야함.
	   SAVE_PTR은 이전 토큰을 추적할 수 있음.
	*/
	
	char* cp_file_name = file_name; /* 원본 말고 복사본 */
	char* token, * save_ptr; /* token : 단어, save_ptr : 이전 토큰으로 쪼갠 문자열 위치 */
	int argc = 0; /* 인자 개수 */
	char* split_tokens[64]; /* 토큰의 포인터를 담을 배열 */
	int total_str = 0; /* 문자열 총 공간 크기 */

	/* 반복문에서 토큰 추출 */
	for(token = strtok_r(cp_file_name, " ", &save_ptr); token != NULL; token = strtok_r(NULL, " ", &save_ptr)){
		split_tokens[argc] = token;
		argc++;
		total_str += strlen(token)+1;
	}
	/* argc가 0이 아닐때만 파일이름이 0이다 */
	if(argc != 0) file_name = split_tokens[0];
	
	char* temp_argv[65]; /* 포인터 담아둘 임시 배열 */

	/* 실행 파일을 연다. */
	file = filesys_open (file_name);
	if (file == NULL) {
		printf ("load: %s: open failed\n", file_name);
		goto done;
	}

	/* 실행 파일 헤더를 읽고 검증한다. */
	if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
			|| memcmp (ehdr.e_ident, "\177ELF\2\1\1", 7)
			|| ehdr.e_type != 2
			|| ehdr.e_machine != 0x3E // amd64
			|| ehdr.e_version != 1
			|| ehdr.e_phentsize != sizeof (struct Phdr)
			|| ehdr.e_phnum > 1024) {
		printf ("load: %s: error loading executable\n", file_name);
		goto done;
	}

	/* 프로그램 헤더들을 읽는다. */
	file_ofs = ehdr.e_phoff;
	for (i = 0; i < ehdr.e_phnum; i++) {
		struct Phdr phdr;

		if (file_ofs < 0 || file_ofs > file_length (file))
			goto done;
		file_seek (file, file_ofs);

		if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
			goto done;
		file_ofs += sizeof phdr;
		switch (phdr.p_type) {
			case PT_NULL:
			case PT_NOTE:
			case PT_PHDR:
			case PT_STACK:
			default:
				/* 이 세그먼트는 무시한다. */
				break;
			case PT_DYNAMIC:
			case PT_INTERP:
			case PT_SHLIB:
				goto done;
			case PT_LOAD:
				if (validate_segment (&phdr, file)) {
					bool writable = (phdr.p_flags & PF_W) != 0;
					uint64_t file_page = phdr.p_offset & ~PGMASK;
					uint64_t mem_page = phdr.p_vaddr & ~PGMASK;
					uint64_t page_offset = phdr.p_vaddr & PGMASK;
					uint32_t read_bytes, zero_bytes;
					if (phdr.p_filesz > 0) {
						/* 일반 세그먼트.
						 * 앞부분은 디스크에서 읽고 나머지는 0으로 채운다. */
						read_bytes = page_offset + phdr.p_filesz;
						zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
								- read_bytes);
					} else {
						/* 전부 0인 세그먼트.
						 * 디스크에서는 아무것도 읽지 않는다. */
						read_bytes = 0;
						zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
					}
					if (!load_segment (file, file_page, (void *) mem_page,
								read_bytes, zero_bytes, writable))
						goto done;
				}
				else
					goto done;
				break;
		}
	}

	/* 스택을 설정한다. */ 
	if (!setup_stack (if_))
		goto done;

	/* 시작 주소. */
	if_->rip = ehdr.e_entry;

	/* TODO: 여기에 코드를 작성할 것.
	 * TODO: 인자 전달(argument passing)을 구현할 것
	 * TODO: (project2/argument_passing.html 참고). */
	 
	/* RSP에 인자 + NULL 거꾸로 담기 */
	for(i=argc-1; i>=0; i--){
		int token_len = strlen(split_tokens[i])+1;
		/* rsp에서 단어 길이만큼 빼줌 */
		if_->rsp = if_->rsp - (sizeof(char) * token_len); /* 시작 주소 */
		/* rsp메모리 위치에 토큰으로 나눈 단어 복사해서 넣기 */
		memcpy((void*)if_->rsp, split_tokens[i], token_len);
		temp_argv[i] = (char *)if_->rsp;
		/* split_tokens의 i 인덱스에 해당하는 원소의 바이트 수 계산 후 rsp 주소 이동 
			스택은 주소가 아래로 향하기 때문에 rsp 시작 주소에서 - offset(split_tokens 각 원소 바이트)	 
			ex) split_tokens[1] = "bar\0" 
			-> if_->rsp - 4 
		*/
		/* 비트연산으로 0으로 변경하면 안에 공간이 0으로 바뀜 */
		if(i==0) if_->rsp = if_->rsp & ~0x7;
	}

	/* 반복 조건 : argc+1만큼 반복 단 거꾸로 넣어야함으로 i--로 거꾸로 반복 */
	for(i = argc; i >= 0; i--){ 
		if_->rsp = if_->rsp - sizeof(char *); /* 주소를 넣을 공간 확보 */
		/* 3. 배열이 끝내는 부분을 알기 위해 0을 추가한다 */
		if(i == argc) 
			*(char**)if_->rsp = 0;
		/* 만약 i와 argc가 다르면 rsp 내려가면서 argv[i] 추가함 */			
		else
			*(char**)if_->rsp = temp_argv[i];
	}
	
	/* 최종 레지스터 셋업 */
	if_->R.rdi = argc;
	if_->R.rsi = if_->rsp;

	/* 가짜 주소 설정 */
	if_->rsp -= sizeof(void *); /* 스택에서 반환 주소 공간 확보 -> 스택 포인터를 8바이트 내린다 */
	*(char **)if_->rsp = 0; /* 스택 포인터 자체를 0으로 변경한다 */

	success = true;

done:
	/* 로드에 성공했든 실패했든 여기로 온다. */
	file_close (file);
	return success;
}


/* PHDR이 FILE 안의 유효하고 로드 가능한 세그먼트를 기술하는지
 * 검사해서, 그렇다면 true, 아니면 false를 리턴한다. */
static bool
validate_segment (const struct Phdr *phdr, struct file *file) {
	/* p_offset과 p_vaddr은 페이지 내 오프셋이 같아야 한다. */
	if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
		return false;

	/* p_offset은 FILE 안을 가리켜야 한다. */
	if (phdr->p_offset > (uint64_t) file_length (file))
		return false;

	/* p_memsz는 최소한 p_filesz만큼은 커야 한다. */
	if (phdr->p_memsz < phdr->p_filesz)
		return false;

	/* 세그먼트는 비어 있으면 안 된다. */
	if (phdr->p_memsz == 0)
		return false;

	/* 가상 메모리 영역은 시작과 끝이 모두 유저 주소 공간 범위
	   안에 있어야 한다. */
	if (!is_user_vaddr ((void *) phdr->p_vaddr))
		return false;
	if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
		return false;

	/* 영역이 커널 가상 주소 공간을 넘어서 "한 바퀴 돌아" 감기면
	   (wrap around) 안 된다. */
	if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
		return false;

	/* 페이지 0의 매핑을 허용하지 않는다.
	   페이지 0을 매핑하는 것 자체가 좋지 않은 생각일 뿐 아니라,
	   허용하면 시스템 콜에 널 포인터를 넘기는 유저 코드가 memcpy() 등의
	   널 포인터 assertion을 통해 커널을 panic시킬 가능성이 높다. */
	if (phdr->p_vaddr < PGSIZE)
		return false;

	/* 문제없다. */
	return true;
}

#ifndef VM
/* 이 블록의 코드는 project 2에서만 사용된다.
 * project 2 전체에 걸쳐 쓸 함수를 구현하려면 #ifndef 매크로
 * 바깥에 구현할 것. */

/* load()의 도우미 함수들. */
static bool install_page (void *upage, void *kpage, bool writable);

/* FILE의 오프셋 OFS에서 시작하는 세그먼트를 주소 UPAGE에 로드한다.
 * 모두 합쳐 READ_BYTES + ZERO_BYTES 바이트의 가상 메모리가 다음과
 * 같이 초기화된다.
 *
 * - UPAGE의 READ_BYTES 바이트는 FILE의 오프셋 OFS부터 읽어 와야 한다.
 *
 * - UPAGE + READ_BYTES의 ZERO_BYTES 바이트는 0으로 채워야 한다.
 *
 * 이 함수가 초기화하는 페이지는 WRITABLE이 true이면 유저 프로세스가
 * 쓸 수 있어야 하고, 그렇지 않으면 읽기 전용이어야 한다.
 *
 * 성공하면 true를, 메모리 할당 오류나 디스크 읽기 오류가 발생하면
 * false를 리턴한다. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
	ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (upage) == 0);
	ASSERT (ofs % PGSIZE == 0);

	file_seek (file, ofs);
	while (read_bytes > 0 || zero_bytes > 0) {
		/* 이 페이지를 어떻게 채울지 계산한다.
		 * FILE에서 PAGE_READ_BYTES 바이트를 읽고, 마지막
		 * PAGE_ZERO_BYTES 바이트는 0으로 채운다. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* 메모리 페이지 하나를 얻는다. */
		uint8_t *kpage = palloc_get_page (PAL_USER);
		if (kpage == NULL)
			return false;

		/* 이 페이지를 로드한다. */
		if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes) {
			palloc_free_page (kpage);
			return false;
		}
		memset (kpage + page_read_bytes, 0, page_zero_bytes);

		/* 이 페이지를 프로세스의 주소 공간에 추가한다. */
		if (!install_page (upage, kpage, writable)) {
			printf("fail\n");
			palloc_free_page (kpage);
			return false;
		}

		/* 다음으로 넘어간다. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;
	}
	return true;
}

/* USER_STACK 위치에 0으로 채워진 페이지를 매핑해서 최소한의
 * 스택을 만든다. */
static bool
setup_stack (struct intr_frame *if_) {
	uint8_t *kpage;
	bool success = false;

	kpage = palloc_get_page (PAL_USER | PAL_ZERO);
	if (kpage != NULL) {
		success = install_page (((uint8_t *) USER_STACK) - PGSIZE, kpage, true);
		if (success)
			if_->rsp = USER_STACK;
		else
			palloc_free_page (kpage);
	}
	return success;
}

/* 유저 가상 주소 UPAGE에서 커널 가상 주소 KPAGE로의 매핑을
 * 페이지 테이블에 추가한다.
 * WRITABLE이 true이면 유저 프로세스가 이 페이지를 수정할 수 있고,
 * 그렇지 않으면 읽기 전용이다.
 * UPAGE는 아직 매핑되어 있지 않아야 한다.
 * KPAGE는 palloc_get_page()로 유저 풀에서 얻은 페이지인 것이
 * 보통이다.
 * 성공하면 true를, UPAGE가 이미 매핑되어 있거나 메모리 할당에
 * 실패하면 false를 리턴한다. */
static bool
install_page (void *upage, void *kpage, bool writable) {
	struct thread *t = thread_current ();

	/* 해당 가상 주소에 이미 페이지가 없는지 확인한 다음,
	 * 우리 페이지를 거기에 매핑한다. */
	return (pml4_get_page (t->pml4, upage) == NULL
			&& pml4_set_page (t->pml4, upage, kpage, writable));
}
#else
/* 여기서부터의 코드는 project 3 이후에 사용된다.
 * project 2만을 위한 함수를 구현하려면 위쪽 블록에 구현할 것. */

static bool
lazy_load_segment (struct page *page, void *aux) {
	/* TODO: 파일에서 세그먼트를 로드한다. */
	/* TODO: 이 함수는 주소 VA에서 첫 page fault가 발생할 때 호출된다. */
	/* TODO: 이 함수를 호출할 때 VA를 사용할 수 있다. */
}

/* FILE의 오프셋 OFS에서 시작하는 세그먼트를 주소 UPAGE에 로드한다.
 * 모두 합쳐 READ_BYTES + ZERO_BYTES 바이트의 가상 메모리가 다음과
 * 같이 초기화된다.
 *
 * - UPAGE의 READ_BYTES 바이트는 FILE의 오프셋 OFS부터 읽어 와야 한다.
 *
 * - UPAGE + READ_BYTES의 ZERO_BYTES 바이트는 0으로 채워야 한다.
 *
 * 이 함수가 초기화하는 페이지는 WRITABLE이 true이면 유저 프로세스가
 * 쓸 수 있어야 하고, 그렇지 않으면 읽기 전용이어야 한다.
 *
 * 성공하면 true를, 메모리 할당 오류나 디스크 읽기 오류가 발생하면
 * false를 리턴한다. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
	ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (upage) == 0);
	ASSERT (ofs % PGSIZE == 0);

	while (read_bytes > 0 || zero_bytes > 0) {
		/* 이 페이지를 어떻게 채울지 계산한다.
		 * FILE에서 PAGE_READ_BYTES 바이트를 읽고, 마지막
		 * PAGE_ZERO_BYTES 바이트는 0으로 채운다. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* TODO: lazy_load_segment에 정보를 전달할 aux를 설정한다. */
		void *aux = NULL;
		if (!vm_alloc_page_with_initializer (VM_ANON, upage,
					writable, lazy_load_segment, aux))
			return false;

		/* 다음으로 넘어간다. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;
	}
	return true;
}

/* USER_STACK 위치에 스택 PAGE 하나를 만든다. 성공하면 true를 리턴한다. */
static bool
setup_stack (struct intr_frame *if_) {
	bool success = false;
	void *stack_bottom = (void *) (((uint8_t *) USER_STACK) - PGSIZE);

	/* TODO: stack_bottom에 스택을 매핑하고 해당 페이지를 즉시 claim한다.
	 * TODO: 성공하면 rsp를 그에 맞게 설정한다.
	 * TODO: 해당 페이지를 스택으로 표시해야 한다. */
	/* TODO: 여기에 코드를 작성할 것 */

	return success;
}
#endif /* VM */
