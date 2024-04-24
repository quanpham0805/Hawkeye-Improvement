gen() {
  config=$1
  for i in "../results/result_hht$config.txt ehc" "../results/result_lru-config$config.txt hawkeye" "../results/result_perceptron$config.txt perceptron" "../results/result_ph$config.txt both"
  do
    set -- $i
    arg1=$1
    arg2=$2
    python3 parse.py $arg1 $arg2
  done
}

echo "scheme,name,total,hit,miss,IPC" > ../results/result_config1.csv
gen 1 >> ../results/result_config1.csv
echo "scheme,name,total,hit,miss,IPC" > ../results/result_config2.csv
gen 2 >> ../results/result_config2.csv
