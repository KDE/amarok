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

#define DEBUG_PREFIX "Constraint::PlaylistFileSize"

#include "PlaylistFileSize.h"

#include "core/meta/Meta.h"
#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include <cmath>
#include <cstdlib>

#include <KFormat>


Constraint*
ConstraintTypes::PlaylistFileSize::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p ) {
        return new PlaylistFileSize( xmlelem, p );
    } else {
        return nullptr;
    }
}

Constraint*
ConstraintTypes::PlaylistFileSize::createNew( ConstraintNode* p )
{
    if ( p ) {
        return new PlaylistFileSize( p );
    } else {
        return nullptr;
    }
}

ConstraintFactoryEntry*
ConstraintTypes::PlaylistFileSize::registerMe()
{
    return new ConstraintFactoryEntry( QStringLiteral("PlaylistFileSize"),
                                       i18n("Total File Size of Playlist"),
                                       i18n("Sets the preferred total file size of the playlist"),
                                       &PlaylistFileSize::createFromXml, &PlaylistFileSize::createNew );
}

ConstraintTypes::PlaylistFileSize::PlaylistFileSize( QDomElement& xmlelem, ConstraintNode* p )
        : Constraint( p )
        , m_size( 700 )
        , m_unit( 1 )
        , m_comparison( CompareNumEquals )
        , m_strictness( 1.0 )
{
    QDomAttr a;

    a = xmlelem.attributeNode( QStringLiteral("size") );
    if ( !a.isNull() )
        m_size = a.value().toInt();

    a = xmlelem.attributeNode( QStringLiteral("unit") );
    if ( !a.isNull() )
        m_unit = a.value().toInt();

    a = xmlelem.attributeNode( QStringLiteral("comparison") );
    if ( !a.isNull() )
        m_comparison = a.value().toInt();

    a = xmlelem.attributeNode( QStringLiteral("strictness") );
    if ( !a.isNull() )
        m_strictness = a.value().toDouble();
}

ConstraintTypes::PlaylistFileSize::PlaylistFileSize( ConstraintNode* p )
        : Constraint( p )
        , m_size( 700 )
        , m_unit( 1 )
        , m_comparison( CompareNumEquals )
        , m_strictness( 1.0 )
{
}

QWidget*
ConstraintTypes::PlaylistFileSize::editWidget() const
{
    PlaylistFileSizeEditWidget* e = new PlaylistFileSizeEditWidget( m_size, m_unit, m_comparison, static_cast<int>( 10*m_strictness ) );
    connect( e, &PlaylistFileSizeEditWidget::comparisonChanged, this, &PlaylistFileSize::setComparison );
    connect( e, &PlaylistFileSizeEditWidget::sizeChanged, this, &PlaylistFileSize::setSize );
    connect( e, &PlaylistFileSizeEditWidget::unitChanged, this, &PlaylistFileSize::setUnit );
    connect( e, &PlaylistFileSizeEditWidget::strictnessChanged, this, &PlaylistFileSize::setStrictness );
    return e;
}

void
ConstraintTypes::PlaylistFileSize::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement c = doc.createElement( QStringLiteral("constraint") );
    c.setAttribute( QStringLiteral("type"), QStringLiteral("PlaylistFileSize") );
    c.setAttribute( QStringLiteral("size"), QString::number( m_size ) );
    c.setAttribute( QStringLiteral("unit"), QString::number( m_unit ) );
    c.setAttribute( QStringLiteral("comparison"), QString::number( m_comparison ) );
    c.setAttribute( QStringLiteral("strictness"), QString::number( m_strictness ) );
    elem.appendChild( c );
}

QString
ConstraintTypes::PlaylistFileSize::getName() const
{
    KLocalizedString v;
    if ( m_comparison == CompareNumEquals ) {
        v = ki18nc( "%1 is a file size (e.g. 50 MB)", "Total file size of playlist: equals %1");
    } else if ( m_comparison == CompareNumGreaterThan ) {
        v = ki18nc( "%1 is a file size (e.g. 50 MB)", "Total file size of playlist: more than %1");
    } else if ( m_comparison == CompareNumLessThan ) {
        v = ki18nc( "%1 is a file size (e.g. 50 MB)", "Total file size of playlist: less than %1");
    } else {
        v = ki18n( "Total file size of playlist: unknown");
    }
    v = v.subs( KFormat().formatByteSize( getWantedSize(), 1, KFormat::MetricBinaryDialect ) );
    return v.toString();
}

double
ConstraintTypes::PlaylistFileSize::satisfaction( const Meta::TrackList& tl ) const
{
    quint64 tlSize = 0;
    for ( const Meta::TrackPtr &t : tl ) {
        // Boy it sure would be nice if Qt had a "reduce" function built in
        tlSize += static_cast<quint64>( t->filesize() );
    }

    quint64 wantedSize = getWantedSize();

    if ( m_comparison == CompareNumEquals ) {
        if ( tlSize > wantedSize )
            return ( tlSize == wantedSize ) ? 1.0 : transformFileSize( tlSize - wantedSize );
        else
            return ( tlSize == wantedSize ) ? 1.0 : transformFileSize( wantedSize - tlSize );
    } else if ( m_comparison == CompareNumGreaterThan ) {
        return ( tlSize > wantedSize ) ? 1.0 : transformFileSize( wantedSize - tlSize );
    } else if ( m_comparison == CompareNumLessThan ) {
        return ( tlSize < wantedSize ) ? 1.0 : transformFileSize( tlSize - wantedSize );
    } else {
        return 0.0;
    }
}

quint32
ConstraintTypes::PlaylistFileSize::suggestPlaylistSize() const
{
    // estimate that each file is about 8MB large
    quint64 s = getWantedSize() / static_cast<quint64>( 8000000 );
    return static_cast<quint32>( s );
}

quint64
ConstraintTypes::PlaylistFileSize::getWantedSize() const
{
    switch ( m_unit ) {
        case 0:
            return static_cast<quint64>( m_size ) * Q_INT64_C( 1000 );
        case 1:
            return static_cast<quint64>( m_size ) * Q_INT64_C( 1000000 );
        case 2:
            return static_cast<quint64>( m_size ) * Q_INT64_C( 1000000000 );
        case 3:
            return static_cast<quint64>( m_size ) * Q_INT64_C( 1000000000000 );
        default:
            return static_cast<quint64>( m_size ) * Q_INT64_C( 1 );
    }
}

double
ConstraintTypes::PlaylistFileSize::transformFileSize( const quint64 delta ) const
{
    // Note: delta must be positive
    const double factor = m_strictness * 3e-9;
    return 1.0 / (1.0 + exp( factor*(double)(delta)));
}

void
ConstraintTypes::PlaylistFileSize::setComparison( const int c )
{
    m_comparison = c;
    Q_EMIT dataChanged();
}

void
ConstraintTypes::PlaylistFileSize::setSize( const int l )
{
    m_size = l;
    Q_EMIT dataChanged();
}

void
ConstraintTypes::PlaylistFileSize::setStrictness( const int sv )
{
    m_strictness = static_cast<double>(sv)/10.0;
}

void
ConstraintTypes::PlaylistFileSize::setUnit( const int u )
{
    m_unit = u;
    Q_EMIT dataChanged();
}

/******************************
 * Edit Widget                *
 ******************************/

ConstraintTypes::PlaylistFileSizeEditWidget::PlaylistFileSizeEditWidget( const int size,
                                                                     const int unit,
                                                                     const int comparison,
                                                                     const int strictness ) : QWidget( nullptr )
{
    ui.setupUi( this );

    ui.spinBox_Size->setValue( size );
    ui.comboBox_Unit->setCurrentIndex( unit );
    ui.comboBox_Comparison->setCurrentIndex( comparison );
    ui.slider_Strictness->setValue( strictness );
}

void
ConstraintTypes::PlaylistFileSizeEditWidget::on_spinBox_Size_valueChanged( const int l )
{
    Q_EMIT sizeChanged( l );
    Q_EMIT updated();
}

void
ConstraintTypes::PlaylistFileSizeEditWidget::on_comboBox_Unit_currentIndexChanged( const int l )
{
    Q_EMIT unitChanged( l );
    Q_EMIT updated();
}

void
ConstraintTypes::PlaylistFileSizeEditWidget::on_comboBox_Comparison_currentIndexChanged( const int v )
{
    Q_EMIT comparisonChanged( v );
    Q_EMIT updated();
}

void
ConstraintTypes::PlaylistFileSizeEditWidget::on_slider_Strictness_valueChanged( const int v )
{
    Q_EMIT strictnessChanged( v );
    Q_EMIT updated();
}
