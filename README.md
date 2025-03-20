# ECS251Project

### Building the library

Library is located in `tholder/`. It has its own Makefile, which will build a static library at `tholder/lib/libtholder.a`.

This library can then be linked with the `-I<path-to-tholder/>`, `-L<path-to-tholder/lib>`, and `-ltholder` compiler/linker flags.

### Building the executables

Each program directory has its own Makefile as well. The `-ltholder` linker flag has been added, among others (`-lm`, `-fopenmp`) depending on the project directory.

To build an executable, simply type `make`. All target executables will be located in `./target/`, while object files will be placed in `./obj/`.

### Tholder Features/API

The following describes the functionality of each of the functions defined in `tholder.h`. Any debug info is ignored here.

- `tholder_create(tholder_t *__newthread, ..., void *(*__start_routine)(void *), ...);` - Gets the first inactive index in the thread pool that's ready to do work (via `get_inactive_index()`). If no thread is alive in this slot, it will use `pthread_create` to spawn one and run `auxiliary_function()`. `pthread_detatch` is used so the thread can exit on its own without blocking. In order to let the user block until the task is completed, a `task_output` struct is created on the heap, and its pointer is cast to `tholder_t` and written to `__newthread`.

- `tholder_join(tholder_t th, void **thread_return);` - This function casts `th` to a pointer, which is where the given`task_output` struct lives. This function will then block on a condition variable located in the struct, which is pinged only once the task is completed by `auxiliary_function`. It also cleans up the `task_output` struct once finished. 

- `tholder_init(size_t num_threads);` - A helper function that simply creates the global thread pool with a specified isze. This function is implicitly called by `tholder_create(8)` if the thread pool has not been initialized yet. 

- `tholder_destroy();` - Cleans up the thread pool allocated by `tholder_init`.

- `auxiliary_function(void *args);` - This function sleeps on a timed condition variable for `DURATION` time. Each time it wakes up, it will check if there is new work in its assigned `thread_data` struct. If so, it will execute the task. If not, it will break the loop and exit. This behavior allows the thread to be "reused" and exit if waiting for too long.

- `get_inactive_index();` - Finds first index that is either: 
    - `NULL`, which signifies that this thread slot is uninitialized and ready to be spawned
    - Idle but alive, in which case this thread is ready to be reused for a task
    Then it creates the `thread_data` struct 

- `task_output_init();` - Allocates the memory for a task. This is used by the `auxiliary_function` to write output data to, but it is uniquely tied to the task, NOT the thread itself. 
