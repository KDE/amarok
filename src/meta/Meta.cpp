/* This file is part of the Amarok project
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
   Copyright (C) 2007 Ian Monroe <ian@monroe.nu>
   Copyright (C) 2008 Mark Kretschmann <kretschmann@kde.org> 

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "Meta.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "Collection.h"
#include "Debug.h"
#include "QueryMaker.h"
#include "SvgHandler.h"

#include <QDir>
#include <QImage>

#include <KStandardDirs>

//Meta::Observer

Meta::Observer::~Observer()
{
    // Unsubscribe all stray Meta subscriptions:

    foreach( TrackPtr ptr, m_trackSubscriptions )
        if( ptr )
            ptr->unsubscribe( this );
    foreach( ArtistPtr ptr, m_artistSubscriptions )
        if( ptr )
            ptr->unsubscribe( this );
    foreach( AlbumPtr ptr, m_albumSubscriptions )
        if( ptr )
            ptr->unsubscribe( this );
    foreach( GenrePtr ptr, m_genreSubscriptions )
        if( ptr )
            ptr->unsubscribe( this );
    foreach( ComposerPtr ptr, m_composerSubscriptions )
        if( ptr )
            ptr->unsubscribe( this );
    foreach( YearPtr ptr, m_yearSubscriptions )
        if( ptr )
            ptr->unsubscribe( this );
}

void
Meta::Observer::subscribeTo( TrackPtr ptr )
{
    if( ptr ) {
        ptr->subscribe( this );
        m_trackSubscriptions.insert( ptr );
    }
}

void
Meta::Observer::unsubscribeFrom( TrackPtr ptr )
{
    if( ptr ) {
        ptr->unsubscribe( this );
        m_trackSubscriptions.remove( ptr );
    }
}

void
Meta::Observer::subscribeTo( ArtistPtr ptr )
{
    if( ptr ) {
        ptr->subscribe( this );
        m_artistSubscriptions.insert( ptr );
    }
}

void
Meta::Observer::unsubscribeFrom( ArtistPtr ptr )
{
    if( ptr ) {
        ptr->unsubscribe( this );
        m_artistSubscriptions.remove( ptr );
    }
}

void
Meta::Observer::subscribeTo( AlbumPtr ptr )
{
    if( ptr ) {
        ptr->subscribe( this );
        m_albumSubscriptions.insert( ptr );
    }
}

void
Meta::Observer::unsubscribeFrom( AlbumPtr ptr )
{
    if( ptr ) {
        ptr->unsubscribe( this );
        m_albumSubscriptions.remove( ptr );
    }
}

void
Meta::Observer::subscribeTo( ComposerPtr ptr )
{
    if( ptr ) {
        ptr->subscribe( this );
        m_composerSubscriptions.insert( ptr );
    }
}

void
Meta::Observer::unsubscribeFrom( ComposerPtr ptr )
{
    if( ptr ) {
        ptr->unsubscribe( this );
        m_composerSubscriptions.remove( ptr );
    }
}

void
Meta::Observer::subscribeTo( GenrePtr ptr )
{
    if( ptr ) {
        ptr->subscribe( this );
        m_genreSubscriptions.insert( ptr );
    }
}

void
Meta::Observer::unsubscribeFrom( GenrePtr ptr )
{
    if( ptr ) {
        ptr->unsubscribe( this );
        m_genreSubscriptions.remove( ptr );
    }
}

void
Meta::Observer::subscribeTo( YearPtr ptr )
{
    if( ptr ) {
        ptr->subscribe( this );
        m_yearSubscriptions.insert( ptr );
    }
}

void
Meta::Observer::unsubscribeFrom( YearPtr ptr )
{
    if( ptr ) {
        ptr->unsubscribe( this );
        m_yearSubscriptions.remove( ptr );
    }
}

void
Meta::Observer::metadataChanged( Track *track )
{
    Q_UNUSED( track );
}

void
Meta::Observer::metadataChanged( Artist *artist )
{
    Q_UNUSED( artist );
}

void
Meta::Observer::metadataChanged( Album *album )
{
    Q_UNUSED( album );
}

void
Meta::Observer::metadataChanged( Composer *composer )
{
    Q_UNUSED( composer );
}

void
Meta::Observer::metadataChanged( Genre *genre )
{
    Q_UNUSED( genre );
}

void
Meta::Observer::metadataChanged( Year *year )
{
    Q_UNUSED( year );
}

//Meta::MetaBase

void
Meta::MetaBase::subscribe( Observer *observer )
{
    if( observer )
        m_observers.insert( observer );
}

void
Meta::MetaBase::unsubscribe( Observer *observer )
{
    m_observers.remove( observer );
}


bool
Meta::MetaBase::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    Q_UNUSED( type );
    return false;
}

Meta::Capability*
Meta::MetaBase::asCapabilityInterface( Meta::Capability::Type type )
{
    Q_UNUSED( type );
    return 0;
}


//Meta::Track

QString
Meta::Track::prettyName() const
{
    if ( !name().isEmpty() )
        return name();
    else
        return prettyUrl();
}

bool
Meta::Track::inCollection() const
{
    return false;
}

Collection*
Meta::Track::collection() const
{
    return 0;
}

QString
Meta::Track::cachedLyrics() const
{
    return QString();
}

void
Meta::Track::setCachedLyrics( const QString &lyrics )
{
    Q_UNUSED( lyrics )
}

void
Meta::Track::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( TrackPtr( this ) );
}

void
Meta::Track::prepareToPlay()
{
}

void
Meta::Track::finishedPlaying( double /*playedFraction*/ )
{
}

void
Meta::Track::notifyObservers() const
{
    foreach( Observer *observer, m_observers )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->metadataChanged( const_cast<Meta::Track*>( this ) );
    }
}

uint
Meta::Track::firstPlayed() const
{
    return 0;
}

bool
Meta::Track::operator==( const Meta::Track &track ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const void*>( &track );
}

bool
Meta::Track::lessThan( const Meta::TrackPtr left, const Meta::TrackPtr right )
{
    if( left->album()->name() == right->album()->name() ) // If the albums are the same
    {
        if ( left->discNumber() < right->discNumber() ) //First compare by disc number
        {
            return true;
        }
        else if( left->discNumber() == right->discNumber() ) //Disc #'s are equal, compare by track number
        {
            return left->trackNumber() < right->trackNumber();
        }
        else
        {
            return false; // Right disc has a lower number
        }
    }
    else if( left->artist()->name() == right->artist()->name() )
        return QString::localeAwareCompare( left->album()->prettyName(), right->album()->prettyName() ) < 0;
    else // compare artists alphabetically
        return QString::localeAwareCompare( left->artist()->prettyName(), right->artist()->prettyName() ) < 0;
}

//Meta::Artist

void
Meta::Artist::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( ArtistPtr( this ) );
}

void
Meta::Artist::notifyObservers() const
{
    DEBUG_BLOCK

    foreach( Observer *observer, m_observers )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->metadataChanged( const_cast<Meta::Artist*>( this ) );
    }
}

bool
Meta::Artist::operator==( const Meta::Artist &artist ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const  void*>( &artist );
}

//Meta::Album

void
Meta::Album::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( AlbumPtr( this ) );
}

void
Meta::Album::notifyObservers() const
{
    DEBUG_BLOCK

    foreach( Observer *observer, m_observers )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->metadataChanged( const_cast<Meta::Album*>( this ) );
    }
}

QPixmap
Meta::Album::image( int size )
{
    // Return "nocover" until it's fetched.
    QDir cacheCoverDir = QDir( Amarok::saveLocation( "albumcovers/cache/" ) );
    if ( size <= 1 )
        size = AmarokConfig::coverPreviewSize();
    QString sizeKey = QString::number( size ) + '@';

    QImage img;
    if( cacheCoverDir.exists( sizeKey + "nocover.png" ) )
         img = QImage( cacheCoverDir.filePath( sizeKey + "nocover.png" ) );
    else
    {
        QImage orgImage = QImage( KStandardDirs::locate( "data", "amarok/images/nocover.png" ) ); //optimise this!
        //scaled() does not change the original image but returns a scaled copy
        img = orgImage.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        img.save( cacheCoverDir.filePath( sizeKey + "nocover.png" ), "PNG" );
    }

    m_noCoverImage = true;
    
    return QPixmap::fromImage( img );
}


QPixmap
Meta::Album::imageWithBorder( int size, int borderWidth )
{
    QPixmap coverWithBorders;

    m_noCoverImage = false;
    
    const int imageSize = size - borderWidth * 2;
    QPixmap cover = image( imageSize );

    QString nameForKey = name();

    if( m_noCoverImage == true )
        nameForKey = "nocover";

    coverWithBorders = The::svgHandler()->addBordersToPixmap( cover, borderWidth, nameForKey );

    return coverWithBorders;
}


bool
Meta::Album::operator==( const Meta::Album &album ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const  void*>( &album );
}

//Meta::Genre

void
Meta::Genre::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( GenrePtr( this ) );
}

void
Meta::Genre::notifyObservers() const
{
    foreach( Observer *observer, m_observers )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->metadataChanged( const_cast<Meta::Genre*>( this ) );
    }
}

bool
Meta::Genre::operator==( const Meta::Genre &genre ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const  void*>( &genre );
}

//Meta::Composer

void
Meta::Composer::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( ComposerPtr( this ) );
}

void
Meta::Composer::notifyObservers() const
{
    foreach( Observer *observer, m_observers )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->metadataChanged( const_cast<Meta::Composer*>( this ) );
    }
}

bool
Meta::Composer::operator==( const Meta::Composer &composer ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const  void*>( &composer );
}

//Meta::Year

void
Meta::Year::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( YearPtr( this ) );
}

void
Meta::Year::notifyObservers() const
{
    foreach( Observer *observer, m_observers )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->metadataChanged( const_cast<Meta::Year*>( this ) );
    }
}

bool
Meta::Year::operator==( const Meta::Year &year ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const  void*>( &year );
}

