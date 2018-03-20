
#ifndef HELICS_SERVER_H
#define HELICS_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/traced-value.h"
#include "ns3/address.h"

#include "ns3/helics.h"
#include "ns3/helics-application.h"
#include "helics/helics.hpp"

#include <map>
#include <string>

namespace ns3 {

class Address;
class Socket;
class Packet;

/**
 * \ingroup helics model
 * \brief A Helics Server Application
 *
 * 1. Recieve packet from the helics client;
 * 2. Send packet to another Helics federate 
 */

class HelicsServer : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  HelicsServer ();

  virtual ~HelicsServer ();
  /**
   * \brief set the name
   * \param name name
   */
  void SetName (const std::string &name);
  /**
   * \brief set the named of local endpoint
   * \param name name
   */
  void SetEndpointName (const std::string &name, bool is_global);

  /**
   * \brief set the name of the destination helics federate+endpoint
   * \param name name
   */
  void SetDestination (const std::string &name);

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
  /**
   * \return the total bytes received in this sink app
   */
  uint32_t GetTotalRx () const;

  /**
   * \return pointer to listening socket
   */
  Ptr<Socket> GetListeningSocket (void) const;

  /**
   * \return list of pointers to accepted sockets
   */
  std::list<Ptr<Socket> > GetAcceptedSockets (void) const;
 
protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop
  helics::endpoint_id_t m_endpoint_id;
  /**
   * \brief Handle a packet received by the application
   * \param socket the receiving socket
   */
  void HandleRead (Ptr<Socket> socket);
  //void DoRead (std::unique_ptr<helics::Message> message);

  // In the case of TCP, each socket accept returns a new socket, so the 
  // listening socket is stored separately from the accepted sockets
  Ptr<Socket>     m_socket;       //!< Listening socket
  std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets
  uint32_t        m_totalRx;      //!< Total bytes received
  uint16_t        m_port; //!< Port on which we listen for incoming packets.
  TypeId          m_tid;          //!< Protocol TypeId
  uint32_t        m_received;     //!< Number of received packets
  double          m_latency;      //!< Average packet delay (ms)
  double          m_throughput;   //!< Average packet throughput (kbps)
  double          m_Maxlatency;      //!< Maximum packet delay (ms)
  // double          m_Minthroughput;   //!< Minimum packet throughput (kbps) 
  double          m_initTx;       //!< Sending time of the first packet (ms)
  uint32_t        m_id;           //!< ID of the client application

  std::string     m_destination; //!< full name of the destination helics federate+endpoint
  std::string     m_name; //!< name of this helics application

  Address         m_localAddress; //!< Local address
  uint16_t        m_localPort; //!< Local port
  
  /// Traced Callback: received packets, source address.
  TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;
  TracedValue<uint32_t>  m_feedback;

};

} // namespace ns3

#endif /* HelicsServer_H */

