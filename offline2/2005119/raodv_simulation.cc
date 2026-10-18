#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/raodv-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RaodvSimulation");

int main(int argc, char *argv[]) {
    double simTime = 10.0; 
    std::string csvOutput = "results_raodv_speed.csv"; 

    std::ofstream outFile;
    outFile.open(csvOutput);
    outFile << "NumNodes,Speed,PacketRate,Throughput,Delay,PDR,PacketDropRatio\n";
    
    // numNodes 20, 40, 70, 100
    // packetRate 100, 200, 300, 400
    // speed 5, 10, 15, 20

    for (uint32_t numNodes : {70}) {
        for (uint32_t packetRate : {200}) {
            for (double speed : {5, 10, 15, 20}) {
                std::cout << "Running simulation for " << numNodes << " nodes, " << packetRate << " packets/sec, " << speed << " m/s...\n";
                NodeContainer nodes;
                nodes.Create(numNodes);

                // Set mobility model
                MobilityHelper mobility;
                mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
                mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                              "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"),
                                              "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));
                mobility.Install(nodes);
                for (uint32_t i = 0; i < nodes.GetN(); ++i) {
                    nodes.Get(i)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(speed, 0, 0));
                }

                // Configure WiFi
                WifiHelper wifi;
                wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

                YansWifiPhyHelper wifiPhy;
                wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

                YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
                wifiPhy.SetChannel(wifiChannel.Create());

                WifiMacHelper wifiMac;
                wifiMac.SetType("ns3::AdhocWifiMac");

                NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

                // Install Internet stack and RAODV routing
                InternetStackHelper stack;
                RaodvHelper raodv;
                stack.SetRoutingHelper(raodv);
                stack.Install(nodes);

                // Assign IP addresses
                Ipv4AddressHelper address;
                address.SetBase("10.1.1.0", "255.255.255.0");
                Ipv4InterfaceContainer interfaces = address.Assign(devices);

                // Install a sink application on the last node
                uint16_t port = 9;
                PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
                ApplicationContainer sinkApp = sink.Install(nodes.Get(numNodes - 1));
                sinkApp.Start(Seconds(1.0));
                sinkApp.Stop(Seconds(simTime));

                // Install traffic generator applications
                OnOffHelper onoff("ns3::UdpSocketFactory", Address());
                onoff.SetAttribute("PacketSize", UintegerValue(512)); // Packet size in bytes
                onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

                // Set data rate based on packetRate
                std::string dataRate = std::to_string(packetRate * 512 * 8) + "bps"; // Convert packets/sec to bits/sec
                onoff.SetAttribute("DataRate", DataRateValue(DataRate(dataRate)));

                AddressValue remoteAddress(InetSocketAddress(interfaces.GetAddress(numNodes - 1), port));
                onoff.SetAttribute("Remote", remoteAddress);

                ApplicationContainer app;
                for (uint32_t i = 0; i < numNodes - 1; ++i) {
                    Ptr<Node> srcNode = nodes.Get(i);
                    ApplicationContainer temp = onoff.Install(srcNode);
                    temp.Start(Seconds(1.0));
                    temp.Stop(Seconds(simTime));
                    app.Add(temp);
                }

                // Set up FlowMonitor
                FlowMonitorHelper flowmon;
                Ptr<FlowMonitor> monitor = flowmon.InstallAll();

                Simulator::Stop(Seconds(simTime));
                Simulator::Run();

                // Analyze FlowMonitor results
                double totalThroughput = 0.0, totalDelay = 0.0, totalRxPackets = 0.0, totalTxPackets = 0.0, totalPacketDrops = 0.0;
                uint32_t validFlows = 0;

                monitor->CheckForLostPackets();
                Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
                auto stats = monitor->GetFlowStats();

                for (const auto &flow : stats) {
                    const FlowMonitor::FlowStats &stat = flow.second;
                    validFlows++;
                    totalThroughput += (stat.rxBytes * 8.0) / (simTime * 1e6); // Mbps
                    totalDelay += stat.delaySum.GetSeconds();
                    totalRxPackets += stat.rxPackets;
                    totalTxPackets += stat.txPackets;
                    totalPacketDrops += (stat.txPackets - stat.rxPackets);
                }

                // Compute averages
                double throughput = validFlows > 0 ? totalThroughput / validFlows : 0.0;
                double delay = totalRxPackets > 0 ? totalDelay / totalRxPackets : 0.0;
                double pdr = totalTxPackets > 0 ? (totalRxPackets / totalTxPackets) * 100.0 : 0.0;
                double dropRatio = totalTxPackets > 0 ? (totalPacketDrops / totalTxPackets) * 100.0 : 0.0;

                outFile << numNodes << "," << speed << "," << packetRate << "," << throughput << "," << delay << "," << pdr << "," << dropRatio << "\n";

                // Close the FlowMonitor and reset for the next loop
                outFile.flush();
                Simulator::Destroy();
            }
        }
    }

    outFile.close();
    return 0;
}
