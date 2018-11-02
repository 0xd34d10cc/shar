package main

const packetsQueueSize = 30

func main() {
	packetsChannel := make(chan Packet, packetsQueueSize)

	receiver := NewReceiver(packetsChannel, ":1337")
	go receiver.Run()

	sender := NewSender(packetsChannel, ":1338")
	sender.Run()
}
