/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef HELICS_SERVER_HELPER_H
#define HELICS_SERVER_HELPER_H

#include "ns3/application-container.h"
#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/core-module.h"
#include "ns3/node-container.h"

#include "ns3/helics.h"

namespace ns3 {

class HelicsServerHelper {
public:
  HelicsServerHelper(std::string protocol);
  /*
  void SetupFederate(void);
  void SetupFederate (int argc, const char *const *argv);
  void SetupFederate (std::string &jsonString);
  void SetupApplicationFederate (void);
  void SetupCommandLine (CommandLine &cmd);*/
 
  ApplicationContainer InstallHelicsServer (Ptr<Node> node, const std::string &name, const std::string &destination, bool is_global=false) const;
  

private:
/*
  std::string broker;
  std::string name;
  std::string type;
  std::string core;
  double stop;
  double timedelta;
  std::string coreinit;*/
  ObjectFactory m_factory_server;

};

}

#endif /* HELICS_SERVER_HELPER_H */

