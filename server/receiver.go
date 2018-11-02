package main

import (
	"bufio"
	"log"
	"net"
)

// StreamReceiver - receives length-delimited packets on |addr| and
//                  forwards them to |packetsChannel|
type StreamReceiver struct {
	packetsChannel chan Packet
	addr           string
}

// NewReceiver - Create new StreamReceiver
func NewReceiver(packetsChannel chan Packet, addr string) StreamReceiver {
	return StreamReceiver{
		packetsChannel,
		addr,
	}
}

// Run - run StreamReceiver
func (receiver *StreamReceiver) Run() {
	listener, err := net.Listen("tcp", receiver.addr)
	if err != nil {
		log.Fatalf("[receiver] Listener creation failed: %v", err)
	}

	log.Printf("[receiver] Started on %v", receiver.addr)
	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Fatalf("[receiver] Accept() failed: %v", err)
		}

		log.Printf("[receiver] Receiving packets from %v", conn.RemoteAddr())
		err = handleReceiverClient(conn, receiver.packetsChannel)
		log.Printf("[receiver] Finished receiving packets from %v: %v", conn.RemoteAddr(), err)
	}
}

func handleReceiverClient(conn net.Conn, packetsChannel chan Packet) error {
	reader := bufio.NewReaderSize(conn, 4096)

	defer func() {
		log.Printf("[receiver] Client %v disconnected", conn.RemoteAddr())
		conn.Close()
	}()

	packet := Packet{}
	for {
		err := packet.readFrom(reader)
		if err != nil {
			log.Printf("[receiver] Failed to read packet: %v", err)
			return err
		}

		packetsChannel <- packet
	}
}
