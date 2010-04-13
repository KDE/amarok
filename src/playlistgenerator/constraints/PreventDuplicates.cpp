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

#define DEBUG_PREFIX "Constraint::PreventDuplicates"

#include "PreventDuplicates.h"
#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"

#include <QtGlobal>
#include <QtGui>
#include <math.h>

Constraint*
ConstraintTypes::PreventDuplicates::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p )
        return new PreventDuplicates( xmlelem, p );
    else
        return 0;
}

Constraint*
ConstraintTypes::PreventDuplicates::createNew( ConstraintNode* p )
{
    if ( p )
        return new PreventDuplicates( p );
    else
        return 0;
}

ConstraintFactoryEntry*
ConstraintTypes::PreventDuplicates::registerMe()
{
    return new ConstraintFactoryEntry( i18n("PreventDuplicates"),
                                i18n("Prevents duplicate tracks, albums, or artists from appearing in the playlist"),
                                &PreventDuplicates::createFromXml, &PreventDuplicates::createNew );
}

ConstraintTypes::PreventDuplicates::PreventDuplicates( QDomElement& xmlelem, ConstraintNode* p )
        : Constraint( p )
        , m_counterPtr( 0 )
{
    QDomAttr a;

    a = xmlelem.attributeNode( "field" );
    if ( !a.isNull() ) {
        m_field = static_cast<DupeField>( a.value().toInt() );
    }
}

ConstraintTypes::PreventDuplicates::PreventDuplicates( ConstraintNode* p )
        : Constraint( p )
        , m_field( DupeTrack )
        , m_counterPtr( 0 )
{ }

QWidget*
ConstraintTypes::PreventDuplicates::editWidget() const
{
    PreventDuplicatesEditWidget* e = new PreventDuplicatesEditWidget( m_field );
    connect( e, SIGNAL( fieldChanged( const int ) ), this, SLOT( setField( const int ) ) );
    return e;
}

void
ConstraintTypes::PreventDuplicates::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement c = doc.createElement( "constraint" );
    QDomText t = doc.createTextNode( getName() );
    c.appendChild( t );
    c.setAttribute( "type", "PreventDuplicates" );
    c.setAttribute( "field", QString::number( m_field ) );
    elem.appendChild( c );
}

QString
ConstraintTypes::PreventDuplicates::getName() const
{
    QString v( i18n("Prevent duplicate %1") );
    switch ( m_field ) {
        case DupeTrack:
            return v.arg( i18n("tracks") );
        case DupeArtist:
            return v.arg( i18n("artists") );
        case DupeAlbum:
            return v.arg( i18n("albums") );
    }
    return QString();
}

Collections::QueryMaker*
ConstraintTypes::PreventDuplicates::initQueryMaker( Collections::QueryMaker* qm ) const
{
    return qm;
}

double
ConstraintTypes::PreventDuplicates::satisfaction( const Meta::TrackList& tl )
{
    delete m_counterPtr;

    switch ( m_field ) {
        case DupeTrack:
            m_counterPtr = new TrackDuplicateCounter( tl );
            break;
        case DupeArtist:
            m_counterPtr = new ArtistDuplicateCounter( tl );
            break;
        case DupeAlbum:
            m_counterPtr = new AlbumDuplicateCounter( tl );
            break;
    }

    m_dupeCount = m_counterPtr->count();
    return penalty( m_dupeCount );
}

double
ConstraintTypes::PreventDuplicates::deltaS_insert( const Meta::TrackList&, const Meta::TrackPtr t, const int ) const
{
    int newcount = m_dupeCount + m_counterPtr->insertionDelta( t );
    return penalty( newcount ) - penalty( m_dupeCount );
}

double
ConstraintTypes::PreventDuplicates::deltaS_replace( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i ) const
{
    int newcount = m_dupeCount + m_counterPtr->insertionDelta( t ) + m_counterPtr->deletionDelta( tl.at( i ) );
    return penalty( newcount ) - penalty( m_dupeCount );
}

double
ConstraintTypes::PreventDuplicates::deltaS_delete( const Meta::TrackList& tl, const int i ) const
{
    int newcount = m_dupeCount + m_counterPtr->deletionDelta( tl.at( i ) );
    return penalty( newcount ) - penalty( m_dupeCount );
}

double
ConstraintTypes::PreventDuplicates::deltaS_swap( const Meta::TrackList&, const int, const int ) const
{
    return 0.0;
}

void
ConstraintTypes::PreventDuplicates::insertTrack( const Meta::TrackList&, const Meta::TrackPtr t, const int )
{
    m_counterPtr->insertTrack( t );
    m_dupeCount = m_counterPtr->count();
}

void
ConstraintTypes::PreventDuplicates::replaceTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i )
{
    m_counterPtr->deleteTrack( tl.at( i ) );
    m_counterPtr->insertTrack( t );
    m_dupeCount = m_counterPtr->count();
}

void
ConstraintTypes::PreventDuplicates::deleteTrack( const Meta::TrackList& tl, const int i )
{
    m_counterPtr->deleteTrack( tl.at( i ) );
    m_dupeCount = m_counterPtr->count();
}

void
ConstraintTypes::PreventDuplicates::swapTracks( const Meta::TrackList&, const int, const int )
{
}

ConstraintNode::Vote*
ConstraintTypes::PreventDuplicates::vote( const Meta::TrackList&, const Meta::TrackList& ) const
{
    ConstraintNode::Vote* v = 0;

    // TODO: voting for a delete or replace is probably useful, but not yet implemented

    return v;
}

#ifndef KDE_NO_DEBUG_OUTPUT
void
ConstraintTypes::PreventDuplicates::audit( const Meta::TrackList& ) const
{
}
#endif

void
ConstraintTypes::PreventDuplicates::setField( const int c )
{
    m_field = static_cast<DupeField>( c );
    emit dataChanged();
}

double
ConstraintTypes::PreventDuplicates::penalty( const int d ) const
{
    return exp( (double)d / -3.0 );
}

/******************************
 * ABC for duplicate counts   *
 ******************************/

ConstraintTypes::PreventDuplicates::DuplicateCounter::DuplicateCounter() : m_duplicateCount( 0 )
{
}

// track duplicates
ConstraintTypes::PreventDuplicates::TrackDuplicateCounter::TrackDuplicateCounter( const Meta::TrackList& tl )
            : PreventDuplicates::DuplicateCounter()
{
    foreach ( const Meta::TrackPtr& t, tl ) {
        if ( m_trackCounts.contains(t) ) {
            m_duplicateCount++;
            m_trackCounts[t]++;
        } else {
            m_trackCounts.insert( t, 1 );
        }
    }
}

int
ConstraintTypes::PreventDuplicates::TrackDuplicateCounter::insertionDelta( const Meta::TrackPtr t ) const
{
    if ( m_trackCounts.contains(t) && ( m_trackCounts.value(t) > 0 ) )
        return 1;
    else
        return 0;
}

int
ConstraintTypes::PreventDuplicates::TrackDuplicateCounter::deletionDelta( const Meta::TrackPtr t ) const
{
    if ( m_trackCounts.contains(t) && ( m_trackCounts.value(t) > 1 ) )
        return -1;
    else
        return 0;
}

void
ConstraintTypes::PreventDuplicates::TrackDuplicateCounter::insertTrack( const Meta::TrackPtr t )
{
    if ( m_trackCounts.contains(t) ) {
        if ( m_trackCounts.value(t) > 0 )
            m_duplicateCount++;
        m_trackCounts[t]++;
    } else {
        m_trackCounts.insert( t, 1 );
    }
}

void
ConstraintTypes::PreventDuplicates::TrackDuplicateCounter::deleteTrack( const Meta::TrackPtr t )
{
    if ( m_trackCounts.value(t) > 1)
        m_duplicateCount--;

    m_trackCounts[t]--;
}

int
ConstraintTypes::PreventDuplicates::TrackDuplicateCounter::trackCount( const Meta::TrackPtr t ) const
{
    return m_trackCounts.value( t );
}

// album duplicates
ConstraintTypes::PreventDuplicates::AlbumDuplicateCounter::AlbumDuplicateCounter( const Meta::TrackList& tl )
            : PreventDuplicates::DuplicateCounter()
{
    foreach ( const Meta::TrackPtr& t, tl ) {
        Meta::AlbumPtr a = t->album();
        if ( a == Meta::AlbumPtr() )
            continue;

        if ( m_albumCounts.contains(a) ) {
            m_duplicateCount++;
            m_albumCounts[a]++;
        } else {
            m_albumCounts.insert( a, 1 );
        }
    }
}

int
ConstraintTypes::PreventDuplicates::AlbumDuplicateCounter::insertionDelta( const Meta::TrackPtr t ) const
{
    Meta::AlbumPtr a = t->album();
    if ( a == Meta::AlbumPtr() )
        return 0;

    if ( m_albumCounts.contains(a) && ( m_albumCounts.value(a) > 0 ) )
        return 1;
    else
        return 0;
}

int
ConstraintTypes::PreventDuplicates::AlbumDuplicateCounter::deletionDelta( const Meta::TrackPtr t ) const
{
    Meta::AlbumPtr a = t->album();
    if ( a == Meta::AlbumPtr() )
        return 0;

    if ( m_albumCounts.contains(a) && ( m_albumCounts.value(a) > 1 ) )
        return -1;
    else
        return 0;
}

void
ConstraintTypes::PreventDuplicates::AlbumDuplicateCounter::insertTrack( const Meta::TrackPtr t )
{
    Meta::AlbumPtr a = t->album();
    if ( a == Meta::AlbumPtr() )
        return;

    if ( m_albumCounts.contains(a) ) {
        if ( m_albumCounts.value(a) > 0 )
            m_duplicateCount++;
        m_albumCounts[a]++;
    } else {
        m_albumCounts.insert( a, 1 );
    }
}

void
ConstraintTypes::PreventDuplicates::AlbumDuplicateCounter::deleteTrack( const Meta::TrackPtr t )
{
    Meta::AlbumPtr a = t->album();
    if ( a == Meta::AlbumPtr() )
        return;

    if ( m_albumCounts.value(a) > 1)
        m_duplicateCount--;

    m_albumCounts[a]--;
}

int
ConstraintTypes::PreventDuplicates::AlbumDuplicateCounter::trackCount( const Meta::TrackPtr t ) const
{
    if ( t->album() == Meta::AlbumPtr() )
        return 0;

    return m_albumCounts.value( t->album() );
}

// artist duplicates
ConstraintTypes::PreventDuplicates::ArtistDuplicateCounter::ArtistDuplicateCounter( const Meta::TrackList& tl )
            : PreventDuplicates::DuplicateCounter()
{
    foreach ( const Meta::TrackPtr& t, tl ) {
        Meta::ArtistPtr a = t->artist();
        if ( a == Meta::ArtistPtr() )
            continue;

        if ( m_artistCounts.contains(a) ) {
            m_duplicateCount++;
            m_artistCounts[a]++;
        } else {
            m_artistCounts.insert( a, 1 );
        }
    }
}

int
ConstraintTypes::PreventDuplicates::ArtistDuplicateCounter::insertionDelta( const Meta::TrackPtr t ) const
{
    Meta::ArtistPtr a = t->artist();
    if ( a == Meta::ArtistPtr() )
        return 0;

    if ( m_artistCounts.contains(a) && ( m_artistCounts.value(a) > 0 ) )
        return 1;
    else
        return 0;
}

int
ConstraintTypes::PreventDuplicates::ArtistDuplicateCounter::deletionDelta( const Meta::TrackPtr t ) const
{
    Meta::ArtistPtr a = t->artist();
    if ( a == Meta::ArtistPtr() )
        return 0;

    if ( m_artistCounts.contains(a) && ( m_artistCounts.value(a) > 1 ) )
        return -1;
    else
        return 0;
}

void
ConstraintTypes::PreventDuplicates::ArtistDuplicateCounter::insertTrack( const Meta::TrackPtr t )
{
    Meta::ArtistPtr a = t->artist();
    if ( a == Meta::ArtistPtr() )
        return;

    if ( m_artistCounts.contains(a) ) {
        if ( m_artistCounts.value(a) > 0 )
            m_duplicateCount++;
        m_artistCounts[a]++;
    } else {
        m_artistCounts.insert( a, 1 );
    }
}

void
ConstraintTypes::PreventDuplicates::ArtistDuplicateCounter::deleteTrack( const Meta::TrackPtr t )
{
    Meta::ArtistPtr a = t->artist();
    if ( a == Meta::ArtistPtr() )
        return;

    if ( m_artistCounts.value(a) > 1)
        m_duplicateCount--;

    m_artistCounts[a]--;
}

int
ConstraintTypes::PreventDuplicates::ArtistDuplicateCounter::trackCount( const Meta::TrackPtr t ) const
{
    if ( t->artist() == Meta::ArtistPtr() )
        return 0;

    return m_artistCounts.value( t->artist() );
}

/******************************
 * Edit Widget                *
 ******************************/

ConstraintTypes::PreventDuplicatesEditWidget::PreventDuplicatesEditWidget( const int field )
    : QWidget( 0 )
{
    ui.setupUi( this );
    ui.comboBox_Field->setCurrentIndex( field );
}

void
ConstraintTypes::PreventDuplicatesEditWidget::on_comboBox_Field_currentIndexChanged( const int v )
{
    emit fieldChanged( v );
    emit updated();
}
