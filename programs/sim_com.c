#include <stdint.h>

volatile uint64_t tohost;
volatile uint64_t fromhost;

static void do_tohost(uint64_t tohost_value)
{
  while (tohost)
    fromhost = 0;
  tohost = tohost_value;
}

static void cputchar(int x)
{
  do_tohost(0x0101000000000000 | (unsigned char)x);
}

static void cputstring(const char* s)
{
  while (*s)
    cputchar(*s++);
}

int main()
{
    cputstring("Hello World!\r\n");

    return 0;
}
