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

#define DEBUG_PREFIX "TagMatchBias"

#include "TagMatchBias.h"

#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"

#include "TrackSet.h"
#include "DynamicBiasWidgets.h"
#include "core/collections/QueryMaker.h"

#include <QDateTime>
#include <QTimer>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


Dynamic::TagMatchBias::TagMatchBias()
    : m_tracksValid( false )
{ }

Dynamic::TagMatchBias::TagMatchBias( QXmlStreamReader *reader )
    : m_tracksValid( false )
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
Dynamic::TagMatchBias::sName()
{
    return QLatin1String( "tagMatchBias" );
}

QString
Dynamic::TagMatchBias::name() const
{
    return Dynamic::TagMatchBias::sName();
}


QWidget*
Dynamic::TagMatchBias::widget( QWidget* parent )
{
    return new PlaylistBrowserNS::TagMatchBiasWidget( this, parent );
}

Dynamic::TrackSet
Dynamic::TagMatchBias::matchingTracks( int position,
                                       const Meta::TrackList& playlist, int contextCount,
                                       Dynamic::TrackCollectionPtr universe ) const
{
    DEBUG_BLOCK

    Q_UNUSED( position );
    Q_UNUSED( playlist );
    Q_UNUSED( contextCount );
    Q_UNUSED( universe );

    if( m_tracksValid )
        return m_tracks;

    m_tracks = Dynamic::TrackSet( universe, false );

    QTimer::singleShot(0,
                       const_cast<TagMatchBias*>(this),
                       SLOT(newQuery())); // create the new query from my parent thread

    return Dynamic::TrackSet();
}

bool
Dynamic::TagMatchBias::trackMatches( int position,
                                     const Meta::TrackList& playlist,
                                     int contextCount ) const
{
    return matches( playlist.at(position) );
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
    debug() << "TagMatchBias::updateFinished" << m_tracks.trackCount();
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
    DEBUG_BLOCK;
    m_filter = filter;
    invalidate();
    emit changed( BiasPtr(this) );
}

void
Dynamic::TagMatchBias::invalidate()
{
    m_tracksValid = false;
    m_tracks = TrackSet();
    // TODO: need to finish a running query
    m_qm.reset();
}

void
Dynamic::TagMatchBias::newQuery() const
{
    DEBUG_BLOCK;

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

#include "TagMatchBias.moc"

