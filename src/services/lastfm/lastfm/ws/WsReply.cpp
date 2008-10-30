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

#include "WsReply.h"
#include <QCoreApplication>
#include <QEventLoop>


WsReply::WsReply( QNetworkReply* r )
{
    r->setParent( this );

    m_error = Ws::NoError;
    m_reply = r;

    connect( m_reply, SIGNAL(finished()), SLOT(onFinished()) );
}


#ifdef OH_MY_GOLLY_GOSH___I_SO_HAVE_A_DEATH_WISH
#ifndef NDEBUG
void
WsReply::finish()
{
    QEventLoop eventLoop;
    QObject::connect( m_reply, SIGNAL(finished()), &eventLoop, SLOT(quit()) );
    eventLoop.exec();
}
#endif
#endif


void
WsReply::onFinished()
{
    try
    {
        m_data = m_reply->readAll();

        if (m_data.size())
        {
            m_xml.setContent( m_data );
            m_lfm = m_xml.documentElement();

            if (m_lfm.isNull())
                throw Ws::MalformedResponse;

            QString const status = m_lfm.attribute( "status" );
            QDomElement error = m_lfm.firstChildElement( "error" );
            uint const n = m_lfm.childNodes().count();

            if (status == "failed" || n == 1 && !error.isNull())
            {
                throw error.isNull()
                        ? Ws::MalformedResponse
                        : Ws::Error( error.attribute( "code" ).toUInt() );
            }

			// no elements beyond the lfm is perfectably acceptable
            //if (n == 0) // nothing useful in the response
        }

        switch (m_reply->error())
        {
            case QNetworkReply::NoError:
                break;

            case QNetworkReply::RemoteHostClosedError:
            case QNetworkReply::ConnectionRefusedError:
            case QNetworkReply::TimeoutError:
            case QNetworkReply::SslHandshakeFailedError:
            case QNetworkReply::ContentAccessDenied:
            case QNetworkReply::ContentOperationNotPermittedError:
            case QNetworkReply::ContentNotFoundError:
            case QNetworkReply::AuthenticationRequiredError:
            case QNetworkReply::UnknownContentError:
            case QNetworkReply::ProtocolInvalidOperationError:
            case QNetworkReply::ProtocolFailure:
                qDebug() << m_reply->error();
                throw Ws::TryAgainLater;

            case QNetworkReply::HostNotFoundError:
            case QNetworkReply::UnknownNetworkError:
            case QNetworkReply::ProtocolUnknownError:
                throw Ws::UrLocalNetworkIsFuckedLol;

            case QNetworkReply::OperationCanceledError:
                throw Ws::Aborted;

            case QNetworkReply::ProxyConnectionRefusedError:
            case QNetworkReply::ProxyConnectionClosedError:
            case QNetworkReply::ProxyNotFoundError:
            case QNetworkReply::ProxyTimeoutError:
            case QNetworkReply::ProxyAuthenticationRequiredError:
            case QNetworkReply::UnknownProxyError:
                throw Ws::UrProxyIsFuckedLol;
        }

        m_error = Ws::NoError;
    }
    catch (Ws::Error e)
    {
        qWarning() << this;

        m_error = e;

        switch (m_error)
        {
            case Ws::OperationFailed:
            case Ws::InvalidSessionKey:
                // NOTE will never be received during the LoginDialog stage
                // since that happens before this slot is registered with
                // QMetaObject in App::App(). Neat :)
                QMetaObject::invokeMethod( qApp, "onWsError", Q_ARG( Ws::Error, m_error ) );
                break;
			
			default: //please, I want a pragma to remove this warning
				break;
        }
    }

    try
    {
        emit finished( this );
    }
    catch (CoreDomElement::Exception& e)
    {
        qWarning() << "Naughty programmer! Unhandled exception:" << e;
    }

    // prevents memory leaks, but I don't like it anyway
    deleteLater();
}


QString
WsReply::method() const
{
	return m_reply->url().queryItemValue( "method" );
}
