#include <stdint.h>

struct pict {
    int64_t rows;
    int64_t cols;
    double *data;
};

int main(int, char**);
int32_t show(char*, void*);
void fail_assertion(char*);
void print(char*);
double get_time(void);
struct pict read_image(char*);
void write_image(struct pict, char*);
void *jpl_alloc(size_t);

