package main

import (
	"errors"
	"log"
	"net"

	"github.com/wernerd/GoRTP/src/net/rtp"
)

// RTPSender - provides RTP stream
type RTPSender struct {
	transport *rtp.TransportUDP
	session   *rtp.Session
	streamID  uint32
}

// NewRTP - create new RTPSender
func NewRTP(ip *net.IPAddr, port uint16) (RTPSender, error) {
	transport, err := rtp.NewTransportUDP(ip, int(port), ip.Zone)
	if err != nil {
		return RTPSender{}, err
	}

	session := rtp.NewSession(transport, transport)
	_, err = session.AddRemote(&rtp.Address{
		IpAddr:   ip.IP,
		DataPort: 2010,
		CtrlPort: 2011,
		Zone:     ip.Zone,
	})

	if err != nil {
		return RTPSender{}, err
	}

	address := rtp.Address{
		IpAddr:   ip.IP,
		DataPort: int(port),
		CtrlPort: int(port + 1),
		Zone:     ip.Zone,
	}
	streamID, err := session.NewSsrcStreamOut(&address, 0, 0)
	if err != rtp.Error("") {
		return RTPSender{}, errors.New(err.Error())
	}

	log.Printf("[RTP] Created session %v on %v:%v", streamID, ip, port)

	session.SsrcStreamOutForIndex(streamID).SetPayloadType(96)
	return RTPSender{
		transport,
		session,
		streamID,
	}, nil
}

// Run - send packets from |packetsChannel| as RTP stream
func (sender *RTPSender) Run(packetsChannel chan Packet) {
	go sender.handleControlPackets()
	go sender.sendPackets(packetsChannel)
	go sender.receiveDataPackets()

	err := sender.session.StartSession()
	if err != nil {
		log.Fatalf("Failed to start RTP service: %v", err)
	}
}

func (sender *RTPSender) sendPackets(packets chan Packet) {
	timestamp := uint32(0) // fake
	n := 0

	for packet := range packets {
		for _, fu_packet := range nalFragmentize(packet.Data) {
			dataPacket := sender.session.NewDataPacket(timestamp)
			dataPacket.SetPayloadType(96)

			// FIXME: packets should be split into FUs (fragmentation units) as described in RFC 6184
			dataPacket.SetPayload(fu_packet.Serialize())
			sender.session.WriteData(dataPacket)
			dataPacket.FreePacket()

			n = n + 1

			if (n % 100) == 0 {
				log.Printf("[RTP] Sent %v packets", n)
			}
		}
		timestamp = timestamp + 100
	}
}

func (sender *RTPSender) receiveDataPackets() {
	packets := sender.session.CreateDataReceiveChan()

	for packet := range packets {
		log.Printf("Received data packet")

		packet.FreePacket()
	}
}

func (sender *RTPSender) handleControlPackets() {
	receiver := sender.session.CreateCtrlEventChan()

	for eventSlice := range receiver {
		for _, event := range eventSlice {
			if event != nil {
				sender.handleControlEvent(event)
			}
		}
	}
}

func (sender *RTPSender) handleControlEvent(event *rtp.CtrlEvent) {
	log.Printf("[RTCP] Received event %v", event)
}
