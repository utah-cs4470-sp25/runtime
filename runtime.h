#include <stdint.h>

typedef struct {
  double r;
  double g;
  double b;
  double a;
} rgba;

typedef struct {
  int64_t d0;
  int64_t d1;
  rgba *data;
} _a2_rgba;

struct args {
  int64_t d0;
  int64_t *data;
};

int main(int, char**);

void fail_assertion(char*);
void *jpl_alloc(int64_t);
double get_time(void);

void show(char*, void*);
void print(char*);
void print_time(double);

_a2_rgba read_image(char*);
void write_image(_a2_rgba, char*);

int64_t to_int(double);
double to_float(int64_t);