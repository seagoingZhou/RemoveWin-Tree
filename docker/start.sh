sudo docker run -itd --mount type=bind,source=/home/zhou/Github/RemoveWin-Tree,target=/Redis/RWTree --net shadownet --ip 192.168.192.1 --name redis0 --privileged zhouzh71106/full_ubuntu:v2 /bin/bash
sudo docker run -itd --mount type=bind,source=/home/zhou/Github/RemoveWin-Tree,target=/Redis/RWTree --net shadownet --ip 192.168.192.2 --name redis1 --privileged zhouzh71106/full_ubuntu:v2 /bin/bash
sudo docker run -itd --mount type=bind,source=/home/zhou/Github/RemoveWin-Tree,target=/Redis/RWTree --net shadownet --ip 192.168.192.3 --name redis2 --privileged zhouzh71106/full_ubuntu:v2 /bin/bash
