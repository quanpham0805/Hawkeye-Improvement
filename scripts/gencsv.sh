gen() {
  config=$1
  for i in "result_hht$config.txt ehc" "result_lru-config$config.txt hawkeye" "result_perceptron$config.txt perceptron" "result_ph$config.txt both"
  do
    set -- $i
    arg1=$1
    arg2=$2
    python3 parse.py $arg1 $arg2
  done
}

echo "scheme,name,total,hit,miss,IPC" > result_config1.csv
gen 1 >> result_config1.csv
echo "scheme,name,total,hit,miss,IPC" > result_config2.csv
gen 2 >> result_config2.csv
