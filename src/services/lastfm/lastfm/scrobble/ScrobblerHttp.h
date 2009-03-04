/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef SCROBBLER_HTTP_H
#define SCROBBLER_HTTP_H


#include <KDE/KIO/MetaData>
 
#include <QDebug> // leave first, due to QtOverrides
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QUrl>


/** facade pattern base class for QHttp for Scrobbler usage */
class ScrobblerHttp : public QNetworkAccessManager
{
    Q_OBJECT

public:
    void retry();
    QNetworkReply* reply() const { return m_reply; }
    
    // implementing for KIO use
    static KIO::MetaData metaDataForRequest(QNetworkRequest request);

protected:
    ScrobblerHttp( QObject* parent = 0 );

protected slots:
    virtual void request() = 0;

signals:
    void done( const QByteArray& data );

protected:
    // HACK this is until we split out to liblastfm proper
    // reimplementing to use KIO
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0);
    
    QNetworkReply *m_reply;
    class QTimer *m_retry_timer;

private slots:
    void onFinished( QNetworkReply* reply );

private:
    void resetRetryTimer();
};


class ScrobblerPostHttp : public ScrobblerHttp
{
    QString m_path;
    QByteArray m_session;

protected:
    QByteArray m_data;
    QNetworkRequest m_request;

public:
    ScrobblerPostHttp()
    {}

    /** if you reimplement call the base version after setting m_data */
    virtual void request();

    void setSession( const QByteArray& id ) { m_session = id; }
    void setUrl( const QUrl& );

    QByteArray postData() const { return m_data; }
};


inline QDebug operator<<( QDebug d, ScrobblerHttp* http )
{
/*
    d << "  Http response: " << http->lastResponse().statusCode() << "\n"
  	  << "  QHttp error code: " << http->error() << "\n"
	  << "  QHttp error text: " << http->errorString() << "\n"
	  << "  Request: " << http->host() + http->currentRequest().path() << "\n"
	  << "  Bytes returned: " << http->bytesAvailable();
*/
    Q_UNUSED(http)
    d << "SOMEDEBUG";    
    return d;
}

#endif
