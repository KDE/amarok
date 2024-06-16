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

#include "core/support/Debug.h"
#include "dynamic/BiasFactory.h"
#include "dynamic/DynamicModel.h"
#include "dynamic/biases/SearchQueryBias.h"

#include <KLocalizedString>

#include <QPainter>
#include <QBuffer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

// -------- AbstractBias -------------

Dynamic::AbstractBias::AbstractBias()
{ }

Dynamic::AbstractBias::~AbstractBias()
{
    // debug() << "destroying bias" << this;
}

void
Dynamic::AbstractBias::fromXml( QXmlStreamReader *reader )
{
    reader->skipCurrentElement();
}


void
Dynamic::AbstractBias::toXml( QXmlStreamWriter *writer ) const
{
    Q_UNUSED( writer );
}

Dynamic::BiasPtr
Dynamic::AbstractBias::clone() const
{
    QByteArray bytes;
    QBuffer buffer( &bytes, nullptr );
    buffer.open( QIODevice::ReadWrite );

    // write the bias
    QXmlStreamWriter xmlWriter( &buffer );
    xmlWriter.writeStartElement( name() );
    toXml( &xmlWriter );
    xmlWriter.writeEndElement();

    // and read a new list
    buffer.seek( 0 );
    QXmlStreamReader xmlReader( &buffer );
    while( !xmlReader.isStartElement() )
        xmlReader.readNext();
    return Dynamic::BiasFactory::fromXml( &xmlReader );
}

QString
Dynamic::AbstractBias::sName()
{
    return QStringLiteral( "abstractBias" );
}

QString
Dynamic::AbstractBias::name() const
{
    return Dynamic::AbstractBias::sName();
}

QWidget*
Dynamic::AbstractBias::widget( QWidget* parent )
{
    Q_UNUSED( parent );
    return nullptr;
}

void
Dynamic::AbstractBias::paintOperator( QPainter* painter, const QRect& rect, Dynamic::AbstractBias* bias )
{
    Q_UNUSED( painter );
    Q_UNUSED( rect );
    Q_UNUSED( bias );
}

void
Dynamic::AbstractBias::invalidate()
{ }

void
Dynamic::AbstractBias::replace( const Dynamic::BiasPtr &newBias )
{
    Q_EMIT replaced( BiasPtr(const_cast<Dynamic::AbstractBias*>(this)), newBias );
}

// -------- RandomBias ------

Dynamic::RandomBias::RandomBias()
{ }

Dynamic::RandomBias::~RandomBias()
{ }

QString
Dynamic::RandomBias::sName()
{
    return QStringLiteral( "randomBias" );
}

QString
Dynamic::RandomBias::name() const
{
    return Dynamic::RandomBias::sName();
}

QString
Dynamic::RandomBias::toString() const
{
    return i18nc("Random bias representation", "Random tracks");
}

QWidget*
Dynamic::RandomBias::widget( QWidget* parent )
{
    Q_UNUSED( parent );
    return nullptr;
}

Dynamic::TrackSet
Dynamic::RandomBias::matchingTracks( const Meta::TrackList& playlist,
                                     int contextCount, int finalCount,
                                     const Dynamic::TrackCollectionPtr &universe ) const
{
    Q_UNUSED( playlist );
    Q_UNUSED( contextCount );
    Q_UNUSED( finalCount );
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

// -------- AndBias ------

Dynamic::AndBias::AndBias()
{ }

Dynamic::AndBias::~AndBias()
{ }

void
Dynamic::AndBias::fromXml( QXmlStreamReader *reader )
{
    while (!reader->atEnd()) {
        reader->readNext();

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
}

void
Dynamic::AndBias::toXml( QXmlStreamWriter *writer ) const
{
    for( Dynamic::BiasPtr bias : m_biases )
    {
        writer->writeStartElement( bias->name() );
        bias->toXml( writer );
        writer->writeEndElement();
    }
}

QString
Dynamic::AndBias::sName()
{
    return QStringLiteral( "andBias" );
}

QString
Dynamic::AndBias::name() const
{
    return Dynamic::AndBias::sName();
}

QString
Dynamic::AndBias::toString() const
{
    return i18nc("And bias representation", "Match all");
}


QWidget*
Dynamic::AndBias::widget( QWidget* parent )
{
    Q_UNUSED( parent );
    return nullptr;
}

void
Dynamic::AndBias::paintOperator( QPainter* painter, const QRect& rect, Dynamic::AbstractBias* bias )
{
    if( m_biases.indexOf( Dynamic::BiasPtr(bias) ) > 0 )
    {
        painter->drawText( rect.adjusted(2, 0, -2, 0),
                           Qt::AlignRight,
                           i18nc("Prefix for AndBias. Shown in front of a bias in the dynamic playlist view", "and" ) );
    }
}

Dynamic::TrackSet
Dynamic::AndBias::matchingTracks( const Meta::TrackList& playlist,
                                  int contextCount, int finalCount,
                                  const Dynamic::TrackCollectionPtr &universe ) const
{
    DEBUG_BLOCK;
debug() << "universe:" << universe.data();

    m_tracks = Dynamic::TrackSet( universe, true );
    m_outstandingMatches = 0;

    for( Dynamic::BiasPtr bias : m_biases )
    {
        Dynamic::TrackSet tracks = bias->matchingTracks( playlist, contextCount, finalCount, universe );
        if( tracks.isOutstanding() )
            m_outstandingMatches++;
        else
            m_tracks.intersect( tracks );

        //    debug() << "AndBias::matchingTracks" << bias->name() << "tracks:"<<tracks.trackCount() << "outstanding?" << tracks.isOutstanding() << "numOUt:" << m_outstandingMatches;

        if( m_tracks.isEmpty() )
            break;
    }

    // debug() << "AndBias::matchingTracks end: tracks:"<<m_tracks.trackCount() << "outstanding?" << m_tracks.isOutstanding() << "numOUt:" << m_outstandingMatches;

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
    for( Dynamic::BiasPtr bias : m_biases )
    {
        if( !bias->trackMatches( position, playlist, contextCount ) )
            return false;
    }
    return true;
}

void
Dynamic::AndBias::invalidate()
{
    for( Dynamic::BiasPtr bias : m_biases )
    {
        bias->invalidate();
    }
    m_tracks = TrackSet();
}

void
Dynamic::AndBias::appendBias(const BiasPtr &bias )
{
    bool newInModel = DynamicModel::instance()->index( bias ).isValid();
    if (newInModel) {
        warning() << "Argh, the old bias "<<bias->toString()<<"is still in a model";
        return;
    }

    BiasPtr thisPtr( this );
    bool inModel = DynamicModel::instance()->index( thisPtr ).isValid();
    if( inModel )
        DynamicModel::instance()->beginInsertBias( thisPtr, m_biases.count() );
    m_biases.append( bias );
    if( inModel )
        DynamicModel::instance()->endInsertBias();

    connect( bias.data(), &Dynamic::AbstractBias::resultReady,
             this, &AndBias::resultReceived );
    connect( bias.data(), &Dynamic::AbstractBias::replaced,
             this, &AndBias::biasReplaced );
    connect( bias.data(), &Dynamic::AbstractBias::changed,
             this, &AndBias::biasChanged );
    Q_EMIT biasAppended( bias );

    // creating a shared pointer and destructing it just afterwards would
    // also destruct this object.
    // so we give the object creating this bias a chance to increment the refcount
    Q_EMIT changed( thisPtr );
}

void
Dynamic::AndBias::moveBias( int from, int to )
{
    if( from == to )
        return;
    if( from < 0 || from >= m_biases.count() )
        return;
    if( to < 0 || to >= m_biases.count() )
        return;

    BiasPtr thisPtr( this );
    bool inModel = DynamicModel::instance()->index( thisPtr ).isValid();
    if( inModel )
        DynamicModel::instance()->beginMoveBias( thisPtr, from, to );
    m_biases.insert( to, m_biases.takeAt( from ) );
    if( inModel )
        DynamicModel::instance()->endMoveBias();

    Q_EMIT biasMoved( from, to );
    Q_EMIT changed( BiasPtr( this ) );
}


void
Dynamic::AndBias::resultReceived( const Dynamic::TrackSet &tracks )
{
    m_tracks.intersect( tracks );
    --m_outstandingMatches;

    if( m_outstandingMatches < 0 )
        warning() << "Received more results than expected.";
    else if( m_outstandingMatches == 0 )
        Q_EMIT resultReady( m_tracks );
}

void
Dynamic::AndBias::biasReplaced( const Dynamic::BiasPtr &oldBias, const Dynamic::BiasPtr &newBias )
{
    DEBUG_BLOCK;
    BiasPtr thisPtr( this );
    int index = m_biases.indexOf( oldBias );
    Q_ASSERT( index >= 0 );

    disconnect( oldBias.data(), nullptr, this, nullptr );
    bool inModel = DynamicModel::instance()->index( thisPtr ).isValid();
    if( inModel )
        DynamicModel::instance()->beginRemoveBias( thisPtr, index );
    m_biases.removeAt( index );
    if( inModel )
        DynamicModel::instance()->endRemoveBias();
    Q_EMIT biasRemoved( index );

    if( newBias )
    {
        connect( newBias.data(), &Dynamic::AbstractBias::resultReady,
                 this, &AndBias::resultReceived );
        connect( newBias.data(), &Dynamic::AbstractBias::replaced,
                 this, &AndBias::biasReplaced );
        connect( newBias.data(), &Dynamic::AbstractBias::changed,
                 this, &AndBias::changed );

        if( inModel )
            DynamicModel::instance()->beginInsertBias( thisPtr, index );
        m_biases.insert( index, newBias );
        if( inModel )
            DynamicModel::instance()->endInsertBias();

        // we don't have an bias inserted signal
        Q_EMIT biasAppended( newBias );
        Q_EMIT biasMoved( m_biases.count()-1, index );
    }

    Q_EMIT changed( thisPtr );
}

void
Dynamic::AndBias::biasChanged( const Dynamic::BiasPtr &bias )
{
    BiasPtr thisPtr( this );
    Q_EMIT changed( thisPtr );
    bool inModel = DynamicModel::instance()->index( thisPtr ).isValid();
    if( inModel )
        DynamicModel::instance()->biasChanged( bias );
}

// -------- OrBias ------

Dynamic::OrBias::OrBias()
    : AndBias()
{ }

QString
Dynamic::OrBias::sName()
{
    return QStringLiteral( "orBias" );
}

QString
Dynamic::OrBias::name() const
{
    return Dynamic::OrBias::sName();
}

void
Dynamic::OrBias::paintOperator( QPainter* painter, const QRect& rect, Dynamic::AbstractBias* bias )
{
    if( m_biases.indexOf( Dynamic::BiasPtr(bias) ) > 0 )
    {
        painter->drawText( rect.adjusted(2, 0, -2, 0),
                           Qt::AlignRight,
                           i18nc("Prefix for OrBias. Shown in front of a bias in the dynamic playlist view", "or" ) );
    }
}


QString
Dynamic::OrBias::toString() const
{
    return i18nc("Or bias representation", "Match any");
}

Dynamic::TrackSet
Dynamic::OrBias::matchingTracks( const Meta::TrackList& playlist,
                                 int contextCount, int finalCount,
                                 const Dynamic::TrackCollectionPtr &universe ) const
{
    m_tracks = Dynamic::TrackSet( universe, false );
    m_outstandingMatches = 0;

    for( Dynamic::BiasPtr bias : m_biases )
    {
        Dynamic::TrackSet tracks = bias->matchingTracks( playlist, contextCount, finalCount, universe );
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
    for( Dynamic::BiasPtr bias : m_biases )
    {
        if( bias->trackMatches( position, playlist, contextCount ) )
            return true;
    }
    return false;
}

void
Dynamic::OrBias::resultReceived( const Dynamic::TrackSet &tracks )
{
    m_tracks.unite( tracks );
    --m_outstandingMatches;

    if( m_outstandingMatches < 0 )
        warning() << "Received more results than expected.";
    else if( m_outstandingMatches == 0 )
        Q_EMIT resultReady( m_tracks );
}


