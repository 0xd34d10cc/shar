package main

import (
	"bufio"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"log"
	"net"
)

// Packet - shar network packet
type Packet struct {
	Data []byte
}

func (packet *Packet) readFrom(reader io.Reader) error {
	var packetSize uint32

	err := binary.Read(reader, binary.LittleEndian, &packetSize)
	if err != nil {
		return err
	}

	if packetSize > 0xffffff {
		e := fmt.Sprintf("Invalid packet size %x", packetSize)
		return errors.New(e)
	}

	packet.Data = make([]byte, packetSize)

	_, err = io.ReadFull(reader, packet.Data)
	return err
}

func (packet *Packet) writeTo(writer io.Writer) error {
	packetSize := uint32(len(packet.Data))
	err := binary.Write(writer, binary.LittleEndian, &packetSize)
	if err != nil {
		return err
	}

	_, err = writer.Write(packet.Data)
	return err
}

func handleReceiverClient(conn net.Conn, packetsChannel chan Packet) error {
	reader := bufio.NewReaderSize(conn, 4096)

	defer func() {
		log.Printf("Client %v disconnected", conn.RemoteAddr())
		conn.Close()
	}()

	packet := Packet{}
	for {
		err := packet.readFrom(reader)
		if err != nil {
			log.Printf("Failed to read packet: %v", err)
			return err
		}

		packetsChannel <- packet
	}
}

func receivePackets(packetsChannel chan Packet) {
	addr := ":1337"
	listener, err := net.Listen("tcp", ":1337")
	if err != nil {
		log.Fatalf("[receiver] Listener creation failed: %v", err)
	}

	log.Printf("[receiver] Started on %v", addr)
	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Fatalf("[receiver] Accept() failed: %v", err)
		}

		log.Printf("[receiver] Receiving packets from %v", conn.RemoteAddr())
		err = handleReceiverClient(conn, packetsChannel)
		log.Printf("[receiver] Finished receiving packets from %v: %v", conn.RemoteAddr(), err)
	}
}

func runAcceptor(listeners chan net.Conn) {
	addr := ":1338"
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
		close(client.packetsChannel)
	}()

	for {
		packet := <-client.packetsChannel
		err := packet.writeTo(writer)
		if err != nil {
			finished <- client.id
			return err
		}
	}
}

func sendPackets(packetsChannel chan Packet) {
	listeners := make(chan net.Conn)
	go runAcceptor(listeners)

	id := 0
	clients := make(map[int]chan Packet)
	closedClients := make(chan int, 1)

	for {
		select {
		case conn := <-listeners:
			log.Printf("[sender] Sending packets to %v [%v]", conn.RemoteAddr(), id)
			c := make(chan Packet, 30)
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

		case packet := <-packetsChannel:
			for _, client := range clients {
				client <- packet
			}
		}
	}
}

func main() {
	packetsChannel := make(chan Packet, 30)
	go receivePackets(packetsChannel)
	sendPackets(packetsChannel)
}
