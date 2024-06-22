/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "PlayUrlRunner.h"

#include "core/support/Debug.h"
#include "amarokconfig.h"
#include "AmarokUrl.h"
#include "AmarokUrlHandler.h"
#include "core-impl/storage/StorageManager.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "EngineController.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/proxymodels/AbstractModel.h"
#include <core/storage/SqlStorage.h>
#include "core/meta/Meta.h"

PlayUrlRunner::PlayUrlRunner() : AmarokUrlRunnerBase()
{
}

PlayUrlRunner::~PlayUrlRunner()
{
    The::amarokUrlHandler()->unRegisterRunner ( this );
}

bool PlayUrlRunner::run (const AmarokUrl &url )
{
    DEBUG_BLOCK
    if ( url.isNull() )
        return false;

    QUrl track_url = QUrl::fromEncoded ( QByteArray::fromBase64 ( url.path().toUtf8() ) );
    debug() << "decoded track url: " << track_url.toString();

    //get the position
    qint64 pos = 0;
    if ( url.args().keys().contains( QStringLiteral("pos") ) )
    {
        pos = (qint64) ( url.args().value( QStringLiteral("pos") ).toDouble() * 1000.0 );
    }

    debug() << "seek pos: " << pos;
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( track_url );
    if ( !track )
        return false;

    The::engineController()->play ( track, pos );

    Playlist::AbstractModel *model = The::playlist();

    int row = model->firstRowForTrack( track );
    if( row != -1 )
        model->setActiveRow( row );
    else
    {
        row = AmarokConfig::dynamicMode() ? model->activeRow() + 1 : model->qaim()->rowCount();
        The::playlistController()->insertTrack( row, track );
        model->setActiveRow( row );
    }

    return true;
}

QString PlayUrlRunner::command() const
{
    return QStringLiteral("play");
}

QString PlayUrlRunner::prettyCommand() const
{
    return i18nc( "A type of command that starts playing at a specific position in a track", "Play" );
}

BookmarkList PlayUrlRunner::bookmarksFromUrl( const QUrl &url )
{
    BookmarkList list;

    //See PlayUrlGenerator for the description of a 'play' amarokurl
    QString track_encoded = url.toEncoded().toBase64();

    // The last character of a base64 encoded string is always '=', which
    // chokes the SQL. Since we are using a substring like text comparison
    // and every url in the database will have the '=', just chop it off.

    // some tracks even seem to have multiple '='s at the end... chop them all off!
    while( track_encoded.endsWith( QLatin1Char('=') ) )
        track_encoded.chop ( 1 );

    // Queries the database for bookmarks where the url field contains
    // the base64 encoded url (minus the '=').
    QString query = QStringLiteral("SELECT id, parent_id, name, url, description, custom FROM bookmarks WHERE url LIKE '%%1%'");
    query = query.arg ( track_encoded );
    QStringList result = StorageManager::instance()->sqlStorage()->query ( query );

    int resultRows = result.count() / 6;

    for ( int i = 0; i < resultRows; i++ )
    {
        QStringList row = result.mid ( i*6, 6 );
        list << AmarokUrlPtr ( new AmarokUrl ( row ) );
    }
    return list;
}

QIcon PlayUrlRunner::icon() const
{
    return QIcon::fromTheme( QStringLiteral("x-media-podcast-amarok") );
}

