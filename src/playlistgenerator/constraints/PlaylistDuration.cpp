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

#define DEBUG_PREFIX "Constraint::PlaylistDuration"

#include "PlaylistDuration.h"

#include "core/meta/Meta.h"
#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include <cmath>
#include <cstdlib>

Constraint*
ConstraintTypes::PlaylistDuration::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p ) {
        return new PlaylistDuration( xmlelem, p );
    } else {
        return nullptr;
    }
}

Constraint*
ConstraintTypes::PlaylistDuration::createNew( ConstraintNode* p )
{
    if ( p ) {
        return new PlaylistDuration( p );
    } else {
        return nullptr;
    }
}

ConstraintFactoryEntry*
ConstraintTypes::PlaylistDuration::registerMe()
{
    return new ConstraintFactoryEntry( QStringLiteral("PlaylistDuration"),
                                       i18n("Playlist Duration"),
                                       i18n("Sets the preferred duration of the playlist"),
                                       &PlaylistDuration::createFromXml, &PlaylistDuration::createNew );
}

ConstraintTypes::PlaylistDuration::PlaylistDuration( QDomElement& xmlelem, ConstraintNode* p )
        : Constraint( p )
        , m_duration( 0 )
        , m_comparison( CompareNumEquals )
        , m_strictness( 1.0 )
{
    QDomAttr a;

    a = xmlelem.attributeNode( QStringLiteral("duration") );
    if ( !a.isNull() ) {
        m_duration = a.value().toInt();
    } else {
        // Accommodate schema change when PlaylistLength became PlaylistDuration
        a = xmlelem.attributeNode( QStringLiteral("length") );
        if ( !a.isNull() )
            m_duration = a.value().toInt();
    }


    a = xmlelem.attributeNode( QStringLiteral("comparison") );
    if ( !a.isNull() )
        m_comparison = a.value().toInt();

    a = xmlelem.attributeNode( QStringLiteral("strictness") );
    if ( !a.isNull() )
        m_strictness = a.value().toDouble();
}

ConstraintTypes::PlaylistDuration::PlaylistDuration( ConstraintNode* p )
        : Constraint( p )
        , m_duration( 0 )
        , m_comparison( CompareNumEquals )
        , m_strictness( 1.0 )
{
}

QWidget*
ConstraintTypes::PlaylistDuration::editWidget() const
{
    PlaylistDurationEditWidget* e = new PlaylistDurationEditWidget( m_duration, m_comparison, static_cast<int>( 10*m_strictness ) );
    connect( e, &PlaylistDurationEditWidget::comparisonChanged, this, &PlaylistDuration::setComparison );
    connect( e, &PlaylistDurationEditWidget::durationChanged, this, &PlaylistDuration::setDuration );
    connect( e, &PlaylistDurationEditWidget::strictnessChanged, this, &PlaylistDuration::setStrictness );
    return e;
}

void
ConstraintTypes::PlaylistDuration::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement c = doc.createElement( QStringLiteral("constraint") );
    c.setAttribute( QStringLiteral("type"), QStringLiteral("PlaylistDuration") );
    c.setAttribute( QStringLiteral("duration"), QString::number( m_duration ) );
    c.setAttribute( QStringLiteral("comparison"), QString::number( m_comparison ) );
    c.setAttribute( QStringLiteral("strictness"), QString::number( m_strictness ) );
    elem.appendChild( c );
}

QString
ConstraintTypes::PlaylistDuration::getName() const
{
    KLocalizedString v;
    if ( m_comparison == CompareNumEquals ) {
        v = ki18nc( "%1 is a length of time (e.g. 5:00 for 5 minutes)", "Playlist duration: equals %1");
    } else if ( m_comparison == CompareNumGreaterThan ) {
        v = ki18nc( "%1 is a length of time (e.g. 5:00 for 5 minutes)", "Playlist duration: more than %1");
    } else if ( m_comparison == CompareNumLessThan ) {
        v = ki18nc( "%1 is a length of time (e.g. 5:00 for 5 minutes)", "Playlist duration: less than %1");
    } else {
        v = ki18n( "Playlist duration: unknown");
    }
    v = v.subs( QTime(0, 0, 0).addMSecs( m_duration ).toString( QStringLiteral("H:mm:ss") ) );
    return v.toString();
}

double
ConstraintTypes::PlaylistDuration::satisfaction( const Meta::TrackList& tl ) const
{
    qint64 l = 0;
    for( const Meta::TrackPtr &t : tl ) {
        l += t->length();
    }

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

quint32
ConstraintTypes::PlaylistDuration::suggestPlaylistSize() const
{
    if ( m_comparison == CompareNumLessThan ) {
        return static_cast<quint32>( m_duration ) / 300000 ;
    } else if ( m_comparison == CompareNumGreaterThan ) {
        return static_cast<quint32>( m_duration ) / 180000;
    } else {
        return static_cast<quint32>( m_duration ) / 240000;
    }
}

void
ConstraintTypes::PlaylistDuration::setComparison( const int c )
{
    m_comparison = c;
    Q_EMIT dataChanged();
}

void
ConstraintTypes::PlaylistDuration::setDuration( const int v )
{
    m_duration = v;
    Q_EMIT dataChanged();
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
                                                                     const int strictness ) : QWidget( nullptr )
{
    ui.setupUi( this );

    ui.timeEdit_Duration->setTime( QTime(0, 0, 0).addMSecs( duration ) );
    ui.comboBox_Comparison->setCurrentIndex( comparison );
    ui.slider_Strictness->setValue( strictness );
}

void
ConstraintTypes::PlaylistDurationEditWidget::on_timeEdit_Duration_timeChanged( const QTime& t )
{
    Q_EMIT durationChanged( QTime(0, 0, 0).msecsTo( t ) );
    Q_EMIT updated();
}

void
ConstraintTypes::PlaylistDurationEditWidget::on_comboBox_Comparison_currentIndexChanged( const int v )
{
    Q_EMIT comparisonChanged( v );
    Q_EMIT updated();
}

void
ConstraintTypes::PlaylistDurationEditWidget::on_slider_Strictness_valueChanged( const int v )
{
    Q_EMIT strictnessChanged( v );
    Q_EMIT updated();
}
