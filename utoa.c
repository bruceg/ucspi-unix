const char* utoa(unsigned long i)
{
#define LONGLEN (sizeof(unsigned long)*4)
  static char buf[LONGLEN];
  char* ptr = buf + LONGLEN;
  if(!i)
    return "0";
  *--ptr = 0;
  while(i) {
    *--ptr = (i % 10) + '0';
    i /= 10;
  }
  return ptr;
}
