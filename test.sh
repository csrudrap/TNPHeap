# change this to where your npheap is.
sudo insmod NPHeap/npheap.ko
sudo chmod 777 /dev/npheap
sudo insmod kernel_module/tnpheap.ko
sudo chmod 777 /dev/tnpheap
./benchmark/benchmark 8192 8192 8
cat *.log > trace
sort -n -k 3 trace > sorted_trace
./benchmark/validate 8192 8192 < sorted_trace
rm -f *.log
sudo rmmod tnpheap
sudo rmmod npheap
