// https://github.com/pjreddie/template

#ifndef ARGS_H
#define ARGS_H

int find_arg(int argc, char* argv[], const char *arg);
int find_int_arg(int argc, char **argv, const char *arg, int def);
float find_float_arg(int argc, char **argv, const char *arg, float def);
const char *find_char_arg(int argc, char **argv, const char *arg, const char *def);

#endif