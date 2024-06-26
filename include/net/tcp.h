#ifndef __MYOS__NET__TCP_H
#define __MYOS__NET__TCP_H


#include <common/types.h>
#include <net/ipv4.h>
#include <memorymanagement.h>


namespace myos
{
    namespace net
    {
        /* enumeration for each of the possible states of the TCP protocol (look at diagrams online) */
        enum TransmissionControlProtocolSocketState
        {
            CLOSED,
            LISTEN,
            SYN_SENT,
            SYN_RECEIVED,
            
            ESTABLISHED,
            
            FIN_WAIT1,
            FIN_WAIT2,
            CLOSING,
            TIME_WAIT,
            
            CLOSE_WAIT
            //LAST_ACK
        };
        
        enum TransmissionControlProtocolFlag
        {
            FIN = 1,
            SYN = 2,
            RST = 4,
            PSH = 8,
            ACK = 16,
            URG = 32,
            ECE = 64,
            CWR = 128,
            NS = 256
        };
        
        struct TransmissionControlProtocolHeader
        {
            common::uint16_t srcPort;
            common::uint16_t dstPort;
            common::uint32_t sequenceNumber;
            common::uint32_t acknowledgementNumber;
            
            common::uint8_t reserved : 4;
            common::uint8_t headerSize32 : 4;
            common::uint8_t flags;
            
            common::uint16_t windowSize;
            common::uint16_t checksum;
            common::uint16_t urgentPtr;
            
            common::uint32_t options;
        } __attribute__((packed));
       
      
        /* this pseudo header is attached to the IPv4 message when sending data from TCP */
        struct TransmissionControlProtocolPseudoHeader
        {
            common::uint32_t srcIP;
            common::uint32_t dstIP;
            common::uint16_t protocol;
            common::uint16_t totalLength;
        } __attribute__((packed));
      
      
        class TransmissionControlProtocolSocket;
        class TransmissionControlProtocolProvider;
        
        class TransmissionControlProtocolHandler
        {
        public:
            TransmissionControlProtocolHandler();
            ~TransmissionControlProtocolHandler();
            virtual bool HandleTransmissionControlProtocolMessage(TransmissionControlProtocolSocket* socket, common::uint8_t* data, common::uint16_t size);
        };
      
        
      
        class TransmissionControlProtocolSocket
        {
        friend class TransmissionControlProtocolProvider;
        protected:
            common::uint16_t remotePort;
            common::uint32_t remoteIP;
            common::uint16_t localPort;
            common::uint32_t localIP;
            /* the socket needs to know which number to send next as the sequence number therefore we need to store our own sequence number */
            common::uint32_t sequenceNumber;
            common::uint32_t acknowledgementNumber;

            TransmissionControlProtocolProvider* backend;
            TransmissionControlProtocolHandler* handler;
            
            TransmissionControlProtocolSocketState state;
        public:
            TransmissionControlProtocolSocket(TransmissionControlProtocolProvider* backend);
            ~TransmissionControlProtocolSocket();
            virtual bool HandleTransmissionControlProtocolMessage(common::uint8_t* data, common::uint16_t size);
            virtual void Send(common::uint8_t* data, common::uint16_t size);
            virtual void Disconnect();
        };
      
      
        class TransmissionControlProtocolProvider : InternetProtocolHandler
        {
        protected:
            TransmissionControlProtocolSocket* sockets[65535];
            common::uint16_t numSockets;
            common::uint16_t freePort;
            
        public:
            TransmissionControlProtocolProvider(InternetProtocolProvider* backend);
            ~TransmissionControlProtocolProvider();
            
            virtual bool OnInternetProtocolReceived(common::uint32_t srcIP_BE, common::uint32_t dstIP_BE,
                                                    common::uint8_t* internetprotocolPayload, common::uint32_t size);

            virtual TransmissionControlProtocolSocket* Connect(common::uint32_t ip, common::uint16_t port);
            virtual void Disconnect(TransmissionControlProtocolSocket* socket);
            virtual void Send(TransmissionControlProtocolSocket* socket, common::uint8_t* data, common::uint16_t size,
                              common::uint16_t flags = 0);

            virtual TransmissionControlProtocolSocket* Listen(common::uint16_t port);
            virtual void Bind(TransmissionControlProtocolSocket* socket, TransmissionControlProtocolHandler* handler);
        };   
    }
}

#endif
