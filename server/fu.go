package main

import (
	"bytes"
)

const nalTypeFUA = 28
const fuPayloadSize = 1100
const nalUHeaderSize = 1
const nalUNotFound = ^uint64(0)

// FUIndicator - fragmentation unit indicator
type FUIndicator struct {
	F    uint8 // forbidden zero bit (1 bit)
	NRI  uint8 // nal ref idc (2 bits)
	Type uint8 // NAL unit payload type (5 bits)
}

// FUHeader - fragmentation unit header
type FUHeader struct {
	S    uint8 // start bit (1 bit)
	E    uint8 // end bit (1 bit)
	R    uint8 // reserved bit (must be ignored) (1 bit)
	Type uint8 // NAL unit payload type (5 bits)
}

// FUPacket - FU-A packet (RFC6184 5.8)
type FUPacket struct {
	Indicator FUIndicator
	Header    FUHeader
	Payload   []byte
}

func findNextNALUnit(data []byte) uint64 {
	// Simply lookup "0x000001" pattern (NAL Unit start pattern)
	ind := bytes.Index(data, []byte{0, 0, 1})
	if ind == -1 {
		return nalUNotFound
	}

	if ind != 0 && data[ind-1] == 0 {
		return uint64(ind - 1)
	}

	return uint64(ind)
}

// fragment NAL (aggregation unit) into FU-A packets
func nalFragmentize(data []byte) []FUPacket {
	var packets []FUPacket
	nalUnits := 0

	start := findNextNALUnit(data)

	for data[start] == 0 {
		start = start + 1
	}
	start = start + 1
	end := findNextNALUnit(data[start:])
	end = end + start

	for start != nalUNotFound {
		rawPackets := nalUnitFramgentize(data[start:end])
		packets = append(packets, rawPackets...)

		start = findNextNALUnit(data[end:])
		if start != nalUNotFound {
			start = start + end
			for data[start] == 0 {
				start = start + 1
			}
			start = start + 1
			end = findNextNALUnit(data[start:])

			if end == nalUNotFound {
				end = uint64(len(data))
			} else {
				end = start + end
			}
		}

		nalUnits = nalUnits + 1
	}

	// fmt.Printf("NalUnits: %v Packets: %v\n", nalUnits, len(packets))
	return packets
}

// fragment single NAL unit into FU-A packets
func nalUnitFramgentize(packet []byte) []FUPacket {
	var packets []FUPacket

	i := 0

	nalUnitType := packet[i] & 0x1F
	NRI := packet[i] & 0x60
	if nalUnitType != 1 {
		// fmt.Printf("NAL unit type: %v\n", nalUnitType)
	}
	//fmt.Printf("packet: %v, NUT %v", packet[0:10], packet[1]&0x1F)
	NAUSize := uint32(len(packet)) - nalUHeaderSize
	byteCounter := nalUHeaderSize

	rightBound := byteCounter + fuPayloadSize
	size := fuPayloadSize
	if len(packet) < rightBound {
		rightBound = len(packet)
		size = len(packet) - nalUHeaderSize
	}

	firstPacket := FUPacket{
		Indicator: FUIndicator{F: 0, NRI: NRI, Type: nalTypeFUA},
		Header:    FUHeader{S: 1, E: 0, R: 0, Type: nalUnitType},
		Payload:   packet[byteCounter:rightBound],
	}
	packets = append(packets, firstPacket)
	NAUSize = NAUSize - uint32(size)
	byteCounter = rightBound

	for NAUSize > fuPayloadSize {
		packet := FUPacket{
			Indicator: FUIndicator{F: 0, NRI: NRI, Type: nalTypeFUA},
			Header:    FUHeader{S: 0, E: 0, R: 0, Type: nalUnitType},
			Payload:   packet[byteCounter : byteCounter+fuPayloadSize],
		}
		packets = append(packets, packet)
		NAUSize = NAUSize - fuPayloadSize
		byteCounter = byteCounter + fuPayloadSize
	}

	lastPacket := FUPacket{
		Indicator: FUIndicator{F: 0, NRI: NRI, Type: nalTypeFUA},
		Header:    FUHeader{S: 0, E: 1, R: 0, Type: nalUnitType},
		Payload:   packet[byteCounter:],
	}
	packets = append(packets, lastPacket)

	return packets
}

// Serialize - convert packet to wire format
func (packet FUPacket) Serialize() []byte {
	var headers [2]byte
	// indicator
	headers[0] = 0
	headers[0] |= packet.Indicator.F << 7
	headers[0] |= packet.Indicator.NRI
	headers[0] |= packet.Indicator.Type

	headers[1] = 0
	headers[1] |= packet.Header.S << 7
	headers[1] |= packet.Header.E << 6
	headers[1] |= packet.Header.R << 5
	headers[1] |= packet.Header.Type

	data := append(headers[:], packet.Payload...)
	return data
}
