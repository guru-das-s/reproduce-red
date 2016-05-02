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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "packet-sink-helper.h"
#include "ns3/string.h"
#include "ns3/inet-socket-address.h"
#include "ns3/names.h"

namespace ns3 {

PacketSinkHelper::PacketSinkHelper (std::string protocol, Address address, request_param_t param1, uint32_t resp_size1)
{
  m_factory.SetTypeId ("ns3::PacketSink");
  m_factory.Set ("Protocol", StringValue (protocol));
  m_factory.Set ("Local", AddressValue (address));
  param = new request_param_t();
  *param = param1;
  resp_size = resp_size1;
}

void 
PacketSinkHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
PacketSinkHelper::Install (Ptr<Node> node) const
{
  ApplicationContainer sourceApps = ApplicationContainer (InstallPriv (node));
  Ptr<PacketSink> sinkptr = DynamicCast<PacketSink> (sourceApps.Get(0));
  sinkptr->SetRequestParam(*param);
  sinkptr->SetRespSize(resp_size);
  delete param;
  return sourceApps;
}

ApplicationContainer
PacketSinkHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
PacketSinkHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
PacketSinkHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
