/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"

#include "TrackSet.h"
#include "DynamicBiasWidgets.h"
// #include "DynamicModel.h"
// #include "core/meta/support/MetaConstants.h"
// #include "core/collections/MetaQueryMaker.h"
#include "core/collections/QueryMaker.h"

// #include <QMutexLocker>
#include <QDateTime>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

Dynamic::AbstractBias::~AbstractBias()
{ }

void
Dynamic::AbstractBias::toXml( QXmlStreamWriter *writer ) const
{
    Q_UNUSED( writer );
}

QString
Dynamic::AbstractBias::name()
{
    return QLatin1String( "abstractBias" );
}


PlaylistBrowserNS::BiasWidget*
Dynamic::AbstractBias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::BiasWidget( this, parent );
}

void
Dynamic::AbstractBias::invalidate()
{ }

Dynamic::AbstractBias*
Dynamic::AbstractBias::fromXml( QXmlStreamReader *reader )
{
    QStringRef name = reader->name();
    if( name == Dynamic::AndBias::name() )
        return new Dynamic::AndBias( reader );
    else if( name == Dynamic::OrBias::name() )
        return new Dynamic::OrBias( reader );
    else if( name == Dynamic::TagMatchBias::name() )
        return new Dynamic::TagMatchBias( reader );
    else
        return 0;
}

// -------- AndBias ------

Dynamic::AndBias::AndBias( QXmlStreamReader *reader )
{
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            Dynamic::AbstractBias *bias = fromXml( reader );
            if( bias )
            {
                m_biases.append(bias);
            }
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

Dynamic::AndBias::~AndBias()
{
    foreach( Dynamic::AbstractBias* bias, m_biases )
    {
        delete bias;
    }
}

void
Dynamic::AndBias::toXml( QXmlStreamWriter *writer ) const
{
    foreach( Dynamic::AbstractBias* bias, m_biases )
    {
        writer->writeStartElement( bias->name() );
        bias->toXml( writer );
        writer->writeEndElement();
    }
}

QString
Dynamic::AndBias::name()
{
    return QLatin1String( "andBias" );
}

Dynamic::TrackSet
Dynamic::AndBias::matchingTracks( int position,
                                  const Meta::TrackList& playlist,
                                  Dynamic::TrackCollectionPtr universe ) const
{
    m_tracks = Dynamic::TrackSet( universe );
    m_outstandingMatches = 0;

    foreach( Dynamic::AbstractBias* bias, m_biases )
    {
        Dynamic::TrackSet tracks = bias->matchingTracks( position, playlist, universe );
        if( tracks.isOutstanding() )
            m_outstandingMatches++;
        else
            m_tracks.intersect( bias->matchingTracks( position, playlist, universe ) );

        if( m_tracks.isEmpty() )
            break;
    }

    if( m_outstandingMatches > 0 )
        return Dynamic::TrackSet();
    else
        return m_tracks;
}

void
Dynamic::AndBias::resultReceived( const Dynamic::TrackSet &tracks )
{
    m_tracks.intersect( tracks );
    --m_outstandingMatches;

    if( m_outstandingMatches < 0 )
        warning() << "Received more results than expected.";
    else if( m_outstandingMatches == 0 )
        emit resultReady( m_tracks );
}

void
Dynamic::AndBias::invalidate()
{
    foreach( Dynamic::AbstractBias* bias, m_biases )
    {
        bias->invalidate();
    }
    m_tracks = TrackSet();
}

void
Dynamic::AndBias::appendBias( Dynamic::AbstractBias* bias )
{
    connect( bias, SLOT( resultReady( const Dynamic::TrackSet & ) ),
             this,  SLOT( resultReceived( const Dynamic::TrackSet & ) ) );
    m_biases.append( bias );
    emit changed( this );
}

void
Dynamic::AndBias::removeBiasAt( int i )
{
    delete m_biases.takeAt( i );
    emit changed( this );
}

void
Dynamic::AndBias::moveBias( int from, int to )
{
    m_biases.insert( to, m_biases.takeAt( from ) );
    emit changed( this );
}

// -------- OrBias ------

QString
Dynamic::OrBias::name()
{
    return QLatin1String( "orBias" );
}

Dynamic::TrackSet
Dynamic::OrBias::matchingTracks( int position,
                                 const Meta::TrackList& playlist,
                                 Dynamic::TrackCollectionPtr universe ) const
{
    m_tracks = Dynamic::TrackSet( universe );
    m_tracks.clear();
    m_outstandingMatches = 0;

    foreach( Dynamic::AbstractBias* bias, m_biases )
    {
        Dynamic::TrackSet tracks = bias->matchingTracks( position, playlist, universe );
        if( tracks.isOutstanding() )
            m_outstandingMatches++;
        else
            m_tracks.unite( bias->matchingTracks( position, playlist, universe ) );

        if( m_tracks.trackCount() == m_tracks.isFull() )
            break;
    }

    if( m_outstandingMatches > 0 )
        return Dynamic::TrackSet();
    else
        return m_tracks;
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


// -------- TagMatchBias ------

Dynamic::TagMatchBias::TagMatchBias( QXmlStreamReader *reader )
{
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "field" )
                m_filter.field = Meta::fieldForPlaylistName( reader->readElementText(QXmlStreamReader::SkipChildElements) );
            else if( name == "numValue" )
                m_filter.numValue = reader->readElementText(QXmlStreamReader::SkipChildElements).toUInt();
            else if( name == "numValue2" )
                m_filter.numValue2 = reader->readElementText(QXmlStreamReader::SkipChildElements).toUInt();
            else if( name == "value" )
                m_filter.value = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == "condition" )
                m_filter.condition = conditionForName( reader->readElementText(QXmlStreamReader::SkipChildElements) );
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
Dynamic::TagMatchBias::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( "field", Meta::playlistNameForField( m_filter.field ) );

    if( m_filter.isNumeric() )
    {
        writer->writeTextElement( "numValue",  QString::number( m_filter.numValue ) );
        writer->writeTextElement( "numValue2", QString::number( m_filter.numValue2 ) );
    }
    else
    {
        writer->writeTextElement( "value", m_filter.value );
    }

    writer->writeTextElement( "condition", nameForCondition( m_filter.condition ) );
}

QString
Dynamic::TagMatchBias::name()
{
    return QLatin1String( "tagMatchBias" );
}

Dynamic::TrackSet
Dynamic::TagMatchBias::matchingTracks( int position,
                                       const Meta::TrackList& playlist,
                                       Dynamic::TrackCollectionPtr universe ) const
{
    Q_UNUSED( position );
    Q_UNUSED( playlist );
    Q_UNUSED( universe );

    if( m_tracksValid )
        return m_tracks;

    m_tracks = Dynamic::TrackSet( universe );
    newQuery();
    return Dynamic::TrackSet();
}

void
Dynamic::TagMatchBias::updateReady( QString collectionId, QStringList uids )
{
    Q_UNUSED( collectionId );
    m_tracks.unite( uids );
}

void
Dynamic::TagMatchBias::updateFinished()
{
    m_tracksValid = true;
    m_qm.reset();
    emit resultReady( m_tracks );
}

MetaQueryWidget::Filter
Dynamic::TagMatchBias::filter() const
{
    return m_filter;
}

void
Dynamic::TagMatchBias::setFilter( const MetaQueryWidget::Filter &filter)
{
    m_filter = filter;
    invalidate();
    emit changed( this );
}

void
Dynamic::TagMatchBias::invalidate()
{
    m_tracksValid = false;
    m_tracks = TrackSet();
    m_qm.reset();
}

void
Dynamic::TagMatchBias::newQuery() const
{
    // ok, I need a new query maker
    m_qm.reset( CollectionManager::instance()->queryMaker() );

    // -- set the querymaker
    switch( m_filter.condition )
    {
    case MetaQueryWidget::Equals:
    case MetaQueryWidget::GreaterThan:
    case MetaQueryWidget::LessThan:
        m_qm->addNumberFilter( m_filter.field, m_filter.numValue,
                               (Collections::QueryMaker::NumberComparison)m_filter.condition );
        break;
    case MetaQueryWidget::Between:
        m_qm->beginAnd();
        m_qm->addNumberFilter( m_filter.field, qMin(m_filter.numValue, m_filter.numValue2)-1,
                               Collections::QueryMaker::GreaterThan );
        m_qm->addNumberFilter( m_filter.field, qMax(m_filter.numValue, m_filter.numValue2)+1,
                               Collections::QueryMaker::LessThan );
        m_qm->endAndOr();
        break;
    case MetaQueryWidget::OlderThan:
        m_qm->addNumberFilter( m_filter.field, QDateTime::currentDateTime().toTime_t() - m_filter.numValue,
                               Collections::QueryMaker::LessThan );
        break;

    case MetaQueryWidget::Contains:
        if( m_filter.field == 0 )
        {
            // simple search
            // TODO: split different words and make seperate searches
            m_qm->beginOr();
            m_qm->addFilter( Meta::valArtist,  m_filter.value );
            m_qm->addFilter( Meta::valTitle,   m_filter.value );
            m_qm->addFilter( Meta::valAlbum,   m_filter.value );
            m_qm->addFilter( Meta::valGenre,   m_filter.value );
            m_qm->addFilter( Meta::valUrl,     m_filter.value );
            m_qm->addFilter( Meta::valComment, m_filter.value );
            m_qm->addFilter( Meta::valLabel,   m_filter.value );
            m_qm->endAndOr();
        }
        else
        {
            m_qm->addFilter( m_filter.field, m_filter.value );
        }
        break;

    default:
        ;// the other conditions are only for the advanced playlist generator
    }

    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );
    m_qm->orderByRandom(); // as to not affect the amortized time (whatever that means)

    connect( m_qm.data(), SIGNAL(newResultReady( QString, QStringList )),
             this, SLOT(updateReady( QString, QStringList )) );
    connect( m_qm.data(), SIGNAL(queryDone()),
             this, SLOT(updateFinished()) );
    m_qm.data()->run();
}

QString
Dynamic::TagMatchBias::nameForCondition( MetaQueryWidget::FilterCondition cond )
{
    switch( cond )
    {
    case MetaQueryWidget::Equals:      return "equals";
    case MetaQueryWidget::GreaterThan: return "greater";
    case MetaQueryWidget::LessThan:    return "less";
    case MetaQueryWidget::Between:     return "between";
    case MetaQueryWidget::OlderThan:   return "older";
    case MetaQueryWidget::Contains:    return "contains";
    default:
        ;// the other conditions are only for the advanced playlist generator
    }
    return QString();
}

MetaQueryWidget::FilterCondition
Dynamic::TagMatchBias::conditionForName( const QString &name )
{
    if( name == "equals" )        return MetaQueryWidget::Equals;
    else if( name == "greater" )  return MetaQueryWidget::GreaterThan;
    else if( name == "less" )     return MetaQueryWidget::LessThan;
    else if( name == "between" )  return MetaQueryWidget::Between;
    else if( name == "older" )    return MetaQueryWidget::OlderThan;
    else if( name == "contains" ) return MetaQueryWidget::Contains;
    else return MetaQueryWidget::Equals;
}

