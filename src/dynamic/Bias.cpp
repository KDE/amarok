/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010,2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "Bias"

#include "Bias.h"
#include "BiasFactory.h"

#include "core/support/Debug.h"
#include "DynamicBiasWidgets.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

// -------- AbstractBias -------------

Dynamic::AbstractBias::AbstractBias()
{ }

Dynamic::AbstractBias::~AbstractBias()
{
    debug() << "destroying bias" << this << this->name();
}

void
Dynamic::AbstractBias::toXml( QXmlStreamWriter *writer ) const
{
    Q_UNUSED( writer );
}

QString
Dynamic::AbstractBias::sName()
{
    return QLatin1String( "abstractBias" );
}

QString
Dynamic::AbstractBias::name() const
{
    return Dynamic::AbstractBias::sName();
}

QWidget*
Dynamic::AbstractBias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::BiasWidget( BiasPtr(this), parent );
}

void
Dynamic::AbstractBias::invalidate()
{ }

void
Dynamic::AbstractBias::replace( Dynamic::BiasPtr newBias )
{
    emit replaced( BiasPtr(const_cast<Dynamic::AbstractBias*>(this)), newBias );
}

double
Dynamic::AbstractBias::energy( const Meta::TrackList& playlist, int contextCount ) const
{
    Q_UNUSED( contextCount );

    if( playlist.count() <= contextCount )
        return 0.0;

    int matchCount = 0;
    for( int i = contextCount; i < playlist.count(); i++ )
    {
        if( trackMatches( i, playlist, contextCount ) )
            matchCount++;
    }

    return 1.0 - (double(matchCount) / (playlist.count() - contextCount));
}


// -------- RandomBias ------

Dynamic::RandomBias::RandomBias()
{ }

Dynamic::RandomBias::RandomBias( QXmlStreamReader *reader )
{
    reader->skipCurrentElement();
}

Dynamic::RandomBias::~RandomBias()
{ }

void
Dynamic::RandomBias::toXml( QXmlStreamWriter *writer ) const
{
    Q_UNUSED( writer );
}

QString
Dynamic::RandomBias::sName()
{
    return QLatin1String( "randomBias" );
}

QString
Dynamic::RandomBias::name() const
{
    return Dynamic::RandomBias::sName();
}


QWidget*
Dynamic::RandomBias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::BiasWidget( BiasPtr(this), parent );
}

Dynamic::TrackSet
Dynamic::RandomBias::matchingTracks( int position,
                                  const Meta::TrackList& playlist, int contextCount,
                                  Dynamic::TrackCollectionPtr universe ) const
{
    Q_UNUSED( position );
    Q_UNUSED( playlist );
    Q_UNUSED( contextCount );
    return Dynamic::TrackSet( universe, true );
}

bool
Dynamic::RandomBias::trackMatches( int position,
                                   const Meta::TrackList& playlist,
                                   int contextCount ) const
{
    Q_UNUSED( position );
    Q_UNUSED( playlist );
    Q_UNUSED( contextCount );
    return true;
}

double
Dynamic::RandomBias::energy( const Meta::TrackList& playlist, int contextCount ) const
{
    Q_UNUSED( playlist );
    Q_UNUSED( contextCount );
    return 0.0;
}

// -------- UniqueBias ------

Dynamic::UniqueBias::UniqueBias()
{ }

Dynamic::UniqueBias::UniqueBias( QXmlStreamReader *reader )
{
    reader->skipCurrentElement();
}

Dynamic::UniqueBias::~UniqueBias()
{ }

void
Dynamic::UniqueBias::toXml( QXmlStreamWriter *writer ) const
{
    Q_UNUSED( writer );
}

QString
Dynamic::UniqueBias::sName()
{
    return QLatin1String( "uniqueBias" );
}

QString
Dynamic::UniqueBias::name() const
{
    return Dynamic::UniqueBias::sName();
}


QWidget*
Dynamic::UniqueBias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::BiasWidget( BiasPtr(this), parent );
}

Dynamic::TrackSet
Dynamic::UniqueBias::matchingTracks( int position,
                                  const Meta::TrackList& playlist, int contextCount,
                                  Dynamic::TrackCollectionPtr universe ) const
{
    Q_UNUSED( contextCount );

    Dynamic::TrackSet result = Dynamic::TrackSet( universe, true );
    for( int i = 0; i < position; i++ )
        result.subtract( playlist.at(i) );

    return result;
}

bool
Dynamic::UniqueBias::trackMatches( int position,
                                   const Meta::TrackList& playlist,
                                   int contextCount ) const
{
    Q_UNUSED( contextCount );

    for( int j = 0; j < position; j++ )
        if( playlist.at(position) == playlist.at(j) )
            return false;
    return true;
}


// -------- AndBias ------

Dynamic::AndBias::AndBias()
    : m_duringConstruction( true )
{
    // add one bias to start with
    appendBias( BiasPtr( new Dynamic::RandomBias() ) );

    m_duringConstruction = false;
}

Dynamic::AndBias::AndBias( QXmlStreamReader *reader )
    : m_duringConstruction( true )
{
    while (!reader->atEnd()) {
        reader->readNext();
debug() << "AndBias" << reader->name() << reader->isStartElement();

        if( reader->isStartElement() )
        {
            Dynamic::BiasPtr bias( Dynamic::BiasFactory::fromXml( reader ) );
            if( bias )
            {
                appendBias( bias );
            }
            else
            {
                warning()<<"Unexpected xml start element"<<reader->name()<<"in input";
                reader->skipCurrentElement();
            }
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }

    m_duringConstruction = false;
}

Dynamic::AndBias::~AndBias()
{ }

void
Dynamic::AndBias::toXml( QXmlStreamWriter *writer ) const
{
    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        writer->writeStartElement( bias->name() );
        bias->toXml( writer );
        writer->writeEndElement();
    }
}

QString
Dynamic::AndBias::sName()
{
    return QLatin1String( "andBias" );
}

QString
Dynamic::AndBias::name() const
{
    return Dynamic::AndBias::sName();
}


QWidget*
Dynamic::AndBias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::LevelBiasWidget( this, false, parent );
}

Dynamic::TrackSet
Dynamic::AndBias::matchingTracks( int position,
                                  const Meta::TrackList& playlist, int contextCount,
                                  Dynamic::TrackCollectionPtr universe ) const
{
    DEBUG_BLOCK;
debug() << "universe:" << universe.data();

    m_tracks = Dynamic::TrackSet( universe, true );
    m_outstandingMatches = 0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        Dynamic::TrackSet tracks = bias->matchingTracks( position, playlist, contextCount, universe );
        if( tracks.isOutstanding() )
            m_outstandingMatches++;
        else
            m_tracks.intersect( tracks );

    debug() << "AndBias::matchingTracks" << bias->name() << "tracks:"<<tracks.trackCount() << "outstanding?" << tracks.isOutstanding() << "numOUt:" << m_outstandingMatches;

        if( m_tracks.isEmpty() )
            break;
    }

    debug() << "AndBias::matchingTracks end: tracks:"<<m_tracks.trackCount() << "outstanding?" << m_tracks.isOutstanding() << "numOUt:" << m_outstandingMatches;

    if( m_outstandingMatches > 0 )
        return Dynamic::TrackSet();
    else
        return m_tracks;
}

bool
Dynamic::AndBias::trackMatches( int position,
                                const Meta::TrackList& playlist,
                                int contextCount ) const
{
    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        if( !bias->trackMatches( position, playlist, contextCount ) )
            return false;
    }
    return true;
}

double
Dynamic::AndBias::energy( const Meta::TrackList& playlist, int contextCount ) const
{
    double result = 0.0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        result = qMax( result, bias->energy( playlist, contextCount ) );
        if( result >= 1.0 )
            return result;
    }
    return result;
}


void
Dynamic::AndBias::invalidate()
{
    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        bias->invalidate();
    }
    m_tracks = TrackSet();
}

void
Dynamic::AndBias::appendBias( Dynamic::BiasPtr bias )
{
    m_biases.append( bias );
    connect( bias.data(), SIGNAL( resultReady( const Dynamic::TrackSet & ) ),
             this,  SLOT( resultReceived( const Dynamic::TrackSet & ) ) );
    connect( bias.data(), SIGNAL( replaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ),
             this, SLOT( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ) );
    connect( bias.data(), SIGNAL( changed( Dynamic::BiasPtr ) ),
             this, SIGNAL( changed( Dynamic::BiasPtr ) ) );
    emit biasAppended( bias );

    // creating a shared pointer and destructing it just afterwards would
    // also destruct this object.
    // so we give the object creating this bias a chance to increment the refcount
    if( !m_duringConstruction )
        emit changed( BiasPtr( this ) );
}

void
Dynamic::AndBias::moveBias( int from, int to )
{
    m_biases.insert( to, m_biases.takeAt( from ) );
    emit biasMoved( from, to );

    if( !m_duringConstruction )
        emit changed( BiasPtr( this ) );
}


void
Dynamic::AndBias::resultReceived( const Dynamic::TrackSet &tracks )
{
    m_tracks.intersect( tracks );
    --m_outstandingMatches;
    debug() << "AndBias::resultReceived" << m_outstandingMatches << "tr" << m_tracks.trackCount();

    if( m_outstandingMatches < 0 )
        warning() << "Received more results than expected.";
    else if( m_outstandingMatches == 0 )
        emit resultReady( m_tracks );
}

void
Dynamic::AndBias::biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias )
{
    int index = m_biases.indexOf( oldBias );
    Q_ASSERT( index >= 0 );

    disconnect( oldBias.data(), 0, this, 0 );

    if( newBias )
    {
        m_biases[ index ] = newBias;
        connect( newBias.data(), SIGNAL( resultReady( const Dynamic::TrackSet & ) ),
                 this,  SLOT( resultReceived( const Dynamic::TrackSet & ) ) );
        connect( newBias.data(), SIGNAL( replaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ),
                 this, SLOT( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ) );

        // we don't have an bias inserted signal
        emit biasRemoved( index );
        emit biasAppended( newBias );
        emit biasMoved( m_biases.count()-1, index );
    }
    else
    {
        m_biases.removeAt( index );
        emit biasRemoved( index );
    }

    if( !m_duringConstruction )
        emit changed( BiasPtr( this ) );
}

// -------- OrBias ------

Dynamic::OrBias::OrBias()
    : AndBias()
{ }

Dynamic::OrBias::OrBias( QXmlStreamReader *reader )
    : AndBias( reader )
{ }

QString
Dynamic::OrBias::sName()
{
    return QLatin1String( "orBias" );
}

QString
Dynamic::OrBias::name() const
{
    return Dynamic::OrBias::sName();
}

Dynamic::TrackSet
Dynamic::OrBias::matchingTracks( int position,
                                 const Meta::TrackList& playlist, int contextCount,
                                 Dynamic::TrackCollectionPtr universe ) const
{
    m_tracks = Dynamic::TrackSet( universe, false );
    m_outstandingMatches = 0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        Dynamic::TrackSet tracks = bias->matchingTracks( position, playlist, contextCount, universe );
        if( tracks.isOutstanding() )
            m_outstandingMatches++;
        else
            m_tracks.unite( tracks );

        if( m_tracks.isFull() )
            break;
    }

    if( m_outstandingMatches > 0 )
        return Dynamic::TrackSet();
    else
        return m_tracks;
}

bool
Dynamic::OrBias::trackMatches( int position,
                               const Meta::TrackList& playlist,
                               int contextCount ) const
{
    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        if( bias->trackMatches( position, playlist, contextCount ) )
            return true;
    }
    return false;
}

double
Dynamic::OrBias::energy( const Meta::TrackList& playlist, int contextCount ) const
{
    double result = 1.0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        result = qMin( result, bias->energy( playlist, contextCount ) );
        if( result <= 0.0 )
            return result;
    }
    return result;
}

void
Dynamic::OrBias::resultReceived( const Dynamic::TrackSet &tracks )
{
    m_tracks.unite( tracks );
    --m_outstandingMatches;

    if( m_outstandingMatches < 0 )
        warning() << "Received more results than expected.";
    else if( m_outstandingMatches == 0 )
        emit resultReady( m_tracks );
}

// -------- NotBias ------

Dynamic::NotBias::NotBias()
    : AndBias()
{ }

Dynamic::NotBias::NotBias( QXmlStreamReader *reader )
    : AndBias( reader )
{ }

QString
Dynamic::NotBias::sName()
{
    return QLatin1String( "notBias" );
}

QString
Dynamic::NotBias::name() const
{
    return Dynamic::NotBias::sName();
}

Dynamic::TrackSet
Dynamic::NotBias::matchingTracks( int position,
                                 const Meta::TrackList& playlist, int contextCount,
                                 Dynamic::TrackCollectionPtr universe ) const
{
    m_tracks = Dynamic::TrackSet( universe, true );
    m_outstandingMatches = 0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        Dynamic::TrackSet tracks = bias->matchingTracks( position, playlist, contextCount, universe );
        if( tracks.isOutstanding() )
            m_outstandingMatches++;
        else
            m_tracks.subtract( tracks );

        if( m_tracks.isEmpty() )
            break;
    }

    if( m_outstandingMatches > 0 )
        return Dynamic::TrackSet();
    else
        return m_tracks;
}

bool
Dynamic::NotBias::trackMatches( int position,
                               const Meta::TrackList& playlist,
                               int contextCount ) const
{
    return ! Dynamic::AndBias::trackMatches( position, playlist, contextCount );
}

double
Dynamic::NotBias::energy( const Meta::TrackList& playlist, int contextCount ) const
{
    return 1.0 - Dynamic::AndBias::energy( playlist, contextCount );
}

void
Dynamic::NotBias::resultReceived( const Dynamic::TrackSet &tracks )
{
    m_tracks.subtract( tracks );
    --m_outstandingMatches;

    if( m_outstandingMatches < 0 )
        warning() << "Received more results than expected.";
    else if( m_outstandingMatches == 0 )
        emit resultReady( m_tracks );
}

#include "Bias.moc"

