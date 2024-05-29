
#include <net/icmp.h>

using namespace myos;
using namespace myos::common;
using namespace myos::net;

 
InternetControlMessageProtocol::InternetControlMessageProtocol(InternetProtocolProvider* backend)
: InternetProtocolHandler(backend, 0x01)
{
}

InternetControlMessageProtocol::~InternetControlMessageProtocol()
{
}
            
void printf(char*);
void printfHex(uint8_t);
bool InternetControlMessageProtocol::OnInternetProtocolReceived(common::uint32_t srcIP_BE, common::uint32_t dstIP_BE,
                                            common::uint8_t* internetprotocolPayload, common::uint32_t size)
{
    /* check if the data is large enough */
    if(size < sizeof(InternetControlMessageProtocolMessage))
        return false;
    
    InternetControlMessageProtocolMessage* msg = (InternetControlMessageProtocolMessage*)internetprotocolPayload;
    
    switch(msg->type)
    {
        /* 0 means we have an answer to a PING */
        case 0:
            printf("ping response from "); printfHex(srcIP_BE & 0xFF);
            printf("."); printfHex((srcIP_BE >> 8) & 0xFF);
            printf("."); printfHex((srcIP_BE >> 16) & 0xFF);
            printf("."); printfHex((srcIP_BE >> 24) & 0xFF);
            printf("\n");
            break;
        
        /* 8 means we are PINGing */
        case 8:
            msg->type = 0; // turn into a response
            msg->checksum = 0;
            msg->checksum = InternetProtocolProvider::Checksum((uint16_t*)msg,
                sizeof(InternetControlMessageProtocolMessage));
            // we want to send the data back hence we return true
            return true;
    }
    
    return false;
}

void InternetControlMessageProtocol::RequestEchoReply(uint32_t ip_be)
{
    InternetControlMessageProtocolMessage icmp;
    icmp.type = 8; // ping
    icmp.code = 0;
    icmp.data = 0x3713; // big endian version of 1337 (it's just data)
    icmp.checksum = 0;
    icmp.checksum = InternetProtocolProvider::Checksum((uint16_t*)&icmp,
        sizeof(InternetControlMessageProtocolMessage));
    
    InternetProtocolHandler::Send(ip_be, (uint8_t*)&icmp, sizeof(InternetControlMessageProtocolMessage));
}
