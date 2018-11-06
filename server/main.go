package main

import (
	"log"
	"net"
	"os"
	"os/signal"
	"runtime"
	"time"

	rtp "github.com/wernerd/GoRTP/src/net/rtp"
)

const packetsQueueSize = 30

func waitForCtrlC() {
	signalChannel := make(chan os.Signal, 1)
	signal.Notify(signalChannel, os.Interrupt)
	<-signalChannel
}

func main() {
	runtime.GOMAXPROCS(runtime.NumCPU())

	rtp.PayloadFormatMap[96] = &rtp.PayloadFormat{
		TypeNumber: 96,
		MediaType:  rtp.Video,
		ClockRate:  90000,
		Channels:   1,
		Name:       "H264",
	}

	start := time.Now()

	packetsChannel := make(chan Packet, packetsQueueSize)

	receiver := NewReceiver(packetsChannel, ":1337")
	go receiver.Run()

	sender := NewSender(packetsChannel, ":1338")
	rtpPackets := make(chan Packet, packetsQueueSize)
	sender.Subscribe(rtpPackets)

	go sender.Run()

	ip, _ := net.ResolveIPAddr("ip", "127.0.0.1")
	rtp, err := NewRTP(ip, 1334)
	if err != nil {
		log.Fatalf("Failed to start RTP service: %v", err)
	}

	go rtp.Run(rtpPackets)

	waitForCtrlC()
	end := time.Now()
	log.Printf("Server was running for %v", end.Sub(start))
}
