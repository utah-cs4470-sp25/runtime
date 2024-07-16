#include <stdint.h>

struct pict {
  int64_t rows;
  int64_t cols;
  double *data;
};

struct args {
  int64_t argnum;
  int64_t *data;
};

int main(int, char**);

void fail_assertion(char*);
void *jpl_alloc(int64_t);
double get_time(void);

void show(char*, void*);
void print(char*);
void print_time(double);

struct pict read_image(char*);
void write_image(struct pict, char*);

int64_t to_int(double);
double to_float(int64_t);
