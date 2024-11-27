for benchmark in sjeng leslie3d lbm astar milc namd;
do
    echo ---------------------------------------
    echo
    echo $benchmark never_predictable
    ./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py -b $benchmark --maxinsts=10000000 --cpu-type=MinorCPU --l1d_size=64kB --l1i_size=16kB --caches --l2cache --lct_bits_per_entry=1 --lvpu_hacks=never_predictable > /dev/null 2> /dev/null
    cat m5out/stats.txt | grep -e ipc -e cpi -e lvpu
    echo
    echo $benchmark Config 1
    ./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py -b $benchmark --maxinsts=10000000 --cpu-type=MinorCPU --l1d_size=64kB --l1i_size=16kB --caches --l2cache --lct_bits_per_entry=1 --num_lvpt_entries=1024 --num_lct_entries=1024 --num_cvt_entries=256 --lvpu_hacks=none > /dev/null 2> /dev/null
    cat m5out/stats.txt | grep -e ipc -e cpi -e lvpu
    echo
    echo $benchmark Config 2
    ./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py -b $benchmark --maxinsts=10000000 --cpu-type=MinorCPU --l1d_size=64kB --l1i_size=16kB --caches --l2cache --lct_bits_per_entry=1 --num_lvpt_entries=2048 --num_lct_entries=2048 --num_cvt_entries=1024 --lvpu_hacks=none > /dev/null 2> /dev/null
    cat m5out/stats.txt | grep -e ipc -e cpi -e lvpu
    echo
    echo $benchmark Config 3
    ./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py -b $benchmark --maxinsts=10000000 --cpu-type=MinorCPU --l1d_size=64kB --l1i_size=16kB --caches --l2cache --lct_bits_per_entry=2 --num_lvpt_entries=2048 --num_lct_entries=2048 --num_cvt_entries=1024 --lvpu_hacks=none > /dev/null 2> /dev/null
    cat m5out/stats.txt | grep -e ipc -e cpi -e lvpu
    echo
    echo $benchmark Config 4
    ./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py -b $benchmark --maxinsts=10000000 --cpu-type=MinorCPU --l1d_size=64kB --l1i_size=16kB --caches --l2cache --lct_bits_per_entry=1 --num_lvpt_entries=2048 --num_lct_entries=2048 --num_cvt_entries=1024 --lvpu_hacks=constant_only > /dev/null 2> /dev/null
    cat m5out/stats.txt | grep -e ipc -e cpi -e lvpu
    echo
    echo $benchmark Config 5
    ./build/ECE565-ARM/gem5.opt configs/spec/spec_se.py -b $benchmark --maxinsts=10000000 --cpu-type=MinorCPU --l1d_size=64kB --l1i_size=16kB --caches --l2cache --lct_bits_per_entry=1 --num_lvpt_entries=2048 --num_lct_entries=2048 --num_cvt_entries=1024 --lvpu_hacks=no_constants > /dev/null 2> /dev/null
    cat m5out/stats.txt | grep -e ipc -e cpi -e lvpu
    echo
done