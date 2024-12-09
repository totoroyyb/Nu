#!/bin/bash

### Usage
# ./run.sh <LOG_PREFIX> <DEBUGGER_ATTACH> <ENABLE_BKPTS>
#
# <LOG_PREFIX> is used for log files to distinguish between experiments
#
# <DEBUGGER_ATTACH> == 1 means it executes with gdb attached
# <DEBUGGER_ATTACH> == 2 means it executes with ddb attached
# <DEBUGGER_ATTACH> with other values means it executes without any debugger attached
#
# <ENABLE_BKPTS> == 1 means it inserts breakpoints
# <ENABLE_BKPTS> with other values means it executes normally.

source ../../shared.sh

# DISK_DEV=/dev/nvme1n1
DISK_DEV=/dev/sdb # compatible to c6525-25g CloudLab

SKIP_BUILD=0

LOG_PREFIX=$1

LPID=1
CTL_IDX=11
NGINX_SRV_IDX=12 # NGINX should be running on a server which doesn't have iokerneld as iokernel will grab the NIC
NGINX_SRV_CALADAN_IP_AND_MASK=18.18.1.254/24
CLT_START_IDX=8
CLT_END_IDX=10 #inclusive

DIR=$(pwd)
SOCIAL_NET_DIR=$DIR/../../../app/socialNetwork/single_proclet/

MOPS=(1.3 1.6 1.9 2.2 2.5 2.8 3.1 3.4 3.7 4 4.3 4.6 4.9 5.2 5.5 5.8 6.1 6.4 6.7 7 7.3 7.6 7.9 8.2 8.5 8.8 9.1 9.4 9.7 10)

cp client.cpp $SOCIAL_NET_DIR/bench
cd $SOCIAL_NET_DIR
if [[ "$SKIP_BUILD" == "0" ]]; then
  ./build.sh
fi

run_cmd $NGINX_SRV_IDX "sudo apt-get update; sudo apt-get install -y python3-pip; pip3 install aiohttp"
run_cmd $NGINX_SRV_IDX "sudo service docker stop;
                        echo N | sudo mkfs.ext4 $DISK_DEV;
                        sudo umount /mnt;
                        sudo mount $DISK_DEV /mnt;
                        sudo mkdir /mnt/docker;
                        sudo mount --rbind /mnt/docker /var/lib/docker;
                        sudo rm -rf /var/lib/docker/*;
                        sudo service docker start;"
run_cmd $NGINX_SRV_IDX "cd $SOCIAL_NET_DIR; ./install_docker.sh"
run_cmd $NGINX_SRV_IDX "cd $SOCIAL_NET_DIR; ./down_nginx.sh; ./up_nginx.sh"
run_cmd $NGINX_SRV_IDX "sudo ip addr add $NGINX_SRV_CALADAN_IP_AND_MASK dev $nic_dev"

DDB_DIR=$DIR/../../../../../ddb
DDB_CONF=$DDB_DIR/configs

# num_srvs=7
for num_srvs in $(
  seq 1 7
); do
  mops=${MOPS[$(expr $num_srvs - 1)]}

  if [[ "$SKIP_BUILD" == "0" ]]; then
    cd $SOCIAL_NET_DIR
    sed "s/constexpr uint32_t kNumEntries.*/constexpr uint32_t kNumEntries = $num_srvs;/g" -i src/main.cpp
    sed "s/constexpr static uint32_t kNumEntries.*/constexpr static uint32_t kNumEntries = $num_srvs;/g" \
      -i bench/client.cpp
    sed "s/constexpr static double kTargetMops.*/constexpr static double kTargetMops = $mops;/g" \
      -i bench/client.cpp
    cd build
    make clean
    make -j
    cd ..
  fi

  for srv_idx in $(seq 1 $num_srvs); do
    run_cmd $srv_idx "mkdir -p $(pwd)/build/src"
    distribute build/src/main $srv_idx
  done

  for clt_idx in $(seq $CLT_START_IDX $CLT_END_IDX); do
    run_cmd $clt_idx "mkdir -p $(pwd)/build/bench"
    distribute build/bench/client $clt_idx
  done

  start_iokerneld $CTL_IDX
  for srv_idx in $(seq 1 $num_srvs); do
    start_iokerneld $srv_idx
  done
  for clt_idx in $(seq $CLT_START_IDX $CLT_END_IDX); do
    start_iokerneld $clt_idx
  done
  sleep 5

  EMPTY_PIPE=$DIR/empty_pipe
  DDB_JOB=0
  if [[ $2 -eq 2 ]]; then
    rm -f $EMPTY_PIPE
    mkfifo $EMPTY_PIPE
    pushd $DDB_DIR
    uv sync
    # cat $EMPTY_PIPE | uv run -- ddb $DDB_CONF/dbg_nu_c6525_exp.yaml >$DIR/logs/.ddb.debugger.tmp 2>&1 &
    cat $EMPTY_PIPE | uv run -- ddb $DDB_CONF/dbg_nu_c6525_exp.yaml >/dev/null 2>&1 &
    DDB_JOB=$!
    echo "Waiting DDB to be ready..."
    sleep 3
    popd
  fi

  start_ctrl $CTL_IDX >$DIR/logs/ctrl.$CTL_IDX.tmp 2>&1
  sleep 5

  for srv_idx in $(seq 1 $num_srvs); do
    if [[ $srv_idx -ne $num_srvs ]]; then
      if [[ $2 -eq 1 ]]; then
        if [[ $3 -eq 1 ]]; then
          start_server_with_gdb_bkpts build/src/main $srv_idx $LPID >$DIR/logs/.bkpts.gdb.$srv_idx.tmp 2>&1 &
        else
          start_server_with_gdb build/src/main $srv_idx $LPID >$DIR/logs/.gdb.$srv_idx.tmp 2>&1 &
        fi
      elif [[ $2 -eq 2 ]]; then
        start_server_with_ddb build/src/main $srv_idx $LPID >$DIR/logs/.ddb.$srv_idx.tmp 2>&1 &
      else
        start_server build/src/main $srv_idx $LPID >$DIR/logs/.$srv_idx.tmp 2>&1 &
      fi
    else
      sleep 5
      if [[ $2 -eq 1 ]]; then
        if [[ $3 -eq 1 ]]; then
          start_server_with_gdb_bkpts build/src/main $srv_idx $LPID >$DIR/logs/.bkpts.gdb.$srv_idx.tmp 2>&1 &
        else
          start_main_server_with_gdb build/src/main $srv_idx $LPID >$DIR/logs/.gdb.tmp 2>&1 &
        fi
      elif [[ $2 -eq 2 ]]; then
        start_main_server_with_ddb build/src/main $srv_idx $LPID >$DIR/logs/.ddb.tmp 2>&1 &
      else
        start_main_server build/src/main $srv_idx $LPID >$DIR/logs/.tmp 2>&1 &
      fi
    fi
  done

  if [[ $2 -eq 1 ]]; then
    (tail -f -n0 $DIR/logs/.gdb.tmp &) | grep -q "Starting the ThriftBackEndServer"
  elif [[ $2 -eq 2 ]]; then
    (tail -f -n0 $DIR/logs/.ddb.tmp &) | grep -q "Starting the ThriftBackEndServer"
  else
    (tail -f -n0 $DIR/logs/.tmp &) | grep -q "Starting the ThriftBackEndServer"
  fi

  sleep 5

  echo "Init Social Graph"
  run_cmd $NGINX_SRV_IDX "cd $SOCIAL_NET_DIR; python3 scripts/init_social_graph.py"
  echo "Finish init Social Graph"
  sleep 5

  # while true; do
  #   sleep 5
  # done

  client_pids=
  for clt_idx in $(seq $CLT_START_IDX $CLT_END_IDX); do
    conf=$DIR/conf/client$(expr $clt_idx - $CLT_START_IDX + 1)

    if [[ $2 -eq 1 ]]; then
      run_program build/bench/client $clt_idx $conf 1>$DIR/logs/gdb.$LOG_PREFIX.$num_srvs.$clt_idx 2>&1 &
    elif [[ $2 -eq 2 ]]; then
      run_program build/bench/client $clt_idx $conf 1>$DIR/logs/ddb.$LOG_PREFIX.$num_srvs.$clt_idx 2>&1 &
    else
      run_program build/bench/client $clt_idx $conf 1>$DIR/logs/$LOG_PREFIX.$num_srvs.$clt_idx 2>&1 &
    fi
    client_pids+=" $!"
  done
  wait $client_pids

  if [[ $2 -eq 2 ]]; then
    kill -9 $DDB_JOB
    rm -f $EMPTY_PIPE
  fi
  cleanup
  sleep 5
done

run_cmd $NGINX_SRV_IDX "cd $SOCIAL_NET_DIR; ./down_nginx.sh;"
run_cmd $NGINX_SRV_IDX "sudo docker rm -vf $(sudo docker ps -aq)"
run_cmd $NGINX_SRV_IDX "sudo docker volume prune -f"
run_cmd $NGINX_SRV_IDX "sudo ip addr delete $NGINX_SRV_CALADAN_IP_AND_MASK dev $nic_dev"
