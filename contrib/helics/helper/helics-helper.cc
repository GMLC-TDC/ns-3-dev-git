/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/ipv4.h"
#include <memory>

#include "ns3/helics.h"
#include "ns3/helics-application.h"
//#include "ns3/helics-filter-application.h"
#include "ns3/helics-static-sink-application.h"
#include "ns3/helics-static-source-application.h"
#include "ns3/helics-helper.h"

#include "helics/core/core-types.hpp"

namespace ns3 {

HelicsHelper::HelicsHelper()
: broker("")
, name("ns3")
, core("zmq")
, timedelta(1.0)
, coreinit("")
{
   
}

void
HelicsHelper::SetupFederate(void)
{
  helics::FederateInfo fi (name);
  fi.coreType = helics::coreTypeFromString (core);
  fi.timeDelta = timedelta;
  if (!coreinit.empty()) {
    fi.coreInitString = coreinit;
  }
  helics_federate = std::make_shared<helics::MessageFederate> (fi);
}

// Recognized args:
// broker - address of the broker to connect
// name - name of the federate
// corename - name of the core to create or find
// core - type of core to connect to
// offset - offset of time steps
// period - period of the federate
// timedelta - the time delta of the federate
// coreinit - the core initialization string
// inputdelay
// outputdelay
// flags - named flag for the federate

// Some default values:
// timeDelta = timeEpsilon (minimum time advance allowed by federate)
// outputDelay, inputDelay, period, offset = timeZero
void
HelicsHelper::SetupFederate(int argc, const char *const *argv)
{
  helics::FederateInfo fi (argc, argv);
  helics_federate = std::make_shared<helics::MessageFederate> (fi);
}

void
HelicsHelper::SetupFederate(std::string &jsonString)
{
  helics::FederateInfo fi = helics::LoadFederateInfo (jsonString);
  helics_federate = std::make_shared<helics::MessageFederate> (fi);
}

void
HelicsHelper::SetupApplicationFederate(void)
{
  SetupFederate ();
  helics_endpoint = helics_federate->registerEndpoint ("fout");
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::HelicsSimulatorImpl"));
}

void
HelicsHelper::SetupCommandLine(CommandLine &cmd)
{
  cmd.AddValue ("broker", "address to connect the broker to", broker);
  cmd.AddValue ("name", "name of the ns3 federate", name);
  cmd.AddValue ("core", "name of the core to connect to", core);
  cmd.AddValue ("timedelta", "the time delta of the federate", timedelta);
  cmd.AddValue ("coreinit", "the core initializion string", coreinit);
}

}

