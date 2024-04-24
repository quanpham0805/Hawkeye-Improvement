gen() {
  config=$1
  for i in "results/result_both$config.txt both" "results/result_ehc$config.txt ehc" "results/result_hawkeye$config.txt hawkeye" "results/result_lru$config.txt lru" "results/result_perceptron$config.txt perceptron"
  do
    set -- $i
    arg1=$1
    arg2=$2
    python3 scripts/parse.py $arg1 $arg2
  done
}

echo "scheme,name,total,hit,miss,IPC" > results/result_config1.csv
gen 1 >> results/result_config1.csv
echo "scheme,name,total,hit,miss,IPC" > results/result_config2.csv
gen 2 >> results/result_config2.csv
