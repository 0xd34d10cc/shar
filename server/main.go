package main

import (
	"log"
	"net"
	"os"
	"os/signal"
	"runtime"
	"sync"
	"time"
)

const packetsQueueSize = 30

func waitForCtrlC() {
	var endWaiter sync.WaitGroup
	endWaiter.Add(1)
	var signalChannel chan os.Signal
	signalChannel = make(chan os.Signal, 1)
	signal.Notify(signalChannel, os.Interrupt)
	go func() {
		<-signalChannel
		endWaiter.Done()
	}()
	endWaiter.Wait()
}

func main() {
	runtime.GOMAXPROCS(runtime.NumCPU())

	start := time.Now()

	packetsChannel := make(chan Packet, packetsQueueSize)

	receiver := NewReceiver(packetsChannel, ":1337")
	go receiver.Run()

	sender := NewSender(packetsChannel, ":1338")
	rtpPackets := make(chan Packet, packetsQueueSize)
	sender.Subscribe(rtpPackets)

	go sender.Run()

	ip, _ := net.ResolveIPAddr("ip", "127.0.0.1")
	rtp, err := NewRTP(ip, 1335)
	if err != nil {
		log.Fatalf("Failed to start RTP service: %v", err)
	}

	go rtp.Run(rtpPackets)

	waitForCtrlC()
	end := time.Now()
	log.Printf("Server was running for %v", end.Sub(start))
}
