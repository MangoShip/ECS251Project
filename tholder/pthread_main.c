#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int bruh_int = ATOMIC_VAR_INIT(0);

int bruh(void)
{
    int hi = atomic_fetch_add(&bruh_int, 1);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s [NUM THREADS]\n", argv[0]);
        exit(1);
    }

    size_t num_threads;

    sscanf(argv[1], "%lu", &num_threads);

    pthread_t dummy[num_threads];

    // Some parallel work
    for (size_t i = 0; i < num_threads; i++)
    {
        pthread_create(&dummy[i], NULL, (void *)bruh, NULL);
        pthread_detach(dummy[i]);
    }
    
    printf("Finished launching threads. Press ENTER to end program");
    getchar();    
    int tasks_completed = atomic_load(&bruh_int);
    printf("%d tasks finished\n", tasks_completed);
    return tasks_completed;
}
