#include <iostream>
#include <pthread.h>

int main(int argc, char* argv[])
{
  pthread_mutex_t mutex;
  pthread_mutex_init(&mutex, NULL);
  std::cout << "Hello World!" << "\n";
  return 0;
}
