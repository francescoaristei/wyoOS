 

#include <net/tcp.h>

using namespace myos;
using namespace myos::common;
using namespace myos::net;



TransmissionControlProtocolHandler::TransmissionControlProtocolHandler()
{
}

TransmissionControlProtocolHandler::~TransmissionControlProtocolHandler()
{
}

bool TransmissionControlProtocolHandler::HandleTransmissionControlProtocolMessage(TransmissionControlProtocolSocket* socket, uint8_t* data, uint16_t size)
{
    return true;
}

TransmissionControlProtocolSocket::TransmissionControlProtocolSocket(TransmissionControlProtocolProvider* backend)
{
    this->backend = backend;
    handler = 0;
    state = CLOSED;
}

TransmissionControlProtocolSocket::~TransmissionControlProtocolSocket()
{
}

bool TransmissionControlProtocolSocket::HandleTransmissionControlProtocolMessage(uint8_t* data, uint16_t size)
{
    if(handler != 0)
        return handler->HandleTransmissionControlProtocolMessage(this, data, size);
    return false;
}

void TransmissionControlProtocolSocket::Send(uint8_t* data, uint16_t size)
{
    while(state != ESTABLISHED)
    {
    }
    backend->Send(this, data, size, PSH|ACK);
}

void TransmissionControlProtocolSocket::Disconnect()
{
    backend->Disconnect(this);
}


TransmissionControlProtocolProvider::TransmissionControlProtocolProvider(InternetProtocolProvider* backend)
: InternetProtocolHandler(backend, 0x06)
{
    for(int i = 0; i < 65535; i++)
        sockets[i] = 0;
    numSockets = 0;
    freePort = 1024;
}

TransmissionControlProtocolProvider::~TransmissionControlProtocolProvider()
{
}

uint32_t bigEndian32(uint32_t x)
{
    return ((x & 0xFF000000) >> 24)
         | ((x & 0x00FF0000) >> 8)
         | ((x & 0x0000FF00) << 8)
         | ((x & 0x000000FF) << 24);
}



bool TransmissionControlProtocolProvider::OnInternetProtocolReceived(uint32_t srcIP_BE, uint32_t dstIP_BE,
                                        uint8_t* internetprotocolPayload, uint32_t size)
{
    /* the size needs to be at least 20 bytes because is the smallest size for a TCP header */
    if(size < 20)
        return false;
    TransmissionControlProtocolHeader* msg = (TransmissionControlProtocolHeader*)internetprotocolPayload;

    uint16_t localPort = msg->dstPort;
    uint16_t remotePort = msg->srcPort;
    
    TransmissionControlProtocolSocket* socket = 0;
    /* we look at our sockets and find the socket which is responsible for the handling of this message */
    for(uint16_t i = 0; i < numSockets && socket == 0; i++)
    {
        if( sockets[i]->localPort == msg->dstPort
        &&  sockets[i]->localIP == dstIP_BE
        &&  sockets[i]->state == LISTEN
        && (((msg -> flags) & (SYN | ACK)) == SYN))
            socket = sockets[i];
        else if( sockets[i]->localPort == msg->dstPort
        &&  sockets[i]->localIP == dstIP_BE
        &&  sockets[i]->remotePort == msg->srcPort
        &&  sockets[i]->remoteIP == srcIP_BE)
            socket = sockets[i];
    }

    
        
    bool reset = false;
    
    /* if we get a reset we do not go in the switch cases, and the socket is immediately removed from our list of sockets */
    if(socket != 0 && msg->flags & RST)
        socket->state = CLOSED;

    
    if(socket != 0 && socket->state != CLOSED)
    {
        switch((msg -> flags) & (SYN | ACK | FIN))
        {
            case SYN:
                /* if we are a server and we are listening and we receive a message we send a SYN|ACK */
                if(socket -> state == LISTEN)
                {
                    socket->state = SYN_RECEIVED;
                    socket->remotePort = msg->srcPort;
                    socket->remoteIP = srcIP_BE;
                    socket->acknowledgementNumber = bigEndian32( msg->sequenceNumber ) + 1;
                    socket->sequenceNumber = 0xbeefcafe;
                    Send(socket, 0,0, SYN|ACK);
                    socket->sequenceNumber++;
                }
                else
                    /* if someone tries to connect with us without us listening than we close the connection */
                    reset = true;
                break;

                
            case SYN | ACK:
                /* we should send a SYN | ACK only if we have sent a SYN before, otherwise close the connection */
                if(socket->state == SYN_SENT)
                {
                    socket->state = ESTABLISHED;
                    socket->acknowledgementNumber = bigEndian32( msg->sequenceNumber ) + 1;
                    socket->sequenceNumber++;
                    Send(socket, 0,0, ACK);
                }
                else
                    reset = true;
                break;
                
            /* cannot SYN and FIN in the same message */
            case SYN | FIN:
            case SYN | FIN | ACK:
                reset = true;
                break;

            
            /* FIN and ACK are treated in the same way */
            case FIN:
            case FIN|ACK:
                /* if i am currently established i shouldn't receive a FIN |ACK unless the other doesn't want to disconnect */
                if(socket->state == ESTABLISHED)
                {
                    socket->state = CLOSE_WAIT;
                    socket->acknowledgementNumber++;
                    /* first send an ACK, then a FIN | ACK */
                    Send(socket, 0,0, ACK);
                    Send(socket, 0,0, FIN|ACK);
                }
                else if(socket->state == CLOSE_WAIT)
                {
                    socket->state = CLOSED;
                }
                /* we have sent a FIN, we have receive a FIN |ACK so we just have to send an ACK */
                else if(socket->state == FIN_WAIT1
                    || socket->state == FIN_WAIT2)
                {
                    socket->state = CLOSED;
                    socket->acknowledgementNumber++;
                    Send(socket, 0,0, ACK);
                }
                else
                    reset = true;
                break;
                
                
            case ACK:
                /* we were listening before, we have receive the FIN and send the ACK so now we move in established */
                if(socket->state == SYN_RECEIVED)
                {
                    socket->state = ESTABLISHED;
                    return false;
                }
                else if(socket->state == FIN_WAIT1)
                {
                    socket->state = FIN_WAIT2;
                    return false;
                }
                else if(socket->state == CLOSE_WAIT)
                {
                    socket->state = CLOSED;
                    break;
                }
                
                if(msg->flags == ACK)
                    break;
                
                // no break, because of piggybacking (send the data together with the ACK of the previous message received)
                
            default:
                
                /* we handle the data that we get */
                if(bigEndian32(msg->sequenceNumber) == socket->acknowledgementNumber) /* if our sequence number and the ack number are different means that we have receive the msgs in wrong order and we cannot handle the msg yet */
                {
                    /* the handler might say: kill this connection or keep the connection alive */
                    reset = !(socket->HandleTransmissionControlProtocolMessage(internetprotocolPayload + msg->headerSize32*4,
                                                                              size - msg->headerSize32*4));
                    /* if we do not reset, we ack the data received */
                    if(!reset)
                    {
                        int x = 0;
                        for(int i = msg->headerSize32*4; i < size; i++)
                            if(internetprotocolPayload[i] != 0)
                                x = i;
                        socket->acknowledgementNumber += x - msg->headerSize32*4 + 1;
                        Send(socket, 0,0, ACK);
                    }
                }
                else
                {
                    // data in wrong order
                    reset = true;
                }
                
        }
    }
    
    
    if(reset)
    {
        /* if we have a socket and receive illegal data on it we send back the reset */
        if(socket != 0)
        {
            Send(socket, 0,0, RST);
        }
        else
        {
            TransmissionControlProtocolSocket socket(this);
            socket.remotePort = msg->srcPort;
            socket.remoteIP = srcIP_BE;
            socket.localPort = msg->dstPort;
            socket.localIP = dstIP_BE;
            socket.sequenceNumber = bigEndian32(msg->acknowledgementNumber);
            socket.acknowledgementNumber = bigEndian32(msg->sequenceNumber) + 1;
            Send(&socket, 0,0, RST);
        }
    }
    

    if(socket != 0 && socket->state == CLOSED)
        for(uint16_t i = 0; i < numSockets && socket == 0; i++)
            if(sockets[i] == socket)
            {
                sockets[i] = sockets[--numSockets];
                MemoryManager::activeMemoryManager->free(socket);
                break;
            }
    
    
    
    return false;
}

// ------------------------------------------------------------------------------------------


void TransmissionControlProtocolProvider::Send(TransmissionControlProtocolSocket* socket, uint8_t* data, uint16_t size, uint16_t flags)
{
    uint16_t totalLength = size + sizeof(TransmissionControlProtocolHeader);
    uint16_t lengthInclPHdr = totalLength + sizeof(TransmissionControlProtocolPseudoHeader);
    
    uint8_t* buffer = (uint8_t*)MemoryManager::activeMemoryManager->malloc(lengthInclPHdr);
    
    TransmissionControlProtocolPseudoHeader* phdr = (TransmissionControlProtocolPseudoHeader*)buffer;
    TransmissionControlProtocolHeader* msg = (TransmissionControlProtocolHeader*)(buffer + sizeof(TransmissionControlProtocolPseudoHeader));
    /* buffer of the data */
    uint8_t* buffer2 = buffer + sizeof(TransmissionControlProtocolHeader)
                              + sizeof(TransmissionControlProtocolPseudoHeader);
    
    msg->headerSize32 = sizeof(TransmissionControlProtocolHeader)/4; // divided by 4 because it is in 32 bit integer size
    msg->srcPort = socket->localPort;
    msg->dstPort = socket->remotePort;
    
    msg->acknowledgementNumber = bigEndian32( socket->acknowledgementNumber );
    msg->sequenceNumber = bigEndian32( socket->sequenceNumber );
    msg->reserved = 0;
    msg->flags = flags;
    msg->windowSize = 0xFFFF;
    msg->urgentPtr = 0;
    
    msg->options = ((flags & SYN) != 0) ? 0xB4050402 : 0;
    
    /* increase the sequence number for the next message by the size of the data we are sending */
    socket->sequenceNumber += size;
        
    for(int i = 0; i < size; i++)
        buffer2[i] = data[i];
    
    phdr->srcIP = socket->localIP;
    phdr->dstIP = socket->remoteIP;
    phdr->protocol = 0x0600; /* number 6 for the IP protocol in the TCP protocol number but written in big endian */
    phdr->totalLength = ((totalLength & 0x00FF) << 8) | ((totalLength & 0xFF00) >> 8);    
    
    msg -> checksum = 0;
    msg -> checksum = InternetProtocolProvider::Checksum((uint16_t*)buffer, lengthInclPHdr);

    InternetProtocolHandler::Send(socket->remoteIP, (uint8_t*)msg, totalLength);
    MemoryManager::activeMemoryManager->free(buffer);
}



TransmissionControlProtocolSocket* TransmissionControlProtocolProvider::Connect(uint32_t ip, uint16_t port)
{
    TransmissionControlProtocolSocket* socket = (TransmissionControlProtocolSocket*)MemoryManager::activeMemoryManager->malloc(sizeof(TransmissionControlProtocolSocket));
    
    if(socket != 0)
    {
        new (socket) TransmissionControlProtocolSocket(this);
        
        socket -> remotePort = port;
        socket -> remoteIP = ip;
        socket -> localPort = freePort++;
        socket -> localIP = backend->GetIPAddress();
        
        socket -> remotePort = ((socket -> remotePort & 0xFF00)>>8) | ((socket -> remotePort & 0x00FF) << 8);
        socket -> localPort = ((socket -> localPort & 0xFF00)>>8) | ((socket -> localPort & 0x00FF) << 8);
        
        sockets[numSockets++] = socket;
        socket -> state = SYN_SENT;
        
        socket -> sequenceNumber = 0xbeefcafe;
        
        /* 
        we send the SYN only after the socket is connected to the TCP socket otherwise we would receive an answer that we wouldn't
        get if we send the SYN before setting the handler correctly 
        */
        Send(socket, 0,0, SYN);
    }
    
    return socket;
}


void TransmissionControlProtocolProvider::Disconnect(TransmissionControlProtocolSocket* socket)
{
    socket->state = FIN_WAIT1;
    Send(socket, 0,0, FIN + ACK);
    socket->sequenceNumber++;
}


TransmissionControlProtocolSocket* TransmissionControlProtocolProvider::Listen(uint16_t port)
{
    TransmissionControlProtocolSocket* socket = (TransmissionControlProtocolSocket*)MemoryManager::activeMemoryManager->malloc(sizeof(TransmissionControlProtocolSocket));
    
    if(socket != 0)
    {
        new (socket) TransmissionControlProtocolSocket(this);
        
        socket -> state = LISTEN;
        socket -> localIP = backend->GetIPAddress();
        socket -> localPort = ((port & 0xFF00)>>8) | ((port & 0x00FF) << 8);
        
        sockets[numSockets++] = socket;
    }
    
    return socket;
}
void TransmissionControlProtocolProvider::Bind(TransmissionControlProtocolSocket* socket, TransmissionControlProtocolHandler* handler)
{
    socket->handler = handler;
}





