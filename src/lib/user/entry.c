#include <syscall.h>
#include <stdio.h>

int main (int, char *[]);
void _start (int argc, char *argv[]);

void
_start (int argc, char *argv[]) 
{
  printf("starting\n");
  exit (main (argc, argv));
}
