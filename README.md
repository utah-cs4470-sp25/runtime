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

This downloads `libpng`, our PNG library, builds that, then builds the
JPL runtime, producing a file called `runtime.a`. This "object
archive" file is simply a single file containing multiple object
files.

Note that the runtime and `libpng` are both compiled for x86-64, even
if you have an ARM computer. This is because JPL only compiles to
x86-64.

If you'd like to change what C compiler is used to do the compilation,
you can pass the `CC` variable to `make`:

    make CC=clang

The default compiler is system-specific.

On macOS you may see warnings like this:

    /Library/Developer/CommandLineTools/usr/bin/ranlib: file: runtime.a(arm_init.o) has no symbols

You can ignore them.

# Linking against the runtime

Once you have the runtime compiled, you can link it against a JPL
object file `program.o` with the following invocation:

    clang -arch x86_64 program.o <runtime directory>/runtime.a -lz -lm -o program

In detail, we are asking the linker, here `clang`, to link together
four sets of object files:

 - The JPL object file `program.o`, which you presumbly got by
   compiling a JPL program and then assembling the resulting assembly;
 - The `runtime.a` file you got by compiling this runtime;
 - The ZLib compression library used for reading PNG files, which you'll need to
   install. The linker finds it in standard system-wide paths.
 - The math library (what in C is called `math.h`), which is called
   LibM.
 
Make sure to pass the linker arguments in exactly this order, because
the linker resolves references left to right. For example, if you put
`-lz` before `runtime.a`, then when the linker goes to resolve
references to Zlib functions, it won't yet have seen the ones inside
`runtime.a`.
    
Here we use `clang` only as a linker. You do not actually compile any
C code; instead `clang` recognizes that you've passed them a bunch of
object files, and invoke the correct linker with the default flags. If
you'd like, you can call `ld` directly, you'll just need to pass
explicit paths to Zlib and LibM.

# Debugging

If you set the environment variable `JPLRTDEBUG`, every public runtime
function will print its arguments, as will the entry point before
calling your `jpl_main` method. This can be helpful for debugging
issues.

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
with exit code 1. The argument to `fail_assertion` must be a pointer
to a *null terminated* sequence of characters---a C string. The string
*should not* contain a newline. Your compiled code should call
`fail_assertion` if any runtime error, like dividing by zero, occurs.

The `jpl_alloc` function has the following signature:

    void *jpl_alloc(int64_t bytes)

It wraps the standard `malloc` method and checks for errors. The
newly-allocated memory is zeroed. If `malloc` fails to allocate
memory, it cleanly exits the program, with exit code 1. If `0` or
fewer bytes are requested, also cleanly exits the program, with error
code 1. Note that its argument is a _signed_ 64-bit integer, even
though `malloc` normally takes an unsigned argument. This is for
uniformity.

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

The similar `print_time` function prints to the screen:

    void print_time(double)

The argument to `print` must be a pointer to a *null terminated*
sequence of characters---a C string. The string *should not* contain a
newline. If, for whatever reason, printing fails, these functions
terminate cleanly with error code 127.

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

# Conversion functions

The `to_int` function converts an integer to a floating-point value:

    int64_t to_int(double)
    
The `to_float` function converts back:

    double to_float(int64_t)
    
The semantics (and implementation) are identical to C-style casts.

# Platform support

On macOS, the standard calling convention requires that all function
names are prefixed with an underscore. However, on Linux, this is not
required. So, normally, a C function called `jpl_alloc` must be
refered to as `jpl_alloc` on Linux and `_jpl_alloc` on macOS, if
called from assembly.

For this reason, the runtime provides every function described above
both with and without an underscore. These do the same thing; the
version with an underscore just calls the version without.

Underscored versions of the math functions, such as `_fmod` and
`_sin`, are provided as well.

This way, when compiled on Linux, the runtime will provide both
`jpl_alloc` and `_jpl_alloc`; if compiled on macOS, it will provide
both `_jpl_alloc` and `__jpl_alloc`. In either case, calling
`_jpl_alloc` is valid.

# Reading the source

The `runtime.c` file contains all of the functions described above.

The `pngstuff.c` contains the bulk of the PNG reading and writing
code.
