# Library Test Script

This stress test simply runs /target/test with a desired number of threads, loops, and trials
In each trial, we get the output of the program. We should expect to see exactly NUM_LOOPS occurences of
the string `Tasks completed: [NUM_THREADS]`. If we don't, then this script will exit and print the real
output of the program. For the printed output to be helpful, the library file tholder/lib/libtholder.a must
have print statements enabled
