/*
Matt Noblett
Troy Veldhuizen
Matt Pairitz
*/
226 lines (193 sloc)  6.93 KB
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/if_ether.h>
#include <netinet/ether.h>
#include <string.h>

int main(){
  int packet_socket;
  unsigned char* ifmacaddr;
  //get list of interfaces (actually addresses)
  struct ifaddrs *ifaddr, *tmp;
  if(getifaddrs(&ifaddr)==-1){
    perror("getifaddrs");
    return 1;
  }
  //have the list, loop over the list
  for(tmp = ifaddr; tmp!=NULL; tmp=tmp->ifa_next){
    //Check if this is a packet address, there will be one per
    //interface.  There are IPv4 and IPv6 as well, but we don't care
    //about those for the purpose of enumerating interfaces. We can
    //use the AF_INET addresses in this list for example to get a list
    //of our own IP addresses
    if(tmp->ifa_addr->sa_family==AF_PACKET){
      //create a packet socket on interface r?-eth1
      if(!strncmp(&(tmp->ifa_name[3]),"eth1",4)){
	printf("Creating Socket on interface %s\n",tmp->ifa_name);

	//create a packet socket
	//AF_PACKET makes it a packet socket
	//SOCK_RAW makes it so we get the entire packet
	//could also use SOCK_DGRAM to cut off link layer header
	//ETH_P_ALL indicates we want all (upper layer) protocols
	//we could specify just a specific one
	packet_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(packet_socket<0){
	  perror("socket");
	  return 2;
	}
	//Bind the socket to the address, so we only get packets
	//recieved on this specific interface. For packet sockets, the
	//address structure is a struct sockaddr_ll (see the man page
	//for "packet"), but of course bind takes a struct sockaddr.
	//Here, we can use the sockaddr we got from getifaddrs (which
	//we could convert to sockaddr_ll if we needed to)
	if(bind(packet_socket,tmp->ifa_addr,sizeof(struct sockaddr_ll))==-1){
	  perror("bind");
	}

	struct sockaddr_ll *macsoc  = (struct sockaddr_ll *) tmp->ifa_addr;
	ifmacaddr = (unsigned char *) macsoc->sll_addr;
      }
    }
  }
  //free the interface list when we don't need it anymore
  //freeifaddrs(ifaddr);

  //loop and recieve packets. We are only looking at one interface,
  //for the project you will probably want to look at more (to do so,
  //a good way is to have one socket per interface and use select to
  //see which ones have data)
  printf("Ready to recieve now\n");
  while(1){
    char buf[1500];
    struct sockaddr_ll recvaddr;
    int recvaddrlen=sizeof(struct sockaddr_ll);
    //we can use recv, since the addresses are in the packet, but we
    //use recvfrom because it gives us an easy way to determine if
    //this packet is incoming or outgoing (when using ETH_P_ALL, we
    //see packets in both directions. Only outgoing can be seen when
    //using a packet socket with some specific protocol)
    int n = recvfrom(packet_socket, buf, 1500,0,(struct sockaddr*)&recvaddr, &recvaddrlen);
    //ignore outgoing packets (we can't disable some from being sent
    //by the OS automatically, for example ICMP port unreachable
    //messages, so we will just ignore them here)
    if(recvaddr.sll_pkttype==PACKET_OUTGOING)
      continue;
    //start processing all others
    printf("Got a %d byte packet\n", n);

    // extract Ethernet information
    char* tempEth;
    unsigned short tempType;
    struct ether_header *eth = (struct ether_header*)buf;

    tempEth=ether_ntoa((struct ether_addr*) &eth->ether_dhost);
    printf("Destination address: %s\n", tempEth);
    tempEth=ether_ntoa((struct ether_addr*) &eth->ether_shost);
    printf("Source address: %s\n", tempEth);
    printf("Type: %04hx\n", ntohs(eth->ether_type));
    int size = sizeof(eth->ether_dhost)+sizeof(eth->ether_shost)+sizeof(eth->ether_type);
    printf("Size of Eth header: %d\n", size);

    struct arpheader {
    	unsigned short int   hardware_type;
    	unsigned short int   protocol_type;
    	unsigned char        hardware_addr_length;
    	unsigned char        protocol_addr_length;
    	unsigned short int   op;
    	unsigned char        sha[6];
    	unsigned char        spa[4];
    	unsigned char        dha[6];
    	unsigned char        dpa[4];
    };

    struct arpheader arp;

    memcpy(&arp, &buf[sizeof(struct ether_header)], sizeof(struct arpheader));

    printf("SENDER MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
      arp.sha[0],
      arp.sha[1],
      arp.sha[2],
      arp.sha[3],
      arp.sha[4],
      arp.sha[5]
    );

    printf("SENDER IP address: %02d:%02d:%02d:%02d\n",
      arp.spa[0],
      arp.spa[1],
      arp.spa[2],
      arp.spa[3]
    );

    unsigned char routerMac[6];


    printf("Router MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
      ifmacaddr[0],
      ifmacaddr[1],
      ifmacaddr[2],
      ifmacaddr[3],
      ifmacaddr[4],
      ifmacaddr[5]
    );
    // setup responce packet

    //arp
    arp.op=(htons(2));
    memcpy(arp.dha, arp.sha, 6);
    memcpy(arp.sha, ifmacaddr, 6);

    // protocal addrs
    unsigned char tmpaddr[4];
    memcpy(tmpaddr, arp.dpa, 4);
    memcpy(arp.dpa, arp.spa, 4);
    memcpy(arp.spa, tmpaddr, 4);

    // ether

    //memcpy(eth->ether_dhost, recvaddr.sll_addr, 6);

    memcpy(eth->ether_dhost, eth->ether_shost, 6);

    memcpy(eth->ether_shost, ifmacaddr, 6);
    // add to buffer

    printf("ether saddr: %02X:%02X:%02X:%02X:%02X:%02X",
	eth->ether_shost[0], eth->ether_shost[1], eth->ether_shost[2],
	eth->ether_shost[3], eth->ether_shost[4], eth->ether_shost[5]);


    printf("ether daddr: %02X:%02X:%02X:%02X:%02X:%02X",
	eth->ether_dhost[0], eth->ether_dhost[1], eth->ether_dhost[2],
	eth->ether_dhost[3], eth->ether_dhost[4], eth->ether_dhost[5]);


    char responce[42];
    memcpy(responce, eth, 14);
    memcpy(&responce[14], &arp, 28);

    printf("Size of eth + arp is: %d\n", (int) sizeof(responce));
      // target is source comp
      /*
      arp.op=(htons(2));
      char* temp_mac = getmac("eth0");
      memcpy(arp.dest_hardware_addr, arp.src_hardware_addr, 6);
      memcpy(arp.src_hardware_addr, temp_mac, 6);
      printf("%s\n", temp_mac);
    */
    // find that packet is for us
    // create respond arp packet
    // send response

    int b = send(packet_socket, responce, 42, 0);

    printf("%d bytes sent back", b);


    //what else to do is up to you, you can send packets with send,
    //just like we used for TCP sockets (or you can use sendto, but it
    //is not necessary, since the headers, including all addresses,
    //need to be in the buffer you are sending)

  }
  //exit
  return 0;
}
Contact GitHub API Training Shop Blog About
© 2017 GitHub, Inc. Terms Privacy Security Status Help
