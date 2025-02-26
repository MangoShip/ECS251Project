for ((i = 1; i < 101; i++)); do
    printf "\rTRIAL $i SUCCESS "
    output=$(./tholder_main $1)
    tasks=$(printf "%s\n" "$output" | grep -c Finished)
    if [[ $tasks -ne $1 ]] then
        printf "\rTRIAL $i FAILURE\n"
        echo Expected $1, got $tasks
        echo $output
        exit 1
    fi
done
echo ALL TRIALS SUCCESS 
