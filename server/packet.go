package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"io"
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
