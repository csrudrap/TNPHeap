make
sudo make install
sudo rmmod npheap
make
sudo make install
sudo insmod ../NPHeap/npheap.ko
sudo chmod 777 /dev/npheap

