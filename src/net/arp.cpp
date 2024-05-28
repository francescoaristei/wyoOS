
#include <net/arp.h>
using namespace myos;
using namespace myos::common;
using namespace myos::net;
using namespace myos::drivers;


AddressResolutionProtocol::AddressResolutionProtocol(EtherFrameProvider* backend)
:  EtherFrameHandler(backend, 0x806) /* 0x806 is the ethertype */
{
    numCacheEntries = 0;
}

AddressResolutionProtocol::~AddressResolutionProtocol()
{
}


bool AddressResolutionProtocol::OnEtherFrameReceived(uint8_t* etherframePayload, uint32_t size)
{
    /* we can cast to the message type only if the message is large enough. */
    if(size < sizeof(AddressResolutionProtocolMessage))
        return false;
    
    /* cast the received message to the message type. */
    AddressResolutionProtocolMessage* arp = (AddressResolutionProtocolMessage*)etherframePayload;
    if(arp->hardwareType == 0x0100)
    {
        /* if we understand the message and it is for us. */
        if(arp->protocol == 0x0008
        && arp->hardwareAddressSize == 6
        && arp->protocolAddressSize == 4
        && arp->dstIP == backend->GetIPAddress())
        {
            /* we look what is the command */
            switch(arp->command)
            {
                case 0x0100: // request
                    arp->command = 0x0200;
                    /* we turn the sender into the recepient and viceversa. */
                    arp->dstIP = arp->srcIP;
                    arp->dstMAC = arp->srcMAC;
                    arp->srcIP = backend->GetIPAddress();
                    arp->srcMAC = backend->GetMACAddress();
                    return true;
                    break;
                
                /* vulnerable to ARP spoofing */
                case 0x0200: // response to a request we have made
                    if(numCacheEntries < 128) 
                    {   /* insert the response into our cache. */
                        IPcache[numCacheEntries] = arp->srcIP;
                        MACcache[numCacheEntries] = arp->srcMAC;
                        numCacheEntries++;
                    }
                    break;
            }
        }
        
    }
    
    return false;
}


void AddressResolutionProtocol::BroadcastMACAddress(uint32_t IP_BE)
{
    AddressResolutionProtocolMessage arp;
    arp.hardwareType = 0x0100; // ethernet
    arp.protocol = 0x0008; // ipv4
    arp.hardwareAddressSize = 6; // mac
    arp.protocolAddressSize = 4; // ipv4
    arp.command = 0x0200; // "response"
    
    arp.srcMAC = backend->GetMACAddress();
    arp.srcIP = backend->GetIPAddress();
    arp.dstMAC = Resolve(IP_BE);
    arp.dstIP = IP_BE;
    
    this->Send(arp.dstMAC, (uint8_t*)&arp, sizeof(AddressResolutionProtocolMessage));

}


void AddressResolutionProtocol::RequestMACAddress(uint32_t IP_BE)
{
    
    /* instantiate the message in the stack */
    AddressResolutionProtocolMessage arp;
    arp.hardwareType = 0x0100; // ethernet
    arp.protocol = 0x0008; // ipv4
    arp.hardwareAddressSize = 6; // mac
    arp.protocolAddressSize = 4; // ipv4
    arp.command = 0x0100; // request (command 1 in big endian encoding)
    
    /* ask the etherframe provider for the MAC and IP addresses. */
    arp.srcMAC = backend->GetMACAddress();
    arp.srcIP = backend->GetIPAddress();
    arp.dstMAC = 0xFFFFFFFFFFFF; // broadcast
    arp.dstIP = IP_BE;
    
    /* send to the broadcast address to request the MAC address of the machine we want to communicate with. */
    this->Send(arp.dstMAC, (uint8_t*)&arp, sizeof(AddressResolutionProtocolMessage));

}

/* iterate through the cache, if we find something return it, otherwise return broadcast address . */
uint64_t AddressResolutionProtocol::GetMACFromCache(uint32_t IP_BE)
{
    for(int i = 0; i < numCacheEntries; i++)
        if(IPcache[i] == IP_BE)
            return MACcache[i];
    return 0xFFFFFFFFFFFF; // broadcast address
}

uint64_t AddressResolutionProtocol::Resolve(uint32_t IP_BE)
{
    uint64_t result = GetMACFromCache(IP_BE);
    /* if the MAC is not in the cache. */
    if(result == 0xFFFFFFFFFFFF)
        RequestMACAddress(IP_BE);

    while(result == 0xFFFFFFFFFFFF) // possible infinite loop: if the machine is not connected, we will never get an answer from the request
                                    // and therefore never get the IP in the cache, looping forever.
        result = GetMACFromCache(IP_BE);
    
    return result;
}
