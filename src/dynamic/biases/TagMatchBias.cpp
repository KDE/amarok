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
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "dynamic/TrackSet.h"

#include <QDateTime>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

QString
Dynamic::TagMatchBiasFactory::i18nName() const
{ return i18nc("Name of the \"TagMatch\" bias", "Match meta tag"); }

QString
Dynamic::TagMatchBiasFactory::name() const
{ return Dynamic::TagMatchBias::sName(); }

QString
Dynamic::TagMatchBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"TagMatch\" bias",
                   "The \"TagMatch\" bias adds tracks that\n"
                   "fulfill a specific condition."); }

Dynamic::BiasPtr
Dynamic::TagMatchBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::TagMatchBias() ); }


// ----- SimpleMatchBias --------

Dynamic::SimpleMatchBias::SimpleMatchBias()
    : m_invert( false )
{ }

void
Dynamic::SimpleMatchBias::fromXml( QXmlStreamReader *reader )
{
    m_invert = reader->attributes().value( QStringLiteral("invert") ).toString().toInt();
}

void
Dynamic::SimpleMatchBias::toXml( QXmlStreamWriter *writer ) const
{
    if( m_invert )
        writer->writeAttribute(QStringLiteral("invert"), QStringLiteral("1"));
}

bool
Dynamic::SimpleMatchBias::isInvert() const
{
    return m_invert;
}

void
Dynamic::SimpleMatchBias::setInvert( bool value )
{
    DEBUG_BLOCK;
    if( value == m_invert )
        return;

    m_invert = value;
    // setting "invert" does not invalidate the search results
    Q_EMIT changed( BiasPtr(this) );
}


Dynamic::TrackSet
Dynamic::SimpleMatchBias::matchingTracks( const Meta::TrackList& playlist,
                                          int contextCount, int finalCount,
                                          const Dynamic::TrackCollectionPtr &universe ) const
{
    Q_UNUSED( playlist );
    Q_UNUSED( contextCount );
    Q_UNUSED( finalCount );

    if( tracksValid() )
        return m_tracks;

    m_tracks = Dynamic::TrackSet( universe, m_invert );

    QTimer::singleShot(0,
                       const_cast<SimpleMatchBias*>(this),
                       &SimpleMatchBias::newQuery); // create the new query from my parent thread

    return Dynamic::TrackSet();
}

void
Dynamic::SimpleMatchBias::updateReady( const QStringList &uids )
{
    if( m_invert )
        m_tracks.subtract( uids );
    else
        m_tracks.unite( uids );
}

void
Dynamic::SimpleMatchBias::updateFinished()
{
    m_tracksTime = QDateTime::currentDateTime();
    m_qm.reset();
    debug() << "SimpleMatchBias::"<<name()<<"updateFinished"<<m_tracks.trackCount();
    Q_EMIT resultReady( m_tracks );
}

bool
Dynamic::SimpleMatchBias::tracksValid() const
{
    return m_tracksTime.isValid() &&
        m_tracksTime.secsTo(QDateTime::currentDateTime()) < 60 * 3;
}

bool
Dynamic::SimpleMatchBias::trackMatches( int position,
                                        const Meta::TrackList& playlist,
                                        int contextCount ) const
{
    Q_UNUSED( contextCount );
    if( tracksValid() )
        return m_tracks.contains( playlist.at(position) );
    return true; // we should have already received the tracks before some-one calls trackMatches
}


void
Dynamic::SimpleMatchBias::invalidate()
{
    m_tracksTime = QDateTime();
    m_tracks = TrackSet();
    // TODO: need to finish a running query
    m_qm.reset();
}

// ---------- TagMatchBias --------


Dynamic::TagMatchBiasWidget::TagMatchBiasWidget( Dynamic::TagMatchBias* bias,
                                                           QWidget* parent )
    : QWidget( parent )
    , m_bias( bias )
{
    QVBoxLayout *layout = new QVBoxLayout( this );

    QHBoxLayout *invertLayout = new QHBoxLayout();
    m_invertBox = new QCheckBox();
    QLabel *label = new QLabel( i18n("Invert condition") );
    label->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    label->setBuddy( m_invertBox );
    invertLayout->addWidget( m_invertBox, 0 );
    invertLayout->addWidget( label, 1 );
    layout->addLayout(invertLayout);

    m_queryWidget = new MetaQueryWidget();
    layout->addWidget( m_queryWidget );

    syncControlsToBias();

    connect( m_invertBox, &QCheckBox::toggled,
             this, &TagMatchBiasWidget::syncBiasToControls );
    connect( m_queryWidget, &MetaQueryWidget::changed,
             this, &TagMatchBiasWidget::syncBiasToControls );
}

void
Dynamic::TagMatchBiasWidget::syncControlsToBias()
{
    m_queryWidget->setFilter( m_bias->filter() );
    m_invertBox->setChecked( m_bias->isInvert() );
}

void
Dynamic::TagMatchBiasWidget::syncBiasToControls()
{
    m_bias->setFilter( m_queryWidget->filter() );
    m_bias->setInvert( m_invertBox->isChecked() );
}



// ----- TagMatchBias --------

Dynamic::TagMatchBias::TagMatchBias()
    : SimpleMatchBias()
{ }

void
Dynamic::TagMatchBias::fromXml( QXmlStreamReader *reader )
{
    SimpleMatchBias::fromXml( reader );
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringView name = reader->name();
            if( name == QStringLiteral("field") )
                m_filter.setField( Meta::fieldForPlaylistName( reader->readElementText(QXmlStreamReader::SkipChildElements) ) );
            else if( name == QStringLiteral("numValue") )
                m_filter.numValue = reader->readElementText(QXmlStreamReader::SkipChildElements).toUInt();
            else if( name == QStringLiteral("numValue2") )
                m_filter.numValue2 = reader->readElementText(QXmlStreamReader::SkipChildElements).toUInt();
            else if( name == QStringLiteral("value") )
                m_filter.value = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == QStringLiteral("condition") )
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
    SimpleMatchBias::toXml( writer );
    writer->writeTextElement( QStringLiteral("field"), Meta::playlistNameForField( m_filter.field() ) );

    if( m_filter.isNumeric() )
    {
        writer->writeTextElement( QStringLiteral("numValue"),  QString::number( m_filter.numValue ) );
        writer->writeTextElement( QStringLiteral("numValue2"), QString::number( m_filter.numValue2 ) );
    }
    else
    {
        writer->writeTextElement( QStringLiteral("value"), m_filter.value );
    }

    writer->writeTextElement( QStringLiteral("condition"), nameForCondition( m_filter.condition ) );
}

QString
Dynamic::TagMatchBias::sName()
{
    return QStringLiteral( "tagMatchBias" );
}

QString
Dynamic::TagMatchBias::name() const
{
    return Dynamic::TagMatchBias::sName();
}

QString
Dynamic::TagMatchBias::toString() const
{
    if( isInvert() )
        return i18nc("Inverted condition in tag match bias",
                     "Not %1", m_filter.toString() );
    else
        return m_filter.toString();
}

QWidget*
Dynamic::TagMatchBias::widget( QWidget* parent )
{
    return new Dynamic::TagMatchBiasWidget( this, parent );
}

bool
Dynamic::TagMatchBias::trackMatches( int position,
                                     const Meta::TrackList& playlist,
                                     int contextCount ) const
{
    Q_UNUSED( contextCount );
    if( tracksValid() )
        return m_tracks.contains( playlist.at(position) );
    else
        return matches( playlist.at(position) );
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
    Q_EMIT changed( BiasPtr(this) );
}

void
Dynamic::TagMatchBias::newQuery()
{
    DEBUG_BLOCK;

    // ok, I need a new query maker
    m_qm.reset( CollectionManager::instance()->queryMaker() );

    // -- set the querymaker
    if( m_filter.isDate() )
    {
        switch( m_filter.condition )
        {
        case MetaQueryWidget::LessThan:
        case MetaQueryWidget::Equals:
        case MetaQueryWidget::GreaterThan:
            m_qm->addNumberFilter( m_filter.field(), m_filter.numValue,
                                (Collections::QueryMaker::NumberComparison)m_filter.condition );
            break;
        case MetaQueryWidget::Between:
            m_qm->beginAnd();
            m_qm->addNumberFilter( m_filter.field(), qMin(m_filter.numValue, m_filter.numValue2)-1,
                                Collections::QueryMaker::GreaterThan );
            m_qm->addNumberFilter( m_filter.field(), qMax(m_filter.numValue, m_filter.numValue2)+1,
                                Collections::QueryMaker::LessThan );
            m_qm->endAndOr();
            break;
        case MetaQueryWidget::OlderThan:
            m_qm->addNumberFilter( m_filter.field(), QDateTime::currentDateTimeUtc().toSecsSinceEpoch() - m_filter.numValue,
                                Collections::QueryMaker::LessThan );
            break;
        case MetaQueryWidget::NewerThan:
            m_qm->addNumberFilter( m_filter.field(), QDateTime::currentDateTimeUtc().toSecsSinceEpoch() - m_filter.numValue,
                                Collections::QueryMaker::GreaterThan );
            break;
        default:
            ;
        }
    }
    else if( m_filter.isNumeric() )
    {
        qint64 nval1 = m_filter.numValue;
        qint64 nval2 = m_filter.numValue2;
        if( m_filter.field() == Meta::valLength )
        {
            // Length is msecs in database, corresponding conversion done in TextualQueryFilter.cpp:157
            // BUG 375565
            nval1 = nval1 * 1000;
            nval2 = nval2 * 1000;
        }
        switch( m_filter.condition )
        {
        case MetaQueryWidget::LessThan:
        case MetaQueryWidget::Equals:
        case MetaQueryWidget::GreaterThan:
            m_qm->addNumberFilter( m_filter.field(), nval1,
                                (Collections::QueryMaker::NumberComparison)m_filter.condition );
            break;
        case MetaQueryWidget::Between:
            m_qm->beginAnd();
            m_qm->addNumberFilter( m_filter.field(), qMin(nval1, nval2)-1,
                                Collections::QueryMaker::GreaterThan );
            m_qm->addNumberFilter( m_filter.field(), qMax(nval1, nval2)+1,
                                Collections::QueryMaker::LessThan );
            m_qm->endAndOr();
            break;
        default:
            ;
        }
    }
    else
    {
        switch( m_filter.condition )
        {
        case MetaQueryWidget::Equals:
            m_qm->addFilter( m_filter.field(), m_filter.value, true, true );
            break;
        case MetaQueryWidget::Contains:
            if( m_filter.field() == 0 )
            {
                // simple search
                // TODO: split different words and make separate searches
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
                m_qm->addFilter( m_filter.field(), m_filter.value );
            }
            break;
        default:
            ;
        }
    }

    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );

    connect( m_qm.data(), &Collections::QueryMaker::newResultReady,
             this, &TagMatchBias::updateReady, Qt::QueuedConnection );
    connect( m_qm.data(), &Collections::QueryMaker::queryDone,
             this, &TagMatchBias::updateFinished, Qt::QueuedConnection );
    m_qm.data()->run();
}

QString
Dynamic::TagMatchBias::nameForCondition( MetaQueryWidget::FilterCondition cond )
{
    switch( cond )
    {
    case MetaQueryWidget::Equals:      return QStringLiteral("equals");
    case MetaQueryWidget::GreaterThan: return QStringLiteral("greater");
    case MetaQueryWidget::LessThan:    return QStringLiteral("less");
    case MetaQueryWidget::Between:     return QStringLiteral("between");
    case MetaQueryWidget::OlderThan:   return QStringLiteral("older");
    case MetaQueryWidget::NewerThan:   return QStringLiteral("newer");
    case MetaQueryWidget::Contains:    return QStringLiteral("contains");
    default:
        ;// the other conditions are only for the advanced playlist generator
    }
    return QString();
}

MetaQueryWidget::FilterCondition
Dynamic::TagMatchBias::conditionForName( const QString &name )
{
    if( name == QLatin1String("equals") )        return MetaQueryWidget::Equals;
    else if( name == QLatin1String("greater") )  return MetaQueryWidget::GreaterThan;
    else if( name == QLatin1String("less") )     return MetaQueryWidget::LessThan;
    else if( name == QLatin1String("between") )  return MetaQueryWidget::Between;
    else if( name == QLatin1String("older") )    return MetaQueryWidget::OlderThan;
    else if( name == QLatin1String("newer") )    return MetaQueryWidget::NewerThan;
    else if( name == QLatin1String("contains") ) return MetaQueryWidget::Contains;
    else return MetaQueryWidget::Equals;
}

bool
Dynamic::TagMatchBias::matches( const Meta::TrackPtr &track ) const
{
    QVariant value = Meta::valueForField( m_filter.field(), track );

    bool result = false;
    if( m_filter.isDate() )
    {
        switch( m_filter.condition )
        {
        case MetaQueryWidget::LessThan:
            result = value.toLongLong() < m_filter.numValue;
            break;
        case MetaQueryWidget::Equals:
            result = value.toLongLong() == m_filter.numValue;
            break;
        case MetaQueryWidget::GreaterThan:
            result = value.toLongLong() > m_filter.numValue;
            break;
        case MetaQueryWidget::Between:
            result = value.toLongLong() > m_filter.numValue &&
                value.toLongLong() < m_filter.numValue2;
            break;
        case MetaQueryWidget::OlderThan:
            result = value.toLongLong() < m_filter.numValue + QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
            break;
        case MetaQueryWidget::NewerThan:
            result = value.toLongLong() > m_filter.numValue + QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
            break;
        default:
            ;
        }
    }
    else if( m_filter.isNumeric() )
    {
        switch( m_filter.condition )
        {
        case MetaQueryWidget::LessThan:
            result = value.toLongLong() < m_filter.numValue;
            break;
        case MetaQueryWidget::Equals:
            result = value.toLongLong() == m_filter.numValue;
            break;
        case MetaQueryWidget::GreaterThan:
            result = value.toLongLong() > m_filter.numValue;
            break;
        case MetaQueryWidget::Between:
            result = value.toLongLong() > m_filter.numValue &&
                value.toLongLong() < m_filter.numValue2;
            break;
        default:
            ;
        }
    }
    else
    {
        switch( m_filter.condition )
        {
        case MetaQueryWidget::Equals:
            result = value.toString() == m_filter.value;
            break;
        case MetaQueryWidget::Contains:
            result = value.toString().contains( m_filter.value, Qt::CaseInsensitive );
            break;
        default:
            ;
        }
    }
    if( m_invert )
        return !result;
    else
        return result;
}


