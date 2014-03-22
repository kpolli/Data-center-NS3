#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/flow-monitor-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/bridge-net-device.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/random-variable.h"
#include "ns3/netanim-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/dce-module.h"

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("Fat-Tree-Architecture");
std::string animFile = "afcdmto.xml" ;

char * toString(int a,int b, int c, int d){

	int first = a;
	int second = b;
	int third = c;
	int fourth = d;

	char *address =  new char[30];
	char firstOctet[30], secondOctet[30], thirdOctet[30], fourthOctet[30];	

	bzero(address,30);

	snprintf(firstOctet,10,"%d",first);
	strcat(firstOctet,".");
	snprintf(secondOctet,10,"%d",second);
	strcat(secondOctet,".");
	snprintf(thirdOctet,10,"%d",third);
	strcat(thirdOctet,".");
	snprintf(fourthOctet,10,"%d",fourth);

	strcat(thirdOctet,fourthOctet);
	strcat(secondOctet,thirdOctet);
	strcat(firstOctet,secondOctet);
	strcat(address,firstOctet);

	return address;
}

// Main function
//
int main(int argc, char *argv[])
{
//=========== Define parameters based on value of k ===========//
//
	int nk = 4;
	CommandLine cmd;
	cmd.AddValue("nk", "Number of ports", nk);
	cmd.Parse(argc, argv);
	int k = nk;			// number of ports per switch
	int num_pod = k;		// number of pod
	int num_host = (k/2);		// number of hosts under a switch
	int num_edge = (k/2);		// number of edge switch in a pod
	int num_agg = (k/2);		// number of aggregation switch in a pod
	int num_group = k/2;		// number of group of core switches
        int num_core = (k/2);		// number of core switch in a group
	int total_host = k*k*k/4;	// number of hosts in the entire network	
	char filename [] = "afcdmtolat.xls";// filename for Flow Monitor xml output file
	Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));

// Initialize other variables
//
	int i = 0;	
	int j = 0;	
	int h = 0;

// Initialize parameters for On/Off application
//
	int port = 9;
	int packetSize = 1024;		// 1024 bytes
	char dataRate_OnOff [] = "1Mbps";
	char maxBytes [] = "0";		// unlimited

// Initialize parameters for Csma and PointToPoint protocol
//
	char dataRate [] = "1000Mbps";	// 1Gbps
	int delay = 0.1;		// 0.1 ms
	double minRto = 0.001;

// Output some useful information
//	
	std::cout << "Value of k =  "<< k<<"\n";
	std::cout << "Total number of hosts =  "<< total_host<<"\n";
	std::cout << "Number of hosts under each switch =  "<< num_host<<"\n";
	std::cout << "Number of edge switch under each pod =  "<< num_edge<<"\n";
	std::cout << "------------- "<<"\n";

// Initialize Internet Stack and Routing Protocols
//	

	InternetStackHelper internet;
	//Ipv4NixVectorHelper nixRouting; 
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4GlobalRoutingHelper globalRouting;
	//OlsrHelper olsr;	
	Ipv4ListRoutingHelper list;
	list.Add (staticRouting, 0);	
	//list.Add (nixRouting, 10);
	list.Add (globalRouting, 10);
	//list.Add (olsr, 10);	
	internet.SetRoutingHelper(list);
	




//=========== Creation of Node Containers ===========//
//
	NodeContainer core[num_group];				// NodeContainer for core switches
	for (i=0; i<num_group;i++){  	
		core[i].Create (num_core);
		internet.Install (core[i]);
		 		
	}
	NodeContainer agg[num_pod];				// NodeContainer for aggregation switches
	for (i=0; i<num_pod;i++){  	
		agg[i].Create (num_agg);
		internet.Install (agg[i]);
	
	}
	NodeContainer edge[num_pod];				// NodeContainer for edge switches
  	for (i=0; i<num_pod;i++){  	
		edge[i].Create (num_edge);
		internet.Install (edge[i]);
	
	}

	NodeContainer host[num_pod][num_edge];		// NodeContainer for hosts
  	for (i=0; i<k;i++){
		for (j=0;j<num_edge;j++){  	
			host[i][j].Create (num_host);		
			internet.Install (host[i][j]);
			
		}
}

//		for (i=0;i<num_pod;i++){
//			for (j=0;j<num_agg;j++){
//				for (h=0;h<num_edge;h++){

	DceManagerHelper dce;
//	LinuxStackHelper stack;
	dce.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
	dce.Install(edge[0].Get(0));
//	dce.Install(agg[0].Get(0));
//	dce.Install(core[0].Get(0));
	ApplicationContainer apps;
	DceApplicationHelper dcea;
	dcea.SetStackSize (1 << 16);
    	dcea.SetBinary ("tc");
	dcea.ResetArguments();
	dcea.ResetEnvironment();
	//dcea.ParseArguments ("tc qdisc add dev eth1 root fifo limit 16666 target 5 interval 100 noecn");

	dcea.ParseArguments ("tc qdisc add dev eth1 parent 1:1 handle 10: sfq 16666 5 100");

	apps = dcea.Install(edge[0].Get(0));
	apps.Start(Seconds (0.0));
	apps.Stop(Seconds (2.5));


    	dcea.SetBinary ("tc");
	dcea.ResetArguments();
	dcea.ResetEnvironment();
	dcea.ParseArguments ("tc qdisc show dev eth1");

	dcea.Install(edge[0].Get(0));
	apps.Start(Seconds (0.0));
	apps.Stop(Seconds (2.5));
//	}}}

	
	
//=========== Initialize settings for On/Off Application ===========//
//

// Generate traffics for the simulation
//	
		ApplicationContainer app[total_host];
		for (i=0;i<num_pod;i++){
			for (j=0;j<num_edge; j++){
				for (h=0; h<num_host; h++){	
	

		PacketSinkHelper sink ("ns3::TcpSocketFactory",Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
    		//set a node as reciever
	    	app[i] = sink.Install (host[0][0].Get(0));
		

    		OnOffHelper onOffHelper ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address ("10.0.0.1"), port)));
    		//onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    		//onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

    		onOffHelper.SetAttribute("PacketSize",UintegerValue (packetSize));
 	       	onOffHelper.SetAttribute("DataRate",StringValue (dataRate_OnOff));      
	        onOffHelper.SetAttribute("MaxBytes",StringValue (maxBytes));
		Config::SetDefault ("ns3::RttEstimator::InitialEstimation", StringValue ("1ms"));
  		Config::SetDefault ("ns3::RttEstimator::MinRTO", TimeValue (Seconds (minRto)));

		NodeContainer onoff;
		onoff.Add(host[i][j].Get(h));
	     	app[i] = onOffHelper.Install (onoff);

			}
		}
	}
	std::cout << "Finished creating On/Off traffic"<<"\n";

// Inintialize Address Helper
//	
  	Ipv4AddressHelper address;

// Initialize PointtoPoint helper
//	
	PointToPointHelper p2p;
  	p2p.SetDeviceAttribute ("DataRate", StringValue (dataRate));
  	p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (delay)));


// Initialize Csma helper
//
  	CsmaHelper csma;
  	csma.SetChannelAttribute ("DataRate", StringValue (dataRate));
  	csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (delay)));

//=========== Connect edge switches to hosts ===========//
//	

	NetDeviceContainer hostSw[num_pod][num_edge];			
	Ipv4InterfaceContainer ipContainer[num_pod][num_edge];

	for (i=0;i<num_pod;i++){
		for (j=0;j<num_edge; j++){
			for (h=0; h<num_host; h++){
			
			//NetDeviceContainer link2 = csma.Install (NodeContainer (host[i][j].Get(h), edge[i].Get(j)));
			NetDeviceContainer link2 = p2p.Install (host[i][j].Get(h), edge[i].Get(j));
			hostSw[i][j].Add(link2.Get(0));			
			hostSw[i][j].Add(link2.Get(1));						
		}	
			//Assign address
			char *subnet;
			subnet = toString(10, i, j, 0);
			address.SetBase (subnet, "255.255.255.0");
			ipContainer[i][j]= address.Assign(hostSw[i][j]);			
		}
	}

	std::cout << "Finished connecting edge switches and hosts  "<< "\n";

//=========== Connect aggregate switches to edge switches ===========//
//
	NetDeviceContainer ae[num_pod][num_agg][num_edge]; 	
	Ipv4InterfaceContainer ipAeContainer[num_pod][num_agg][num_edge];
	//NetDeviceContainer test2[num_pod][num_agg][num_edge];
	//NetDeviceContainer ea[num_pod][num_agg][num_edge]; 
	for (i=0;i<num_pod;i++){
		for (j=0;j<num_agg;j++){
			for (h=0;h<num_edge;h++){
				ae[i][j][h] = p2p.Install(edge[i].Get(h), agg[i].Get(j));
/*				ea[i][j][h] = csma.Install(NodeContainer (agg[i].Get(j), edge[i].Get(h))); 

				Ptr<Node> switchNode = agg[i].Get(j);
				Ptr<Node> switchNode2 = edge[i].Get(h);
				
				OpenFlowSwitchHelper swtch;

				if (use_drop)
    				{
					Ptr<ns3::ofi::DropController> controller = CreateObject<ns3::ofi::DropController> ();
					test2[i][j][h].Add (swtch.Install (switchNode, ea[i][j][h], controller));
					test2[i][j][h].Add (swtch.Install (switchNode2, ea[i][j][h], controller));
				 }
 				 else
    				{
      					Ptr<ns3::ofi::LearningController> controller = CreateObject<ns3::ofi::LearningController> ();
      					if (!timeout.IsZero ()) controller->SetAttribute ("ExpirationTime", TimeValue (timeout));
      					test2[i][j][h].Add (swtch.Install (switchNode, ea[i][j][h], controller));
					test2[i][j][h].Add (swtch.Install (switchNode2, ea[i][j][h], controller));
				}
*/
				int second_octet = i;		
				int third_octet = j+(k/2);	
				int fourth_octet;
				if (h==0) fourth_octet = 1;
				else fourth_octet = h*2+1;
				//Assign subnet
				char *subnet;
				subnet = toString(10, second_octet, third_octet, 0);
				//Assign base
				char *base;
				base = toString(0, 0, 0, fourth_octet);
				address.SetBase (subnet, "255.255.255.0",base);
				ipAeContainer[i][j][h] = address.Assign(ae[i][j][h]);
			}	
		}		
	}
	std::cout << "Finished connecting aggregation switches and edge switches  "<< "\n";

//=========== Connect core switches to aggregate switches ===========//
//
	NetDeviceContainer ca[num_group][num_core][num_pod]; 		
	Ipv4InterfaceContainer ipCaContainer[num_group][num_core][num_pod];
	int fourth_octet =1;
//	NetDeviceContainer test1[num_group][num_core][num_pod];
//	NetDeviceContainer ac[num_group][num_core][num_pod]; 
	for (i=0; i<num_group; i++){		
		for (j=0; j < num_core; j++){
			fourth_octet = 1;
			for (h=0; h < num_pod; h++){			
				ca[i][j][h] = p2p.Install(core[i].Get(j), agg[h].Get(i));	
/*				ac[i][j][h] = csma.Install(NodeContainer (core[i].Get(j), agg[h].Get(i))); 				

				Ptr<Node> switchNode = core[i].Get(j);
				Ptr<Node> switchNode2 = agg[h].Get(i);
				OpenFlowSwitchHelper swtch;

				if (use_drop)
    				{
					Ptr<ns3::ofi::DropController> controller = CreateObject<ns3::ofi::DropController> ();
					test1[i][j][h].Add (swtch.Install (switchNode, ac[i][j][h], controller));
					test1[i][j][h].Add (swtch.Install (switchNode2, ac[i][j][h], controller));
				 }
 				 else
    				{
      					Ptr<ns3::ofi::LearningController> controller = CreateObject<ns3::ofi::LearningController> ();
      					if (!timeout.IsZero ()) controller->SetAttribute ("ExpirationTime", TimeValue (timeout));
      					test1[i][j][h].Add (swtch.Install (switchNode, ac[i][j][h], controller));
					test1[i][j][h].Add (swtch.Install (switchNode2, ac[i][j][h], controller));
				}
*/
				int second_octet = k+i;		
				int third_octet = j;
				//Assign subnet
				char *subnet;
				subnet = toString(10, second_octet, third_octet, 0);
				//Assign base
				char *base;
				base = toString(0, 0, 0, fourth_octet);
				address.SetBase (subnet, "255.255.255.0",base);
				ipCaContainer[i][j][h] = address.Assign(ca[i][j][h]);
				fourth_octet +=2;
			}
		}
	}
	std::cout << "Finished connecting core switches and aggregation switches  "<< "\n";
	std::cout << "------------- "<<"\n";

//=========== Set netAnim posisitons ===========//
	for (int f=1; f<=total_host;){	
		for (i=0;i<num_pod;i++){
			for (j=0;j<num_agg;j++){
				for (h=0;h<num_edge;h++){
				
	ns3::AnimationInterface::SetConstantPosition(host[i][j].Get(h), f, 1,0);
	f++;	
							}
						}
					}
				}
	
	for (double g=1.5; g<=total_host;){	
		for (i=0;i<num_pod;i++){
			for (j=0;j<num_edge;j++){
	  	ns3::AnimationInterface::SetConstantPosition(edge[i].Get(j), g, 3,0);
	  	ns3::AnimationInterface::SetConstantPosition(agg[i].Get(j), g, 5,0);
		g = g + 2;
						}
					}
				}
		
	for (int f=3; f<=total_host;){	
			for (i=0;i<num_group;i++){
				for (j=0;j<num_core;j++){
  			ns3::AnimationInterface::SetConstantPosition(core[i].Get(j), f - 12, 7,0);
			f = f + 3;
							}
						}
					}


//=========== Start the simulation ===========//
//
	std::cout << "Start Simulation.. "<<"\n";
	for (i=0;i<total_host;i++){
		app[i].Start (Seconds (0.0));
  		app[i].Stop (Seconds (2.0));
	}
 	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

// Calculate Throughput using Flowmonitor
//
  	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();
	
// Run simulation.
//

//	AnimationInterface anim (animFile);
	std::cout << "Start Animation.. "<<"\n";

  	NS_LOG_INFO ("Run Simulation.");
  	Simulator::Stop (Seconds(2.5));
  	Simulator::Run ();

  	monitor->CheckForLostPackets ();

  	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
    		{
	  	Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

        //ofstream myfile;
	//myfile.open("results.xls");
    	  	NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
    	  	NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
    	  	NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
    	  	NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps");
	//myfile.close();
        
    		}
  	monitor->SerializeToXmlFile(filename, true, true);
	std::cout << "Simulation finished "<<"\n";

  	Simulator::Destroy ();
  	NS_LOG_INFO ("Done.");

	return 0;
}
