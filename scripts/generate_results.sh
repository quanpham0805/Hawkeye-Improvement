task() {
  pn=$1
  tn=$2
  ni=$3
  dir=$4
  lockfile results/result_$pn.txt.lock
  echo $pn $tn $ni
  echo $tn >> results/result_$pn.txt
  prog/$pn -hide_heartbeat -warmup_instructions $ni -simulation_instructions $ni -traces trace/$dir/$tn | grep -E "LLC|Final|IPC" >> results/result_$pn.txt
  printf "\n\n" >> results/result_$pn.txt
  echo "done"
  rm -f results/result_$pn.txt.lock
}

dir=$1
nins=$2
progs=$(ls prog/)
traces=$(ls trace/$1/)

for progn in $progs; do
  rm results/result_$progn.txt ;
  touch results/result_$progn.txt ;
done

for progn in $progs; do
  for tr in $traces; do
    task $progn $tr $nins $dir &
  done
done
