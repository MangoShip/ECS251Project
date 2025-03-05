if [[ $# -ne 3 ]] then
    echo Usage: $0 [NUM_THREADS] [NUM_LOOPS] [NUM_TEST_TRIALS]
    exit 1
fi

# This stress test simply runs /target/test with a desired number of threads, loops, and trials
# In each trial, we get the output of the program. We should expect to see exactly NUM_LOOPS occurences of
# the string "Tasks completed: [NUM_THREADS]". If we don't, then this script will exit and print the real
# output of the program. For the printed output to be helpful, the library file tholder/lib/libtholder.a must
# have print statements enabled

for ((i = 0; i < $3; i++)); do
    ret=$(./target/test-tholder $1 $2)
    num_tasks=$(printf "%s\n" "$ret" | egrep -c "Tasks completed: $1\$")
    if [[ $num_tasks -ne $2 ]] then
        printf "\rTRIAL $i FAILED  \n"
        echo Expected $2, got $num_tasks
        printf "%s\n" "$ret"
        exit 2
    else
        printf "\rTRIAL $i SUCCESS"
    fi
done

printf "\rALL $3 TRIALS PASSED"

