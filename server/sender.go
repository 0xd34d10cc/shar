package main

import (
	"bufio"
	"log"
	"net"
)

// StreamSender - sends packets from |packetsChannel| for clients connecting to |addr|
type StreamSender struct {
	packetsChannel chan Packet
	addr           string
}

// NewSender - create new StreamSender with given |addr| and |packetsChannel|
func NewSender(packetsChannel chan Packet, addr string) StreamSender {
	return StreamSender{
		packetsChannel,
		addr,
	}
}

// Run - run StreamSender
func (sender *StreamSender) Run() {
	listeners := make(chan net.Conn)
	go runAcceptor(listeners, sender.addr)

	id := 0
	clients := make(map[int]chan Packet)
	closedClients := make(chan int, 1)

	for {
		select {
		case conn := <-listeners:
			log.Printf("[sender] Sending packets to %v [%v]", conn.RemoteAddr(), id)
			c := make(chan Packet, packetsQueueSize)
			clients[id] = c

			client := SenderClient{
				packetsChannel: c,
				conn:           conn,
				id:             id,
			}

			go client.run(closedClients)
			id = id + 1

		case clientID := <-closedClients:
			delete(clients, clientID)

		case packet := <-sender.packetsChannel:
			for _, client := range clients {
				client <- packet
			}
		}
	}
}

func runAcceptor(listeners chan net.Conn, addr string) {
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		log.Fatalf("[sender] Listener creation failed: %v", err)
	}

	log.Printf("[sender] Started on %v", addr)
	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Fatalf("[sender] Accept() failed: %v", err)
		}

		listeners <- conn
	}
}

// SenderClient - sends packets to clients
type SenderClient struct {
	packetsChannel chan Packet
	conn           net.Conn
	id             int
}

func (client *SenderClient) run(finished chan int) error {
	writer := bufio.NewWriter(client.conn)

	defer func() {
		log.Printf("[sender] Client %v [%v] disconnected", client.conn.RemoteAddr(), client.id)
		client.conn.Close()
		finished <- client.id
		close(client.packetsChannel)
	}()

	for {
		packet := <-client.packetsChannel
		err := packet.writeTo(writer)
		if err != nil {
			return err
		}
	}
}
