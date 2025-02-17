for ((i = 0; i < 100; i++)); do
  ./tholder_main $1 $2 > temp.log
  ret=$(cat temp.log | grep -c "Yo what up")
  if [[ ret -ne $1 ]] then
    echo FAILURE ON TRIAL $i! Check temp.log
    exit 1
  else
    echo SUCCESS ON TRIAL $i
  fi
done


