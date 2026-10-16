#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "intrinsic.h"

/* 처리한 page fault의 횟수. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* 유저 프로그램이 일으킬 수 있는 인터럽트들의 핸들러를 등록한다.

   실제 유닉스 계열 OS에서는 이런 인터럽트 대부분이 시그널 형태로
   유저 프로세스에 전달된다([SV-386] 3-24, 3-25 참고). 하지만 우리는
   시그널을 구현하지 않는다. 대신 해당 유저 프로세스를 그냥 죽이도록
   만든다.

   page fault는 예외다. 여기서는 다른 예외와 똑같이 처리하지만,
   가상 메모리를 구현하려면 이 부분을 바꿔야 한다.

   각 예외에 대한 설명은 [IA32-v3a] 5.15절 "Exception and Interrupt
   Reference"를 참고할 것. */
void
exception_init (void) {
	/* 아래 예외들은 INT, INT3, INTO, BOUND 명령어를 통해 유저 프로그램이
	   명시적으로 일으킬 수 있다. 그래서 DPL==3으로 설정해서, 유저
	   프로그램이 이 명령어들로 예외를 호출하는 것을 허용한다. */
	intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
	intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
	intr_register_int (5, 3, INTR_ON, kill,
			"#BR BOUND Range Exceeded Exception");

	/* 아래 예외들은 DPL==0이라서 유저 프로세스가 INT 명령어로 직접
	   호출할 수 없다. 하지만 간접적으로는 일어날 수 있다. 예를 들어
	   0으로 나누면 #DE가 발생한다. */
	intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
	intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
	intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
	intr_register_int (7, 0, INTR_ON, kill,
			"#NM Device Not Available Exception");
	intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
	intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
	intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
	intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
	intr_register_int (19, 0, INTR_ON, kill,
			"#XF SIMD Floating-Point Exception");

	/* 대부분의 예외는 인터럽트를 켠 상태로 처리할 수 있다.
	   page fault만은 인터럽트를 꺼야 한다. fault가 난 주소가 CR2에
	   저장되는데, 이 값을 보존해야 하기 때문이다. */
	intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* 예외 통계를 출력한다. */
void
exception_print_stats (void) {
	printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* (아마도) 유저 프로세스가 일으킨 예외의 핸들러. */
static void
kill (struct intr_frame *f) {
	/* 이 인터럽트는 (아마도) 유저 프로세스가 일으킨 것이다.
	   예를 들어 프로세스가 매핑되지 않은 가상 메모리에 접근하려 했을
	   수 있다(page fault). 지금은 그 유저 프로세스를 그냥 죽인다.
	   나중에는 page fault를 커널에서 처리하도록 바꿀 것이다. 실제
	   유닉스 계열 OS는 대부분의 예외를 시그널로 프로세스에 돌려주지만,
	   우리는 시그널을 구현하지 않는다. */

	/* 인터럽트 프레임의 코드 세그먼트 값으로 예외가 어디서
	   발생했는지 알 수 있다. */
	switch (f->cs) {
		case SEL_UCSEG:
			/* 유저의 코드 세그먼트이므로, 예상대로 유저 예외다.
			   유저 프로세스를 죽인다. */
			printf ("%s: dying due to interrupt %#04llx (%s).\n",
					thread_name (), f->vec_no, intr_name (f->vec_no));
			intr_dump_frame (f);
			thread_exit ();

		case SEL_KCSEG:
			/* 커널의 코드 세그먼트이므로 커널 버그다.
			   커널 코드에서는 예외가 발생하면 안 된다. (page fault는
			   커널 예외를 일으킬 수 있지만, 여기까지 오면 안 된다.)
			   확실히 알리기 위해 커널을 panic시킨다. */
			intr_dump_frame (f);
			PANIC ("Kernel bug - unexpected interrupt in kernel");

		default:
			/* 그 밖의 코드 세그먼트? 일어나면 안 되는 일이다.
			   커널을 panic시킨다. */
			printf ("Interrupt %#04llx (%s) in unknown segment %04x\n",
					f->vec_no, intr_name (f->vec_no), f->cs);
			thread_exit ();
	}
}

/* page fault 핸들러. 가상 메모리를 구현하려면 채워 넣어야 하는
   뼈대(skeleton) 코드다. Project 2의 일부 해법에서도 이 코드를 수정해야
   할 수 있다.

   진입 시점에 fault가 난 주소는 CR2(Control Register 2)에 들어 있고,
   fault에 대한 정보는 exception.h의 PF_* 매크로에 설명된 형식으로 F의
   error_code 멤버에 들어 있다. 여기 있는 예제 코드는 그 정보를 파싱하는
   방법을 보여준다. 이 둘에 대한 자세한 내용은 [IA32-v3a] 5.15절
   "Exception and Interrupt Reference"의 "Interrupt 14--Page Fault
   Exception (#PF)" 설명을 참고할 것. */
static void
page_fault (struct intr_frame *f) {
	bool not_present;  /* true: 페이지가 없음, false: 읽기 전용 페이지에 씀. */
	bool write;        /* true: 쓰기 접근, false: 읽기 접근. */
	bool user;         /* true: 유저의 접근, false: 커널의 접근. */
	void *fault_addr;  /* fault가 난 주소. */

	/* fault를 일으킨 주소, 즉 접근하려던 가상 주소를 얻는다.
	   코드를 가리킬 수도, 데이터를 가리킬 수도 있다. fault를 일으킨
	   명령어의 주소(f->rip)와는 반드시 같지는 않다. */

	fault_addr = (void *) rcr2();

	/* 인터럽트를 다시 켠다. (CR2가 바뀌기 전에 읽는 것을 보장하려고
	   꺼 두었을 뿐이다.) */
	intr_enable ();


	/* 원인을 판별한다. */
	not_present = (f->error_code & PF_P) == 0;
	write = (f->error_code & PF_W) != 0;
	user = (f->error_code & PF_U) != 0;

#ifdef VM
	/* Project 3 이후용. */
	if (vm_try_handle_fault (f, fault_addr, user, write, not_present))
		return;
#endif

	/* page fault 횟수를 센다. */
	page_fault_cnt++;

	/* 진짜 fault라면 정보를 출력하고 종료한다. */
	printf ("Page fault at %p: %s error %s page in %s context.\n",
			fault_addr,
			not_present ? "not present" : "rights violation",
			write ? "writing" : "reading",
			user ? "user" : "kernel");
	kill (f);
}

