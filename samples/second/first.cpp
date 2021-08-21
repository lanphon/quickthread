#include "qt.h"
#include <cstdint>
#include <iostream>
#include<cassert>
#include <cstring>

inline
void*
/**
 * @brief round up the stack size to satisfy the alignment requirement
 *
 * @param sp teh original stack pointer
 * @param alignment the alignment requirement, must be power of 2
 * @param stack_size the stack size after rounded up
 */
stack_align( void* sp, int alignment, std::size_t* stack_size )
{
    int round_up_mask = alignment - 1;
    *stack_size = (*stack_size + round_up_mask) & ~round_up_mask;
    return ( (void*)(((qt_word_t) sp + round_up_mask) & ~round_up_mask) );
}

/**
 * @brief the coroutine thread information type
 */
struct cort_info_t
{
    char * m_stack = nullptr; // the allocated stack memory pointer
    std::size_t m_stack_size = 0; // stack size
    qt_t *m_sp; // the stack pointer, it would grow up/down inside the stack, according tot ehe function call/return

    ~cort_info_t()
    {
        if (m_stack)
        {
            assert(m_stack_size != 0);
            delete [] m_stack;
            m_stack = nullptr;
            m_stack_size = 0;
        }
    }
};

cort_info_t s_main_cor; // the main thread's context, just left it empty, it would be filled up with when the main thread(routine)
// is scheduled out with other threads
cort_info_t *s_curr_cor = nullptr; // a pointer point to the current coroutine, should be initialized with &m_main_cor

typedef void (cor_fn)( void* );

extern "C"
void
cor_qt_wrapper( void* arg, void* cor, qt_userf_t* fn )
{
    // here we do not need to save the coroutine to s_curr_cor, since this has already done
    // in yield/abort part
    //s_curr_cor = reinterpret_cast<cort_info_t*>( cor );
    // invoke the user function
    register long rdi asm("rdi");
    register long rsi asm("rsi");
    register long rcx asm("rcx");
    register long rdx asm("rdx");
    register long *rbp asm("rbp");
    register long rsp asm("rsp");
    printf("cor_qt_wrapper ******************************************\n");
    printf("rdi: %016llx\n", rdi);
    printf("rsi: %016llx\n", rsi);
    printf("rcx: %016llx\n", rcx);
    printf("rdx: %016llx\n", rdx);
    printf("rbp: %016llx --> %016llx\n", rbp, *rbp);
    printf("*rbp+1: %016llx\n",  *(rbp+1));
    printf("rsp: %016llx\n", rsp);
    printf("******************************************\n");
    (*(cor_fn*) fn)( arg );
    // not reached
}

cort_info_t *create_thread(std::size_t stack_size, cor_fn *fn, void *args)
{
    cort_info_t* cor = new cort_info_t;
    cor->m_stack_size = stack_size;

    // allocate a stack for the thread
    cor->m_stack = new char[stack_size];
    std::memset(cor->m_stack, 0xdb, stack_size);
    printf("original stack, the stack pointer is: %p\n", cor->m_stack);

    // align the stack, also set the stack size after alignment
    void *sto = stack_align(cor->m_stack, QUICKTHREADS_STKALIGN, &cor->m_stack_size);

    printf("before assign, the stack top pointer is: %p\n", sto);

    // for x86_64, the QUICKTHREADS_STKALIGN is 16byte
    cor->m_sp = QUICKTHREADS_SP(sto, cor->m_stack_size - QUICKTHREADS_STKALIGN);
    printf("after assign, the stack pointer is: %p\n", cor->m_sp);
    cor->m_sp = QUICKTHREADS_ARGS( cor->m_sp
            , args // pu
            , cor // pt
            , (qt_userf_t*) fn // userf
            , cor_qt_wrapper ); //  only
    /*
     * QUICKTHREADS_ARGS would adjust the sp with QUICKTHREADS_STKBASE, for x86_64, it's 0x80 byte
     */
    printf("args is %016llx\n", args);
    printf("cor is %016llx\n", cor);
    printf("fn is %016llx\n", fn);
    printf("cor_qt_wrapper is %016llx\n", cor_qt_wrapper);

    printf("stack is\n");
    printf("after assign2, the stack pointer is: %p\n", cor->m_sp);
    printf("sp+80(16)->: %016lx\n", *((uint64_t*)cor->m_sp + 16));
    printf("sp+78(15)->: %016lx\n", *((uint64_t*)cor->m_sp + 15));
    printf("sp+70(14)->: %016lx\n", *((uint64_t*)cor->m_sp + 14));
    printf("sp+68(13)->: %016lx\n", *((uint64_t*)cor->m_sp + 13));
    printf("sp+60(12)->: %016lx -- cor_qt_wrapper/only\n", *((uint64_t*)cor->m_sp + 12)); // cor_qt_wrapper
    printf("sp+58(11)->: %016lx\n", *((uint64_t*)cor->m_sp + 11));
    printf("sp+50(10)->: %016lx\n", *((uint64_t*)cor->m_sp + 10));
    printf("sp+48( 9)->: %016lx\n", *((uint64_t*)cor->m_sp + 9));
    printf("sp+40( 8)->: %016lx\n", *((uint64_t*)cor->m_sp + 8));
    printf("sp+38( 7)->: %016lx\n", *((uint64_t*)cor->m_sp + 7));
    printf("sp+30( 6)->: %016lx\n", *((uint64_t*)cor->m_sp + 6));
    printf("sp+28( 5)->: %016lx\n", *((uint64_t*)cor->m_sp + 5));
    printf("sp+20( 4)->: %016lx\n", *((uint64_t*)cor->m_sp + 4));
    printf("sp+18( 3)->: %016lx\n", *((uint64_t*)cor->m_sp + 3));
    printf("sp+10( 2)->: %016lx -- fn/userf\n", *((uint64_t*)cor->m_sp + 2));
    printf("sp+ 8( 1)->: %016lx -- args/pu\n", *((uint64_t*)cor->m_sp + 1));
    printf("sp-------->: %016lx -- cor/pt\n", *((uint64_t*)cor->m_sp));
    printf("sp- 4->: %016lx\n", *((uint64_t*)cor->m_sp - 1));
    printf("sp- 8->: %016lx\n", *((uint64_t*)cor->m_sp - 2));
    printf("sp- c->: %016lx\n", *((uint64_t*)cor->m_sp - 3));

    return cor;
}

extern "C"
void*
cor_qt_yieldhelp( qt_t* sp, void* old_cor, void* )
{
    reinterpret_cast<cort_info_t*>( old_cor )->m_sp = sp;
    return 0;
}

void
yield( cort_info_t* next_cor )
{
    cort_info_t* new_cor = next_cor;
    cort_info_t* old_cor = s_curr_cor;
    s_curr_cor = new_cor;
    printf("Yield to new stack pointer %016llx\n", new_cor->m_sp);
    QUICKTHREADS_BLOCK( cor_qt_yieldhelp, old_cor, 0, new_cor->m_sp );
    /*
     * first, call cor_qt_yieldhelp("old cor's stack pointer $rsp", old_cor, 0); via qt_block
     * on the new coroutine's stack
     * the old cor's stack is fetched by ASM language
     */
}


// abort the current coroutine (and resume the next coroutine)

extern "C"
void*
cor_qt_aborthelp( qt_t*, void*, void* )
{
    return 0;
}

void
s_abort( cort_info_t* next_cor )
{
    cort_info_t* new_cor = next_cor;
    cort_info_t* old_cor = s_curr_cor;
    s_curr_cor = new_cor;
    QUICKTHREADS_ABORT( cor_qt_aborthelp, old_cor, 0, new_cor->m_sp );
}

// all the coroutines should be managed globally
cort_info_t *tarr[10];

void _f0(void *);
void f0(void *p)
{
    printf("test ----------------------\n");
    _f0(p);

    // NEVER REACHED, since _f0() has an infinite loop

    printf("test ---------------------over\n");
}

void _f0(void *)
{
    using namespace std;
    register long rdi asm("rdi");
    register long rsi asm("rsi");
    register long rcx asm("rcx");
    register long rdx asm("rdx");
    register long *rbp asm("rbp");
    register long rsp asm("rsp");

    printf("f0 ******************************************\n");
    printf("rdi: %016llx\n", rdi);
    printf("rsi: %016llx\n", rsi);
    printf("rcx: %016llx\n", rcx);
    printf("rdx: %016llx\n", rdx);
    printf("rbp: %016llx --> %016llx\n", rbp, *rbp);
    printf("*rbp+1: %016llx\n",  *(rbp+1));
    printf("rsp: %016llx\n", rsp);
    printf("******************************************\n");

    // here is the difference of a coroutine scheduled and a function call
    // with the while(1) loop, the normal function call would never ever return until the while(1)
    // is breaked by an explicitly break
    // but for a coroutine, it could stop the current execution inner a function, return the execution
    // flow to another coroutine
    //
    // a coroutine is a user-scope thread, this is what it means
    // the user must do the management work for the coroutines, just like it's done inside the kernel space
    // for a real thread
    while(1)
    {
        cout << "enter f0, first stage before yield" << endl;
        yield((cort_info_t*)tarr[1]);

        cout << "enter f0, second stage after yield" << endl;

        yield(&s_main_cor);
    }

    return;
}

void f1(void *)
{
    using namespace std;
    cout << "enter f1" << endl;
    s_abort(tarr[0]); // finish the f1 execute, and then schedule back to the tarr[0]
}

int main(int argc, char **argv)
{
    using namespace std;
    s_curr_cor = &s_main_cor; // init the current coroutine to the main coroutine

    auto t0 = create_thread(0x1000, f0, nullptr);
    tarr[0] = t0;

    auto t1 = create_thread(0x1000, f1, nullptr);
    tarr[1] = t1;

    cout << "before yield, s_main_cor->m_sp " << (uint64_t)s_main_cor.m_sp << endl;
    yield(t0); // schedule the main coroutine out, and exchange to execute a thread(couroutine)

    cout << "return main" << endl;
    cout << "s_main_cor->m_stack " << (uint64_t)s_main_cor.m_stack << endl;
    cout << "s_main_cor->m_stack_size " << (uint64_t)s_main_cor.m_stack_size << endl;
    cout << "s_main_cor->m_sp ";// << (uint64_t)s_main_cor.m_sp << endl;
    printf("%p\n", s_main_cor.m_sp);

    delete t0;
    delete t1;

    return 0;
}
