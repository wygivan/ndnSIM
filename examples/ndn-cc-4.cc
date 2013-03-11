/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Yaogong Wang <ywang15@ncsu.edu>
 */
// ndn-cc-4: multiflow
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include <ns3/ndnSIM/utils/tracers/ndn-l3-aggregate-tracer.h>
#include <ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.h>

using namespace ns3;

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Read topology
  AnnotatedTopologyReader topologyReader ("", 25);
  topologyReader.SetFileName ("src/ndnSIM/examples/topologies/topo-6-node.txt");
  topologyReader.Read ();

  // Install CCNx stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute",
                                   "EnableNACKs", "true");
  ndnHelper.EnableShaper (true);
  ndnHelper.SetContentStore ("ns3::ndn::cs::Lru", "MaxSize", "1"); // almost no caching
  ndnHelper.InstallAll ();

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();

  // Getting containers for the consumer/producer
  Ptr<Node> c1 = Names::Find<Node> ("Src1");
  Ptr<Node> c2 = Names::Find<Node> ("Src2");
  Ptr<Node> p1 = Names::Find<Node> ("Dst1");
  Ptr<Node> p2 = Names::Find<Node> ("Dst2");

  // Install consumer
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerWindowAIMD");

  consumerHelper.SetPrefix ("/p1");
  consumerHelper.Install (c1);

  consumerHelper.SetPrefix ("/p2");
  consumerHelper.SetAttribute ("StartTime", TimeValue (Seconds (5.0)));
  consumerHelper.Install (c2);
  
  // Register prefix with global routing controller and install producer
  ndn::AppHelper producerHelper ("ns3::ndn::Producer");
  producerHelper.SetAttribute ("PayloadSize", StringValue("1000"));

  ndnGlobalRoutingHelper.AddOrigins ("/p1", p1);
  producerHelper.SetPrefix ("/p1");
  producerHelper.Install (p1);

  ndnGlobalRoutingHelper.AddOrigins ("/p2", p2);
  producerHelper.SetPrefix ("/p2");
  producerHelper.Install (p2);

  // Calculate and install FIBs
  ndnGlobalRoutingHelper.CalculateRoutes ();

  Simulator::Stop (Seconds (20.0));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3AggregateTracer> > >
    aggTracers = ndn::L3AggregateTracer::InstallAll ("aggregate-trace.txt", Seconds (0.5));

  boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<ndn::L3RateTracer> > >
    rateTracers = ndn::L3RateTracer::InstallAll ("rate-trace.txt", Seconds (0.5));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}