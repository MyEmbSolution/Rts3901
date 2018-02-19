#include <stdlib.h>
#include <stdio.h>

#define HAVE_POSIX_MEMALIGN

void *jack_pool(size_t bytes)
{
#ifdef HAVE_POSIX_MEMALIGN
    void *m;

    int err = posix_memalign (&m, 16, bytes);

    return (!err) ? m : 0;
#else
    return malloc (bytes);
#endif /* HAVE_POSIX_MEMALIGN */
}

int main(int argc, char **argv)
{
  void *m;
  m = jack_pool(32);

  if (m == NULL) {
    printf("jack_pool: failed\n");
    return -1;
  }

  printf("jack_pool, jack pot\n");
  return 0;
}
