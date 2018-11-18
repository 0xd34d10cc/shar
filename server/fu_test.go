package main

import (
	"fmt"
	"reflect"
	"testing"
)

func assertEqual(expected interface{}, actual interface{}, t *testing.T) {
	if !reflect.DeepEqual(expected, actual) {
		panic(fmt.Sprintf("Assert! expected: %v, actual: %v", expected, actual))
	}
}

func TestNalFragmentize(t *testing.T) {
	nalUnit := []byte{
		0x00, 0x00, 0x01, 0x09, 0x10,
		0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32,
		0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80, 0x00,
		0x00, 0x00, 0x01, 0x65, 0x88, 0x80, 0x1a} // Some "real" NAL unit

	fragmentizedNals := nalFragmentize(nalUnit)

	// we have 4 "000001" patterns, each NAL unit with size < FUPayloadSize will split into first packet and last packet
	assertEqual(8, len(fragmentizedNals), t)

	assertEqual(uint8(0x09&0x1F), fragmentizedNals[0].Header.Type, t)
	assertEqual(uint8(1), fragmentizedNals[0].Header.S, t)
	assertEqual(uint8(0), fragmentizedNals[0].Header.E, t)
	assertEqual(uint8(0), fragmentizedNals[0].Header.R, t)
	assertEqual(uint8(0), fragmentizedNals[0].Indicator.F, t)
	assertEqual(uint8(28), fragmentizedNals[0].Indicator.Type, t)
	assertEqual(uint8((0x09&0x60)>>5), fragmentizedNals[0].Indicator.NRI, t)
	assertEqual([]byte{}, fragmentizedNals[0].Payload, t)

	assertEqual(uint8(0x09&0x1F), fragmentizedNals[1].Header.Type, t)
	assertEqual(uint8(0), fragmentizedNals[1].Header.S, t)
	assertEqual(uint8(1), fragmentizedNals[1].Header.E, t)
	assertEqual(uint8(0), fragmentizedNals[1].Header.R, t)
	assertEqual(uint8(0), fragmentizedNals[1].Indicator.F, t)
	assertEqual(uint8(28), fragmentizedNals[1].Indicator.Type, t)
	assertEqual(uint8((0x09&0x60)>>5), fragmentizedNals[1].Indicator.NRI, t)
	assertEqual([]byte{}, fragmentizedNals[1].Payload, t)

	assertEqual(uint8(0x67&0x1F), fragmentizedNals[2].Header.Type, t)
	assertEqual(uint8(1), fragmentizedNals[2].Header.S, t)
	assertEqual(uint8(0), fragmentizedNals[2].Header.E, t)
	assertEqual(uint8(0), fragmentizedNals[2].Header.R, t)
	assertEqual(uint8(0), fragmentizedNals[2].Indicator.F, t)
	assertEqual(uint8(28), fragmentizedNals[2].Indicator.Type, t)
	assertEqual(uint8((0x67&0x60)>>5), fragmentizedNals[2].Indicator.NRI, t)
	assertEqual([]byte{0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32}, fragmentizedNals[2].Payload, t)

	assertEqual(uint8(0x67&0x1F), fragmentizedNals[3].Header.Type, t)
	assertEqual(uint8(0), fragmentizedNals[3].Header.S, t)
	assertEqual(uint8(1), fragmentizedNals[3].Header.E, t)
	assertEqual(uint8(0), fragmentizedNals[3].Header.R, t)
	assertEqual(uint8(0), fragmentizedNals[3].Indicator.F, t)
	assertEqual(uint8(28), fragmentizedNals[3].Indicator.Type, t)
	assertEqual(uint8((0x67&0x60)>>5), fragmentizedNals[3].Indicator.NRI, t)
	assertEqual([]byte{}, fragmentizedNals[3].Payload, t)

	assertEqual(uint8(0x68&0x1F), fragmentizedNals[4].Header.Type, t)
	assertEqual(uint8(1), fragmentizedNals[4].Header.S, t)
	assertEqual(uint8(0), fragmentizedNals[4].Header.E, t)
	assertEqual(uint8(0), fragmentizedNals[4].Header.R, t)
	assertEqual(uint8(0), fragmentizedNals[4].Indicator.F, t)
	assertEqual(uint8(28), fragmentizedNals[4].Indicator.Type, t)
	assertEqual(uint8((0x68&0x60)>>5), fragmentizedNals[4].Indicator.NRI, t)
	assertEqual([]byte{0x3c, 0x80}, fragmentizedNals[4].Payload, t) // skip it after findNextNALUnit

	assertEqual(uint8(0x68&0x1F), fragmentizedNals[5].Header.Type, t)
	assertEqual(uint8(0), fragmentizedNals[5].Header.S, t)
	assertEqual(uint8(1), fragmentizedNals[5].Header.E, t)
	assertEqual(uint8(0), fragmentizedNals[5].Header.R, t)
	assertEqual(uint8(0), fragmentizedNals[5].Indicator.F, t)
	assertEqual(uint8(28), fragmentizedNals[5].Indicator.Type, t)
	assertEqual(uint8((0x68&0x60)>>5), fragmentizedNals[5].Indicator.NRI, t)
	assertEqual([]byte{}, fragmentizedNals[5].Payload, t)
}
