tc qdisc del dev eth0 root
tc qdisc del dev lo root

tc qdisc add dev eth0 root handle 1: prio
tc qdisc add dev eth0 parent 1:1 handle 10: netem delay $3 $4 distribution normal limit 100000
tc filter add dev eth0 protocol ip parent 1: prio 1 u32 match ip dst $1 flowid 1:1

tc qdisc add dev eth0 parent 1:2 handle 20: netem delay $3 $4 distribution normal limit 100000
tc filter add dev eth0 protocol ip parent 1: prio 2 u32 match ip dst $2 flowid 1:2

tc qdisc add dev eth0 parent 1:3 handle 30: pfifo limit 10000
tc filter add dev eth0 protocol ip parent 1: prio 3 u32 match ip dst 0.0.0.0/0 flowid 1:3

tc qdisc add dev lo root netem delay $5 $6 distribution normal limit 100000
