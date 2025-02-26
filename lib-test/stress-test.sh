if [[ $# -ne 3 ]] then
    echo Usage: $0 [NUM_THREADS] [NUM_LOOPS] [NUM_TEST_TRIALS]
    exit 1
fi

for ((i = 0; i < $3; i++)); do
    ret=$(target/test $1 $2 | grep -c $1)
    if [[ $ret -ne $2 ]] then
        printf "\rTRIAL $i FAILED  \n"
        echo $ret
        exit 2
    else
        printf "\rTRIAL $i SUCCESS"
    fi
done

printf "\rALL $3 TRIALS PASSED"

