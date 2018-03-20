/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef HELICS_CLIENT_HELPER_H
#define HELICS_CLIENT_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/application-container.h"
#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/address.h"

#include "ns3/helics.h"

namespace ns3 {

class HelicsClientHelper {
public:
  HelicsClientHelper(std::string protocol, Address address, uint16_t port);
  ApplicationContainer InstallHelicsClient (Ptr<Node> node, const std::string &name, bool is_global=false) const;
 
  
private:
  ObjectFactory m_factory_client;
 

};

}

#endif /* HELICS_CLIENT_HELPER_H */

