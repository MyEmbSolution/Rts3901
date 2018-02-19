#include <signal.h>
#include <stdio.h>

static void my_alarm(int);

int
main(int argc, char **argv)
{
  struct passwd *ptr;
  
  signal(SIGALRM, my_alarm);
  alarm(1);

  for (;;) {
  }
}

static void
my_alarm(int signo)
{
  struct passwd *rootptr;

  printf("in signal handler\n");

  alarm(1);
  return;
}
