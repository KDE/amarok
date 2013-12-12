/****************************************************************************************
 * Copyright (c) 2013 Vedant Agarwala <vedant.kota@gmail.com>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MUSICDNSFINDER_H
#define MUSICDNSFINDER_H

#include "Version.h"
#include "tagguessing/WebRequestsHandler.h"
#include "MusicDNSXmlParser.h"
#include "ThreadWeaver/Job"

#define AMAROK_MUSICDNS_CLIENT_ID "0c6019606b1d8a54d0985e448f3603ca"

class MusicDNSFinder : public TagGuessing::WebRequestsHandler
{
    public:
        MusicDNSFinder( QObject *parent = 0,
                        const QString &host = "ofa.musicdns.org",
                        const int port = 80,
                        const QString &pathPrefix = "/ofa/1",
                        const QString &clietnId = AMAROK_MUSICDNS_CLIENT_ID,
                        const QString &clientVersion = AMAROK_VERSION );
        ~MusicDNSFinder();

    private slots:
        void gotReply( QNetworkReply *reply );
        void parsingDone( ThreadWeaver::Job *_parser );

    private:
        void checkDone();

        QMap < MusicDNSXmlParser *, Meta::TrackPtr > m_parsers;
};

#endif // MUSICDNSFINDER_H
