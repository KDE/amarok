/****************************************************************************************
 * Copyright (c) 2008-2012 Soren Harward <stharward@gmail.com>                          *
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

#include <stdlib.h>
#include <math.h>

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
    return new ConstraintFactoryEntry( "PlaylistLength",
                                       i18n("Playlist Length"),
                                       i18n("Sets the preferred number of tracks in the playlist"),
                                       &PlaylistLength::createFromXml, &PlaylistLength::createNew );
}

ConstraintTypes::PlaylistLength::PlaylistLength( QDomElement& xmlelem, ConstraintNode* p )
        : Constraint( p )
        , m_length( 30 )
        , m_comparison( CompareNumEquals )
        , m_strictness( 1.0 )
{
    QDomAttr a;

    a = xmlelem.attributeNode( "length" );
    if ( !a.isNull() ) {
        m_length = a.value().toInt();
        /* after 2.3.2, what was the PlaylistLength constraint became the
         * PlaylistDuration constraint, so this works around the instance when
         * a user loads an XML file generated with the old code -- sth*/
        if ( m_length > 1000 )
            m_length /= 240000;
    }

    a = xmlelem.attributeNode( "comparison" );
    if ( !a.isNull() )
        m_comparison = a.value().toInt();

    a = xmlelem.attributeNode( "strictness" );
    if ( !a.isNull() )
        m_strictness = a.value().toDouble();
}

ConstraintTypes::PlaylistLength::PlaylistLength( ConstraintNode* p )
        : Constraint( p )
        , m_length( 30 )
        , m_comparison( CompareNumEquals )
        , m_strictness( 1.0 )
{
}

QWidget*
ConstraintTypes::PlaylistLength::editWidget() const
{
    PlaylistLengthEditWidget* e = new PlaylistLengthEditWidget( m_length, m_comparison, static_cast<int>( 10*m_strictness ) );
    connect( e, SIGNAL(comparisonChanged(int)), this, SLOT(setComparison(int)) );
    connect( e, SIGNAL(lengthChanged(int)), this, SLOT(setLength(int)) );
    connect( e, SIGNAL(strictnessChanged(int)), this, SLOT(setStrictness(int)) );
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

    KLocalizedString v;
    if ( m_comparison == CompareNumEquals ) {
        v = ki18ncp( "%1 is a number", "Playlist length: 1 track", "Playlist length: %1 tracks");
    } else if ( m_comparison == CompareNumGreaterThan ) {
        v = ki18ncp( "%1 is a number", "Playlist length: more than 1 track",
                     "Playlist length: more than %1 tracks");
    } else if ( m_comparison == CompareNumLessThan ) {
        v = ki18ncp( "%1 is a number", "Playlist length: less than 1 track",
                     "Playlist length: less than %1 tracks");
    } else {
        v = ki18n( "Playlist length: unknown");
    }
    v = v.subs( m_length );
    return v.toString();
}

double
ConstraintTypes::PlaylistLength::satisfaction( const Meta::TrackList& tl ) const
{
    quint32 l = static_cast<quint32>( tl.size() );
    if ( m_comparison == CompareNumEquals ) {
        if ( l > m_length )
            return ( l == m_length ) ? 1.0 : transformLength( l - m_length );
        else
            return ( l == m_length ) ? 1.0 : transformLength( m_length - l );
    } else if ( m_comparison == CompareNumGreaterThan ) {
        return ( l > m_length ) ? 1.0 : transformLength( m_length - l );
    } else if ( m_comparison == CompareNumLessThan ) {
        return ( l < m_length ) ? 1.0 : transformLength( l - m_length );
    } else {
        return 0.0;
    }
}

quint32
ConstraintTypes::PlaylistLength::suggestPlaylistSize() const
{
    return m_length;
}

double
ConstraintTypes::PlaylistLength::transformLength( const int delta ) const
{
    // Note: delta must be positive
    const double w = 5.0;
    return exp( -2.0 * ( 0.01 + m_strictness ) / w * ( delta + 1 ) );
}

void
ConstraintTypes::PlaylistLength::setComparison( const int c )
{
    m_comparison = c;
    emit dataChanged();
}

void
ConstraintTypes::PlaylistLength::setLength( const int l )
{
    m_length = static_cast<quint32>(l);
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

    ui.spinBox_Length->setValue( length );
    ui.comboBox_Comparison->setCurrentIndex( comparison );
    ui.slider_Strictness->setValue( strictness );
}

void
ConstraintTypes::PlaylistLengthEditWidget::on_spinBox_Length_valueChanged( const int l )
{
    emit lengthChanged( l );
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
