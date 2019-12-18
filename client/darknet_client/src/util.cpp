#include "util.hpp"

// utility
int str_to_int(const char* str, int len)
{
  int i;
  int ret = 0;
  for (i = 0; i < len; ++i)
  {
    ret = ret * 10 + (str[i] - '0');
  }
  return ret;
}