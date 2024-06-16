/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "AlbumItem.h"

#include "SvgHandler.h"
#include "AlbumsDefs.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"

#include <KLocalizedString>

#include <QCollator>
#include <QIcon>
#include <QPixmap>

AlbumItem::AlbumItem()
    : QStandardItem()
    , m_iconSize( 40 )
    , m_showArtist( false )
{
    setEditable( false );
}

AlbumItem::~AlbumItem()
{
    if( m_album )
        unsubscribeFrom( m_album );
}

void
AlbumItem::setAlbum( const Meta::AlbumPtr &albumPtr )
{
    if( m_album )
        unsubscribeFrom( m_album );
    m_album = albumPtr;
    subscribeTo( m_album );
    update();
}

void
AlbumItem::setIconSize( const int iconSize )
{
    static const int padding = 5;

    m_iconSize = iconSize;

    QSize size = sizeHint();
    size.setHeight( iconSize + padding*2 );
    setSizeHint( size );
}

void
AlbumItem::setShowArtist( const bool showArtist )
{
    if( showArtist != m_showArtist )
    {
        m_showArtist = showArtist;
        metadataChanged( m_album );
    }
}

void
AlbumItem::metadataChanged(const Meta::AlbumPtr &album )
{
    Q_UNUSED( album );
    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
}

void
AlbumItem::update()
{
    if( !m_album )
        return;

    Meta::TrackList tracks = m_album->tracks();
    if( !tracks.isEmpty() )
    {
        Meta::TrackPtr first = tracks.first();
        Meta::YearPtr year = first->year();
        if( year )
            setData( year->year(), AlbumYearRole );
    }

    QString albumName = m_album->name();
    albumName = albumName.isEmpty() ? i18n("Unknown") : albumName;
    QString name = ( m_showArtist && m_album->hasAlbumArtist() )
                 ? QString( "%1 - %2" ).arg( m_album->albumArtist()->name(), albumName )
                 : albumName;
    setData( name, NameRole );

    qint64 totalTime = 0;
    for( Meta::TrackPtr item : tracks )
        totalTime += item->length();

    QString trackCount = i18np( "%1 track", "%1 tracks", tracks.size() );
    QString lengthText = QString( "%1, %2" ).arg( trackCount, Meta::msToPrettyTime( totalTime ) );
    setData( lengthText, AlbumLengthRole );

    QPixmap cover = The::svgHandler()->imageWithBorder( m_album, m_iconSize, 3 );
    setIcon( QIcon( cover ) );
    setData( cover, AlbumCoverRole );
}

int
AlbumItem::type() const
{
    return AlbumType;
}

bool
AlbumItem::operator<( const QStandardItem &other ) const
{
    // compare the opposite for descending year order
    int yearA = data( AlbumYearRole ).toInt();
    int yearB = other.data( AlbumYearRole ).toInt();
    if( yearA > yearB )
        return true;
    else if( yearA == yearB )
    {
        const QString nameA = data( NameRole ).toString();
        const QString nameB = other.data( NameRole ).toString();
        QCollator collator;
        collator.setNumericMode( true );
        collator.setCaseSensitivity( Qt::CaseInsensitive );
        return collator.compare( nameA, nameB ) < 0;
    }
    else
        return false;
}

