#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "runtime.h"

int64_t sub_ints(int64_t a, int64_t b) {
  return a - b;
}

int64_t _sub_ints(int64_t a, int64_t b) {
  return sub_ints(a, b);
}

double sub_floats(double a, double b) {
  return a - b;
}

double _sub_floats(double a, double b) {
  return sub_floats(a, b);
}

int32_t has_size(struct pict input, int64_t rows, int64_t cols) {
  return input.rows == rows && input.cols == cols;
}

int32_t _has_size(struct pict input, int64_t rows, int64_t cols) {
  return has_size(input, rows, cols);
}

#define INDX(p, x, y, chan) p.data[(4 * (((x) * (p).cols) + (y)) + (chan))]

// sepia(pict) : pict
struct pict sepia(struct pict p) {
  struct pict ret;
  ret.rows = p.rows;
  ret.cols = p.cols;
  ret.data = malloc(p.rows * p.cols * 4 * sizeof(double));
  if (!ret.data)
    fail_assertion("malloc failed\n");
  for (long i=0; i<p.rows; ++i) {
    for (long j=0; j<p.cols; ++j) {
      double oldR = INDX(p, i, j, 0);
      double oldG = INDX(p, i, j, 1);
      double oldB = INDX(p, i, j, 2);
      INDX(ret, i, j, 0) = 0.393 * oldR + 0.769 * oldG + 0.189 * oldB;
      INDX(ret, i, j, 1) = 0.349 * oldR + 0.686 * oldG + 0.168 * oldB;
      INDX(ret, i, j, 2) = 0.272 * oldR + 0.534 * oldG + 0.131 * oldB;
      INDX(ret, i, j, 3) = 1.0;
    }
  }
  return ret;
}
struct pict _sepia(struct pict p) {
  return sepia(p);
}

// blur(pict, float) : pict
struct pict blur(struct pict p, double f) {
  if (f <= 0.0)
    fail_assertion("Blur radius must be positive\n");

  int64_t sides = (int) fmin(3.0 * f + 0.5, p.rows < p.cols ? p.rows : p.cols);
  int64_t size = 2 * sides + 1;
  double *filter = malloc(size * size * sizeof(double));
  if (!filter)
    fail_assertion("malloc failed\n");

  double denom = 1.0 / (2.0 * M_PI * f * f);
  double edenom = -1.0 / (2.0 * f * f);
  for (long i = -sides; i <= sides; i++) {
    for (long j = -sides; j <= sides; j++) {
      double r = i * i + j * j;
      filter[(i + sides) * size + j + sides] = denom * exp(r * edenom);
    }
  }

  struct pict ret;
  ret.rows = p.rows;
  ret.cols = p.cols;
  ret.data = malloc(p.rows * p.cols * 4 * sizeof(double));
  if (!ret.data)
    fail_assertion("malloc failed\n");
  for (long i = 0; i < p.rows; i++) {
    for (long j = 0; j < p.cols; j++) {
      double r = 0.0;
      double g = 0.0;
      double b = 0.0;
      double total = 0.0;
      for (long n = i - sides; n <= i + sides; n++) {
        if (n < 0) continue;
        if (n >= p.rows) continue;
        for (long m = j - sides; m <= j + sides; m++) {
          if (m < 0) continue;
          if (m >= p.cols) continue;
          double scale = filter[size * (n - i + sides) + (m - j + sides)];
          total += scale;
          r += p.data[4 * (n * p.cols + m) + 0] * scale;
          g += p.data[4 * (n * p.cols + m) + 1] * scale;
          b += p.data[4 * (n * p.cols + m) + 2] * scale;
        }
      }
      ret.data[4 * (i * p.cols + j) + 0] = r / total;
      ret.data[4 * (i * p.cols + j) + 1] = g / total;
      ret.data[4 * (i * p.cols + j) + 2] = b / total;
      ret.data[4 * (i * p.cols + j) + 3] = 1.0;
    }
  }
  return ret;
}

struct pict _blur(struct pict p, double f) {
  return blur(p, f);
}

// resize(pict, int, int) : pict
struct pict resize(struct pict p, int x, int y) {
  if (x <= 0 || y <= 0)
    fail_assertion("Must resize to a positive size\n");
  struct pict ret;
  ret.rows = x;
  ret.cols = y;
  ret.data = malloc(x * y * 4 * sizeof(double));
  if (!ret.data)
    fail_assertion("malloc failed\n");
  for (long i=0; i<x; ++i) {
    for (long j=0; j<y; ++j) {
      double oldi = (i + 0.5) * p.rows / ((double) x) - 0.5;
      double oldj = (j + 0.5) * p.rows / ((double) x) - 0.5;
      
      int64_t oii, oji;
      double oif, ojf;
      oii = (int64_t) oldi;
      oji = (int64_t) oldj;
      oif = oldi - (double) oii;
      ojf = oldj - (double) oji;

      double r, g, b, a;
      r = (1.0 - oif) * (1.0 - ojf) * INDX(p, oii, oji, 0) \
        + (oif > 0.0 ? (0.0 + oif) * (1.0 - ojf) * INDX(p, oii + 1, oji, 0) : 0.0) \
        + (ojf > 0.0 ? (1.0 - oif) * (0.0 + ojf) * INDX(p, oii, oji + 1, 0) : 0.0) \
        + (oif * ojf > 0.0 ? (0.0 + oif) * (0.0 + ojf) * INDX(p, oii + 1, oji + 1, 0) : 0.0);
      g = (1.0 - oif) * (1.0 - ojf) * INDX(p, oii, oji, 1) \
        + (oif > 0.0 ? (0.0 + oif) * (1.0 - ojf) * INDX(p, oii + 1, oji, 1) : 0.0) \
        + (ojf > 0.0 ? (1.0 - oif) * (0.0 + ojf) * INDX(p, oii, oji + 1, 1) : 0.0) \
        + (oif * ojf > 0.0 ? (0.0 + oif) * (0.0 + ojf) * INDX(p, oii + 1, oji + 1, 1) : 0.0);
      b = (1.0 - oif) * (1.0 - ojf) * INDX(p, oii, oji, 2) \
        + (oif > 0.0 ? (0.0 + oif) * (1.0 - ojf) * INDX(p, oii + 1, oji, 2) : 0.0) \
        + (ojf > 0.0 ? (1.0 - oif) * (0.0 + ojf) * INDX(p, oii, oji + 1, 2) : 0.0) \
        + (oif * ojf > 0.0 ? (0.0 + oif) * (0.0 + ojf) * INDX(p, oii + 1, oji + 1, 2) : 0.0);
      a = 1.0;

      INDX(ret, i, j, 0) = r;
      INDX(ret, i, j, 1) = g;
      INDX(ret, i, j, 2) = b;
      INDX(ret, i, j, 3) = a;
    }
  }
  return ret;
}

struct pict _resize(struct pict p, int x, int y) {
  return resize(p, x, y);
}

// crop(pict, int, int, int, int) : pict
struct pict crop(struct pict p, int x1, int y1, int x2, int y2) {
  struct pict ret;
  ret.rows = y2 - y1;
  ret.cols = x2 - x1;
  ret.data = malloc(ret.rows * ret.cols * 4 * sizeof(double));
  if (!ret.data)
    fail_assertion("malloc failed\n");
  for (long i=0; i<ret.cols; ++i) {
    for (long j=0; j<ret.rows; ++j) {
      long ii = i + y1;
      long jj = j + x1;
      INDX(ret, i, j, 0) = INDX(p, ii, jj, 0);
      INDX(ret, i, j, 1) = INDX(p, ii, jj, 1);
      INDX(ret, i, j, 2) = INDX(p, ii, jj, 2);
      INDX(ret, i, j, 3) = INDX(p, ii, jj, 3);
    }
  }
  return ret;
}
struct pict _crop(struct pict p, int x1, int y1, int x2, int y2) {
  return crop(p, x1, y1, x2, y2);
}
