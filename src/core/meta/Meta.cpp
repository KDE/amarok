/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "core/meta/Meta.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "collection/Collection.h"
#include "Debug.h"
#include "collection/QueryMaker.h"
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
Meta::Observer::metadataChanged( TrackPtr track )
{
    Q_UNUSED( track );
}

void
Meta::Observer::metadataChanged( ArtistPtr artist )
{
    Q_UNUSED( artist );
}

void
Meta::Observer::metadataChanged( AlbumPtr album )
{
    Q_UNUSED( album );
}

void
Meta::Observer::metadataChanged( ComposerPtr composer )
{
    Q_UNUSED( composer );
}

void
Meta::Observer::metadataChanged( GenrePtr genre )
{
    Q_UNUSED( genre );
}

void
Meta::Observer::metadataChanged( YearPtr year )
{
    Q_UNUSED( year );
}

//Meta::MetaCapability

bool
Meta::MetaCapability::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    Q_UNUSED( type );
    return false;
}

Meta::Capability*
Meta::MetaCapability::createCapabilityInterface( Meta::Capability::Type type )
{
    Q_UNUSED( type );
    return 0;
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

//Meta::Track

QString
Meta::Track::prettyName() const
{
    if( !name().isEmpty() )
        return name();
    return prettyUrl();
}

bool
Meta::Track::inCollection() const
{
    return false;
}

Amarok::Collection*
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

QDateTime
Meta::Track::createDate() const
{
    return QDateTime();
}
qreal
Meta::Track::replayGain( Meta::Track::ReplayGainMode mode ) const
{
    Q_UNUSED( mode )
    return 0.0;
}

qreal
Meta::Track::replayPeakGain( Meta::Track::ReplayGainMode mode ) const
{
    Q_UNUSED( mode )
    return 0.0;
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
            observer->metadataChanged( Meta::TrackPtr( const_cast<Meta::Track*>(this) ) );
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
Meta::Track::lessThan( const Meta::TrackPtr& left, const Meta::TrackPtr& right )
{
    if( !left || !right ) // These should never be 0, but it can apparently happen (http://bugs.kde.org/show_bug.cgi?id=181187)
        return false;

    if( left->album() && right->album() )
        if( left->album()->name() == right->album()->name() )
        {
            if( left->discNumber() < right->discNumber() )
                return true;
            else if( left->discNumber() > right->discNumber() )
                return false;

            if( left->trackNumber() < right->trackNumber() )
                return true;
            if( left->trackNumber() > right->trackNumber() )
                return false;
        }

    if( left->artist() && right->artist() )
    {
        int compare = QString::localeAwareCompare( left->artist()->prettyName(), right->artist()->prettyName() );
        if ( compare < 0 )
            return true;
        else if ( compare > 0 )
            return false;
    }

    if( left->album() && right->album() )
    {
        int compare = QString::localeAwareCompare( left->album()->prettyName(), right->album()->prettyName() );
        if ( compare < 0 )
            return true;
        else if ( compare > 0 )
            return false;
    }

    return QString::localeAwareCompare( left->prettyName(), right->prettyName() ) < 0;
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
    foreach( Observer *observer, m_observers )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->metadataChanged( Meta::ArtistPtr( const_cast<Meta::Artist*>(this) ) );
    }
}

bool
Meta::Artist::operator==( const Meta::Artist &artist ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const  void*>( &artist );
}

QString
Meta::Artist::sortableName() const
{
    if ( m_sortableName.isEmpty() && !name().isEmpty() ) {
        if ( name().startsWith( "the ", Qt::CaseInsensitive ) ) {
            QString begin = name().left( 3 );
            m_sortableName = QString( "%1, %2" ).arg( name(), begin );
            m_sortableName = m_sortableName.mid( 4 );
        }
        else
            m_sortableName = name();
    }
    return m_sortableName;
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
    foreach( Observer *observer, m_observers )
    {
        if( m_observers.contains( observer ) ) // guard against observers removing themselves in destructors
            observer->metadataChanged( Meta::AlbumPtr( const_cast<Meta::Album*>(this) ));
    }
}

QPixmap
Meta::Album::image( int size )
{
    /*
     * This is the base class's image() function, which returns "nocover" by default.
     * Retrieval of the cover for the actual album is done by subclasses.
     */
    const QDir &cacheCoverDir = QDir( Amarok::saveLocation( "albumcovers/cache/" ) );
    if( size <= 1 )
        size = 100;
    const QString &noCoverKey = QString::number( size ) + "@nocover.png";

    QPixmap pixmap;
    if( cacheCoverDir.exists( noCoverKey ) )
    {
        pixmap.load( cacheCoverDir.filePath( noCoverKey ) );
    }
    else
    {
        const QPixmap orgPixmap( KStandardDirs::locate( "data", "amarok/images/nocover.png" ) );
        pixmap = orgPixmap.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        pixmap.save( cacheCoverDir.filePath( noCoverKey ), "PNG" );
    }
    return pixmap;
}


QPixmap
Meta::Album::imageWithBorder( int size, int borderWidth )
{
    const int imageSize = size - ( borderWidth * 2 );
    const QString loc   = imageLocation( imageSize ).url();
    const QString key   = !loc.isEmpty() ? loc : name();
    const QPixmap cover = image( imageSize );
    return The::svgHandler()->addBordersToPixmap( cover, borderWidth, key );
}


bool
Meta::Album::operator==( const Meta::Album &album ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const void*>( &album );
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
        {
            observer->metadataChanged( Meta::GenrePtr( const_cast<Meta::Genre*>(this) ) );
        }
    }
}

bool
Meta::Genre::operator==( const Meta::Genre &genre ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const void*>( &genre );
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
            observer->metadataChanged( Meta::ComposerPtr( const_cast<Meta::Composer*>(this) ) );
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
            observer->metadataChanged( Meta::YearPtr( const_cast<Meta::Year *>(this) ) );
    }
}

bool
Meta::Year::operator==( const Meta::Year &year ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const  void*>( &year );
}

