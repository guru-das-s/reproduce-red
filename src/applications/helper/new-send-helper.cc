/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Geoge Riley <riley@ece.gatech.edu>
 * Adapted from OnOffHelper by:
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "new-send-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"
#include "ns3/names.h"
#include "ns3/core-module.h"

namespace ns3 {

NewSendHelper::NewSendHelper (std::string protocol, Address addressS, Address addressD, uint32_t resp_size, uint32_t max_size, request_param_t param)
{
  secondaryRequestCounter = param.secondaryRequestCounter;
  consecPageCounter = param.consecPageCounter;
  func = param.func;
  m_factory.SetTypeId ("ns3::NewSendApplication");
  m_factory.Set ("Protocol", StringValue (protocol));
  m_factory.Set ("Remote", AddressValue (addressD));
  m_factory.Set ("Local", AddressValue (addressS));
  m_factory.Set ("RecvBytes", UintegerValue(resp_size));
  m_factory.Set ("MaxBytes", UintegerValue(max_size));
  m_factory.Set("BrowserNum", UintegerValue(param.browserNum));
}

void
NewSendHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
NewSendHelper::Install (Ptr<Node> node) const
{
  ApplicationContainer sourceApps = ApplicationContainer (InstallPriv (node));
  Ptr<NewSendApplication> sendptr = DynamicCast<NewSendApplication> (sourceApps.Get(0));
  sendptr->SetConsecPageCounter(consecPageCounter);
  sendptr->SetSecondaryRequestCounter(secondaryRequestCounter);
  sendptr->SetFunction(func);
  return sourceApps;
}

ApplicationContainer
NewSendHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
NewSendHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
NewSendHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
