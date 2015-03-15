# Introduction #

There are three types of C++/C packages:
  * source code
  * development libraries with .a and .obj files compiled to a particular platform with a particular compiler
  * binary only with .dll files

## Source code ##

Please use the suffix `-source` for source code libraries.
Example: quazip-source

## Development libraries ##

A development library in C++ normally contain `.h` files in the `include` directory and `.a` and `.dll` files in the `bin` directory. The suffix `-dev` should also be used for complete development environments for programs to simplify initial setup including all the dependent libraries and necessary tools (compilers, profilers, etc.). The development libraries should form a tree of dependencies with other development libraries.

The created binaries are compiler and platform dependent. Please use the `-dev-<platform>-<compiler>-<build options>` suffix for the name of the development library.

Please use the values from the following lists to create such suffixes (in this order):

Platforms:
  * x86\_64 - 64 bit binaries in MinGW-w64
  * i686 - 32 bit binaries in MinGW or MinGW-w64 (default, can be omitted)
  * win32 - 32 bit binaries in Visual C++ (default, can be omitted)
  * amd64 - 64 bit binaries in Visual C++
  * ia32 - 32 bit binaries in Intel Composer (default, can be omitted)
  * intel64 - 64 bit binaries in Intel Composer
  * other suffixes as defined by different compilers

Although this should not change anything for most of the end-users CPUs, it is important to understand that i686 may not be the same as win32 or ia32. Each compiler supports its own set of CPU architectures and may not fully support everything supported by a specific Windows version.

Compilers:
  * mingw32 - MinGW
  * w64 - MinGW-w64
  * vc6 - Visual Studio 98 (VC6)
  * vc7 - Visual Studio 2002
  * vc7\_1 - Visual Studio 2003
  * vc8 - Visual Studio 2005
  * vc9 - Visual Studio 2008
  * vc10- Visual Studio 2010
  * vc11- Visual Studio 2012
  * vc12- Visual Studio 2013
  * sdk6 - Windows SDK 6.0 C++ compiler
  * sdk7 - Windows SDK 7.0 C++ compiler
  * sdk7\_1 - Windows SDK 7.1 C++ compiler

Compiler specific suffixes:
  * Visual C++ specific: mt - causes your application to use the multi-threaded and DLL-specific version of the run-time library
  * Visual C++ specific: md - causes your application to use the multi-threaded, static version of the run-time library
  * Visual C++ specific: see http://msdn.microsoft.com/en-us/library/2kzt1wy3(v=vs.110).aspx

Different set of compiler or linker options:
  * release - release build (default, can be omitted)
  * debug - debug build
  * other user defined suffixes

Static or dynamic library loading:
  * static - static build
  * dynamic - dynamic loading libraries (default, can be omitted)

Example: xapian-dev
Example 2: qt-dev-i686-w64-debug-static

## Binaries ##

The packages with binaries (only .dll files, no .o or .lib) should be named simply as the library. It is also possible to use the simple suffix `64` for 64-bit binaries as in program packages.

Example: qt-i686-debug
Example 2: qt64
