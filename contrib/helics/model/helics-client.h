

#ifndef HELICS_CLIENT_H
#define HELICS_CLIENT_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ipv4-address.h"

#include <map>
#include <string>
#include "helics/helics.hpp"

namespace ns3 {

class Address;
class Socket;
class Packet;
class InetSocketAddress;
class Inet6SocketAddress;

/**
 * \ingroup helics model
 * \brief A Helics Client Application
 *
 * 1. Recieve Helics message from the corresponting helics endpoint which also attaches to the other HELICS federate node;
 * 2. Parse helics message into UDP or TCP packet format
 * 3. Send packet to the Helics Server application
 */

class HelicsClient : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  static std::string& SanitizeName (std::string &name);
  static std::string SanitizeName (const std::string &name);

  HelicsClient ();

  virtual ~HelicsClient();
  /* For NS3 outbound parameter configuration*/
  

  /**
   * \brief set the name
   * \param name name
   */
  void SetName (const std::string &name);
  /**
   * \brief set the named endpoint to filter
   * \param name name
   */
  //void SetFilterName (const std::string &name);
  /**
   * \brief set the named of this endpoint
   * \param name name
   */
  void SetEndpointName (const std::string &name, bool is_global);
  /**
   * \brief set the remote address and port
   * \param ip remote IPv4 address
   * \param port remote port
   */
  void SetRemote (Ipv4Address ip, uint16_t port);
  /**
   * \brief set the remote address and port
   * \param ip remote IPv6 address
   * \param port remote port
   */
  void SetRemote (Ipv6Address ip, uint16_t port);
  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Address ip, uint16_t port);

    /**
   * \brief set the local address and port
   * \param ip local IPv4 address
   * \param port local port
   */
  void SetLocal (Ipv4Address ip, uint16_t port);
  /**
   * \brief set the local address and port
   * \param ip local IPv6 address
   * \param port local port
   */
  void SetLocal (Ipv6Address ip, uint16_t port);
  /**
   * \brief set the local address and port
   * \param ip local IP address
   * \param port local port
   */
  void SetLocal (Address ip, uint16_t port);



  /* For NS3 inbound parameter configuration */

  /**
   * \brief Return a pointer to associated socket.
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;

  virtual void EndpointCallback (helics::endpoint_id_t id, helics::Time time);

protected:
  virtual void DoDispose (void);

private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop
  helics::endpoint_id_t m_endpoint_id;

  /* For NS3 outbound operation functions, Actually receive Helics messages from other federate */
  
  /**
   * \brief Step1: Receive a HELICS message.
   *
   * This function is called internally by HELICS.
   */
  
  void DoEndpoint (helics::endpoint_id_t id, helics::Time time);
  void DoEndpoint (helics::endpoint_id_t id, helics::Time time, std::unique_ptr<helics::Message> message);
  /**
   * \brief Setp2&3: Packet creation based on HELICS data sent to the Helics sever.
   */
  void SendPacket (std::unique_ptr<helics::Message> message);

  std::string     m_name; //!< name of this helics application

  uint32_t        m_count; //!< Maximum number of packets the application will receice and send
  uint32_t        m_sent;  //!< Counter for received/sent packets
  Time            m_interval; //!< Packet inter-receive/send time which is the granted time
  EventId         m_sendEvent;    //!< Event id of pending "send packet" event

  Ptr<Socket>     m_socket;       //!< Associated socket
  Address         m_peerAddress;         //!< Peer address
  uint16_t        m_peerPort; //!< Remote peer port
  Address         m_localAddress; //!< Local address
  uint16_t        m_localPort; //!< Local port

  TypeId          m_tid;          //!< Type of the socket used
  uint32_t        m_id;            //!< ID of the client application
  helics::Time    granted;

  /// Traced Callback: transmitted packets.
  TracedCallback<Ptr<const Packet> > m_txTrace; 
  std::map<uint32_t,std::unique_ptr<helics::Message> > m_message; //for store the message received from helics
  std::unique_ptr<helics::Message> message;
  

};

} 

#endif /* CLIENT_H */
