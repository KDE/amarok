/****************************************************************************************
 * Copyright (c) 2012 Ryan McCoskrie <ryan.mccoskrie@gmail.com>                         *
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "CollectionLocationDelegateImpl.h"

#include "core/logger/Logger.h"
#include "core/collections/CollectionLocation.h"
#include "core/support/Components.h"
#include "transcoding/TranscodingAssistantDialog.h"

#include <KLocalizedString>
#include <KMessageBox>

using namespace Collections;

bool
CollectionLocationDelegateImpl::reallyDelete( CollectionLocation *loc, const Meta::TrackList &tracks ) const
{
    QStringList files = trackList( tracks );
    const QString text( i18ncp( "@info",
        "Do you really want to delete this track? It will be removed from %2 and from underlying storage medium.",
        "Do you really want to delete these %1 tracks? They will be removed from %2 and from underlying storage medium.",
         tracks.count(), loc->prettyLocation()) );
    int ret = KMessageBox::warningContinueCancelList(nullptr, text, files,
        i18nc( "@title:window", "Confirm Delete" ), KStandardGuiItem::del() );
    return ret == KMessageBox::Continue;
}

bool
CollectionLocationDelegateImpl::reallyTrash( CollectionLocation *loc, const Meta::TrackList &tracks ) const
{
    QStringList files = trackList( tracks );
    const QString text( i18ncp( "@info",
        "Do you really want to move this track to the trash? It will be removed from %2.",
        "Do you really want to move these %1 tracks to the trash? They will be removed from %2.",
        tracks.count(), loc->prettyLocation() ) );
    int ret = KMessageBox::warningContinueCancelList( nullptr, text, files,
        i18nc( "@title:window", "Confirm Move to Trash" ), KStandardGuiItem::remove() );
    return ret == KMessageBox::Continue;
}

bool
CollectionLocationDelegateImpl::reallyMove( CollectionLocation *loc, const Meta::TrackList &tracks ) const
{
    Q_UNUSED( loc )
    QStringList files = trackList( tracks );
    const QString text( i18ncp( "@info",
        "Do you really want to move this track? It will be renamed and the original deleted.",
        "Do you really want to move these %1 tracks? They will be renamed and the originals deleted.",
        tracks.count() ) );
    int ret = KMessageBox::warningContinueCancelList( nullptr, text, files,
        i18nc( "@title:window", "Move Files" ), KGuiItem( i18nc( "rename files button", "&Rename" ), QStringLiteral("go-jump") ) );
    return ret == KMessageBox::Continue;
}

void
CollectionLocationDelegateImpl::errorDeleting( CollectionLocation *loc, const Meta::TrackList &tracks ) const
{
    Q_UNUSED( loc )
    QStringList files = trackList( tracks );
    const QString text( i18ncp( "@info",
        "There was a problem and this track could not be removed. Make sure the directory is writable.",
        "There was a problem and %1 tracks could not be removed. Make sure the directory is writable.",
        files.count() ) );
    KMessageBox::informationList( nullptr, text, files, i18n( "Unable to remove tracks") );
}

void
CollectionLocationDelegateImpl::notWriteable( CollectionLocation *loc ) const
{
    Q_UNUSED( loc )
    Amarok::Logger::longMessage(
            i18n( "The collection does not have enough free space available or is not writable." ),
            Amarok::Logger::Error );
}

bool
CollectionLocationDelegateImpl::deleteEmptyDirs( CollectionLocation *loc ) const
{
    const QString text( i18n( "Do you want to remove empty folders?" ) );
    const QString caption( i18n( "Remove empty folders?" ) );
    int result = KMessageBox::questionYesNo( nullptr, text, caption, KStandardGuiItem::yes(),
        KStandardGuiItem::no(), QStringLiteral("Delete empty dirs from ") + loc->prettyLocation() );
    return result == KMessageBox::Yes;
}

Transcoding::Configuration
CollectionLocationDelegateImpl::transcode( const QStringList &playableFileTypes,
                                           bool *remember, OperationType operation,
                                           const QString &destCollectionName,
                                           const Transcoding::Configuration &prevConfiguration ) const
{
    Transcoding::AssistantDialog dialog( playableFileTypes, remember != nullptr, operation,
                                         destCollectionName, prevConfiguration );
    if( dialog.exec() )
    {
        if( remember )
            *remember = dialog.shouldSave();
        return dialog.configuration();
    }
    return Transcoding::Configuration( Transcoding::INVALID );
}

QStringList
CollectionLocationDelegateImpl::trackList( const Meta::TrackList &tracks ) const
{
    QStringList trackList;
    for( Meta::TrackPtr track : tracks )
    {
        QString url = track->prettyUrl();
        Meta::ArtistPtr artist = track->artist();
        QString artistName = artist ? artist->name() : QString();
        QString trackName = track->name();

        QString str;
        // Add track and artist name if available
        if( !trackName.isEmpty() && !artistName.isEmpty() )
            str = i18nc( "%1 is track url, %2 track title, %3 track artist",
                         "%1 (%2 by %3)", url, trackName, artistName );
        else if( !trackName.isEmpty() )
            str = i18nc( "%1 is track url, %2 track name", "%1 (%2)", url, trackName );
        else if( !artistName.isEmpty() )
            str = i18nc( "%1 is track url, %2 artist name", "%1 (by %2)", url, artistName );
        else
            str = url;

        trackList << str;
    }

    return trackList;
}
