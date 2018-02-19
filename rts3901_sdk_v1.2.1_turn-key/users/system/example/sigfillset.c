#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    sigset_t sigset;

    /*
     * Blocking all signals ensures that the signal
     * handling action for the signals in the set is
     * not taken until the signals are unblocked.
     */

    sigfillset(&sigset);
    sigprocmask(SIG_SETMASK, &sigset, NULL);

    printf("before kill()\n");
    kill(getpid(), SIGUSR2);
    printf("after kill()\n");

    return(0);
}
