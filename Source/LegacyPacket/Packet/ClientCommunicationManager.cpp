//--------------------------------------------------------------------------------
// ClientCommunicationManager.h
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
#include "Client_PCH.h"
#include "ClientCommunicationManager.h"
#include "Datagram.h"
#include "DatagramPacket.h"
#include "ClientConfig.h"
#include "PacketValidator.h"
#include "DebugInfo.h"
#include "MTestDef.h"

//--------------------------------------------------------------------------------
// Global
//--------------------------------------------------------------------------------
ClientCommunicationManager*	g_pClientCommunicationManager = NULL;

void	SendBugReport(const char *bug, ...);

//--------------------------------------------------------------------------------
// constructor
//--------------------------------------------------------------------------------
ClientCommunicationManager::ClientCommunicationManager ()
    
: m_pDatagramSocket(NULL)
{
    __BEGIN_TRY

	try {
		// create datagram server socket
		m_pDatagramSocket = new DatagramSocket( g_pClientConfig->CLIENT_COMMUNICATION_UDP_PORT );
		
		SocketAPI::setsocketnonblocking_ex( m_pDatagramSocket->getSOCKET(), true );

//		m_pDatagramSocket->
	} catch (Throwable& t)	{
		DEBUG_ADD_FORMAT_ERR("[Error] CCM-%s", t.toString().c_str());
	}

    __END_CATCH
}

//--------------------------------------------------------------------------------
// destructor
//--------------------------------------------------------------------------------
ClientCommunicationManager::~ClientCommunicationManager() {
    __BEGIN_TRY

    if ( m_pDatagramSocket != NULL ) {
        delete m_pDatagramSocket;
        m_pDatagramSocket = NULL;
    }

    __END_CATCH
}


//--------------------------------------------------------------------------------
// send datagram to datagram-socket
//--------------------------------------------------------------------------------
void ClientCommunicationManager::sendDatagram ( Datagram * pDatagram )
    
{
    __BEGIN_TRY

    try
    {
        m_pDatagramSocket->send( pDatagram );
    }
    catch ( ConnectException )
    {
		throw ConnectException( "ClientCommunicationManager::sendDatagram Throw it upward");
    }

    __END_CATCH
}


//--------------------------------------------------------------------------------
// send datagram-packet to datagram-socket
//--------------------------------------------------------------------------------
void ClientCommunicationManager::sendPacket ( const std::string& host , uint port , DatagramPacket * pPacket )
    
{
    __BEGIN_TRY
    __BEGIN_DEBUG

	if (host.size()==0)
	{
		DEBUG_ADD("[Error] ClientCommunicationManager-sendPacket-host NULL");
		return;
	}

    try {

        // Create a datagram object and specify the host and port of the peer to which the data will be transmitted.
        Datagram datagram;

		datagram.setHost(host);
        datagram.setPort(port);

        // Place the datagram packet into the datagram.
        datagram.write(pPacket);

        // Datagrams are transmitted via datagram sockets.
        m_pDatagramSocket->send( &datagram );

		#ifdef __METROTECH_TEST__
			g_UDPTest.UDPPacketSend ++;
		#endif

		#ifdef __DEBUG_OUTPUT__
			DEBUG_ADD_FORMAT("[To] %s(%d)", host.c_str(), port);
			DEBUG_ADD_FORMAT("[Send] %s", pPacket->toString().c_str());
		#endif

    } catch ( Throwable & t ) {
		// -_- Since it's going to bounce anyway, I'll use a string.
		if( strstr( t.toString().c_str(), "InvalidProtocolException") != NULL )
			if( !strstr( t.toString().c_str(), "(datagram)" ) == NULL )
				SendBugReport( t.toString().c_str() );
		
        DEBUG_ADD( t.toString().c_str() );        
    }

    __END_DEBUG
    __END_CATCH
}

//--------------------------------------------------------------------------------
// Update
//--------------------------------------------------------------------------------
void
ClientCommunicationManager::Update()
{	
	const int maxPacket = g_pClientConfig->MAX_PROCESS_PACKET;
	
	for (int i=0; i<maxPacket; i++)
	{
		//DEBUG_ADD_FORMAT("[CC-Update] %d", i);

		Datagram*       pDatagram       = NULL;
		DatagramPacket* pDatagramPacket = NULL;
	
		try
		{
			// Retrieves the datagram object.
			pDatagram = m_pDatagramSocket->receive();

			if (pDatagram==NULL)
				break;

			DEBUG_ADD("[CCM-Update] something");
			
			// Retrieves the datagram packet object.
			pDatagram->read( pDatagramPacket );

			#ifdef __METROTECH_TEST__
				g_UDPTest.UDPPacketRead ++;
			#endif

			if (pDatagramPacket!=NULL)
			{
				#ifdef __DEBUG_OUTPUT__
					DEBUG_ADD_FORMAT("[RECEIVE] %s", pDatagramPacket->toString().c_str());
				#endif

				// Just checking...
				if ( !g_pPacketValidator->isValidPacketID( CPS_CLIENT_COMMUNICATION_NORMAL, pDatagramPacket->getPacketID() ))
				{
					throw InvalidProtocolException("invalid packet ORDER");
				}			

				// Execute the retrieved datagram packet object.
				DEBUG_ADD_FORMAT("[From] %s(%d)", pDatagramPacket->getHost().c_str(),
													pDatagramPacket->getPort());

				pDatagramPacket->execute(NULL);

				#ifdef __METROTECH_TEST__
					g_UDPTest.UDPPacketExecute ++;
				#endif

				// Deletes the datagram packet object.
				delete pDatagramPacket;
			}

			// Deletes the datagram object.
			delete pDatagram;
			
		}
		catch ( Throwable & t )
		{
			// -_- Since it's going to bounce anyway, I'll use a string.
			if( strstr( t.toString().c_str(), "InvalidProtocolException") != NULL )
				if( !strstr( t.toString().c_str(), "(datagram)" ) == NULL )
					SendBugReport( t.toString().c_str() );
		
			DEBUG_ADD( t.toString().c_str() );

			if (pDatagramPacket!=NULL)
				delete pDatagramPacket;

			if (pDatagram!=NULL)
				delete pDatagram;			
		}
	}

}
