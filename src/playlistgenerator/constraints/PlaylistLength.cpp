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

#define DEBUG_PREFIX "Constraint::PlaylistLength"

#include "PlaylistLength.h"

#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"

#include <KRandom>

#include <QtGlobal>

#include <stdlib.h>

Constraint*
ConstraintTypes::PlaylistLength::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p ) {
        return new PlaylistLength( xmlelem, p );
    } else {
        return 0;
    }
}

Constraint*
ConstraintTypes::PlaylistLength::createNew( ConstraintNode* p )
{
    if ( p ) {
        return new PlaylistLength( p );
    } else {
        return 0;
    }
}

ConstraintFactoryEntry*
ConstraintTypes::PlaylistLength::registerMe()
{
    return new ConstraintFactoryEntry( i18n("PlaylistLength"),
                                i18n("Sets the preferred duration of the playlist"),
                                &PlaylistLength::createFromXml, &PlaylistLength::createNew );
}

ConstraintTypes::PlaylistLength::PlaylistLength( QDomElement& xmlelem, ConstraintNode* p )
        : Constraint( p )
{
    QDomAttr a;

    a = xmlelem.attributeNode( "length" );
    if ( !a.isNull() )
        m_length = a.value().toInt();

    a = xmlelem.attributeNode( "comparison" );
    if ( !a.isNull() )
        m_comparison = a.value().toInt();

    a = xmlelem.attributeNode( "strictness" );
    if ( !a.isNull() )
        m_strictness = a.value().toDouble();
}

ConstraintTypes::PlaylistLength::PlaylistLength( ConstraintNode* p )
        : Constraint( p )
        , m_length( 0 )
        , m_comparison( Constraint::CompareNumEquals )
        , m_strictness( 1.0 )
{
}

QWidget*
ConstraintTypes::PlaylistLength::editWidget() const
{
    PlaylistLengthEditWidget* e = new PlaylistLengthEditWidget( m_length, m_comparison, static_cast<int>( 10*m_strictness ) );
    connect( e, SIGNAL( comparisonChanged( const int ) ), this, SLOT( setComparison( const int ) ) );
    connect( e, SIGNAL( lengthChanged( const int ) ), this, SLOT( setLength( const int ) ) );
    connect( e, SIGNAL( strictnessChanged( const int ) ), this, SLOT( setStrictness( const int ) ) );
    return e;
}

void
ConstraintTypes::PlaylistLength::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement c = doc.createElement( "constraint" );
    c.setAttribute( "type", "PlaylistLength" );
    c.setAttribute( "length", QString::number( m_length ) );
    c.setAttribute( "comparison", QString::number( m_comparison ) );
    c.setAttribute( "strictness", QString::number( m_strictness ) );
    elem.appendChild( c );
}

QString
ConstraintTypes::PlaylistLength::getName() const
{
    QString v( i18n("Playlist length %1 %2") );
    return v.arg( comparisonToString() ).arg( QTime().addMSecs( m_length ).toString( "H:mm:ss" ) );
}

Collections::QueryMaker*
ConstraintTypes::PlaylistLength::initQueryMaker( Collections::QueryMaker* qm ) const
{
    return qm;
}

double
ConstraintTypes::PlaylistLength::satisfaction( const Meta::TrackList& tl )
{
    m_totalLength = 0;
    foreach( Meta::TrackPtr t, tl ) {
        m_totalLength += t->length();
    }
    return transformLength( m_totalLength );
}

double
ConstraintTypes::PlaylistLength::deltaS_insert( const Meta::TrackList&, const Meta::TrackPtr t, const int ) const
{
    qint64 l = m_totalLength + t->length();
    double newS = transformLength( l );
    double oldS = transformLength( m_totalLength );
    return newS - oldS;
}

double
ConstraintTypes::PlaylistLength::deltaS_replace( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i ) const
{
    int l = m_totalLength + t->length() - tl.at( i )->length();
    double newS = transformLength( l );
    double oldS = transformLength( m_totalLength );
    return newS - oldS;
}

double
ConstraintTypes::PlaylistLength::deltaS_delete( const Meta::TrackList& tl, const int i ) const
{
    int l = m_totalLength - tl.at( i )->length();
    double newS = transformLength( l );
    double oldS = transformLength( m_totalLength );
    return newS - oldS;
}

double
ConstraintTypes::PlaylistLength::deltaS_swap( const Meta::TrackList&, const int, const int ) const
{
    return 0.0;
}

void
ConstraintTypes::PlaylistLength::insertTrack( const Meta::TrackList&, const Meta::TrackPtr t, const int )
{
    m_totalLength += t->length();
}

void
ConstraintTypes::PlaylistLength::replaceTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i )
{
    m_totalLength += t->length();
    m_totalLength -= tl.at( i )->length();
}

void
ConstraintTypes::PlaylistLength::deleteTrack( const Meta::TrackList& tl, const int i )
{
    m_totalLength -= tl.at( i )->length();
}

void
ConstraintTypes::PlaylistLength::swapTracks( const Meta::TrackList&, const int, const int ) {}

int
ConstraintTypes::PlaylistLength::suggestInitialPlaylistSize() const
{
    if ( m_comparison == Constraint::CompareNumLessThan ) {
        return m_length / 300000;
    } else if ( m_comparison == Constraint::CompareNumGreaterThan ) {
        return m_length / 180000;
    } else {
        return m_length / 240000;
    }
}

ConstraintNode::Vote*
ConstraintTypes::PlaylistLength::vote( const Meta::TrackList& playlist, const Meta::TrackList& domain ) const
{
    ConstraintNode::Vote* v = 0;

    if ( m_comparison == Constraint::CompareNumLessThan ) {
        if ( m_totalLength > m_length) {
            int longestLength = 0;
            int longestPosition = -1;
            for ( int i = 0; i < playlist.size(); i++ ) {
                Meta::TrackPtr t = playlist.at( i );
                if ( t->length() > longestLength ) {
                    longestLength = t->length();
                    longestPosition = i;
                }
            }

            v = new ConstraintNode::Vote;
            v->operation = ConstraintNode::OperationDelete;
            v->place = longestPosition;
        }
    } else if ( m_comparison == Constraint::CompareNumGreaterThan ) {
        if ( m_totalLength < m_length) {
            v = new ConstraintNode::Vote;
            v->operation = ConstraintNode::OperationInsert;
            v->place = KRandom::random() % ( playlist.size() + 1 );
            v->track = domain.at( KRandom::random() % domain.size() );
        }
    } else if ( m_comparison == Constraint::CompareNumEquals ) {
        int deviation = qAbs( m_totalLength - m_length );
        if ( m_totalLength > m_length ) {
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
ConstraintTypes::PlaylistLength::comparisonToString() const
{
    if ( m_comparison == Constraint::CompareNumEquals ) {
        return QString( i18n("equals") );
    } else if ( m_comparison == Constraint::CompareNumGreaterThan ) {
        return QString( i18n("longer than") );
    } else if ( m_comparison == Constraint::CompareNumLessThan ) {
        return QString( i18n("shorter than") );
    } else {
        return QString( i18n("unknown comparison") );
    }
}

double
ConstraintTypes::PlaylistLength::transformLength( const qint64 l ) const
{
    double factor = m_strictness * 0.0003;
    if ( m_comparison == Constraint::CompareNumEquals ) {
        return 4.0 / ( ( 1.0 + exp( factor*( double )( l - m_length ) ) )*( 1.0 + exp( factor*( double )( m_length - l ) ) ) );
    } else if ( m_comparison == Constraint::CompareNumLessThan ) {
        return 1.0 / ( 1.0 + exp( factor*( double )( l - m_length ) ) );
    } else if ( m_comparison == Constraint::CompareNumGreaterThan ) {
        return 1.0 / ( 1.0 + exp( factor*( double )( m_length - l ) ) );
    }
    return 1.0;
}

void
ConstraintTypes::PlaylistLength::setComparison( const int c )
{
    m_comparison = c;
    emit dataChanged();
}

void
ConstraintTypes::PlaylistLength::setLength( const int v )
{
    m_length = v;
    emit dataChanged();
}

void
ConstraintTypes::PlaylistLength::setStrictness( const int sv )
{
    m_strictness = static_cast<double>(sv)/10.0;
}

/******************************
 * Edit Widget                *
 ******************************/

ConstraintTypes::PlaylistLengthEditWidget::PlaylistLengthEditWidget( const int length,
                                                                     const int comparison,
                                                                     const int strictness ) : QWidget( 0 )
{
    ui.setupUi( this );

    ui.timeEdit_Duration->setTime( QTime().addMSecs( length ) );
    ui.comboBox_Comparison->setCurrentIndex( comparison );
    ui.slider_Strictness->setValue( strictness );
}

void
ConstraintTypes::PlaylistLengthEditWidget::on_timeEdit_Duration_timeChanged( const QTime& t )
{
    emit lengthChanged( QTime().msecsTo( t ) );
    emit updated();
}

void
ConstraintTypes::PlaylistLengthEditWidget::on_comboBox_Comparison_currentIndexChanged( const int v )
{
    emit comparisonChanged( v );
    emit updated();
}

void
ConstraintTypes::PlaylistLengthEditWidget::on_slider_Strictness_valueChanged( const int v )
{
    emit strictnessChanged( v );
    emit updated();
}
