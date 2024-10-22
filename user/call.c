#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int g(int x) {
  return x+3;
}

__attribute__((noinline)) int f(int x) {
  return g(x);
}

void main(int argc, char* argv[]) {
  sleep(0); // testing backtrace
  printf("%d %d %d %s\n", f(8)+1, 13, argc, argv[0]);
  unsigned int i = 0x00646c72;
	printf("H%x Wo%s", 57616, &i);
  printf("x=%d y=%d\n", 3);
  exit(0);
}
