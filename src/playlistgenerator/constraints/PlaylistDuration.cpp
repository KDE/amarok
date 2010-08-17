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

#define DEBUG_PREFIX "Constraint::PlaylistDuration"

#include "PlaylistDuration.h"

#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"

#include <KRandom>

#include <QtGlobal>

#include <stdlib.h>
#include <math.h>

Constraint*
ConstraintTypes::PlaylistDuration::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p ) {
        return new PlaylistDuration( xmlelem, p );
    } else {
        return 0;
    }
}

Constraint*
ConstraintTypes::PlaylistDuration::createNew( ConstraintNode* p )
{
    if ( p ) {
        return new PlaylistDuration( p );
    } else {
        return 0;
    }
}

ConstraintFactoryEntry*
ConstraintTypes::PlaylistDuration::registerMe()
{
    return new ConstraintFactoryEntry( "PlaylistDuration",
                                       i18n("Playlist Duration"),
                                       i18n("Sets the preferred duration of the playlist"),
                                       &PlaylistDuration::createFromXml, &PlaylistDuration::createNew );
}

ConstraintTypes::PlaylistDuration::PlaylistDuration( QDomElement& xmlelem, ConstraintNode* p )
        : Constraint( p )
{
    DEBUG_BLOCK
    QDomAttr a;

    a = xmlelem.attributeNode( "duration" );
    if ( !a.isNull() )
        m_duration = a.value().toInt();

    a = xmlelem.attributeNode( "comparison" );
    if ( !a.isNull() )
        m_comparison = a.value().toInt();

    a = xmlelem.attributeNode( "strictness" );
    if ( !a.isNull() )
        m_strictness = a.value().toDouble();

    debug() << getName();
}

ConstraintTypes::PlaylistDuration::PlaylistDuration( ConstraintNode* p )
        : Constraint( p )
        , m_duration( 0 )
        , m_comparison( CompareNumEquals )
        , m_strictness( 1.0 )
{
    DEBUG_BLOCK
    debug() << "new default PlaylistLength";
}

QWidget*
ConstraintTypes::PlaylistDuration::editWidget() const
{
    PlaylistDurationEditWidget* e = new PlaylistDurationEditWidget( m_duration, m_comparison, static_cast<int>( 10*m_strictness ) );
    connect( e, SIGNAL( comparisonChanged( const int ) ), this, SLOT( setComparison( const int ) ) );
    connect( e, SIGNAL( durationChanged( const int ) ), this, SLOT( setDuration( const int ) ) );
    connect( e, SIGNAL( strictnessChanged( const int ) ), this, SLOT( setStrictness( const int ) ) );
    return e;
}

void
ConstraintTypes::PlaylistDuration::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement c = doc.createElement( "constraint" );
    c.setAttribute( "type", "PlaylistDuration" );
    c.setAttribute( "duration", QString::number( m_duration ) );
    c.setAttribute( "comparison", QString::number( m_comparison ) );
    c.setAttribute( "strictness", QString::number( m_strictness ) );
    elem.appendChild( c );
}

QString
ConstraintTypes::PlaylistDuration::getName() const
{
    QString v( i18n("Playlist duration: %1 %2") );
    return v.arg( comparisonToString() ).arg( QTime().addMSecs( m_duration ).toString( "H:mm:ss" ) );
}

Collections::QueryMaker*
ConstraintTypes::PlaylistDuration::initQueryMaker( Collections::QueryMaker* qm ) const
{
    return qm;
}

double
ConstraintTypes::PlaylistDuration::satisfaction( const Meta::TrackList& tl )
{
    m_totalDuration = 0;
    foreach( Meta::TrackPtr t, tl ) {
        m_totalDuration += t->length();
    }
    return transformDuration( m_totalDuration );
}

double
ConstraintTypes::PlaylistDuration::deltaS_insert( const Meta::TrackList&, const Meta::TrackPtr t, const int ) const
{
    qint64 l = m_totalDuration + t->length();
    double newS = transformDuration( l );
    double oldS = transformDuration( m_totalDuration );
    return newS - oldS;
}

double
ConstraintTypes::PlaylistDuration::deltaS_replace( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i ) const
{
    int l = m_totalDuration + t->length() - tl.at( i )->length();
    double newS = transformDuration( l );
    double oldS = transformDuration( m_totalDuration );
    return newS - oldS;
}

double
ConstraintTypes::PlaylistDuration::deltaS_delete( const Meta::TrackList& tl, const int i ) const
{
    int l = m_totalDuration - tl.at( i )->length();
    double newS = transformDuration( l );
    double oldS = transformDuration( m_totalDuration );
    return newS - oldS;
}

double
ConstraintTypes::PlaylistDuration::deltaS_swap( const Meta::TrackList&, const int, const int ) const
{
    return 0.0;
}

void
ConstraintTypes::PlaylistDuration::insertTrack( const Meta::TrackList&, const Meta::TrackPtr t, const int )
{
    m_totalDuration += t->length();
}

void
ConstraintTypes::PlaylistDuration::replaceTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i )
{
    m_totalDuration += t->length();
    m_totalDuration -= tl.at( i )->length();
}

void
ConstraintTypes::PlaylistDuration::deleteTrack( const Meta::TrackList& tl, const int i )
{
    m_totalDuration -= tl.at( i )->length();
}

void
ConstraintTypes::PlaylistDuration::swapTracks( const Meta::TrackList&, const int, const int ) {}

int
ConstraintTypes::PlaylistDuration::suggestInitialPlaylistSize() const
{
    if ( m_comparison == CompareNumLessThan ) {
        return m_duration / 300000;
    } else if ( m_comparison == CompareNumGreaterThan ) {
        return m_duration / 180000;
    } else {
        return m_duration / 240000;
    }
}

ConstraintNode::Vote*
ConstraintTypes::PlaylistDuration::vote( const Meta::TrackList& playlist, const Meta::TrackList& domain ) const
{
    ConstraintNode::Vote* v = 0;

    if ( m_comparison == CompareNumLessThan ) {
        if ( m_totalDuration > m_duration) {
            int longestDuration = 0;
            int longestPosition = -1;
            for ( int i = 0; i < playlist.size(); i++ ) {
                Meta::TrackPtr t = playlist.at( i );
                if ( t->length() > longestDuration ) {
                    longestDuration = t->length();
                    longestPosition = i;
                }
            }

            v = new ConstraintNode::Vote;
            v->operation = ConstraintNode::OperationDelete;
            v->place = longestPosition;
        }
    } else if ( m_comparison == CompareNumGreaterThan ) {
        if ( m_totalDuration < m_duration) {
            v = new ConstraintNode::Vote;
            v->operation = ConstraintNode::OperationInsert;
            v->place = KRandom::random() % ( playlist.size() + 1 );
            v->track = domain.at( KRandom::random() % domain.size() );
        }
    } else if ( m_comparison == CompareNumEquals ) {
        int deviation = qAbs( m_totalDuration - m_duration );
        if ( m_totalDuration > m_duration ) {
            int randomIdx = KRandom::random() % playlist.size();
            if ( ( playlist.at( randomIdx )->length() / 2 ) < deviation ) {
                v = new ConstraintNode::Vote;
                v->operation = ConstraintNode::OperationDelete;
                v->place = randomIdx;
            }
        } else {
            Meta::TrackPtr randomTrack = domain.at( KRandom::random() % domain.size() );
            if ( ( randomTrack->length() / 2 ) < deviation ) {
                v = new ConstraintNode::Vote;
                v->operation = ConstraintNode::OperationInsert;
                v->place = KRandom::random() % ( playlist.size() + 1 );
                v->track = randomTrack;
            }
        }
    }

    return v;
}

QString
ConstraintTypes::PlaylistDuration::comparisonToString() const
{
    if ( m_comparison == CompareNumEquals ) {
        return QString( i18nc("duration of playlist equals some time", "equals") );
    } else if ( m_comparison == CompareNumGreaterThan ) {
        return QString( i18n("longer than") );
    } else if ( m_comparison == CompareNumLessThan ) {
        return QString( i18n("shorter than") );
    } else {
        return QString( i18n("unknown comparison") );
    }
}

double
ConstraintTypes::PlaylistDuration::transformDuration( const qint64 l ) const
{
    double factor = m_strictness * 0.0003;
    if ( m_comparison == CompareNumEquals ) {
        return 4.0 / ( ( 1.0 + exp( factor*( double )( l - m_duration ) ) )*( 1.0 + exp( factor*( double )( m_duration - l ) ) ) );
    } else if ( m_comparison == CompareNumLessThan ) {
        return 1.0 / ( 1.0 + exp( factor*( double )( l - m_duration ) ) );
    } else if ( m_comparison == CompareNumGreaterThan ) {
        return 1.0 / ( 1.0 + exp( factor*( double )( m_duration - l ) ) );
    }
    return 1.0;
}

void
ConstraintTypes::PlaylistDuration::setComparison( const int c )
{
    m_comparison = c;
    emit dataChanged();
}

void
ConstraintTypes::PlaylistDuration::setDuration( const int v )
{
    m_duration = v;
    emit dataChanged();
}

void
ConstraintTypes::PlaylistDuration::setStrictness( const int sv )
{
    m_strictness = static_cast<double>(sv)/10.0;
}

/******************************
 * Edit Widget                *
 ******************************/

ConstraintTypes::PlaylistDurationEditWidget::PlaylistDurationEditWidget( const int duration,
                                                                     const int comparison,
                                                                     const int strictness ) : QWidget( 0 )
{
    ui.setupUi( this );

    ui.timeEdit_Duration->setTime( QTime().addMSecs( duration ) );
    ui.comboBox_Comparison->setCurrentIndex( comparison );
    ui.slider_Strictness->setValue( strictness );
}

void
ConstraintTypes::PlaylistDurationEditWidget::on_timeEdit_Duration_timeChanged( const QTime& t )
{
    emit durationChanged( QTime().msecsTo( t ) );
    emit updated();
}

void
ConstraintTypes::PlaylistDurationEditWidget::on_comboBox_Comparison_currentIndexChanged( const int v )
{
    emit comparisonChanged( v );
    emit updated();
}

void
ConstraintTypes::PlaylistDurationEditWidget::on_slider_Strictness_valueChanged( const int v )
{
    emit strictnessChanged( v );
    emit updated();
}
