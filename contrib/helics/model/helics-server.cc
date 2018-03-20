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
#include "ns3/names.h"
#include "helics-server.h"
#include "qos-header.h"


#include "ns3/helics.h"
#include "ns3/helics-application.h"
#include "helics/helics.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HelicsServer");

NS_OBJECT_ENSURE_REGISTERED (HelicsServer);

TypeId 
HelicsServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HelicsServer")
    .SetParent<Application> ()
    .AddConstructor<HelicsServer> ()
    .AddAttribute ("Port",
                   "Port on which we listen for incoming packets.",
                   UintegerValue (8080),
                   MakeUintegerAccessor (&HelicsServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&HelicsServer::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("feedback",
                    "QOE feedback",
                    MakeTraceSourceAccessor (&HelicsServer::m_feedback),
                    "ns3::TracedValueCallback::Int32")
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&HelicsServer::m_rxTrace),
                     "ns3::Packet::AddressTracedCallback")
  ;
  return tid;
}

HelicsServer::HelicsServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_received = 0;
  m_latency = 0;
  m_throughput = 0;
  m_Maxlatency = 0;
  m_initTx = 0;
  m_id = 1;
}

HelicsServer::~HelicsServer()
{
  NS_LOG_FUNCTION (this);
}

void HelicsServer::SetName(const std::string &name)
{
  NS_LOG_FUNCTION(this << name);
  m_name = name;
  Names::Add("helics_" + name, this);
}

void HelicsServer::SetEndpointName (const std::string &name, bool is_global)
{
  NS_LOG_FUNCTION (this << name << is_global);
  SetName(name);
  if (is_global) {
    m_endpoint_id = helics_federate->registerGlobalEndpoint (name);
    NS_LOG_INFO ("Register a global endpoint "<<name<<", and endpoint ID is "<<m_endpoint_id.value());
  }
  else {
    m_endpoint_id = helics_federate->registerEndpoint (name);
    NS_LOG_INFO ("Register a local endpoint "<<name<<", and endpoint ID is "<<m_endpoint_id.value());
  }
  /*
  using std::placeholders::_1;
  using std::placeholders::_2;
  std::function<void(helics::endpoint_id_t,helics::Time)> func;
  func = std::bind (&HelicsServer::EndpointCallback, this, _1, _2);
  helics_federate->registerEndpointCallback(m_endpoint_id, func);
  */
}

void
HelicsServer::SetDestination (const std::string &destination)
{
  NS_LOG_FUNCTION (this << destination);

  m_destination = destination;
}

void HelicsServer::SetLocal(Address ip, uint16_t port)
{
  NS_LOG_FUNCTION(this << ip << port);
  m_localAddress = ip;
  m_localPort = port;
}

void HelicsServer::SetLocal(Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION(this << ip << port);
  NS_LOG_INFO ("This is Ipv4Address case.");
  m_localAddress = Address(ip);
  m_localPort = port;
}

void HelicsServer::SetLocal(Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION(this << ip << port);
  m_localAddress = Address(ip);
  m_localPort = port;
}

uint32_t HelicsServer::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

Ptr<Socket>
HelicsServer::GetListeningSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
HelicsServer::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void HelicsServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void HelicsServer::StartApplication ()    // Called at time specified by Start
{
  // NS_LOG_INFO ("HelicsServer starts");
  NS_LOG_FUNCTION (this);
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (),
                                                   m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      m_socket->Listen ();
      m_socket->ShutdownSend ();
      if (addressUtils::IsMulticast (local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
            }
        }
    }
  NS_LOG_INFO ("Before HandleRead function at HelicsServer Start Application...");
  m_socket->SetRecvCallback (MakeCallback (&HelicsServer::HandleRead, this));
  NS_LOG_INFO ("After HandleRead function at HelicsServer Start Application...");
}

void HelicsServer::StopApplication ()     // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()<< "s HelicsServer received "<<  m_totalRx << " bytes from Client "<<m_id);
  NS_LOG_INFO ("Average Latency: " << m_latency << "ms, Average Throughput: "<<m_throughput<<"kbps, Max Latency: "<<m_Maxlatency<<"ms");

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
  // NS_LOG_INFO ("HelicsServer is closed");
}

void HelicsServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_INFO ("We are at HandleRead()...... ");
  Ptr<Packet> packet;
  Address from;
  std::string sdata;
  double delay;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      NS_LOG_INFO ("received packet size is "<<packet->GetSize ()<<", and packet content "<<*packet);
      //Process QoS info including TX time, client ID.
      QosHeader QosTx;
      packet->RemoveHeader (QosTx);
      NS_LOG_INFO ("I am here and packet ID");
      NS_LOG_INFO ("client ID is "<<QosTx.GetID());
      if(QosTx.GetID()){
        if(m_received == 0){
          if(QosTx.GetTs().GetMilliSeconds()>0)
            m_initTx = QosTx.GetTs().GetMilliSeconds();
          else
            continue;
          m_id = QosTx.GetID();
        }
      
      NS_LOG_INFO ("I am here2");
      //Convert Packet to the helics message
      uint32_t size = packet->GetSize();
      std::ostringstream odata;
      packet->CopyData(&odata, size);
      sdata = odata.str();
      
      NS_LOG_INFO ("Simulator time now is "<<Simulator::Now().GetMilliSeconds()<<", and QosTx time is "<<QosTx.GetTs().GetMilliSeconds());
      //Post-processing the metrics of latency, througput
        delay=Simulator::Now().GetMilliSeconds() - QosTx.GetTs().GetMilliSeconds();
        
        NS_LOG_INFO ("We received packet size is "<<size<<", with delay time "<<delay);
        if(delay>0){
          if(m_Maxlatency<=delay)
            m_Maxlatency = delay;
          m_totalRx += (packet->GetSize ()+16);
          m_latency = (m_latency*m_received + delay)/(m_received+1);
          m_received++;
          m_throughput = m_totalRx*8000/(Simulator::Now().GetMilliSeconds() - m_initTx)/1024;
          
        }
        else
          continue;
      }
      
      // if (InetSocketAddress::IsMatchingType (from))
      //   {
      //     NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
      //                  << "s HelicsServer received "
      //                  <<  packet->GetSize () << " bytes from Client "
      //                  << QosTx.GetID());
      //   }
      // else if (Inet6SocketAddress::IsMatchingType (from))
      //   {
      //     NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
      //                  << "s HelicsServer received "
      //                  <<  packet->GetSize () << " bytes from Client "
      //                  << QosTx.GetID());
      //   }
      m_rxTrace (packet, from);
      
      //Send message back to helics federate
      sdata.append("with delay time "+std::to_string(delay)+"ms...");
      NS_LOG_INFO ("HelicsServer is ready to send out the new message with size "<<sdata.size()<<", and context is "<<sdata.data());
      helics_federate->sendMessage(m_endpoint_id, m_destination, sdata.data(), sdata.size());
    }
}

/*void
HelicsServer::DoRead (std::unique_ptr<helics::Message> message)
{
 // NS_LOG_FUNCTION (this << *message);

  NS_LOG_INFO ("sending message on to " << m_destination<<", from NS3 m_endpoint_id "<<m_endpoint_id.value());

  helics_federate->sendMessage (m_endpoint_id, m_destination, message->data.data(), message->data.size());
}*/


} // Namespace ns3
