#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <stdnoreturn.h>
#include <errno.h>

#include "pngstuff.h"
#include "runtime.h"

char *full_type_string = 0;

noreturn void fail(char *who, char *msg) {
  if (full_type_string) {
    fprintf(stderr, "[abort] %s: %s in '%s'\n", who, msg, full_type_string);
  } else {
    fprintf(stderr, "[abort] %s: %s\n", who, msg);
  }
  exit(127);
}


void *jpl_alloc(int64_t size) {
  if (getenv("JPLRTDEBUG")) {
    printf("[debug] jpl_alloc(%lld)\n", size);
  }
  if (size <= 0) {
    fail("jpl_alloc", "Could not allocate 0 or negative amount of memory");
  } else {
    void *mem = malloc(size);
    memset(mem, 0, size);
    if (!mem) fail("jpl_alloc", "Could not allocate array memory");
    return mem;
  }
}

void *_jpl_alloc(int64_t size) {
  return jpl_alloc(size);
}

/* Note that this isn't the struct args that the documentation
   describes: it has 24 bytes of zeros padded at the end. The reason
   for this is to make it so large that it gets passed on the stack,
   in keeping with the JPL ABI, which is simplified compared to the
   x86_64 Linux ABI. */
struct actual_args {
  int64_t d0;
  int64_t *data;
  int64_t pad1;
  int64_t pad2;
  int64_t pad3;
};

void jpl_main(struct actual_args);

int main(int argc, char **argv) {
  setbuf(stdout, NULL);
  int64_t argnum = argc - 1;
  // Can't overflow
  int64_t *argdata = malloc(sizeof(int64_t) * argnum);
  if (argnum && !argdata) fail("main", "Could not allocate memory for arguments");
  for (int i = 1; i < argc; i++) {
    argdata[i] = strtol(argv[i], 0, 10);
    if (errno) fail("main", "Command line argument too large");
  }
  struct actual_args args = { argnum, argdata, 0, 0, 0 };
  if (getenv("JPLRTDEBUG")) {
    printf("[debug] calling jpl_main({%lld, [", argnum);
    for (int i = 0; i < argnum; i++) {
      printf("%lld, ", argdata[i]);
    }
    printf("]})\n");
  }
  jpl_main(args);
  if (getenv("JPLRTDEBUG")) {
    printf("[debug] jpl_main returned cleanly\n");
  }
  return 0;
}

#define PB 1
#define MAX 256

struct {
  size_t i;
  uint8_t data[MAX];
} mem;

uint8_t getmem(size_t amt) {
  if (mem.i + amt < MAX) {
    mem.i += amt;
    return mem.i - amt;
  } else fail("show", "Type too complex, cannot parse");
}

void tprintf(const char *fmt, ...) {
  int bad = 0;
  va_list args;
  va_start(args, fmt);
  if (vprintf(fmt, args) < 0) bad = 1;
  va_end(args);
  if (bad) fail("show", "printf failed, aborting");
}

// Types are:
enum BType {
  TUPLE = 0,
  VOID = 250,
  BOOL = 251,
  INT = 252,
  FLOAT = 253,
  ARRAY = 254,
  NDARRAY = 255,
};

void skip_whitespace(char *type_str, char **new_type_str) {
  while (*type_str == ' ' || *type_str == '\n') type_str++;
  *new_type_str = type_str;
}

void ensure_literal(char *literal, char *type_str, char**new_type_str, char *error) {
  for (; *literal; literal++) {
    if (*type_str++ != *literal) fail("show", error);
  }
  *new_type_str = type_str;
}

uint8_t parse_type(char *type_str, char **new_type_str);
size_t size_type(uint8_t t);
void show_type(uint8_t t, void *data);

uint8_t parse_void_type(char *type_str, char **new_type_str) {
  ensure_literal("VoidType", type_str, &type_str, "Could not parse void type");
  skip_whitespace(type_str, &type_str);
  ensure_literal(")", type_str, &type_str, "Could not parse void type");

  uint8_t t = getmem(1);
  mem.data[t] = VOID;
  *new_type_str = type_str;
  return t;
}

uint8_t parse_bool_type(char *type_str, char **new_type_str) {
  ensure_literal("BoolType", type_str, &type_str, "Could not parse boolean type");
  skip_whitespace(type_str, &type_str);
  ensure_literal(")", type_str, &type_str, "Could not parse boolean type");

  uint8_t t = getmem(1);
  mem.data[t] = BOOL;
  *new_type_str = type_str;
  return t;
}

uint8_t parse_int_type(char *type_str, char **new_type_str) {
  ensure_literal("IntType", type_str, &type_str, "Could not parse integer type");
  skip_whitespace(type_str, &type_str);
  ensure_literal(")", type_str, &type_str, "Could not parse integer type");

  uint8_t t = getmem(1);
  mem.data[t] = INT;
  *new_type_str = type_str;
  return t;
}

uint8_t parse_float_type(char *type_str, char **new_type_str) {
  ensure_literal("FloatType", type_str, &type_str, "Could not parse floating type");
  skip_whitespace(type_str, &type_str);
  ensure_literal(")", type_str, &type_str, "Could not parse floating type");

  uint8_t t = getmem(1);
  mem.data[t] = FLOAT;
  *new_type_str = type_str;
  return t;
}

uint8_t parse_tuple_type(char *type_str, char **new_type_str) {
  ensure_literal("TupleType", type_str, &type_str, "Could not parse tuple type");
  skip_whitespace(type_str, &type_str);

  uint8_t tuple_mem[256];
  size_t i = 0;
  while (*type_str != ')') {
    if (i >= VOID) fail("show", "Tuple has too many fields");
    tuple_mem[i++] = parse_type(type_str, &type_str);
    skip_whitespace(type_str, &type_str);
  }
  ensure_literal(")", type_str, &type_str, "Could not parse tuple type");

  uint8_t t, p;
  p = t = getmem(1 + i);
  mem.data[t++] = i;
  for (int j = 0; j < i; j++) {
    mem.data[t++] = tuple_mem[j];
  }
  *new_type_str = type_str;
  return p;
}

uint8_t parse_array_type(char *type_str, char **new_type_str) {
  ensure_literal("ArrayType", type_str, &type_str, "Could not parse array type");
  skip_whitespace(type_str, &type_str);

  uint8_t t = parse_type(type_str, &type_str);
  skip_whitespace(type_str, &type_str);

  uint32_t rank = 0;
  while ('0' <= *type_str && *type_str <= '9') {
    rank = 10 * rank + (*type_str++ - '0');
    if (rank > 255) fail("show", "Array rank is too large");
  }
  if (rank <= 0) fail("show", "Array rank is too small");
  skip_whitespace(type_str, &type_str);
  ensure_literal(")", type_str, &type_str, "Could not parse integer type");
  *new_type_str = type_str;

  uint8_t p2, p;
  if (rank == 1) {
    p2 = p = getmem(2);
    mem.data[p++] = ARRAY;
    mem.data[p++] = t;
  } else {
    p2 = p = getmem(3);
    mem.data[p++] = NDARRAY;
    mem.data[p++] = rank;
    mem.data[p++] = t;
  }
  return p2;
}

uint8_t parse_type(char *type_str, char **new_type_str) {
  skip_whitespace(type_str, &type_str);
  if (*type_str++ != '(') fail("show", "Could not parse type");

  uint8_t t = 0;
  switch (*type_str) {
  case 'T':
    t = parse_tuple_type(type_str, &type_str);
    break;
  case 'I':
    t = parse_int_type(type_str, &type_str);
    break;
  case 'F':
    t = parse_float_type(type_str, &type_str);
    break;
  case 'B':
    t = parse_bool_type(type_str, &type_str);
    break;
  case 'A':
    t = parse_array_type(type_str, &type_str);
    break;
  case 'V':
    t = parse_void_type(type_str, &type_str);
    break;
  default:
    fail("show", "Could not parse type");
  }
  *new_type_str = type_str;
  return t;
}

size_t size_type(uint8_t t) {
  int rank, fields, size;
  switch (mem.data[t]) {
  case BOOL:
    return sizeof(int64_t);
  case INT:
    return sizeof(int64_t);
  case FLOAT:
    return sizeof(double);
  case ARRAY:
    return sizeof(int64_t) + sizeof(void *);
  case NDARRAY:
    rank = (int) mem.data[t + 1];
    return rank * sizeof(int64_t) + sizeof(void *);
  default:
    fields = (int) mem.data[t];
    size = 0;
    for (int i = 0; i < fields; i++) {
      size += size_type(mem.data[t + i + 1]);
    }
    return size;
  }
}

void show_array(uint8_t subtype, int rank, uint64_t *data2) {
  char *subdata = (char*)data2[rank];
  uint64_t size = 1;
  for (int i = 0; i < rank; i++) {
    uint64_t dim = data2[i];
    if (__builtin_mul_overflow(size, dim, &size))
      fail("show", "Overflow when computing total size of array");
  }

  size_t step = size_type(subtype);
  tprintf("[");
  for (int64_t i = 0; i < size; i++) {
    show_type(subtype, subdata + i * step);
    int64_t j = i + 1;
    if (j == size) break;
    int rankstep = 0;
    while (j % data2[rank - rankstep - 1] == 0) {
      j /= data2[rank - rankstep - 1];
      rankstep++;
    }
    if (i + 1 < size) {
      if (rankstep == 0) {
        tprintf(", ");
      } else {
        for (j = 0; j < rankstep; j++) tprintf(";");
        tprintf(" ");
      }
    }
  }
  tprintf("]");
}

void show_type(uint8_t t, void *data) {
  switch (mem.data[t]) {
  case VOID:
    tprintf("void");
    return;
  case BOOL:
    if (*(bool*)data) {
      tprintf("true");
    } else {
      tprintf("false");
    }
    return;
  case INT:
    tprintf("%lld", *(int64_t*)data);
    return;
  case FLOAT:
    tprintf("%f", *(double*)data);
    return;
  case ARRAY: {
    show_array(mem.data[t + 1], 1, data);
    return;
  }
  case NDARRAY: {
    show_array(mem.data[t + 2], (int) mem.data[t + 1], data);
    return;
  }
  default: {
    int fields = (int) mem.data[t];
    int offset = 0;
    tprintf("{");
    char *data_char = data;
    for (int i = 0; i < fields; i++) {
      show_type(mem.data[t + 1 + i], data_char + offset);
      offset += size_type(mem.data[t + 1 + i]);
      if (i + 1 != fields) {
        tprintf(", ");
      }
    }
    tprintf("}");
    return;
  }
  }
}

void show(char *type_str, void *data) {
  if (getenv("JPLRTDEBUG")) {
    printf("[debug] show(\"%s\", %p)\n", type_str, data);
  }

  /*  */
  full_type_string = type_str;
  if (strnlen(type_str, 256) == 256) fail("show", "Type string is too long");
  mem.i = 0; // Allocate memory

  uint8_t t = parse_type(type_str, &type_str);
  skip_whitespace(type_str, &type_str);
  if (*type_str != '\0') fail("show", "Could not parse type string");

  show_type(t, data);
  tprintf("\n");
  full_type_string = 0;
}

void _show(char *type_str, void *data) {
  show(type_str, data);
}

void fail_assertion(char *s) {
  if (getenv("JPLRTDEBUG")) {
    printf("[debug] fail_assertion(\"%s\")\n", s);
  }
  printf("[abort] %s\n", s);
  exit(1);
}

void _fail_assertion(char *s) {
  fail_assertion(s);
}

void print(char *s) {
  if (getenv("JPLRTDEBUG")) {
    printf("[debug] print(\"%s\")\n", s);
  }
  int i = printf("%s\n", s);
  if (i < 0) fail("print", "Failed to print");
}

void _print(char *s) {
  print(s);
}

void print_time(double d) {
  if (getenv("JPLRTDEBUG")) {
    printf("[debug] print(%g)\n", d);
  }
  int i = printf("[time] %.6fms\n", d * 1000.0);
  if (i < 0) fail("print_time", "Failed to print");
}

void _print_time(double d) {
  print_time(d);
}

double get_time(void) {
  if (getenv("JPLRTDEBUG")) {
    printf("[debug] get_time()\n");
  }
  clock_t c = clock();
  return ((double) c) / CLOCKS_PER_SEC;
}

double _get_time(void) {
  return get_time();
}

_a2_rgba read_image(char *filename) {
  if (getenv("JPLRTDEBUG")) {
    printf("[debug] read_image(\"%s\")\n", filename);
  }
  _a2_rgba out;
  _readPNG(&out.d0, &out.d1, (double**)&out.data, filename);
  return out;
}

_a2_rgba _read_image(char *filename) {
  return read_image(filename);
}

void write_image(_a2_rgba input, char *filename) {
  if (getenv("JPLRTDEBUG")) {
    printf("[debug] write_image({%lld, %lld, %p}, \"%s\")\n", input.d0, input.d1, (void*)input.data, filename);
  }

  _writePNG(input.d0, input.d1, (double*)input.data, filename);
}

void _write_image(_a2_rgba input, char *filename) {
  write_image(input, filename);
}

int64_t to_int(double x) {
  if (isnan(x)) return 0;
  if (isinf(x) && x > 0.0) return LLONG_MAX;
  if (isinf(x) && x < 0.0) return LLONG_MIN;
  return (int64_t) x;
}

int64_t _to_int(double x) {
  return to_int(x);
}

double to_float(int64_t x) {
  return (double) x;
}

double _to_float(int64_t x) {
  return to_float(x);
}

#define FORWARD1(name) \
  double _##name(double x) { \
    return name(x); \
  }

#define FORWARD2(name) \
  double _##name(double x, double y) { \
    return name(x, y); \
  }

FORWARD1(sqrt)
FORWARD1(exp)
FORWARD1(sin)
FORWARD1(cos)
FORWARD1(tan)
FORWARD1(asin)
FORWARD1(acos)
FORWARD1(atan)
FORWARD1(log)

FORWARD2(fmod)
FORWARD2(pow)
FORWARD2(atan2)
