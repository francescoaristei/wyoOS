 
#ifndef __MYOS__NET__UDP_H
#define __MYOS__NET__UDP_H


#include <common/types.h>
#include <net/ipv4.h>
#include <memorymanagement.h>

namespace myos
{
    namespace net
    {
        
        struct UserDatagramProtocolHeader
        {
            common::uint16_t srcPort;
            common::uint16_t dstPort;
            common::uint16_t length;
            common::uint16_t checksum;
        } __attribute__((packed));
       
      
        /* when an application wants to connect to another machine, the UDP creates a socket */
        class UserDatagramProtocolSocket;
        class UserDatagramProtocolProvider;
        
        
        
        class UserDatagramProtocolHandler
        {
        public:
            UserDatagramProtocolHandler();
            ~UserDatagramProtocolHandler();
            /* the socket passes its information to the UDP handler */
            virtual void HandleUserDatagramProtocolMessage(UserDatagramProtocolSocket* socket, common::uint8_t* data, common::uint16_t size);
        };
      
        
      
        class UserDatagramProtocolSocket
        {
        /* data flow: UDP class --> Socket class --> UDP Handler class */
        friend class UserDatagramProtocolProvider;
        protected:
            /* the socket will mantain some information of the UDP message */
            common::uint16_t remotePort;
            common::uint32_t remoteIP;
            common::uint16_t localPort;
            common::uint32_t localIP;
            UserDatagramProtocolProvider* backend;
            UserDatagramProtocolHandler* handler;
            bool listening;
        public:
            UserDatagramProtocolSocket(UserDatagramProtocolProvider* backend);
            ~UserDatagramProtocolSocket();
            virtual void HandleUserDatagramProtocolMessage(common::uint8_t* data, common::uint16_t size);
            virtual void Send(common::uint8_t* data, common::uint16_t size);
            virtual void Disconnect();
        };
      
      
        class UserDatagramProtocolProvider : InternetProtocolHandler /* the UDP takes an IPv4 packet in input */
        {
        protected:
            /* array of sockets */
            UserDatagramProtocolSocket* sockets[65535];
            common::uint16_t numSockets;
            /* free ports on the local machine (my machine) */
            common::uint16_t freePort;
            
        public:
            UserDatagramProtocolProvider(InternetProtocolProvider* backend);
            ~UserDatagramProtocolProvider();
            
            virtual bool OnInternetProtocolReceived(common::uint32_t srcIP_BE, common::uint32_t dstIP_BE,
                                                    common::uint8_t* internetprotocolPayload, common::uint32_t size);

            /* given a certain IP and Port gives us the socket */
            virtual UserDatagramProtocolSocket* Connect(common::uint32_t ip, common::uint16_t port);
            virtual UserDatagramProtocolSocket* Listen(common::uint16_t port);
            virtual void Disconnect(UserDatagramProtocolSocket* socket);
            virtual void Send(UserDatagramProtocolSocket* socket, common::uint8_t* data, common::uint16_t size);
            virtual void Bind(UserDatagramProtocolSocket* socket, UserDatagramProtocolHandler* handler);
        };
        
        
    }
}

#endif