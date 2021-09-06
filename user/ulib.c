#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char*
strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  char *cdst = (char *) dst;
  int i;
  for(i = 0; i < n; i++){
    cdst[i] = c;
  }
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  if (src > dst) {
    while(n-- > 0)
      *dst++ = *src++;
  } else {
    dst += n;
    src += n;
    while(n-- > 0)
      *--dst = *--src;
  }
  return vdst;
}

int
memcmp(const void *s1, const void *s2, uint n)
{
  const char *p1 = s1, *p2 = s2;
  while (n-- > 0) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

void *
memcpy(void *dst, const void *src, uint n)
{
  typedef uint64 __attribute__((__may_alias__)) u64;
  unsigned char *d = dst;
  const unsigned char *s = src;
  // slow copy initial bytes until destination is aligend.
  for (; (unsigned long)s % 8 && n; n--)
    *d++ = *s++;

  // Fast copy 32 bytes at a time.
  for (; n >= 32; s += 32, d += 32, n -= 32)
  {
    *(u64 *)(d + 0) = *(u64 *)(s + 0);
    *(u64 *)(d + 8) = *(u64 *)(s + 8);
    *(u64 *)(d + 16) = *(u64 *)(s + 16);
    *(u64 *)(d + 24) = *(u64 *)(s + 24);
  }
  if (n >= 16)
  {
    *(u64 *)(d + 0) = *(u64 *)(s + 0);
    *(u64 *)(d + 8) = *(u64 *)(s + 8);
    s += 16;
    d += 16;
    n -= 16;
  }
  if (n >= 8)
  {
    *(u64 *)(d + 0) = *(u64 *)(s + 0);
    s += 8;
    d += 8;
    n -= 8;
  }

  // Slow copy remaining bytes.
  for (; n; n--)
    *d++ = *s++;
  return dst;
}