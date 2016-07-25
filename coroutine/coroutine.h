#include <map>
#include <iostream>
#include <ucontext.h>


using namespace std;
enum CoState
{
    CO_READY,
    CO_SUSPENDED,
    CO_FINISHED,
    CO_RUNNING,
};

struct coroutine
{
    void* yield;
    int       status;
    int       id;
    void (*func) (void *);
    void*   arg;
    char*  stack;
    ucontext_t cxt;
};

class SchedulerImpl
{
    public:
        SchedulerImpl():running_(-1)
                        ,alloc_id_(0)
                        ,stacksize_(1024)
        {
        }

        void* ResumeCoroutine(int id, void* y);
        void* Yield(void* y);
        int CreateCoroutine( void(*func)(void*) , void* args);
        bool IsCoroutineAlive( int id );
        void DestroyCoroutine( int id);

        int getRunning(){ return running_; }
        void resetRuning(){ running_ = -1; }
        coroutine* getCoroutine( int id ){ return id2routine_[id]; }
        
    private:
        map< int, coroutine* > id2routine_;
        int running_;
        int alloc_id_;
        ucontext_t mainContext_;
        int stacksize_;
};

int SchedulerImpl::CreateCoroutine( void(*func)(void*) , void* args)
{
    coroutine* pcor = new coroutine();

    pcor->id = ++alloc_id_;
    pcor->arg = args;
    pcor->status = CO_READY;
    pcor->func = func;
    pcor->stack = new char[stacksize_];

    id2routine_[ pcor->id ] = pcor;

    cout<<"create coroutine id"<<pcor->id<<endl;

    return pcor->id;
}

// static function
void Schedule(void* arg)
{
    SchedulerImpl* sched = (SchedulerImpl*) arg;

    int running = sched->getRunning();

    coroutine* cor = sched->getCoroutine(running);

    cor->func(cor->arg);

    sched->resetRuning();
    cor->status = CO_FINISHED;
}

// resume coroutine.
void* SchedulerImpl::ResumeCoroutine(int id, void* y)
{
    coroutine* cor = id2routine_[id];
    if (cor == NULL || cor->status == CO_RUNNING) return 0;

    cor->yield = y;
    switch (cor->status)
    {
        case CO_READY:
            {
                getcontext(&cor->cxt);

                cor->status = CO_RUNNING;
                cor->cxt.uc_stack.ss_sp = cor->stack;
                cor->cxt.uc_stack.ss_size = stacksize_;
                // sucessor context.
                cor->cxt.uc_link = &mainContext_;

                running_ = id;
                makecontext(&cor->cxt, (void (*)( ))Schedule, 1, this);
                cout<<"makecontext success...."<<endl;
                swapcontext(&mainContext_, &cor->cxt);
            }
            break;
        case CO_SUSPENDED:
            {
                running_ = id;
                cor->status = CO_RUNNING;
                swapcontext(&mainContext_, &cor->cxt);
            }
            break;
    }

    void* ret = cor->yield;

    if (running_ == -1 && cor->status == CO_FINISHED) DestroyCoroutine(id);

    return ret;
}

void SchedulerImpl::DestroyCoroutine( int id)
{
    coroutine* cor = id2routine_[id];
    if( !cor )
        return;

    id2routine_.erase(id);

    cout<<"destroy coroutine: " << id << endl;
}

void* SchedulerImpl::Yield(void* y)
{
    if (running_ < 0) return 0;

    int cur = running_;
    running_ = -1;

    coroutine* cor = id2routine_[cur];

    cor->yield = y;
    cor->status = CO_SUSPENDED;

    swapcontext(&cor->cxt, &mainContext_);
    return cor->yield;
}

bool SchedulerImpl::IsCoroutineAlive( int id )
{
    return id2routine_[id] != NULL;
}

