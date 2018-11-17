package main

import (
	"bytes"
)

const NALTypeFUA = 28
const FUPayloadSize = 1100
const NALUHeaderSize = 2
const NALUNotFound = ^uint64(0)

type FUIndicator struct {
	F    uint8 // forbidden zero bit (1 bit)
	NRI  uint8 // nal ref idc (2 bits)
	Type uint8 // NAL unit payload type (5 bits)
}

type FUHeader struct {
	S    uint8 // start bit (1 bit)
	E    uint8 // end bit (1 bit)
	R    uint8 // reserved bit (must be ignored) (1 bit)
	Type uint8 // NAL unit payload type (5 bits)
}

// FU-A packet (RFC6184 5.8)
type FUPacket struct {
	Indicator FUIndicator
	Header    FUHeader
	Payload   []byte
}

func findNextNALUnit(data []byte) uint64 {
	// Simply lookup "0x000001" pattern (NAL Unit start pattern)
	ind := bytes.Index(data, []byte{0, 0, 1})
	if ind == -1 {
		return NALUNotFound
	}

	if ind != 0 && data[ind - 1] == 0 {
		return uint64(ind - 1)
	}

	return uint64(ind)
}

func nalFragmentize(data []byte) []FUPacket {
	var packets []FUPacket

	start := findNextNALUnit(data)

	for data[start] == 0 {
		start = start + 1
	}
	start = start + 1
	end := findNextNALUnit(data[start:])
	end = end + start
	for start != NALUNotFound {
		raw_packets := nalUnitFramgentize(data[start:end])
		packets = append(packets, raw_packets...)

		start = findNextNALUnit(data[end:])
		if start != NALUNotFound {
			start = start + end
			for data[start] == 0 {
				start = start + 1
			}
			start = start + 1
			end = findNextNALUnit(data[start:])
			if end == NALUNotFound {
				end = uint64(len(data))
			} else {
				end = start + end
			}
		}
	}
	return packets
}

func nalUnitFramgentize(packet []byte) []FUPacket {
	var packets []FUPacket

	var nal_unit_type = packet[0] & 0x1F
	var NRI = (packet[0] & 0x60) >> 5
	if nal_unit_type != 1 {
		//fmt.Printf("NUT: %v\n", nal_unit_type)
	}
	//fmt.Printf("packet: %v, NUT %v", packet[0:10], packet[1]&0x1F)
	NAUSize := uint32(len(packet)) - NALUHeaderSize

	if NAUSize < FUPayloadSize {
		firstPacket := FUPacket{
			Indicator: FUIndicator{F: 0, NRI: NRI, Type: NALTypeFUA},
			Header:    FUHeader{S: 1, E: 0, R: 0, Type: nal_unit_type},
			Payload:   packet[NALUHeaderSize : NAUSize+NALUHeaderSize],
		}

		lastPacket := FUPacket{
			Indicator: FUIndicator{F: 0, NRI: NRI, Type: NALTypeFUA},
			Header:    FUHeader{S: 0, E: 1, R: 0, Type: nal_unit_type},
			Payload:   []byte{},
		}

		packets = append(packets, firstPacket)
		packets = append(packets, lastPacket)

	} else {
		byteCounter := NALUHeaderSize

		firstPacket := FUPacket{
			Indicator: FUIndicator{F: 0, NRI: NRI, Type: NALTypeFUA},
			Header:    FUHeader{S: 1, E: 0, R: 0, Type: nal_unit_type},
			Payload:   packet[NALUHeaderSize : byteCounter+FUPayloadSize],
		}
		packets = append(packets, firstPacket)
		NAUSize = NAUSize - FUPayloadSize;
		byteCounter = byteCounter + FUPayloadSize

		for NAUSize > FUPayloadSize {
			packet := FUPacket{
				Indicator: FUIndicator{F: 0, NRI: NRI, Type: NALTypeFUA},
				Header:    FUHeader{S: 0, E: 0, R: 0, Type: nal_unit_type},
				Payload:   packet[byteCounter : byteCounter+FUPayloadSize],
			}
			packets = append(packets, packet)
			NAUSize = NAUSize - FUPayloadSize;
			byteCounter = byteCounter + FUPayloadSize
		}

		lastPacket := FUPacket{
			Indicator: FUIndicator{F: 0, NRI: NRI, Type: NALTypeFUA},
			Header:    FUHeader{S: 0, E: 1, R: 0, Type: nal_unit_type},
			Payload:   packet[byteCounter:],
		}
		packets = append(packets, lastPacket)

	}

	return packets
}

func (packet FUPacket) Serialize() []byte {
	var headers [2]byte
	// indicator
	headers[0] = 0
	headers[0] |= packet.Indicator.F
	headers[0] |= packet.Indicator.NRI << 1
	headers[0] |= packet.Indicator.Type << 3

	headers[1] = 0
	headers[1] |= packet.Header.S
	headers[1] |= packet.Header.E << 1
	headers[1] |= packet.Header.R << 2
	headers[1] |= packet.Header.Type << 3

	data := append(headers[:], packet.Payload...)
	return data
}


