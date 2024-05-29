 
#ifndef __MYOS__NET__ICMP_H
#define __MYOS__NET__ICMP_H

#include <common/types.h>
#include <net/ipv4.h>


namespace myos
{
    namespace net
    {
        
        /* struct of the ICMP message */
        struct InternetControlMessageProtocolMessage
        {
            common::uint8_t type;
            common::uint8_t code;
            
            common::uint16_t checksum;
            common::uint32_t data;

        } __attribute__((packed));
        
        class InternetControlMessageProtocol : InternetProtocolHandler
        {
        public:
            InternetControlMessageProtocol(InternetProtocolProvider* backend);
            ~InternetControlMessageProtocol();
            
            /* overridden from InternetProtocolHandler */
            bool OnInternetProtocolReceived(common::uint32_t srcIP_BE, common::uint32_t dstIP_BE,
                                            common::uint8_t* internetprotocolPayload, common::uint32_t size);

            /* request a PING it takes as parameter the IP that we want to PING */
            void RequestEchoReply(common::uint32_t ip_be);
        };
        
        
    }
}


#endif