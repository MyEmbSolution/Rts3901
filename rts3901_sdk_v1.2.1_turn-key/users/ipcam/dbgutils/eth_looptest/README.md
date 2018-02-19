#eth_looptest
eth_looptest is a socket-based utility to test transition error when ethernet TX and RX are short circuited.

The utility transmit pseudo random ethernet packet from TX and wait for the same packet to RX.
If all of the transmitted packet is indentical to the received packet, test passed. Else, it will printf error message and continue the test.

## Usage
./eth_looptest   [interface]     [queue_number]
[interface]
	This is the eth device name get by ifconfig command. e.g. eth0
[queue_number]
	This is the transmission buffer, larger number will cause heavy incoming and outgoing traffic. when it is 20, network traffic will be close to 94Mbps, which is almost the maximum throughput we can get.

##File list
### data_rand.h
	Randomly generated 100 data packet for the loop test.
### eth_looptest.c
	Program entry point file.
	It uses raw Socket to access Layer 2 protocol and bind ethernet interface. It manully generate ethernet header and ip header, then add the randomly generated data as the ip data. Please refer to man page raw for more details about raw socket.
