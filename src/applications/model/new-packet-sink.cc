/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */

 // This is the web server


#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "new-packet-sink.h"
#include <cstdio>
#include <cstdlib>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NewPacketSink");

NS_OBJECT_ENSURE_REGISTERED (NewPacketSink);

TypeId
NewPacketSink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NewPacketSink")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<NewPacketSink> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&NewPacketSink::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&NewPacketSink::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&NewPacketSink::m_rxTrace),
                     "ns3::Packet::AddressTracedCallback")
  ;
  return tid;
}

NewPacketSink::NewPacketSink ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_totalRx = 0;
}

NewPacketSink::~NewPacketSink()
{
  NS_LOG_FUNCTION (this);
}

uint32_t NewPacketSink::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
NewPacketSink::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
NewPacketSink::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void NewPacketSink::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void NewPacketSink::StartApplication ()    // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      m_socket->Bind (m_local);
      m_socket->Listen ();
      m_socket->ShutdownSend ();
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&NewPacketSink::HandleRead, this));
  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&NewPacketSink::HandleAccept, this));
  m_socket->SetCloseCallbacks (
    MakeCallback (&NewPacketSink::HandlePeerClose, this),
    MakeCallback (&NewPacketSink::HandlePeerError, this));
}

void NewPacketSink::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
     Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
  if (m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

uint32_t convert_uint8_uint32(uint8_t *buffer)
{
  uint32_t output = 0;
  for(int i = 1; i < 5; i++)
  {
    output = output*256 + buffer[i];
  }
  return output;
}


void NewPacketSink::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();
      printf("Sink: Received %u bytes so far\n", m_totalRx);

      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes from "
                       << Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
                       << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }

      uint8_t* packet_contents = (uint8_t *) malloc(sizeof(uint8_t) * 5);
      uint8_t* opcode = (uint8_t *) malloc(sizeof(uint8_t));
      /* Read opcode*/
      packet->CopyData(opcode, 1);

      printf("Sink: Opcode is %d\n", *opcode);
      switch(*opcode){
        case 0: // Ignore
                NS_LOG_INFO("Received Opcode: 0");
                break;
        case 1: /* Read response size */
                NS_LOG_INFO("Received Opcode: 1");
                packet->CopyData(packet_contents, 5);
                uint32_t response_size = convert_uint8_uint32(packet_contents);
                NS_LOG_INFO("Read response size: "<<response_size);
		printf("SInk: resp size is %u\n", response_size);
                break;
      }

      m_rxTrace (packet, from);

      free(packet_contents);
      free(opcode);
    }
}


void NewPacketSink::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void NewPacketSink::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}


void NewPacketSink::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&NewPacketSink::HandleRead, this));
  m_socketList.push_back (s);
}

} // Namespace ns3
