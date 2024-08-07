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

#include "MagnatuneSqlCollection.h"

#include "core/support/Debug.h"
#include "MagnatuneCollectionLocation.h"
#include "MagnatuneMeta.h"

using namespace Collections;

MagnatuneSqlCollection::MagnatuneSqlCollection(const QString & id, const QString & prettyName, ServiceMetaFactory * metaFactory, ServiceSqlRegistry * registry)
    : ServiceSqlCollection( id, prettyName, metaFactory, registry )
{
}

Meta::TrackPtr MagnatuneSqlCollection::trackForUrl(const QUrl &url)
{
    //DEBUG_BLOCK

    QString pristineUrl = url.url();

    if ( pristineUrl.startsWith( QStringLiteral("http://magnatune.com/playlist_redirect.php?url=") ) ) {

        //if we are not a member of the right type, we need to preserve this or we will not be able to play the track. Actually... use the original url in any case so plays are attributed to the person whose playlist it is...
        QString orgUrl = pristineUrl;

        int endIndex = pristineUrl.indexOf( QStringLiteral("&key=") );

        pristineUrl = pristineUrl.mid( 47, endIndex - 47 );

        //debug() << "got redirected url: " << pristineUrl;


        pristineUrl.remove( QStringLiteral("_nospeech") );
        pristineUrl.replace( QStringLiteral(".ogg"), QStringLiteral(".mp3") );
        pristineUrl.replace( QStringLiteral("-lofi.mp3"), QStringLiteral(".mp3") );

        pristineUrl.replace( QRegularExpression( QStringLiteral("http://download") ), QStringLiteral("http://he3") );
        pristineUrl.replace( QRegularExpression( QStringLiteral("http://stream") ), QStringLiteral("http://he3") );

        //debug() << "after a quick makeover: " << pristineUrl;

        Meta::TrackPtr trackPtr = ServiceSqlCollection::trackForUrl( QUrl( pristineUrl ) );

        if ( trackPtr ) {
            Meta::ServiceTrack * mTrack = dynamic_cast< Meta::ServiceTrack * >( trackPtr.data() );
            if ( mTrack ) {

                mTrack->setUidUrl( orgUrl );
            }
        }

        return trackPtr;

    } else {

        pristineUrl.remove( QStringLiteral("_nospeech") );
        pristineUrl.replace( QStringLiteral(".ogg"), QStringLiteral(".mp3") );
        pristineUrl.replace( QStringLiteral("-lofi.mp3"), QStringLiteral(".mp3") );

        pristineUrl.replace( QRegularExpression( QStringLiteral(".*:.*@download") ), QStringLiteral("http://he3") );
        pristineUrl.replace( QRegularExpression( QStringLiteral(".*:.*@stream") ), QStringLiteral("http://he3") );

        return ServiceSqlCollection::trackForUrl( QUrl( pristineUrl ) );

    }
    
}

CollectionLocation * MagnatuneSqlCollection::location()
{
    return new MagnatuneCollectionLocation( this );
}




