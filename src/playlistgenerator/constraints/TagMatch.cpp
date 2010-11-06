/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#define DEBUG_PREFIX "Constraint::TagMatch"

#include "TagMatch.h"

#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"

#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"

#include <KRandom>

Constraint*
ConstraintTypes::TagMatch::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p )
        return new TagMatch( xmlelem, p );
    else
        return 0;
}

Constraint*
ConstraintTypes::TagMatch::createNew( ConstraintNode* p )
{
    if ( p )
        return new TagMatch( p );
    else
        return 0;
}

ConstraintFactoryEntry*
ConstraintTypes::TagMatch::registerMe()
{
    return new ConstraintFactoryEntry( "TagMatch",
                                       i18n("Match Tags"),
                                       i18n("Make all tracks in the playlist match the specified characteristic"),
                                       &TagMatch::createFromXml, &TagMatch::createNew );
}

ConstraintTypes::TagMatch::TagMatch( QDomElement& xmlelem, ConstraintNode* p )
        : MatchingConstraint( p )
{
    DEBUG_BLOCK
    QDomAttr a;

    a = xmlelem.attributeNode( "field" );
    if ( !a.isNull() ) {
        m_filter.field = Meta::fieldForPlaylistName( a.value() );
        if( !m_filter.field )
            debug() << a.value() << "is not a recognized field name" << endl;
    }

    a = xmlelem.attributeNode( "comparison" );
    if ( !a.isNull() ) {
        int comparison = a.value().toInt();

        if( m_filter.isDate() )
        {
            if( comparison == 0 )
                m_filter.condition = MetaQueryWidget::LessThan;
            else if( comparison == 1 )
                m_filter.condition = MetaQueryWidget::Equals;
            else if( comparison == 2 )
                m_filter.condition = MetaQueryWidget::GreaterThan;
            else if( comparison == 3 )
                m_filter.condition = MetaQueryWidget::Within;
        }
        else if( m_filter.isNumeric() )
        {
            if( comparison == 0 )
                m_filter.condition = MetaQueryWidget::LessThan;
            else if( comparison == 1 )
                m_filter.condition = MetaQueryWidget::Equals;
            else if( comparison == 2 )
                m_filter.condition = MetaQueryWidget::GreaterThan;
        }
        else
        {
            if( comparison == 0 )
                m_filter.condition = MetaQueryWidget::Matches;
            else if( comparison == 1 )
                m_filter.condition = MetaQueryWidget::StartsWith;
            else if( comparison == 2 )
                m_filter.condition = MetaQueryWidget::EndsWith;
            else
                m_filter.condition = MetaQueryWidget::Contains;
        }
    }

    a = xmlelem.attributeNode( "value" );
    if ( !a.isNull() ) {
        if( m_filter.isDate() )
        {
            if( m_filter.condition == MetaQueryWidget::OlderThan ||
                m_filter.condition == MetaQueryWidget::Within )
            {
                QStringList parts = a.value().split(" ");
                if ( parts.size() == 2 )
                {
                    qint64 u = parts.at( 0 ).toLong();
                    u *= 60 * 60 * 24; // days
                    if ( parts.at( 1 ) == "months" )
                        u *= 30;
                    else if ( parts.at( 1 ) == "years" )
                        u *= 365;

                    m_filter.numValue = u;
                }
                else
                {
                    m_filter.numValue = a.value().toLong();
                }
            }
            else
                m_filter.numValue = QDateTime::fromString( a.value(), Qt::ISODate ).toTime_t();
        }

        else if( m_filter.isNumeric() )
            m_filter.numValue = a.value().toLong();

        else
            m_filter.value = a.value();
    }

    a = xmlelem.attributeNode( "invert" );
    if ( !a.isNull() && a.value() == "true" )
        m_invert = true;
    else
        m_invert = false;

    a = xmlelem.attributeNode( "strictness" );
    if ( !a.isNull() )
        m_strictness = a.value().toDouble();

    debug() << getName();
}

ConstraintTypes::TagMatch::TagMatch( ConstraintNode* p )
        : MatchingConstraint( p )
        , m_invert( false )
        , m_strictness( 1.0 )
{
    DEBUG_BLOCK
    debug() << "new default TagMatch";
}

ConstraintTypes::TagMatch::~TagMatch()
{
}

QWidget*
ConstraintTypes::TagMatch::editWidget() const
{
    TagMatchEditWidget* e = new TagMatchEditWidget( m_filter, m_invert, int(m_strictness * 10) );
    connect( e, SIGNAL( filterChanged( const MetaQueryWidget::Filter& ) ), this, SLOT( setFilter( const MetaQueryWidget::Filter& ) ) );
    connect( e, SIGNAL( invertChanged( bool ) ), this, SLOT( setInvert( bool ) ) );
    connect( e, SIGNAL( strictnessChanged( int ) ), this, SLOT( setStrictness( int ) ) );
    return e;
}

void
ConstraintTypes::TagMatch::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement c = doc.createElement( "constraint" );

    c.setAttribute( "type", "TagMatch" );
    c.setAttribute( "field", Meta::playlistNameForField( m_filter.field ) );

    switch( m_filter.condition )
    {
    case MetaQueryWidget::LessThan:
        c.setAttribute( "comparison", 0 ); break;
    case MetaQueryWidget::Equals:
        c.setAttribute( "comparison", 1 ); break;
    case MetaQueryWidget::GreaterThan:
        c.setAttribute( "comparison", 2 ); break;
    case MetaQueryWidget::Within:
        c.setAttribute( "comparison", 3 ); break;

    case MetaQueryWidget::Matches:
        c.setAttribute( "comparison", 0 ); break;
    case MetaQueryWidget::StartsWith:
        c.setAttribute( "comparison", 1 ); break;
    case MetaQueryWidget::EndsWith:
        c.setAttribute( "comparison", 2 ); break;
    case MetaQueryWidget::Contains:
        c.setAttribute( "comparison", 3 ); break;

    default:
        c.setAttribute( "comparison", -1 );
    }

    if( m_filter.isDate() )
    {
        if( m_filter.condition == MetaQueryWidget::OlderThan ||
            m_filter.condition == MetaQueryWidget::Within )
            c.setAttribute( "value", QString::number( m_filter.numValue / (60*60*24) /*days*/  ) );
        else
            c.setAttribute( "value", QString::number( m_filter.numValue ) );
    }
    else if( m_filter.isNumeric() )
        c.setAttribute( "value", QString::number( m_filter.numValue ) );
    else
        c.setAttribute( "value", m_filter.value );

    if ( m_invert )
        c.setAttribute( "invert", "true" );
    else
        c.setAttribute( "invert", "false" );

    c.setAttribute( "strictness", QString::number( m_strictness ) );

    elem.appendChild( c );
    }

QString
ConstraintTypes::TagMatch::getName() const
{
    QString v( i18n("Match tag:%1 %2 %3 %4") );
    v = v.arg( ( m_invert ? i18n(" not") : "" ), m_filter.fieldToString(),
               MetaQueryWidget::conditionToString( m_filter.condition,
                                                   m_filter.isDate()) );

    return v.arg( valueToString() );
}

Collections::QueryMaker*
ConstraintTypes::TagMatch::initQueryMaker( Collections::QueryMaker* qm ) const
{
    // --- compute a range and the min and max values for fuzzy compare
    int range = 0;
    range = Comparer::rangeNum( m_strictness, m_filter.field );

    int minValue = m_filter.numValue - range;
    int maxValue = m_filter.numValue + range;

    if( m_filter.condition == MetaQueryWidget::Between )
    {
        minValue = qMin(m_filter.numValue, m_filter.numValue2) - 1 - range;
        minValue = qMax(m_filter.numValue, m_filter.numValue2) + 1 + range;
    }

    // --- set the filters
    switch( m_filter.condition )
    {
    case MetaQueryWidget::GreaterThan:
        if ( m_invert )
            qm->excludeNumberFilter( m_filter.field, maxValue, Collections::QueryMaker::GreaterThan );
        else
            qm->addNumberFilter( m_filter.field, minValue, Collections::QueryMaker::GreaterThan );
        break;

    case MetaQueryWidget::LessThan:
        if ( m_invert )
            qm->excludeNumberFilter( m_filter.field, minValue, Collections::QueryMaker::LessThan );
        else
            qm->addNumberFilter( m_filter.field, maxValue, Collections::QueryMaker::LessThan );
        break;

    case MetaQueryWidget::Equals:
    case MetaQueryWidget::Between:
        if ( minValue == maxValue )
        {
            if( m_invert )
                qm->excludeNumberFilter( m_filter.field, minValue, Collections::QueryMaker::Equals );
            else
                qm->addNumberFilter( m_filter.field, minValue, Collections::QueryMaker::Equals );
        }
        else
        {
            if( m_invert )
            {
                qm->beginOr();
                qm->excludeNumberFilter( m_filter.field, minValue, Collections::QueryMaker::LessThan );
                qm->excludeNumberFilter( m_filter.field, maxValue, Collections::QueryMaker::GreaterThan );
                qm->endAndOr();
            }
            else
            {
                qm->beginAnd();
                qm->addNumberFilter( m_filter.field, minValue, Collections::QueryMaker::GreaterThan );
                qm->addNumberFilter( m_filter.field, maxValue, Collections::QueryMaker::LessThan );
                qm->endAndOr();
            }
        }
        break;

    case MetaQueryWidget::OlderThan:
    case MetaQueryWidget::Within:
    {
        bool inv = (m_filter.condition == MetaQueryWidget::Within) ? m_invert : !m_invert;
        if ( inv  )
            qm->addNumberFilter( m_filter.field, QDateTime::currentDateTime().toTime_t() - maxValue, Collections::QueryMaker::GreaterThan );
        else
            qm->excludeNumberFilter( m_filter.field, QDateTime::currentDateTime().toTime_t() - minValue, Collections::QueryMaker::LessThan );
        break;
    }

    case MetaQueryWidget::Contains:
    case MetaQueryWidget::Matches:
    case MetaQueryWidget::StartsWith:
    case MetaQueryWidget::EndsWith:
    {
        bool matchBegin = (m_filter.condition == MetaQueryWidget::Matches ||
                           m_filter.condition == MetaQueryWidget::StartsWith);
        bool matchEnd =   (m_filter.condition == MetaQueryWidget::Matches ||
                           m_filter.condition == MetaQueryWidget::EndsWith);
        if( m_filter.field == 0 )
        {
            // simple search
            // TODO: split different words and make seperate searches
            if ( m_invert )
            {
                qm->beginAnd();
                qm->excludeFilter( Meta::valArtist,  m_filter.value, matchBegin, matchEnd );
                qm->excludeFilter( Meta::valTitle,   m_filter.value, matchBegin, matchEnd );
                qm->excludeFilter( Meta::valAlbum,   m_filter.value, matchBegin, matchEnd );
                qm->excludeFilter( Meta::valGenre,   m_filter.value, matchBegin, matchEnd );
                qm->excludeFilter( Meta::valUrl,     m_filter.value, matchBegin, matchEnd );
                qm->excludeFilter( Meta::valComment, m_filter.value, matchBegin, matchEnd );
                qm->excludeFilter( Meta::valLabel,   m_filter.value, matchBegin, matchEnd );
                qm->endAndOr();
            }
            else
            {
                qm->beginOr();
                qm->addFilter( Meta::valArtist,  m_filter.value, matchBegin, matchEnd );
                qm->addFilter( Meta::valTitle,   m_filter.value, matchBegin, matchEnd );
                qm->addFilter( Meta::valAlbum,   m_filter.value, matchBegin, matchEnd );
                qm->addFilter( Meta::valGenre,   m_filter.value, matchBegin, matchEnd );
                qm->addFilter( Meta::valUrl,     m_filter.value, matchBegin, matchEnd );
                qm->addFilter( Meta::valComment, m_filter.value, matchBegin, matchEnd );
                qm->addFilter( Meta::valLabel,   m_filter.value, matchBegin, matchEnd );
                qm->endAndOr();
            }
        }
        else
        {
            if ( m_invert )
                qm->excludeFilter( m_filter.field, m_filter.value, matchBegin, matchEnd );
            else
                qm->addFilter( m_filter.field, m_filter.value, matchBegin, matchEnd );
        }
    }
    }

    return qm;
}

double
ConstraintTypes::TagMatch::satisfaction( const Meta::TrackList& tl )
{
    m_satisfaction = 0.0;
    foreach( Meta::TrackPtr t, tl ) {
        if ( matches( t ) ) {
            m_satisfaction += 1.0;
        }
    }
    m_satisfaction /= ( double )tl.size();
    return m_satisfaction;
}

double
ConstraintTypes::TagMatch::deltaS_insert( const Meta::TrackList& tl, const Meta::TrackPtr t, const int ) const
{
    double s = m_satisfaction * tl.size();
    if ( matches( t ) ) {
        s += 1.0;
    }
    return s / ( double )( tl.size() + 1 ) - m_satisfaction;
}

double
ConstraintTypes::TagMatch::deltaS_replace( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i ) const
{
    bool oldT = matches( tl.at( i ) );
    bool newT = matches( t );
    if ( !( oldT ^ newT ) ) {
        return 0.0;
    } else if ( newT ) {
        return 1.0 / ( double )tl.size();
    } else if ( oldT ) {
        return -1.0 / ( double )tl.size();
    } else {
        return 0.0;
    }
}

double
ConstraintTypes::TagMatch::deltaS_delete( const Meta::TrackList& tl, const int i ) const
{
    double s = m_satisfaction * tl.size();
    if ( matches( tl.at( i ) ) ) {
        s -= 1.0;
    }
    return s / ( double )( tl.size() - 1 ) - m_satisfaction;
}

double
ConstraintTypes::TagMatch::deltaS_swap( const Meta::TrackList&, const int, const int ) const
{
    return 0.0;
}

void
ConstraintTypes::TagMatch::insertTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i )
{
    m_satisfaction += deltaS_insert( tl, t, i );
}

void
ConstraintTypes::TagMatch::replaceTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i )
{
    m_satisfaction += deltaS_replace( tl, t, i );
}

void
ConstraintTypes::TagMatch::deleteTrack( const Meta::TrackList& tl, const int i )
{
    m_satisfaction += deltaS_delete( tl, i );
}

void
ConstraintTypes::TagMatch::swapTracks( const Meta::TrackList&, const int, const int ) {}

ConstraintNode::Vote*
ConstraintTypes::TagMatch::vote( const Meta::TrackList& playlist, const Meta::TrackList& domain ) const
{
    ConstraintNode::Vote* v = new ConstraintNode::Vote();
    v->operation = ConstraintNode::OperationReplace;
    v->place = -1;
    v->track = Meta::TrackPtr();

    // find a non-matching track in the playlist
    for ( int i = 0; i < playlist.length() ; i++ ) {
        if ( !matches( playlist.at( i ) ) ) {
            v->place = i;
            break;
        }
    }
    if ( v->place < 0 ) {
        delete v;
        return 0;
    }

    // replace it with a track from the domain that matches
    for ( int i = 0; i < 100; i++ ) {
        v->track = domain.at( KRandom::random() % domain.size() );
        if ( matches( v->track ) ) {
            return v;
        }
    }

    delete v;
    return 0;
}

void
ConstraintTypes::TagMatch::audit( const Meta::TrackList& tl ) const
{
    foreach( const Meta::TrackPtr t, tl ) {
        debug() << t->prettyName() << matches( t );
    }
}

const QBitArray
ConstraintTypes::TagMatch::whatTracksMatch( const Meta::TrackList& tl )
{
    QBitArray match = QBitArray( tl.size() );
    for ( int i = 0; i < tl.size(); i++ ) {
        if ( matches( tl.at( i ) ) )
            match.setBit( i, true );
    }
    return match;
}

int
ConstraintTypes::TagMatch::constraintMatchType() const
{
    // find the index of the field (the field is a bitvector)
    int index = 0;
    qint64 field = m_filter.field;
    while( field > 1 )
    {
        index++;
        field = field >> 1;
    }

    return ( 0 << 28 ) + index; // no idea what the senseless shift should mean.
}

QString
ConstraintTypes::TagMatch::valueToString() const
{
    if( m_filter.field == Meta::valRating )
    {
        qreal r = qreal(m_filter.numValue) / 2.0;
        return i18ncp("number of stars in the rating of a track", "%1 star", "%1 stars", r);
    }

    else if ( m_filter.field == Meta::valLength )
        return QTime().addSecs( m_filter.numValue ).toString(/* "H:mm:ss" */);

    else if ( m_filter.field == Meta::valFilesize )
            return Meta::prettyFilesize( m_filter.numValue );

    /* the prettyBitrate is stupid
    else if ( m_filter.field == Meta::valBitrate )
            return Meta::prettyBitrate( m_filter.numValue );
            */

    else if( m_filter.isDate() )
    {
        if( m_filter.condition == MetaQueryWidget::Within ||
            m_filter.condition == MetaQueryWidget::OlderThan)
        {
            return Meta::secToPrettyTimeLong( m_filter.numValue );
        }
        else
            return QDateTime::fromTime_t( m_filter.numValue ).date().toString();
    }

    else if( m_filter.isNumeric() )
        return QString::number( m_filter.numValue );

    else
        return QString( i18nc("an arbitrary string surrounded by quotes", "\"%1\"") ).arg( m_filter.value );
}

bool
ConstraintTypes::TagMatch::matches( Meta::TrackPtr track ) const
{
    if ( !m_matchCache.contains( track ) )
    {
        double v = 0.0;

        if( m_filter.isNumeric() )
            v = Comparer::compareNum( Meta::valueForField( m_filter.field, track ).toDouble(),
                                      m_filter.condition, m_filter.numValue, m_strictness, m_filter.field );
        else if( m_filter.field == Meta::valLabel )
            v = Comparer::compareLabels( track, m_filter.condition, m_filter.value );
        else
            v = Comparer::compareStr( Meta::valueForField( m_filter.field, track ).toString(),
                                      m_filter.condition, m_filter.value );

        if ( m_invert )
            v = 1.0 - v;

        m_matchCache.insert( track, ( v > ( (double)qrand() / (double)RAND_MAX ) ) );
    }
    return m_matchCache.value( track );
}


void
ConstraintTypes::TagMatch::setFilter( const MetaQueryWidget::Filter &filter )
{
    m_filter = filter;
    m_matchCache.clear();
    emit dataChanged();
}

void
ConstraintTypes::TagMatch::setInvert( bool v )
{
    if ( m_invert != v ) {
        foreach( const Meta::TrackPtr t, m_matchCache.keys() ) {
            m_matchCache.insert( t, !m_matchCache.value( t ) );
        }
    }
    m_invert = v;
    emit dataChanged();
}

void
ConstraintTypes::TagMatch::setStrictness( int v )
{
    m_strictness = static_cast<double>( v ) / 10.0;
    m_matchCache.clear();
}


/******************************
 * Edit Widget                *
 ******************************/

ConstraintTypes::TagMatchEditWidget::TagMatchEditWidget( const MetaQueryWidget::Filter &filter,
                                                         bool invert, int strictness)
        : QWidget( 0 )
{
    ui.setupUi( this );

    ui.attributeQuery->setAPGMode( true );
    ui.slider_Strictness->setValue( strictness );
    ui.checkBox_Invert->setChecked( invert );

    ui.attributeQuery->setFilter( filter );
    slotUpdateStrictness();

    connect( ui.checkBox_Invert, SIGNAL( clicked( bool ) ),
             this, SIGNAL( invertChanged( bool ) ) );

    connect( ui.slider_Strictness, SIGNAL( valueChanged( int ) ),
             this, SIGNAL( strictnessChanged( int ) ) );

    connect( ui.attributeQuery, SIGNAL( changed( const MetaQueryWidget::Filter& )),
             this, SIGNAL( filterChanged( const MetaQueryWidget::Filter& ) ) );
    connect( ui.attributeQuery, SIGNAL( changed( const MetaQueryWidget::Filter& )),
             this, SLOT( slotUpdateStrictness() ) );
}

ConstraintTypes::TagMatchEditWidget::~TagMatchEditWidget()
{ }

void
ConstraintTypes::TagMatchEditWidget::slotUpdateStrictness()
{
    ui.label_Fuzzy->setEnabled( ui.attributeQuery->filter().isNumeric() );
    ui.slider_Strictness->setEnabled( ui.attributeQuery->filter().isNumeric() );
    ui.label_Exact->setEnabled( ui.attributeQuery->filter().isNumeric() );
}

