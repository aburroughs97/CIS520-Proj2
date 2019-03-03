#include <syscall.h>
#include <stdio.h>

int main (int, char *[]);
void _start (int argc, char *argv[]);

void
_start (int argc, char *argv[]) 
{
	char *a[] = { "hi" };
	argv = a;
  exit (main (argc, argv));
}
