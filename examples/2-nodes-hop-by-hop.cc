#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include <ns3/ndnSIM/utils/tracers/ndn-l3-aggregate-tracer.h>
#include <ns3/ndnSIM/utils/tracers/ndn-l3-rate-tracer.h>

using namespace ns3;
using namespace std;
using namespace boost;

/*
void IntTrace (uint32_t oldValue, uint32_t newValue)
{
    std::cout << "Traced " << oldValue << " to " << newValue << std::endl;
}
*/

int main (int argc, char *argv[])
{
    double freq = 200.0;
    double rtt_estimate = 0.13;

    CommandLine cmd;
    cmd.AddValue("Frequency", "CBR sending frequency", freq);
    cmd.AddValue("RttEstimate", "Rtt Estimate", rtt_estimate);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::ndn::DropTailQueue::MaxPackets", StringValue ("20"));
    Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue ("1Mbps"));
    Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue ("10ms"));

    NodeContainer nodes;
    nodes.Create(2);
    /*
       0---1
       */
    PointToPointHelper p2p;
    p2p.SetQueue("ns3::ndn::DropTailQueue",
            "LinkBandwidth", StringValue("1Mbps"),
            "MeanPktSize", StringValue("1024"),
            "QW", StringValue("0.01"));

    p2p.SetChannelAttribute ("Delay", StringValue ("50ms"));
    p2p.Install (nodes.Get (0), nodes.Get (1));

    ndn::StackHelper ndnHelper;
    ndnHelper.SetDefaultRoutes (true);
    ndnHelper.SetContentStore ("ns3::ndn::cs::Lru", "MaxSize", "1");
    ndnHelper.EnableLimits (true, Seconds (rtt_estimate), 40, 1024);
    ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute::PerOutFaceLimits::CongestionControl",
            "Limit", "ns3::ndn::Limits::Window",
            "EnableNACKs", "true",
            "MaxP", "0.5");

    ndnHelper.InstallAll ();  

    ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
    consumerHelper.SetPrefix ("/prefix");
    consumerHelper.SetAttribute ("Frequency", DoubleValue(freq));
    consumerHelper.SetAttribute ("MaxSeq", IntegerValue (10241));
    ApplicationContainer apps = consumerHelper.Install (nodes.Get (0));
//    apps.Get(0)->TraceConnectWithoutContext("WindowTrace", MakeCallback(&IntTrace));

    ndn::AppHelper producerHelper ("ns3::ndn::Producer");
    producerHelper.SetPrefix ("/prefix");
    producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
    producerHelper.Install (nodes.Get (1));


    std::stringstream ss;
    ss << "trace/2-nodes-hop-by-hop-" << freq << "-" << rtt_estimate << ".tr";
    AsciiTraceHelper ascii;
    p2p.EnableAsciiAll (ascii.CreateFileStream (ss.str()));
    p2p.EnablePcapAll (ss.str(), false);

    Simulator::Stop (Seconds (1000.0));
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
}
