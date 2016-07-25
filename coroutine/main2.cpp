#include <iostream>
#include <ucontext.h>
using namespace std;

static char g_stack[2048];
static ucontext_t ctx,ctx_main;

void func()
{
    // do something.
    cout << "enter func" << endl;

    swapcontext(&ctx, &ctx_main);

    cout << "func1 resume from yield" << endl;
    // continue to do something.
}

int main()
{
   getcontext(&ctx);
   ctx.uc_stack.ss_sp = g_stack;
   ctx.uc_stack.ss_size = sizeof g_stack;
   ctx.uc_link = &ctx_main;
    
   makecontext(&ctx, func, 0);

   cout << "in main, before coroutine starts" << endl;

   swapcontext(&ctx_main, &ctx);

   cout << "back to main" << endl;

   swapcontext(&ctx_main, &ctx);
   
   cout << "back to main again" << endl;
   return 0;
}
