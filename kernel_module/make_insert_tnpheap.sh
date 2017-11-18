sudo rmmod tnpheap
make
sudo make install
sudo insmod tnpheap.ko
sudo chmod 777 /dev/tnpheap
sudo dmesg -c

