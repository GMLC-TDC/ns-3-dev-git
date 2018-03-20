
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "helics-client.h"
#include "qos-header.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/names.h"

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

NS_LOG_COMPONENT_DEFINE ("HelicsClient");

NS_OBJECT_ENSURE_REGISTERED (HelicsClient);

std::string &
HelicsClient::SanitizeName(std::string &name)
{
  std::replace(name.begin(), name.end(), '/', '+');
  return name;
}

std::string
HelicsClient::SanitizeName(const std::string &name)
{
  std::string copy = name;
  std::replace(copy.begin(), copy.end(), '/', '+');
  return copy;
}

TypeId
HelicsClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HelicsClient")
    .SetParent<Application> ()
    .AddConstructor<HelicsClient> ()
    .AddAttribute("Name",
                  "The name of the application",
                  StringValue(),
                  MakeStringAccessor(&HelicsClient::m_name),
                  MakeStringChecker())
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&HelicsClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute("Sent",
                  "The counter for outbound packets",
                  UintegerValue(0),
                  MakeUintegerAccessor(&HelicsClient::m_sent),
                  MakeUintegerChecker<uint32_t>())
/*    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue (512),
                   MakeUintegerAccessor (&HelicsClient::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))*/
    .AddAttribute ("RemoteAddress", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&HelicsClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (900),
                   MakeUintegerAccessor (&HelicsClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())             
    .AddAttribute ("HelicsClientID", "The ID of the HelicsClient application",
                   UintegerValue (0),
                   MakeUintegerAccessor (&HelicsClient::m_id),
                   MakeUintegerChecker<uint32_t> (0))
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&HelicsClient::m_tid),
                   MakeTypeIdChecker ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&HelicsClient::m_interval),
                   MakeTimeChecker ())              
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&HelicsClient::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}


HelicsClient::HelicsClient ()
  : m_sent (0),
    m_interval (2),
    m_socket (0),
    m_id (3)
{
  NS_LOG_FUNCTION (this);
}

HelicsClient::~HelicsClient()
{
  NS_LOG_FUNCTION (this);
}

void HelicsClient::SetName(const std::string &name)
{
  NS_LOG_FUNCTION(this << name);
  m_name = name;
  Names::Add("helics_" + name, this);
}

void HelicsClient::SetEndpointName (const std::string &name, bool is_global)
{
  NS_LOG_FUNCTION (this << name << is_global);
  SetName(name);
  if (is_global) {
    m_endpoint_id = helics_federate->registerGlobalEndpoint (name);
    NS_LOG_FUNCTION ("Register a global endpoint "<<name<<", and endpoint ID is "<<m_endpoint_id.value());
  }
  else {
    m_endpoint_id = helics_federate->registerEndpoint (name);
    NS_LOG_FUNCTION ("Register a local endpoint "<<name<<", and endpoint ID is "<<m_endpoint_id.value());
  }
  
  using std::placeholders::_1;
  using std::placeholders::_2;
  std::function<void(helics::endpoint_id_t,helics::Time)> func;
  func = std::bind (&HelicsClient::EndpointCallback, this, _1, _2);
  helics_federate->registerEndpointCallback(m_endpoint_id, func); 
}

void HelicsClient::SetRemote(Address ip, uint16_t port)
{
  NS_LOG_FUNCTION(this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void HelicsClient::SetRemote(Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION(this << ip << port);
  NS_LOG_FUNCTION ("This is Ipv4Address case.");
  m_peerAddress = Address(ip);
  m_peerPort = port;
}

void HelicsClient::SetRemote(Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION(this << ip << port);
  m_peerAddress = Address(ip);
  m_peerPort = port;
}

void HelicsClient::SetLocal(Address ip, uint16_t port)
{
  NS_LOG_FUNCTION(this << ip << port);
  m_localAddress = ip;
  m_localPort = port;
}

void HelicsClient::SetLocal(Ipv4Address ip, uint16_t port)
{
  NS_LOG_FUNCTION(this << ip << port);
  NS_LOG_FUNCTION ("This is Ipv4Address case.");
  m_localAddress = Address(ip);
  m_localPort = port;
}

void HelicsClient::SetLocal(Ipv6Address ip, uint16_t port)
{
  NS_LOG_FUNCTION(this << ip << port);
  m_localAddress = Address(ip);
  m_localPort = port;
}

Ptr<Socket>
HelicsClient::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

void
HelicsClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void HelicsClient::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION ("Start Client application");
  NS_LOG_FUNCTION (this);
  // NS_LOG_INFO ("HelicsClient starts");
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
        }
    }

   /* if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      if (Inet6SocketAddress::IsMatchingType (m_peerAddress))
        {
          m_socket->Bind6 ();
        }
      else if (InetSocketAddress::IsMatchingType (m_peerAddress) ||
               PacketSocketAddress::IsMatchingType (m_peerAddress))
        {
          m_socket->Bind ();
        }
      m_socket->Connect (m_peerAddress);
      m_socket->SetAllowBroadcast (true);
      m_socket->ShutdownRecv ();
    }*/

  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_socket->SetAllowBroadcast (true);
  //m_sendEvent = Simulator::Schedule (Seconds (0.0), &HelicsClient::SendPacket, this);
  
}

void HelicsClient::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  if(m_socket != 0)
    {
      m_socket->Close ();
      //m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
      m_socket = 0; 
    }
  else
    {
      NS_LOG_WARN ("HelicsClient found null socket to close in StopApplication");
    }
  // NS_LOG_INFO ("HelicsClient is closed");
}

void HelicsClient::EndpointCallback(helics::endpoint_id_t id, helics::Time time)
{
  NS_LOG_FUNCTION (this << id.value() << time);
  NS_LOG_FUNCTION ("EndpointCallback happened at "<<id.value()<<", at Helics time "<<time<<", and at NS3 simulator time "<<Simulator::Now().GetSeconds()<<"s.");
  DoEndpoint (id, time);
}

void 
HelicsClient::DoEndpoint (helics::endpoint_id_t id, helics::Time time)
{
  NS_LOG_FUNCTION (this << id.value() << time);
  auto message = helics_federate->getMessage(id);
  NS_LOG_FUNCTION ("At first DoEndpoint function, the helics endpoint id  "<<id.value()<<" receives the message with size "<<message->data.size ()<<", and content begin is "<<message->data[1]<<", and end with "<<message->data[2]<<", at Helics time "<<time<<" and at NS3 simulator time "<< Simulator::Now().GetSeconds()<<"s.");
  DoEndpoint (id, time, std::move (message));
}

void
HelicsClient::DoEndpoint (helics::endpoint_id_t id, helics::Time time, std::unique_ptr<helics::Message> message)
{
  NS_LOG_FUNCTION (this << id.value() << time << message->data.size());
  NS_LOG_FUNCTION ("At HelicsClient App, the helics endpoint id  "<<id.value()<<" receives the message with size "<<message->data.size()<<", and content begin is "<<message->data[1]<<", and with number "<<message->data[4]<<"th message, at Helics time "<<time<<" and at NS3 simulator time "<< Simulator::Now().GetSeconds()<<"s.");
  SendPacket(std::move (message));
}

static uint8_t
char_to_uint8_t(char c)
{
  return uint8_t(c);
}


void HelicsClient::SendPacket (std::unique_ptr<helics::Message> message)
{
  //granted = helics_federate->requestTime (2*(m_sent+1));
  NS_LOG_FUNCTION ("We are at SendPacketFunction of Client()...");
  //message = helics_federate->getMessage(m_endpoint_id);
  NS_LOG_FUNCTION ("The helics endpoint id  "<<m_endpoint_id.value()<<" receives the message with size "<<message->data.size ()<<", and content begin is "<<message->data[1]<<", and end with "<<message->data[2]<<" and at NS3 simulator time "<< Simulator::Now().GetSeconds()<<"s.");

  //Start to send message

  NS_LOG_FUNCTION (this);
  Ptr<Packet> packet;
  //Step1: Parse helics message into a Packet
  // Convert given Message into a Packet.
  size_t total_size = message->data.size();
  uint8_t *buffer = new uint8_t[total_size];
  uint8_t *buffer_ptr = buffer;
  std::transform(message->data.begin(), message->data.end(), buffer_ptr, char_to_uint8_t);
  packet = Create<Packet>(buffer, total_size);
  NS_LOG_INFO("buffer='" << packet << "'");
  delete[] buffer;
  
  NS_ASSERT (m_sendEvent.IsExpired ());
  QosHeader QosTx;
  m_id = 3;
  NS_LOG_FUNCTION ("m_id is "<<m_id);
  QosTx.SetID(m_id);
  packet->AddHeader (QosTx);
  

  m_txTrace (packet);
 
  std::stringstream peerAddressStringStream;
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);
    }


  if ((m_socket->Send (packet)) >= 0)
    {
      m_sent++;
      NS_LOG_INFO ("HelicsClient TX " << total_size << " bytes to "
                                    << peerAddressStringStream.str () << " Uid: "
                                    << packet->GetUid () << " Time: "
                                    << (Simulator::Now ()).GetSeconds ());

    }
  else
    {
      NS_LOG_INFO ("Error while sending " << total_size << " bytes to "
                                          << peerAddressStringStream.str ());
    }
  /*
  if (m_sent < m_count)
    {
      m_sendEvent = Simulator::Schedule (m_interval, &HelicsClient::SendPacket, this);
    }*/
}


} // Namespace ns3
