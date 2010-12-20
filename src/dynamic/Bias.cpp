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
#include "BiasFactory.h"

#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"

#include "TrackSet.h"
#include "DynamicBiasWidgets.h"
#include "core/collections/QueryMaker.h"

#include <QDateTime>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


Dynamic::AbstractBias::AbstractBias()
{ }

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


QWidget*
Dynamic::AbstractBias::widget( QStandardItem* item, QWidget* parent )
{
    return new PlaylistBrowserNS::BiasWidget( BiasPtr(this), item, parent );
}

void
Dynamic::AbstractBias::addToModel( QStandardItemModel *model, QWidget *parentWidget, QModelIndex parentIndex )
{
    QStandardItem *item = new QStandardItem();
    item->setData( QVariant::fromValue( widget( item, parentWidget ) ), WidgetRole );

    if( parentIndex.isValid() )
        model->itemFromIndex( parentIndex )->appendRow( item );
    else
        model->appendRow( item );
}


void
Dynamic::AbstractBias::invalidate()
{ }

void
Dynamic::AbstractBias::replace( Dynamic::BiasPtr newBias )
{
    emit replaced( BiasPtr(const_cast<Dynamic::AbstractBias*>(this)), newBias );
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
Dynamic::RandomBias::name()
{
    return QLatin1String( "randomBias" );
}

QWidget*
Dynamic::RandomBias::widget( QStandardItem* item, QWidget* parent )
{
    return new PlaylistBrowserNS::BiasWidget( BiasPtr(this), item, parent );
}

Dynamic::TrackSet
Dynamic::RandomBias::matchingTracks( int position,
                                  const Meta::TrackList& playlist, int contextCount,
                                  Dynamic::TrackCollectionPtr universe ) const
{
    Q_UNUSED( position );
    Q_UNUSED( playlist );
    Q_UNUSED( contextCount );
    return Dynamic::TrackSet( universe );
}

double
Dynamic::RandomBias::energy( const Meta::TrackList& playlist, int contextCount ) const
{
    Q_UNUSED( playlist );
    Q_UNUSED( contextCount );
    return 0.0;
}

// -------- AndBias ------

Dynamic::AndBias::AndBias()
{ }

Dynamic::AndBias::AndBias( QXmlStreamReader *reader )
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
Dynamic::AndBias::name()
{
    return QLatin1String( "andBias" );
}

QWidget*
Dynamic::AndBias::widget( QStandardItem* item, QWidget* parent )
{
    return new PlaylistBrowserNS::LevelBiasWidget( this, item, parent );
}

Dynamic::TrackSet
Dynamic::AndBias::matchingTracks( int position,
                                  const Meta::TrackList& playlist, int contextCount,
                                  Dynamic::TrackCollectionPtr universe ) const
{
    m_tracks = Dynamic::TrackSet( universe );
    m_outstandingMatches = 0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        Dynamic::TrackSet tracks = bias->matchingTracks( position, playlist, contextCount, universe );
        if( tracks.isOutstanding() )
            m_outstandingMatches++;
        else
            m_tracks.intersect( bias->matchingTracks( position, playlist, contextCount, universe ) );

        if( m_tracks.isEmpty() )
            break;
    }

    if( m_outstandingMatches > 0 )
        return Dynamic::TrackSet();
    else
        return m_tracks;
}

double
Dynamic::AndBias::energy( const Meta::TrackList& playlist, int contextCount ) const
{
    double result = 0.0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        result = qMax( result, bias->energy( playlist, contextCount ) );
    }
    return result;
}

void
Dynamic::AndBias::addToModel( QStandardItemModel *model, QWidget *parentWidget, QModelIndex parentIndex )
{
    QStandardItem *item = new QStandardItem();
    item->setData( QVariant::fromValue( widget( item, parentWidget ) ), WidgetRole );

    if( parentIndex.isValid() )
        model->itemFromIndex( parentIndex )->appendRow( item );
    else
        model->appendRow( item );

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        bias->addToModel( model, parentWidget, item->index() );
    }

    // create the add bias widget
    QStandardItem *addItem = new QStandardItem();
    addItem->setData( QVariant::fromValue( qobject_cast<QWidget*>(new PlaylistBrowserNS::BiasAddWidget( addItem, parentWidget ) ) ), WidgetRole );
    item->appendRow( addItem );
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
    connect( bias.data(), SLOT( resultReady( const Dynamic::TrackSet & ) ),
             this,  SLOT( resultReceived( const Dynamic::TrackSet & ) ) );
    connect( bias.data(), SIGNAL( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ),
             this, SIGNAL( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ) );
    m_biases.append( bias );
    emit changed( BiasPtr(this) );
}

void
Dynamic::AndBias::moveBias( int from, int to )
{
    m_biases.insert( to, m_biases.takeAt( from ) );
    emit changed( BiasPtr(this) );
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
Dynamic::AndBias::biasReplaced( Dynamic::BiasPtr oldBias, Dynamic::BiasPtr newBias )
{
    int index = m_biases.indexOf( oldBias );
    Q_ASSERT( index >= 0 );

    disconnect( oldBias.data(), 0, this, 0 );
    m_biases[ index ] = newBias;
    connect( newBias.data(), SLOT( resultReady( const Dynamic::TrackSet & ) ),
             this,  SLOT( resultReceived( const Dynamic::TrackSet & ) ) );
    connect( newBias.data(), SIGNAL( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ),
             this, SIGNAL( biasReplaced( Dynamic::BiasPtr, Dynamic::BiasPtr ) ) );
    emit changed( BiasPtr(this) );
}


// -------- OrBias ------

Dynamic::OrBias::OrBias()
    : AndBias()
{ }

Dynamic::OrBias::OrBias( QXmlStreamReader *reader )
    : AndBias( reader )
{ }

QString
Dynamic::OrBias::name()
{
    return QLatin1String( "orBias" );
}

Dynamic::TrackSet
Dynamic::OrBias::matchingTracks( int position,
                                 const Meta::TrackList& playlist, int contextCount,
                                 Dynamic::TrackCollectionPtr universe ) const
{
    m_tracks = Dynamic::TrackSet( universe );
    m_tracks.clear();
    m_outstandingMatches = 0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        Dynamic::TrackSet tracks = bias->matchingTracks( position, playlist, contextCount, universe );
        if( tracks.isOutstanding() )
            m_outstandingMatches++;
        else
            m_tracks.unite( bias->matchingTracks( position, playlist, contextCount, universe ) );

        if( m_tracks.trackCount() == m_tracks.isFull() )
            break;
    }

    if( m_outstandingMatches > 0 )
        return Dynamic::TrackSet();
    else
        return m_tracks;
}

double
Dynamic::OrBias::energy( const Meta::TrackList& playlist, int contextCount ) const
{
    double result = 1.0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        result = qMin( result, bias->energy( playlist, contextCount ) );
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
Dynamic::NotBias::name()
{
    return QLatin1String( "orBias" );
}

Dynamic::TrackSet
Dynamic::NotBias::matchingTracks( int position,
                                 const Meta::TrackList& playlist, int contextCount,
                                 Dynamic::TrackCollectionPtr universe ) const
{
    m_tracks = Dynamic::TrackSet( universe );
    m_outstandingMatches = 0;

    foreach( Dynamic::BiasPtr bias, m_biases )
    {
        Dynamic::TrackSet tracks = bias->matchingTracks( position, playlist, contextCount, universe );
        if( tracks.isOutstanding() )
            m_outstandingMatches++;
        else
            m_tracks.subtract( bias->matchingTracks( position, playlist, contextCount, universe ) );

        if( m_tracks.trackCount() == m_tracks.isEmpty() )
            break;
    }

    if( m_outstandingMatches > 0 )
        return Dynamic::TrackSet();
    else
        return m_tracks;
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


// -------- TagMatchBias ------

Dynamic::TagMatchBias::TagMatchBias()
{ }

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

QWidget*
Dynamic::TagMatchBias::widget( QStandardItem* item, QWidget* parent )
{
    return new PlaylistBrowserNS::TagMatchBiasWidget( this, item, parent );
}

Dynamic::TrackSet
Dynamic::TagMatchBias::matchingTracks( int position,
                                       const Meta::TrackList& playlist, int contextCount,
                                       Dynamic::TrackCollectionPtr universe ) const
{
    Q_UNUSED( position );
    Q_UNUSED( playlist );
    Q_UNUSED( contextCount );
    Q_UNUSED( universe );

    if( m_tracksValid )
        return m_tracks;

    m_tracks = Dynamic::TrackSet( universe );
    newQuery();
    return Dynamic::TrackSet();
}

double
Dynamic::TagMatchBias::energy( const Meta::TrackList& playlist, int contextCount ) const
{
    if( playlist.count() - contextCount <= 0 )
        return 0.0;

    int matchCount = 0;
    for( int i = contextCount; i < playlist.count(); i++ )
    {
        if( matches( playlist.at( i ) ) )
            matchCount++;
    }
    return 1.0 - (double(matchCount) / (playlist.count() - contextCount));
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
    emit changed( BiasPtr(this) );
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

bool
Dynamic::TagMatchBias::matches( const Meta::TrackPtr &track ) const
{
    QVariant value = Meta::valueForField( m_filter.field, track );

    switch( m_filter.condition )
    {
    case MetaQueryWidget::Equals:
        return value.toLongLong() == m_filter.numValue;
    case MetaQueryWidget::GreaterThan:
        return value.toLongLong() > m_filter.numValue;
    case MetaQueryWidget::LessThan:
        return value.toLongLong() < m_filter.numValue;
    case MetaQueryWidget::Between:
        return value.toLongLong() > m_filter.numValue &&
               value.toLongLong() < m_filter.numValue2;
    case MetaQueryWidget::OlderThan:
        return value.toLongLong() < m_filter.numValue + QDateTime::currentDateTime().toTime_t();
    case MetaQueryWidget::Contains:
        return m_filter.value.contains( value.toString(), Qt::CaseInsensitive );
    default:
        ;// the other conditions are only for the advanced playlist generator
    }
    return false;
}

#include "Bias.moc"

