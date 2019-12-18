// https://github.com/pjreddie/template

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "args.hpp"

void del_arg(int argc, char **argv, int index)
{
  int i;
  for (i = index; i < argc - 1; ++i) argv[i] = argv[i + 1];
  argv[i] = 0;
}

int find_arg(int argc, char* argv[], const char *arg)
{
  int i;
  for (i = 0; i < argc; ++i) {
    if (!argv[i]) continue;
    if (0 == strcmp(argv[i], arg)) {
      del_arg(argc, argv, i);
      return 1;
    }
  }
  return 0;
}

int find_int_arg(int argc, char **argv, const char *arg, int def)
{
  int i;
  for (i = 0; i < argc - 1; ++i) {
    if (!argv[i]) continue;
    if (0 == strcmp(argv[i], arg)) {
      def = atoi(argv[i + 1]);
      del_arg(argc, argv, i);
      del_arg(argc, argv, i);
      break;
    }
  }
  return def;
}

float find_float_arg(int argc, char **argv, const char *arg, float def)
{
  int i;
  for (i = 0; i < argc - 1; ++i) {
    if (!argv[i]) continue;
    if (0 == strcmp(argv[i], arg)) {
      def = atof(argv[i + 1]);
      del_arg(argc, argv, i);
      del_arg(argc, argv, i);
      break;
    }
  }
  return def;
}

const char *find_char_arg(int argc, char **argv, const char *arg, const char *def)
{
  int i;
  for (i = 0; i < argc - 1; ++i) {
    if (!argv[i]) continue;
    if (0 == strcmp(argv[i], arg)) {
      def = argv[i + 1];
      del_arg(argc, argv, i);
      del_arg(argc, argv, i);
      break;
    }
  }
  return def;
}
