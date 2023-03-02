JPL Runtime Library
===================

This repository contains the JPL runtime library (currently for the
2023 edition of JPL). The JPL runtime provides a collection of
functions that JPL programs can use. It is written in standard C and
is intended to be compiled by a standard C compiler (probably Clang).

# Dependencies and Compiling

You should be able to compile the JPL runtime on your machine simply
by running:

    make

This should produce a file called `runtime.a`. This "object archive"
file is simply a single file containing multiple object files. If
you'd like to change what C compile is used to do the compilation, you
can pass the `CC` variable to `make`:

    make CC=clang
    
The default compiler is system-specific.

Once you have the runtime compiled, you can link it against a JPL
object file `program.o` with the following invocation:

    ld program.o <runtime directory>/runtime.a -lpng -lm -o program
    
In detail, we are asking the linker, called `ld`, to link together
four sets of object files:

 - The JPL object file `program.o`, which you presumbly got by
   compiling a JPL program and then assembling the resulting assembly;
 - The `runtime.a` file you got by compiling this runtime;
 - The LibPNG library for reading PNG files, which you'll need to
   install. The linker finds it in standard system-wide paths. If you
   instead download LibPNG and compile it yourself, you'd need to pass
   `-L<path to libpng.o>` to `ld`, before the `-lpng` argument.
 - The math library (what in C is called `math.h`), which is called
   LibM.
 
Make sure to pass the linker arguments in exactly this order, because
the linker resolves references left to right. For example, if you put
`-lpng` before `runtime.a`, then when the linker goes to resolve
references to PNG functions, it won't yet have seen the ones inside
`runtime.a`.
    
Any linker should work fine. If you do not have an `ld` on your
computer, try `gcc` or `clang` instead. (You will not actually compile
any C code; instead, `gcc` or `clang` will recognize that you've
passed them a bunch of object files, and invoke the correct linker.)

# Main functions

The runtime expects a single function to be available to it, with the
following C-style declaration:

    struct args { int64_t argnum; int64_t *data; }
    void jpl_main(struct args args)
    
Note that `jpl_main` is passed a single argument, which has the same
representation as a JPL `int[]` type. Further note that `jpl_main`
returns `void`; that is, your compiler does not need to return `0`
from its `main` method to indicate no error occurred. (That is simply
assumed if your compiled code does not call `fail_assertion`.)

In exchange, the runtime provides a `main` function with a standard C
signature. This method takes care of allocating heap memory for the
arguments, parsing the command line arguments as integers, and
checking for errors. It will cleanly exit the program if any error
conditions occur, and will always return a `0` exit code if no error
occurs.

# Helper functions

The `fail_assertion` method has the following signature:

    void fail_assertion(char *message)

This method prints the error message to standard error and then exits
with exit code 1. Your compiled code should call `fail_assertion` if
any runtime error, like dividing by zero, occurs.

The `jpl_alloc` function has the following signature:

    void *jpl_alloc(size_t bytes)

It wraps the standard `malloc` method and checks for errors. If
`malloc` fails to allocate memory, it cleanly exits the program, with
exit code 1. If `0` bytes are requested, it always returns a null
pointer. Note that its argument is an _unsigned_ 64-bit integer.

The `get_time` function has the following signature:

    double get_time(void)
    
That is, it takes no arguments and returns a 64-bit floating-point
number. That number is measured in seconds with no particular starting
point. You are intended to take the difference between two `get_time`
invocations to measure how long something took (to implement the JPL
`time` command).

# Printing functions

The `print` function prints to the screen:

    void print(char *message)

The argument to `print` must be a pointer to a *null terminated*
sequence of characters---a C string. If, for whatever reason, printing
fails, the `print` function terminates cleanly with error code 127.

The `show` function has the following signature:

    void show(char *type, void *data)

The first argument to `show` must be a *null terminated* C string. The
contents of this string are the S-expression form of the type of the
data being shown. For example, the following invocation prints
`3.200000` to the screen:

    show("(FloatType)", &3.2)
    
Make sure that only resolved type are passed (that is, no type
definitions).

The second argument is a pointer to data in the specified type. For
example, above, the specified type is a JPL `float`, so a pointer to a
`double` value is passed as the second argument. Importantly, if an
array is being printed, you must pass a pointer to the array record,
not the heap pointer inside the array itself.

Internally, the `show` routine parses the type definition into an
internal binary format, and then interprets that format to print the
data using a type-specific printing method. If any of these steps
fail---parsing, allocating data, or printing---it will cleanly
terminate the program with error code 127.

# I/O functions

The `read_image` function reads an image from disk:

    struct pict { int64_t rows; int64_t cols; double *data; }
    struct pict read_image(char *filename)
    
The `pict` struct here is the C representation of the JPL type
`{float,float,float,float}[,]`, which JPL uses to represent images.
The filename passed as the second argument must be a *null terminated*
string and is interpreted relative to the current directory. You
should pass this file name through from the file name given in a JPL
`read image` command.

The `write_image` function writes an image to disk:

    void write_image(struct pict image, char *filename)

The image is written out as an 8-bit-depth RGBA image
Pixel data is handled as described in the JPL specification: channel
values greater than `1.0` or less than `0.0` are clamped at the
minimum or maximum value, and infinities and NaNs are treated as if
they were `0.0` values.

In either function, if any error occurs, including permissions errors,
file system errors, memory allocation errors, or decoding errors, this
function will cleanly exit with some non-zero exit code. (It depends
on the PNG library internals.)
