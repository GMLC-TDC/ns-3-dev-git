/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/ipv4.h"
#include <memory>
#include <iostream>
#include "ns3/helics.h"
#include "ns3/helics-client.h"
#include "ns3/helics-client-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"

#include "helics/core/core-types.hpp"

namespace ns3 {

HelicsClientHelper::HelicsClientHelper(std::string protocol, Address address, uint16_t port)
{
    m_factory_client.SetTypeId (HelicsClient::GetTypeId ());
    m_factory_client.Set ("Protocol", StringValue (protocol));
    m_factory_client.Set ("RemoteAddress", AddressValue (address));
    m_factory_client.Set ("RemotePort", UintegerValue (port));
}


ApplicationContainer
HelicsClientHelper::InstallHelicsClient (Ptr<Node> node, const std::string &name, bool is_global) const
{
    ApplicationContainer apps;
    Ptr<HelicsClient> app = m_factory_client.Create<HelicsClient> ();
    if (!app) {
      NS_FATAL_ERROR ("Failed to create HelicsClient");
    }
    app->SetEndpointName (name, is_global);
    Ptr<Ipv4> net = node->GetObject<Ipv4>();
    Ipv4InterfaceAddress interface_address = net->GetAddress(1,0);
    Ipv4Address address = interface_address.GetLocal();
    app->SetLocal(address, 1234);
    node->AddApplication (app);
    apps.Add (app);
    return apps;
}




}

