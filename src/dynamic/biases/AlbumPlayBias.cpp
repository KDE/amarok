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

#include "core/support/Debug.h"

#include "TrackSet.h"
#include "DynamicBiasWidgets.h"

#include <QComboBox>
#include <QFormLayout>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <klocale.h>

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

Dynamic::BiasPtr
Dynamic::AlbumPlayBiasFactory::createBias( QXmlStreamReader *reader )
{ return Dynamic::BiasPtr( new Dynamic::AlbumPlayBias( reader ) ); }





Dynamic::AlbumPlayBias::AlbumPlayBias()
{ }

Dynamic::AlbumPlayBias::AlbumPlayBias( QXmlStreamReader *reader )
{
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "follow" )
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
    writer->writeTextElement( "follow", nameForFollow( m_follow ) );
}

QString
Dynamic::AlbumPlayBias::sName()
{
    return QLatin1String( "albumPlayBias" );
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
    case Follow:         combo->setCurrentIndex(0); break;
    case DontCare:       combo->setCurrentIndex(0); break;
    }
    connect( combo, SIGNAL( currentIndexChanged(int) ),
             this, SLOT( selectionChanged( int ) ) );

    return combo;
}

Dynamic::TrackSet
Dynamic::AlbumPlayBias::matchingTracks( int position,
                                       const Meta::TrackList& playlist, int contextCount,
                                       Dynamic::TrackCollectionPtr universe ) const
{
    Q_UNUSED( contextCount );

    if( position < 1 || position >= playlist.count() )
        return Dynamic::TrackSet( universe, true );

    Meta::TrackPtr track = playlist[position-1];
    Meta::AlbumPtr album = track->album();

    if( !album )
        return Dynamic::TrackSet( universe, true );

    Meta::TrackList albumTracks = album->tracks();
    if( albumTracks.count() == 1 ||
        (track == albumTracks.last() && m_follow != DontCare) )
        return Dynamic::TrackSet( universe, true );

    // we assume that the album tracks are sorted by cd and track number which
    // is at least true for the SqlCollection
    TrackSet result( universe, false );
    if( m_follow == DirectlyFollow )
    {
        for( int i = 1; i < albumTracks.count(); i++ )
            if( albumTracks[i-1] == track )
                result.unite( albumTracks[i] );
    }
    else if( m_follow == Follow )
    {
        bool found = false;
        for( int i = 0; i < albumTracks.count(); i++ )
        {
            if( found )
                result.unite( albumTracks[i] );
            if( albumTracks[i] == track )
                found = true;
        }
    }
    else if( m_follow == DontCare )
    {
        for( int i = 0; i < albumTracks.count(); i++ )
        {
            if( albumTracks[i] != track )
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

    if( !album )
        return true;

    Meta::TrackList albumTracks = album->tracks();
    if( track == albumTracks.last() && m_follow != DontCare )
        return true;

    // we assume that the album tracks are sorted by cd and track number which
    // is at least true for the SqlCollection
    if( m_follow == DirectlyFollow )
    {
        for( int i = 1; i < albumTracks.count(); i++ )
            if( albumTracks[i-1] == track )
                return albumTracks[i] == currentTrack;
        return false;
    }
    else if( m_follow == Follow )
    {
        bool found = false;
        for( int i = 0; i < albumTracks.count(); i++ )
        {
            if( found && albumTracks[i] == currentTrack )
                return true;
            if( albumTracks[i] == track )
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
    emit changed( BiasPtr(this) );
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
    case Dynamic::AlbumPlayBias::DirectlyFollow: return "directlyFollow";
    case Dynamic::AlbumPlayBias::Follow:         return "follow";
    case Dynamic::AlbumPlayBias::DontCare:       return "dontCare";
    }
    return QString();
}

Dynamic::AlbumPlayBias::FollowType
Dynamic::AlbumPlayBias::followForName( const QString &name )
{
    if( name == "directlyFollow" ) return DirectlyFollow;
    else if( name == "follow" )    return Follow;
    else if( name == "dontCare" )  return DontCare;
    else return DontCare;
}



#include "AlbumPlayBias.moc"

