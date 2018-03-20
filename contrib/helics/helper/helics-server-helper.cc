/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/ipv4.h"
#include <memory>
#include <iostream>
#include "ns3/helics.h"
#include "ns3/helics-server.h"
#include "ns3/helics-server-helper.h"
#include "ns3/inet-socket-address.h"

#include "helics/core/core-types.hpp"

namespace ns3 {

HelicsServerHelper::HelicsServerHelper(std::string protocol)
{
    m_factory_server.SetTypeId (HelicsServer::GetTypeId ());
    m_factory_server.Set ("Protocol", StringValue (protocol));
    //m_factory.Set ("Port", UintegerValue (port));
}


ApplicationContainer
HelicsServerHelper::InstallHelicsServer (Ptr<Node> node, const std::string &name, const std::string &destination, bool is_global) const
{
    ApplicationContainer apps;
    Ptr<HelicsServer> app = m_factory_server.Create<HelicsServer> ();
    if (!app) {
      NS_FATAL_ERROR ("Failed to create HelicsServer");
    }
    app->SetEndpointName (name, is_global);
    app->SetDestination (destination);
    //Why do we need to aggregate a Ipv4 to this app?
   Ptr<Ipv4> net = node->GetObject<Ipv4>();
    Ipv4InterfaceAddress interface_address = net->GetAddress(1,0);
    Ipv4Address address = interface_address.GetLocal();
    app->SetLocal(address, 1234);
    node->AddApplication (app);
    apps.Add (app);
    return apps;
}


}

