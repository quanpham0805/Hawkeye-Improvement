task() {
  pn=$1
  tn=$2
  ni=$3
  lockfile ../results/result_$pn.txt.lock
  echo $pn $tn $ni
  echo $tn >> ../results/result_$pn.txt
  ../prog/$pn -hide_heartbeat -warmup_instructions $ni -simulation_instructions $ni -traces trace/big/$tn | grep -E "LLC|Final|IPC" >> ../results/result_$pn.txt
  printf "\n\n" >> ../results/result_$pn.txt
  echo "done"
  rm -f ../results/result_$pn.txt.lock
}

for progn in "hht1" "hht2" "ph1" "ph2"; do
  rm ../results/result_$progn.txt ;
  touch ../results/result_$progn.txt ;
done

for progn in "hht1" "hht2" "ph1" "ph2"; do
  for tr in "astar_163B.trace.gz" "h264ref_178B.trace.gz" "gcc_13B.trace.gz" "sphinx3_1339B.trace.gz" "bzip2_183B.trace.gz" "GemsFDTD_109B.trace.gz" "cactusADM_1039B.trace.gz" "calculix_2655B.trace.gz"; do
    task $progn $tr 100000000 &
  done
done
