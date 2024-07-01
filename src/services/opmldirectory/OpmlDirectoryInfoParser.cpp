/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "OpmlDirectoryInfoParser.h"

#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core/logger/Logger.h"
#include "OpmlDirectoryMeta.h"

#include <QDomDocument>

#include <KLocalizedString>
#include <KIO/StoredTransferJob>

using namespace Meta;

OpmlDirectoryInfoParser::OpmlDirectoryInfoParser()
 : InfoParserBase()
 , m_rssDownloadJob( nullptr )
{
}


OpmlDirectoryInfoParser::~OpmlDirectoryInfoParser()
{
}

void OpmlDirectoryInfoParser::getInfo(const ArtistPtr &artist)
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( artist );
}

void OpmlDirectoryInfoParser::getInfo(const AlbumPtr &album)
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( album );
}

void OpmlDirectoryInfoParser::getInfo(const TrackPtr &track )
{
    DEBUG_BLOCK
    showLoading( i18n( "Loading Podcast Info..." ) );

    OpmlDirectoryFeed * feed = dynamic_cast<OpmlDirectoryFeed *>( track.data() );

    if ( !feed ) return;

    debug() << "OpmlDirectoryInfoParser: getInfo about feed: " << feed->uidUrl();

    m_rssDownloadJob = KIO::storedGet( QUrl( feed->uidUrl() ), KIO::Reload, KIO::HideProgressInfo );
    Amarok::Logger::newProgressOperation( m_rssDownloadJob,
                                                        i18n( "Fetching Podcast Info" ) );
    connect( m_rssDownloadJob, &KJob::result, this, &OpmlDirectoryInfoParser::rssDownloadComplete );
}

void OpmlDirectoryInfoParser::rssDownloadComplete(KJob * downLoadJob)
{

    if ( downLoadJob->error() )
    {
        //TODO: error handling here
        return ;
    }
    
    if ( downLoadJob != m_rssDownloadJob )
        return ; //not the right job, so let's ignore it

    QString rssString = ((KIO::StoredTransferJob* ) downLoadJob)->data();

    debug() << "rss: " << rssString;

    QDomDocument doc( QStringLiteral("reply") );
    if ( !doc.setContent( rssString ) )
    {
        debug() << "could not set reply document to given RSS string";
        return;
    }

    //there might be an rss node, there might not...

    QDomElement element = doc.firstChildElement( QStringLiteral("rss") );
    if ( !element.isNull() ) {
        element = element.firstChildElement( QStringLiteral("channel") );
    } else {
        element = doc.firstChildElement( QStringLiteral("channel") );
    }

    QString description = element.firstChildElement( QStringLiteral("description") ).text();
    QString title = element.firstChildElement( QStringLiteral("title") ).text();
    
    QString imageUrl;
    QDomElement image = element.firstChildElement( QStringLiteral("image") );
    
    if ( !image.isNull() )
        imageUrl = image.firstChildElement( QStringLiteral("url") ).text();

    QString infoHtml = QStringLiteral("<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
            "CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>");

    infoHtml += QStringLiteral("<div align=\"center\"><strong>");
    infoHtml += title;
    infoHtml += QStringLiteral("</strong><br><br>");

    if ( !imageUrl.isEmpty() ) 
        infoHtml += QStringLiteral("<img src=\"") + imageUrl + QStringLiteral("\" align=\"middle\" border=\"1\">");
    
    infoHtml += QStringLiteral("<br><p align=\"left\" >") + description;
    infoHtml += QStringLiteral("</BODY></HTML>");

    Q_EMIT ( info( infoHtml ) );

    downLoadJob->deleteLater();
}

