/****************************************************************************************
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "AlbumPlayBias"

#include "AlbumPlayBias.h"

#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "dynamic/TrackSet.h"

#include <KLocalizedString>

#include <QComboBox>
#include <QFormLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

QString
Dynamic::AlbumPlayBiasFactory::i18nName() const
{ return i18nc("Name of the \"AlbumPlay\" bias", "Album play"); }

QString
Dynamic::AlbumPlayBiasFactory::name() const
{ return Dynamic::AlbumPlayBias::sName(); }

QString
Dynamic::AlbumPlayBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"AlbumPlay\" bias",
                   "The \"AlbumPlay\" bias adds tracks that belong to one album."); }

Dynamic::BiasPtr
Dynamic::AlbumPlayBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::AlbumPlayBias() ); }




Dynamic::AlbumPlayBias::AlbumPlayBias()
    : m_follow( DirectlyFollow )
{ }

void
Dynamic::AlbumPlayBias::fromXml( QXmlStreamReader *reader )
{
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringView name = reader->name();
            if( name == QStringLiteral("follow") )
                m_follow = followForName( reader->readElementText(QXmlStreamReader::SkipChildElements) );
            else
            {
                debug()<<"Unexpected xml start element"<<reader->name()<<"in input";
                reader->skipCurrentElement();
            }
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }
}

void
Dynamic::AlbumPlayBias::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( QStringLiteral("follow"), nameForFollow( m_follow ) );
}

QString
Dynamic::AlbumPlayBias::sName()
{
    return QStringLiteral( "albumPlayBias" );
}

QString
Dynamic::AlbumPlayBias::name() const
{
    return Dynamic::AlbumPlayBias::sName();
}

QString
Dynamic::AlbumPlayBias::toString() const
{
    switch( m_follow )
    {
    case DirectlyFollow:
        return i18nc("AlbumPlay bias representation",
                     "The next track from the album");
    case Follow:
        return i18nc("AlbumPlay bias representation",
                     "Any later track from the album");
    case DontCare:
        return i18nc("AlbumPlay bias representation",
                     "Tracks from the same album");
    }
    return QString();
}


QWidget*
Dynamic::AlbumPlayBias::widget( QWidget* parent )
{
    QComboBox *combo = new QComboBox( parent );
    combo->addItem( i18n( "Track directly follows previous track in album" ),
                    nameForFollow( DirectlyFollow ) );
    combo->addItem( i18n( "Track comes after previous track in album" ),
                    nameForFollow( Follow ) );
    combo->addItem( i18n( "Track is in the same album as previous track" ),
                    nameForFollow( DontCare ) );
    switch( m_follow )
    {
    case DirectlyFollow: combo->setCurrentIndex(0); break;
    case Follow:         combo->setCurrentIndex(1); break;
    case DontCare:       combo->setCurrentIndex(2); break;
    }
    connect( combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
             this, &AlbumPlayBias::selectionChanged );

    return combo;
}

Dynamic::TrackSet
Dynamic::AlbumPlayBias::matchingTracks( const Meta::TrackList& playlist,
                                        int contextCount, int finalCount,
                                        const Dynamic::TrackCollectionPtr &universe ) const
{
    Q_UNUSED( contextCount );
    Q_UNUSED( finalCount );

    if( playlist.isEmpty() ) // no track means we can't find any tracks in the same album
        return Dynamic::TrackSet( universe, false );

    Meta::TrackPtr track = playlist.last();
    Meta::AlbumPtr album = track->album();

    if( !album ) // no album means we can't find any tracks in the same album
        return Dynamic::TrackSet( universe, false );

    Meta::TrackList albumTracks = album->tracks();

    if( ( albumTracks.count() <= 1 ) || // the album has only one track (or even less) so there can't be any other tracks in the same album
        ( m_follow != DontCare && sameTrack( track, albumTracks.last() ) ) ) // track is the last one and we want to find a later one.
        return Dynamic::TrackSet( universe, false );

    // we assume that the album tracks are sorted by cd and track number which
    // is at least true for the SqlCollection
    TrackSet result( universe, false );
    if( m_follow == DirectlyFollow )
    {
        for( int i = 1; i < albumTracks.count(); i++ )
            if( sameTrack( albumTracks[i-1], track ) )
                result.unite( albumTracks[i] );
    }
    else if( m_follow == Follow )
    {
        bool found = false;
        for( int i = 0; i < albumTracks.count(); i++ )
        {
            if( found )
                result.unite( albumTracks[i] );
            if( sameTrack( albumTracks[i], track ) )
                found = true;
        }
    }
    else if( m_follow == DontCare )
    {
        for( int i = 0; i < albumTracks.count(); i++ )
        {
            if( !sameTrack( albumTracks[i], track ) )
                result.unite( albumTracks[i] );
        }
    }

    return result;
}

bool
Dynamic::AlbumPlayBias::trackMatches( int position,
                                      const Meta::TrackList& playlist,
                                      int contextCount ) const
{
    Q_UNUSED( contextCount );

    if( position <= 0 || playlist.count() <= position )
        return true;

    Meta::TrackPtr track = playlist[position-1];
    Meta::AlbumPtr album = track->album();
    Meta::TrackPtr currentTrack = playlist[position];
    Meta::AlbumPtr currentAlbum = currentTrack->album();

    if( !album || album->tracks().isEmpty() )
        return false;

    Meta::TrackList albumTracks = album->tracks();
    if( sameTrack( track, albumTracks.last() ) && m_follow != DontCare )
        return false;

    // we assume that the album tracks are sorted by cd and track number which
    // is at least true for the SqlCollection
    if( m_follow == DirectlyFollow )
    {
        for( int i = 1; i < albumTracks.count(); i++ )
            if( sameTrack( albumTracks[i-1], track ) )
                return sameTrack( albumTracks[i], currentTrack );
        return false;
    }
    else if( m_follow == Follow )
    {
        bool found = false;
        for( int i = 0; i < albumTracks.count(); i++ )
        {
            if( found && sameTrack( albumTracks[i], currentTrack ) )
                return true;
            if( sameTrack( albumTracks[i], track ) )
                found = true;
        }
        return false;
    }
    else if( m_follow == DontCare )
    {
        return album == currentAlbum;
    }
    return false;
}


Dynamic::AlbumPlayBias::FollowType
Dynamic::AlbumPlayBias::follow() const
{
    return m_follow;
}

void
Dynamic::AlbumPlayBias::setFollow( Dynamic::AlbumPlayBias::FollowType value )
{
    m_follow = value;
    invalidate();
    Q_EMIT changed( BiasPtr(this) );
}

void
Dynamic::AlbumPlayBias::selectionChanged( int which )
{
    if( QComboBox *box = qobject_cast<QComboBox*>(sender()) )
        setFollow( followForName( box->itemData( which ).toString() ) );
}

QString
Dynamic::AlbumPlayBias::nameForFollow( Dynamic::AlbumPlayBias::FollowType match )
{
    switch( match )
    {
    case Dynamic::AlbumPlayBias::DirectlyFollow: return QStringLiteral("directlyFollow");
    case Dynamic::AlbumPlayBias::Follow:         return QStringLiteral("follow");
    case Dynamic::AlbumPlayBias::DontCare:       return QStringLiteral("dontCare");
    }
    return QString();
}

Dynamic::AlbumPlayBias::FollowType
Dynamic::AlbumPlayBias::followForName( const QString &name )
{
    if( name == QLatin1String("directlyFollow") ) return DirectlyFollow;
    else if( name == QLatin1String("follow") )    return Follow;
    else if( name == QLatin1String("dontCare") )  return DontCare;
    else return DontCare;
}

bool
Dynamic::AlbumPlayBias::sameTrack( Meta::TrackPtr track1, Meta::TrackPtr track2 ) const
{
    // We compare items which may be MetaProxy::Track or Meta::Track. For the
    // same underlying track, MetaProxy::Track == Meta;:Track will be true, but
    // Meta::Track == MetaProxy::Track false. Check both ways, and if either
    // returns true, it's the same track.
    return ( *track1 == *track2 ) || ( *track2 == *track1 );
}



