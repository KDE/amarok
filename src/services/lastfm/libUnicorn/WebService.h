/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
 *      Jono Cole, Last.fm Ltd <jono@last.fm>                              *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef WEBSERVICE_H
#define WEBSERVICE_H

#include <QApplication>
#include <QUrl>
#include <QDateTime>

#include "WebService/fwd.h"
#include "Track.h"
#include "WeightedStringList.h"
#include "StationUrl.h"


/** @author <max@last.fm> */

class UNICORN_DLLEXPORT WebService : public QObject
{
    Q_OBJECT

        WebService( QObject* ); //class is singleton, thus ctor is private

        friend WebService *The::webService();

    public:
        void setUsername( const QString& username ) { m_username = username; }
        void setPassword( const QString& password ) { m_password = password; }

        QString currentUsername() const { return m_username; }
        QString currentPassword() const { return m_password; }

        // Added by Amarok.. why isn't this a method already?
        bool isSubscriber() const { return m_isSubscriber; }

    //////
        QUrl streamUrl() const { return m_streamUrl; }

        /// Returns the time string used as a challenge for authorising web calls
        QString challengeString();

    //////
        PROP_GET( bool, isAutoDetectedProxy )
        PROP_GET( QString, proxyHost )
        PROP_GET( int, proxyPort )

    signals:
        //TODO this system sucks
        // instead an observer controller system, and then we can incorporate events
        // from the whole system and handle in general everything better

        void handshakeResult( Handshake* );
        void changeStationResult( ChangeStationRequest* );
        void setTagResult( SetTagRequest* );
        void skipResult( SkipRequest* );

        /// clearly you only get these if successful, this suits our current purposes
        void friendDeleted( QString username );
        void stationChanged( StationUrl url, QString name );

        /// data corresponds to currentUsername(), no other neighbour requests come from here
        void friends( QStringList );
        void neighbours( WeightedStringList );
        void userTags( WeightedStringList );
        void recentTracks( QList<Track> );
        void recentLovedTracks( QList<Track> );
        void recentBannedTracks( QList<Track> );
        void loved( Track );
        void unloved( Track );
        void banned( Track );
        void unbanned( Track );
        void unlistened( Track );

        void proxyTestResult( bool );

        /**
         * Beware, if you connect to these functions, you get notified for every
         * single webservice-request, which can be useful of course.
         *
         * You also get Requests relevant to other usernames and streams etc.
         * so you should check the data is relevant to you before operating on
         * it.
         *
         * Really, don't use these unless you are error handling, instead connect
         * to the Request where you create the Request itself.
         */
        void success( Request* );
        void failure( Request* );
        void result( Request* );

    private slots:
        /// all requests end up here
        void requestResult( Request* );

    private:
        void autoDetectProxy();

        QString m_username;
        QString m_password;
        QUrl m_streamUrl;
        bool m_isSubscriber;
};


namespace The
{
    inline WebService *webService()
    {
        static WebService *o = 0;
        if (!o)
        {
            o = qApp->findChild<WebService*>( "WebService-Instance" );
            if (!o)
            {
                o = new WebService( qApp );
                o->setObjectName( "WebService-Instance" );
            }
        }
        return o;
    }
}

#endif

