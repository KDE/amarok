#include "ServerregistryPing.h"

#include "debug.h"

ServerregistryPing::ServerregistryPing(const QString &host, Q_UINT16 port)
    : QSocket()
{
    connect( this, SIGNAL(connected()),
            SLOT(socketConnected()) );
    connect( this, SIGNAL(connectionClosed()),
            SLOT(socketConnectionClosed()) );
    connect( this, SIGNAL(error(int)),
            SLOT(socketError(int)) );
    
    connectToHost(host, port);
}

void ServerregistryPing::socketConnected()
{
    DEBUG_FUNC_INFO
    emit registryAvailable( true );
}

void ServerregistryPing::socketConnectionClosed()
{
    DEBUG_FUNC_INFO
    emit registryAvailable( false );
}

void ServerregistryPing::socketError( int )
{
    DEBUG_FUNC_INFO
    emit registryAvailable( false );
}

#include "ServerregistryPing.moc"
