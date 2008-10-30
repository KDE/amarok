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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef SCROBBLER_HTTP_H
#define SCROBBLER_HTTP_H

#include <QDebug> // leave first, due to QtOverrides
#include <QtNetwork/QHttp>
#include <QUrl>


/** facade pattern base class for QHttp for Scrobbler usage */
class ScrobblerHttp : public QHttp
{
    Q_OBJECT

public:
    void retry();
    int requestId() const { return m_id; }

    QString host() const { return m_host; }

protected:
    ScrobblerHttp( QObject* parent = 0 );

    void setHost( QString s, int i=80 ) { m_host = s; QHttp::setHost( s, i ); }

protected slots:
    virtual void request() = 0;
    // Never use the QHttp::get method directly. Makes it possible to test without needing to mock QHttp.
    int get( QString );
signals:
    void done( const QByteArray& data );

protected:
    int m_id;
    class QTimer *m_retry_timer;

private slots:
    void onRequestFinished( int id, bool error );

private:
    void resetRetryTimer();

    QString m_host;
};


class ScrobblerPostHttp : public ScrobblerHttp
{
    QString m_path;
    QByteArray m_session;

protected:
    QByteArray m_data;

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
    d << "  Http response: " << http->lastResponse().statusCode() << "\n"
  	  << "  QHttp error code: " << http->error() << "\n"
	  << "  QHttp error text: " << http->errorString() << "\n"
	  << "  Request: " << http->host() + http->currentRequest().path() << "\n"
	  << "  Bytes returned: " << http->bytesAvailable();
	
    return d;
}

#endif
