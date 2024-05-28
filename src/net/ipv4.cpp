#include <net/ipv4.h>

using namespace myos;
using namespace myos::common;
using namespace myos::net;

        
        
            
InternetProtocolHandler::InternetProtocolHandler(InternetProtocolProvider* backend, uint8_t protocol)
{
    this->backend = backend;
    this->ip_protocol = protocol;
    /* we register ourself as handler for the protocol (which will be used by the next "higher level" protocol) */
    backend->handlers[protocol] = this;
}

InternetProtocolHandler::~InternetProtocolHandler()
{
    if(backend->handlers[ip_protocol] == this)
        backend->handlers[ip_protocol] = 0;
}
            
bool InternetProtocolHandler::OnInternetProtocolReceived(uint32_t srcIP_BE, uint32_t dstIP_BE,
                                            uint8_t* internetprotocolPayload, uint32_t size)
{
    return false;
}

void InternetProtocolHandler::Send(uint32_t dstIP_BE, uint8_t* internetprotocolPayload, uint32_t size)
{
    backend->Send(dstIP_BE, ip_protocol, internetprotocolPayload, size);
}


     

InternetProtocolProvider::InternetProtocolProvider(EtherFrameProvider* backend, 
                                                   AddressResolutionProtocol* arp,
                                                   uint32_t gatewayIP, uint32_t subnetMask)
: EtherFrameHandler(backend, 0x800)
{
    for(int i = 0; i < 255; i++)
        handlers[i] = 0;
    this->arp = arp;
    this->gatewayIP = gatewayIP;
    this->subnetMask = subnetMask;
}

InternetProtocolProvider::~InternetProtocolProvider()
{
}
            
bool InternetProtocolProvider::OnEtherFrameReceived(uint8_t* etherframePayload, uint32_t size)
{
    if(size < sizeof(InternetProtocolV4Message))
        return false;
    
    InternetProtocolV4Message* ipmessage = (InternetProtocolV4Message*)etherframePayload;
    bool sendBack = false;
    
    /* if the message is for us */
    if(ipmessage->dstIP == backend->GetIPAddress())
    {
        int length = ipmessage->totalLength;
        if(length > size)
            length = size; /* size is the length of the ethernet frame */
        
        /* if we have a handler for that protocol */
        if(handlers[ipmessage->protocol] != 0)
            sendBack = handlers[ipmessage->protocol]->OnInternetProtocolReceived(
                ipmessage->srcIP, ipmessage->dstIP, 
                /* the payload is after the header, but we have optional fields, so we use the headerLenght multiplied by 4 (32 bits) */
                etherframePayload + 4*ipmessage->headerLength, length - 4*ipmessage->headerLength);
        
    }
    
    if(sendBack)
    {
        uint32_t temp = ipmessage->dstIP;
        ipmessage->dstIP = ipmessage->srcIP;
        ipmessage->srcIP = temp;
        
        ipmessage->timeToLive = 0x40; /* reset TTL */
        ipmessage->checksum = 0;
        /* checksum must be correct otherwise IPv4 discards the whole packet */
        ipmessage->checksum = Checksum((uint16_t*)ipmessage, 4*ipmessage->headerLength);
    }
    
    return sendBack;
}


void InternetProtocolProvider::Send(uint32_t dstIP_BE, uint8_t protocol, uint8_t* data, uint32_t size)
{
    
    uint8_t* buffer = (uint8_t*)MemoryManager::activeMemoryManager->malloc(sizeof(InternetProtocolV4Message) + size);
    InternetProtocolV4Message *message = (InternetProtocolV4Message*)buffer;
    
    message->version = 4;
    message->headerLength = sizeof(InternetProtocolV4Message)/4;
    message->tos = 0;
    message->totalLength = size + sizeof(InternetProtocolV4Message);
    message->totalLength = ((message->totalLength & 0xFF00) >> 8) /* switch the bytes to move to little endian */
                         | ((message->totalLength & 0x00FF) << 8);
    message->ident = 0x0100;
    message->flagsAndOffset = 0x0040;
    message->timeToLive = 0x40;
    message->protocol = protocol;
    
    message->dstIP = dstIP_BE;
    message->srcIP = backend->GetIPAddress();
    
    message->checksum = 0;
    message->checksum = Checksum((uint16_t*)message, sizeof(InternetProtocolV4Message));

    /* header is now finished, copy message to sent */
    
    uint8_t* databuffer = buffer + sizeof(InternetProtocolV4Message);
    for(int i = 0; i < size; i++)
        databuffer[i] = data[i];
    /*
    By default we want to send the data to the destination. Route is the IP address that we resolve(). But if the IP is not in our
    local net, (we understand it via the subnet) than we need to send the message to the gateway which will handle the sending on our 
    behalf.
    */
    uint32_t route = dstIP_BE;
    if((dstIP_BE & subnetMask) != (message->srcIP & subnetMask))
        route = gatewayIP;
    
    /* we ask the ARP for the MAC address that we then pass to the Ethernet handler (can cause infinite loop as described) */
    backend->Send(arp->Resolve(route), this->etherType_BE, buffer, sizeof(InternetProtocolV4Message) + size);
    
    /* once the data has been sent, we can free the buffer */
    MemoryManager::activeMemoryManager->free(buffer);
}


uint16_t InternetProtocolProvider::Checksum(uint16_t* data, uint32_t lengthInBytes)
{
    uint32_t temp = 0;

    for(int i = 0; i < lengthInBytes/2; i++)
        /* we add the data in big endian. */
        temp += ((data[i] & 0xFF00) >> 8) | ((data[i] & 0x00FF) << 8);

    /* If the data we send is an odd number, there will be one byte left at the end */
    if(lengthInBytes % 2)
        /* cast to an array of single byte values and then take the last one  and then shift to the left */
        temp += ((uint16_t)((char*)data)[lengthInBytes-1]) << 8;
    
    /* if we have an overflow when we sum this up, we take the overflowed bits and add it until the last 16 bits are all zeros */
    while(temp & 0xFFFF0000)
        temp = (temp & 0xFFFF) + (temp >> 16);
    
    /* return the value to big endian */
    return ((~temp & 0xFF00) >> 8) | ((~temp & 0x00FF) << 8);
}



