package main

import (
	"bufio"
	"log"
	"net"
)

// StreamSender - sends packets from |packetsChannel| for clients connecting to |addr|
type StreamSender struct {
	packetsChannel chan Packet // input channel
	addr           string
	clients        map[int]chan Packet
	id             int

	// clients should send clientID in this channel
	// if they don't need to receive packets anymore
	finished chan int
}

// NewSender - create new StreamSender with given |addr| and |packetsChannel|
func NewSender(packetsChannel chan Packet, addr string) StreamSender {
	return StreamSender{
		packetsChannel: packetsChannel,
		addr:           addr,
		clients:        make(map[int]chan Packet),
		id:             0,
		finished:       make(chan int, 1),
	}
}

// Subscribe - subscribe for packets
func (sender *StreamSender) Subscribe(consumer chan Packet) (int, chan int) {
	id := sender.id
	sender.clients[id] = consumer
	sender.id = sender.id + 1

	return id, sender.finished
}

// Run - run StreamSender
func (sender *StreamSender) Run() {
	listeners := make(chan net.Conn)
	go runAcceptor(listeners, sender.addr)

	for {
		select {
		// new client connected
		case conn := <-listeners:
			c := make(chan Packet, packetsQueueSize)
			id, finished := sender.Subscribe(c)
			client := SenderClient{
				packetsChannel: c,
				conn:           conn,
				id:             id,
			}

			log.Printf("[sender] Sending packets to %v [%v]", conn.RemoteAddr(), id)
			go client.run(finished)

		// some client has unsubscribed from packets
		case clientID := <-sender.finished:
			delete(sender.clients, clientID)

		// broadcast packet for all clients
		case packet := <-sender.packetsChannel:
			for _, client := range sender.clients {
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
