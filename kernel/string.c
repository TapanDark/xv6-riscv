#include "types.h"

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

int
memcmp(const void *v1, const void *v2, uint n)
{
  const uchar *s1, *s2;

  s1 = v1;
  s2 = v2;
  while(n-- > 0){
    if(*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }

  return 0;
}

void*
memmove(void *dst, const void *src, uint n)
{
  const char *s;
  char *d;

  s = src;
  d = dst;
  if(s < d && s + n > d){
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else
    while(n-- > 0)
      *d++ = *s++;

  return dst;
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

int
strncmp(const char *p, const char *q, uint n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}

char*
strncpy(char *s, const char *t, int n)
{
  char *os;

  os = s;
  while(n-- > 0 && (*s++ = *t++) != 0)
    ;
  while(n-- > 0)
    *s++ = 0;
  return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char*
safestrcpy(char *s, const char *t, int n)
{
  char *os;

  os = s;
  if(n <= 0)
    return os;
  while(--n > 0 && (*s++ = *t++) != 0)
    ;
  *s = 0;
  return os;
}

int
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

