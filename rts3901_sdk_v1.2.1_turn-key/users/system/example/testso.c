#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

//extern void libtest1 (void);
//extern void libtest1_rm1 (void);

int
main (int argc, char **argv)
{
  void *handle;
  void (*symbol) (void);

  printf("Testing dlopen\n");

  handle = dlopen ("./libtest.so", RTLD_LAZY);
  if (!handle)
    {
      fprintf (stderr, "%s\n", dlerror ());
      exit (1);
    }

  printf("libtest.so opened\n");

  symbol = dlsym (handle, "libtest2");
  if (symbol == NULL)
    {
      dlclose(handle);
      fprintf (stderr, "%s\n", dlerror());
      exit (1);
    }

  (*symbol) ();
  dlclose (handle);
  return 0;
}
