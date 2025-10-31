/*


    Welcome to MiniRTS, an (almost) lock-free, low-overhead, low-latency runtime system!


    MiniRTS provides an easy-to-use API to configure your runtime, create and schedule tasks and
    attach continuations to existing tasks. Let's start by instantiating your runtime.

    To use MiniRTS, you simply need to #include "api/api.h". This will import all existing user-facing facilities.
*/




/*
    To instantiate a new runtime, simply call rts::instantiate_runtime().
*/


// #include "SPSCQueue/include/rigtorp/SPSCQueue.h"
// #include "runtime.h"
//
// rigtorp::SPSCQueue<int> a(64);
//
// rts::initialize_runtime();
//

#include "api.h"

int main() {
    rts::initialize_runtime<core::DefaultThreadPool>();
    return 0;
}
