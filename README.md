# ECS251Project

### Building the library

Library is located in `tholder/`. It has its own Makefile, which will build a static library at `tholder/lib/libtholder.a`.

This library can then be linked with the `-I<path-to-tholder/>`, `-L<path-to-tholder/lib>`, and `-ltholder` compiler/linker flags.

### Building the executables

Each program directory has its own Makefile as well. The `-ltholder` linker flag has been added, among others (`-lm`, `-fopenmp`) depending on the project directory.

To build an executable, simply type `make`. All target executables will be located in `./target/`, while object files will be placed in `./obj/`.
