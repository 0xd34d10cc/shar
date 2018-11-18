package main

import (
	"fmt"
	"reflect"
	"testing"
)

func assertEqual(expected interface{}, actual interface{}) {
	if !reflect.DeepEqual(expected, actual) {
		panic(fmt.Sprintf("Assert failed!\nexpected: %v\nactual:   %v", expected, actual))
	}
}

func TestNalFragmentize(t *testing.T) {
	nalUnit := []byte{
		0x00, 0x00, 0x01, 0x09, 0x10,
		0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32,
		0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
		0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x80, 0x1a} // Some "real" NAL unit

	fragmentizedNals := nalFragmentize(nalUnit)

	// we have 4 "000001" patterns, each NAL unit with size < FUPayloadSize will split into first packet and last packet
	assertEqual(8, len(fragmentizedNals))

	assertEqual(FUPacket{
		Indicator: FUIndicator{F: 0, NRI: 0x9 & 0x60, Type: 28},
		Header:    FUHeader{S: 1, E: 0, R: 0, Type: 0x9 & 0x1f},
		Payload:   []byte{0x10},
	}, fragmentizedNals[0])

	assertEqual(FUPacket{
		Indicator: FUIndicator{F: 0, NRI: 0x9 & 0x60, Type: 28},
		Header:    FUHeader{S: 0, E: 1, R: 0, Type: 0x9 & 0x1f},
		Payload:   []byte{},
	}, fragmentizedNals[1])

	assertEqual(FUPacket{
		Indicator: FUIndicator{F: 0, NRI: 0x67 & 0x60, Type: 28},
		Header:    FUHeader{S: 1, E: 0, R: 0, Type: 0x67 & 0x1f},
		Payload:   []byte{0x42, 0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32},
	}, fragmentizedNals[2])

	assertEqual(FUPacket{
		Indicator: FUIndicator{F: 0, NRI: 0x67 & 0x60, Type: 28},
		Header:    FUHeader{S: 0, E: 1, R: 0, Type: 0x67 & 0x1f},
		Payload:   []byte{},
	}, fragmentizedNals[3])

	assertEqual(FUPacket{
		Indicator: FUIndicator{F: 0, NRI: 0x68 & 0x60, Type: 28},
		Header:    FUHeader{S: 1, E: 0, R: 0, Type: 0x68 & 0x1f},
		Payload:   []byte{0xce, 0x3c, 0x80},
	}, fragmentizedNals[4])

	assertEqual(FUPacket{
		Indicator: FUIndicator{F: 0, NRI: 0x68 & 0x60, Type: 28},
		Header:    FUHeader{S: 0, E: 1, R: 0, Type: 0x68 & 0x1f},
		Payload:   []byte{},
	}, fragmentizedNals[5])
}

func TestFUSerialization(t *testing.T) {
	packet := FUPacket{
		Indicator: FUIndicator{F: 0, NRI: 0x67 & 0x60, Type: 28},
		Header:    FUHeader{S: 1, E: 0, R: 0, Type: 0x67 & 0x1f},
		Payload:   []byte{0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32},
	}

	data := packet.Serialize()

	assertEqual(data, []byte{
		(0x67 & 0x60) | 28,                       // indicator
		(1 << 7) | (0x67 & 0x1f),                 // header
		0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32, // payload
	})
}
