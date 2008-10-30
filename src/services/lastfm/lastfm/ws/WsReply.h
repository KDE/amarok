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

#ifndef LASTFM_WS_REPLY_H
#define LASTFM_WS_REPLY_H

#include <lastfm/DllExportMacro.h>
#include <lastfm/core/CoreDomElement.h>
#include <lastfm/ws/WsError.h>
#include <QNetworkReply>
#include <QDomDocument>


/** @brief Essentially our QNetworkReply, hence the name 
  * @author <max@last.fm>
  *
  * When you get this back from WsRequestBuilder the request has already been
  * sent. So connect to the finished signal before the event loop resumes!
  *
  * Create a slot in your derived QApplication object onWsError( Ws::Error )
  * to receive errors that require user interaction. If you don't your user
  * experience will suck :P
  */
class LASTFM_WS_DLLEXPORT WsReply : public QObject
{
    Q_OBJECT

    Ws::Error m_error;
    QNetworkReply* m_reply;
    QDomDocument m_xml;
    QDomElement m_lfm;
    QByteArray m_data;

    friend class WsRequestBuilder;
	friend QDebug operator<<( QDebug, WsReply* );

    WsReply( QNetworkReply* );

public:
    /** the <lfm> element from the XML response, see http://last.fm/api */
    CoreDomElement lfm() const { return CoreDomElement( m_lfm ); }
    Ws::Error error() const { return m_error; }
    QNetworkReply::NetworkError networkError() const { return m_reply->error(); }
    static QString networkErrorString( QNetworkReply::NetworkError );

	QString method() const;
	
#ifdef OH_MY_GOLLY_GOSH___I_SO_HAVE_A_DEATH_WISH
    /** blocks until complete
      * SERIOUSLY NEVER USE THIS APART FROM FOR EXPERIMENTATION!
      * It crashes like crazy!
      * As all sorts of things break when you start running your own event loop
      * sadly, especially don't chain WsReplys since they do deleteLater() */
    void finish();
#endif

    QByteArray data() const { return m_data; }
	
	bool failed() const { return m_error != Ws::NoError; }

signals:
    /** we call deleteLater() immediately after emitting this signal, so don't
      * store copies of the pointer */
    void finished( WsReply* );

private slots:
    void onFinished();
};


#include <QDebug>
inline QDebug operator<<( QDebug d, WsReply* r )
{
	return d << r->method() + ":" << "\n"
	         << r->m_reply->url() << "\n"
			 << r->data().trimmed();
}


inline QDebug operator<<( QDebug d, QNetworkReply::NetworkError e )
{    
#define CASE( x ) case x: return d << #x;
    switch (e)
    {
	    CASE( QNetworkReply::NoError )
	    CASE( QNetworkReply::ConnectionRefusedError )
	    CASE( QNetworkReply::RemoteHostClosedError )
	    CASE( QNetworkReply::HostNotFoundError )
	    CASE( QNetworkReply::TimeoutError )
	    CASE( QNetworkReply::OperationCanceledError )
	    CASE( QNetworkReply::SslHandshakeFailedError )
	    CASE( QNetworkReply::ProxyConnectionRefusedError )
	    CASE( QNetworkReply::ProxyConnectionClosedError )
	    CASE( QNetworkReply::ProxyNotFoundError )
	    CASE( QNetworkReply::ProxyTimeoutError )
	    CASE( QNetworkReply::ProxyAuthenticationRequiredError )
	    CASE( QNetworkReply::ContentAccessDenied )
	    CASE( QNetworkReply::ContentOperationNotPermittedError )
	    CASE( QNetworkReply::ContentNotFoundError )
	    CASE( QNetworkReply::AuthenticationRequiredError )
	    CASE( QNetworkReply::ProtocolUnknownError )
	    CASE( QNetworkReply::ProtocolInvalidOperationError )
	    CASE( QNetworkReply::UnknownNetworkError )
	    CASE( QNetworkReply::UnknownProxyError )
	    CASE( QNetworkReply::UnknownContentError )
	    CASE( QNetworkReply::ProtocolFailure )    
		default: return d << "Unknown error";
    }
#undef CASE
}

#endif
