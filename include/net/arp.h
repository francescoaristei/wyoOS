 
#ifndef __MYOS__NET__ARP_H
#define __MYOS__NET__ARP_H


#include <common/types.h>
#include <net/etherframe.h>


namespace myos
{
    namespace net
    {
        
        struct AddressResolutionProtocolMessage
        {
            common::uint16_t hardwareType;
            common::uint16_t protocol;
            common::uint8_t hardwareAddressSize; // 6
            common::uint8_t protocolAddressSize; // 4
            common::uint16_t command;
            

            common::uint64_t srcMAC : 48;
            common::uint32_t srcIP;
            common::uint64_t dstMAC : 48;
            common::uint32_t dstIP;
            
        } __attribute__((packed));
        
        class AddressResolutionProtocol : public EtherFrameHandler
        {
            
            common::uint32_t IPcache[128];
            common::uint64_t MACcache[128];
            int numCacheEntries;
            
        public:
            AddressResolutionProtocol(EtherFrameProvider* backend);
            ~AddressResolutionProtocol();
            
            bool OnEtherFrameReceived(common::uint8_t* etherframePayload, common::uint32_t size);

            /*
            Request MAC address for the IP address passed as parameter, when we receive an answer we store the MAC address in the MACcache 
            when we get the answer, this is goind in the onEtherFrameReceived method which stores it and then we have the GetMACFromCache which
            looks in the cache to get the MAC address.
            */
            void RequestMACAddress(common::uint32_t IP_BE);
            common::uint64_t GetMACFromCache(common::uint32_t IP_BE);
            common::uint64_t Resolve(common::uint32_t IP_BE);
            void BroadcastMACAddress(common::uint32_t IP_BE);
        };
    }
}

#endif