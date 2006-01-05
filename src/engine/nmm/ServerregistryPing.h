#ifndef SERVERREGISTRYPING_H 
#define SERVERREGISTRYPING_H 

#include <qsocket.h>

/**
 * Connects to a remote host on the default NMM serverregistry port.
 */
class ServerregistryPing
    : public QSocket
{
    Q_OBJECT

    public:
        ServerregistryPing(const QString & host, Q_UINT16 port = 22801);

    private slots:
        void socketConnected();
        void socketConnectionClosed();
        void socketError(int);

    signals:
        /**
         * This signal is emitted when the serverregistry gets available/unavailable.
         */
        void registryAvailable(bool);
};

#endif
