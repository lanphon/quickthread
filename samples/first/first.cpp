#include "qt.h"
#include <cstdint>
#include <iostream>

inline
void*
stack_align( void* sp, int alignment, std::size_t* stack_size )
{
    int round_up_mask = alignment - 1;
    *stack_size = (*stack_size + round_up_mask) & ~round_up_mask;
    return ( (void*)(((qt_word_t) sp + round_up_mask) & ~round_up_mask) );
}

struct cort_info_t
{
    char * m_stack = nullptr;
    std::size_t m_stack_size = 0;
    qt_t *m_sp;
};

cort_info_t s_main_cor; // the main thread's context
cort_info_t *s_curr_cor = nullptr;

typedef void (cor_fn)( void* );

extern "C"
void
cor_qt_wrapper( void* arg, void* cor, qt_userf_t* fn )
{
    s_curr_cor = reinterpret_cast<cort_info_t*>( cor );
    // invoke the user function
    (*(cor_fn*) fn)( arg );
    // not reached
}

cort_info_t *create_thread(std::size_t stack_size, cor_fn *fn, void *args)
{
    cort_info_t* cor = new cort_info_t;
    cor->m_stack_size = stack_size;

    // allocate a stack for the thread
    cor->m_stack = new char[stack_size];

    // align the stack, also set the stack size after alignment
    void *sto = stack_align(cor->m_stack, QUICKTHREADS_STKALIGN, &cor->m_stack_size);

    cor->m_sp = QUICKTHREADS_SP(sto, cor->m_stack_size - QUICKTHREADS_STKALIGN);
    cor->m_sp = QUICKTHREADS_ARGS( cor->m_sp
            , args
            , cor
            , (qt_userf_t*) fn
            , cor_qt_wrapper );

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
    QUICKTHREADS_BLOCK( cor_qt_yieldhelp, old_cor, 0, new_cor->m_sp );
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

void f0(void *)
{
    using namespace std;

    cout << "enter f0, first stage before yield" << endl;
    yield((cort_info_t*)tarr[1]);

    cout << "enter f0, second stage after yield" << endl;

    yield(&s_main_cor);

    return;
}

void f1(void *)
{
    using namespace std;
    cout << "enter f1" << endl;
    s_abort(tarr[0]);
}

int main(int argc, char **argv)
{
    using namespace std;
    s_curr_cor = &s_main_cor;

    auto t0 = create_thread(0x1000, f0, nullptr);
    tarr[0] = t0;

    auto t1 = create_thread(0x1000, f1, nullptr);
    tarr[1] = t1;

    cout << "before yield, s_main_cor->m_sp " << (uint64_t)s_main_cor.m_sp << endl;
    yield(t0);

    cout << "return main" << endl;
    cout << "s_main_cor->m_stack " << (uint64_t)s_main_cor.m_stack << endl;
    cout << "s_main_cor->m_stack_size " << (uint64_t)s_main_cor.m_stack_size << endl;
    cout << "s_main_cor->m_sp " << (uint64_t)s_main_cor.m_sp << endl;

    return 0;
}
