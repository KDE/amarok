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

#define DEBUG_PREFIX "Constraint::Checkpoint"

#include "Checkpoint.h"
#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"

#include <QtGlobal>

Constraint*
ConstraintTypes::Checkpoint::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p ) {
        return new Checkpoint( xmlelem, p );
    } else {
        return 0;
    }
}

Constraint*
ConstraintTypes::Checkpoint::createNew( ConstraintNode* p )
{
    if ( p ) {
        return new Checkpoint( p );
    } else {
        return 0;
    }
}

ConstraintFactoryEntry*
ConstraintTypes::Checkpoint::registerMe()
{
    return new ConstraintFactoryEntry( i18n("Checkpoint"),
                                i18n("Fixes a track to a certain position in the playlist"),
                                &Checkpoint::createFromXml, &Checkpoint::createNew );
}

ConstraintTypes::Checkpoint::Checkpoint( QDomElement& xmlelem, ConstraintNode* p )
        : Constraint( p )
        , m_position( 0 )
        , m_strictness( 1.0 )
        , m_checkpointType( CheckpointTrack )
        , m_handler( 0 )
{
    QDomAttr a;

    a = xmlelem.attributeNode( "position" );
    if ( !a.isNull() )
        m_position = a.value().toInt();

    // TODO: read in track

    a = xmlelem.attributeNode( "strictness" );
    if ( !a.isNull() )
        m_strictness = a.value().toDouble();
}

ConstraintTypes::Checkpoint::Checkpoint( ConstraintNode* p )
        : Constraint( p )
        , m_position( 0 )
        , m_strictness( 1.0 )
        , m_checkpointType( CheckpointTrack )
        , m_handler( 0 )
{
}

QWidget*
ConstraintTypes::Checkpoint::editWidget() const
{
    CheckpointEditWidget* e = new CheckpointEditWidget( m_position, static_cast<int>( 10*m_strictness ) );
    connect( e, SIGNAL( positionChanged( const int ) ), this, SLOT( setPosition( const int ) ) );
    connect( e, SIGNAL( strictnessChanged( const int ) ), this, SLOT( setStrictness( const int ) ) );
    connect( e, SIGNAL( checkpointChanged( const Meta::DataPtr& ) ), this, SLOT( setCheckpoint( const Meta::DataPtr& ) ) );
    return e;
}

void
ConstraintTypes::Checkpoint::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    QDomElement c = doc.createElement( "constraint" );
    QDomText t = doc.createTextNode( getName() );
    c.appendChild( t );
    c.setAttribute( "type", "Checkpoint" );
    c.setAttribute( "position", m_position );
    // TODO: write track
    c.setAttribute( "strictness", QString::number( m_strictness ) );
    elem.appendChild( c );
}

QString
ConstraintTypes::Checkpoint::getName() const
{
    // FIXME
    return QString( "Checkpoint constraint" );
}

Collections::QueryMaker*
ConstraintTypes::Checkpoint::initQueryMaker( Collections::QueryMaker* qm ) const
{
    return qm;
}

double
ConstraintTypes::Checkpoint::satisfaction( const Meta::TrackList& tl )
{
#if 0
    delete m_handler;

    Meta::TrackPtr t;
    Meta::ArtistPtr a;
    Meta::AlbumPtr l;
    switch ( m_checkpointType ) {
        case CheckpointTrack:
            t = Meta::TrackPtr::dynamicCast( m_checkpointObject );
            m_handler = new TrackCheckpointHandler( m_position, t, tl );
            break;
        case CheckpointArtist:
            a = Meta::ArtistPtr::dynamicCast( m_checkpointObject );
            m_handler = new ArtistCheckpointHandler( m_position, a, tl );
            break;
        case CheckpointAlbum:
            l = Meta::AlbumPtr::dynamicCast( m_checkpointObject );
            m_handler = new AlbumCheckpointHandler( m_position, l, tl );
            break;
    }

    m_distance = m_handler->distance();
    return penalty( m_distance );
#endif
    // FIXME
    Q_UNUSED(tl)
    return 0.0;
}

double
ConstraintTypes::Checkpoint::deltaS_insert( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i ) const
{
    // FIXME
    Q_UNUSED(tl)
    Q_UNUSED(t)
    Q_UNUSED(i)
    return 0.0;
}

double
ConstraintTypes::Checkpoint::deltaS_replace( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i ) const
{
    // FIXME
    Q_UNUSED(tl)
    Q_UNUSED(t)
    Q_UNUSED(i)
    return 0.0;
}

double
ConstraintTypes::Checkpoint::deltaS_delete( const Meta::TrackList& tl, const int i ) const
{
    // FIXME
    Q_UNUSED(tl)
    Q_UNUSED(i)
    return 0.0;
}

double
ConstraintTypes::Checkpoint::deltaS_swap( const Meta::TrackList& tl, const int i, const int j ) const
{
    // FIXME
    Q_UNUSED(tl)
    Q_UNUSED(i)
    Q_UNUSED(j)
    return 0.0;
}

void
ConstraintTypes::Checkpoint::insertTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i )
{
    // FIXME
    Q_UNUSED(tl)
    Q_UNUSED(t)
    Q_UNUSED(i)
}

void
ConstraintTypes::Checkpoint::replaceTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i )
{
    // FIXME
    Q_UNUSED(tl)
    Q_UNUSED(t)
    Q_UNUSED(i)
}

void
ConstraintTypes::Checkpoint::deleteTrack( const Meta::TrackList& tl, const int i )
{
    // FIXME
    Q_UNUSED(tl)
    Q_UNUSED(i)
}

void
ConstraintTypes::Checkpoint::swapTracks( const Meta::TrackList& tl, const int i, const int j )
{
    // FIXME
    Q_UNUSED(tl)
    Q_UNUSED(i)
    Q_UNUSED(j)
}

ConstraintNode::Vote*
ConstraintTypes::Checkpoint::vote( const Meta::TrackList& playlist, const Meta::TrackList& domain ) const
{
    ConstraintNode::Vote* v = 0;

    // FIXME: vote to put a needed track at the needed position
    Q_UNUSED(playlist)
    Q_UNUSED(domain)

    return v;
}

void
ConstraintTypes::Checkpoint::setPosition( const int v )
{
    m_position = v;
}

void
ConstraintTypes::Checkpoint::setStrictness( const int sv )
{
    m_strictness = static_cast<double>(sv)/10.0;
}

void
ConstraintTypes::Checkpoint::setCheckpoint( const Meta::DataPtr& data )
{
    if ( data == Meta::DataPtr() )
        return;

    if ( Meta::TrackPtr track = Meta::TrackPtr::dynamicCast( data ) ) {
        m_checkpointType = CheckpointTrack;
    } else if ( Meta::AlbumPtr album = Meta::AlbumPtr::dynamicCast( data ) ) {
        m_checkpointType = CheckpointAlbum;
    } else if ( Meta::ArtistPtr track = Meta::ArtistPtr::dynamicCast( data ) ) {
        m_checkpointType = CheckpointArtist;
    }
    m_checkpointObject = data;
}

double
ConstraintTypes::Checkpoint::penalty( const int d ) const
{
    // FIXME
    Q_UNUSED(d)
    return 0.0;
}

/******************************
 * Edit Widget                *
 ******************************/

ConstraintTypes::CheckpointEditWidget::CheckpointEditWidget( const int length,
                                                             const int strictness ) : QWidget( 0 )
{
    ui.setupUi( this );

    ui.timeEdit_Position->setTime( QTime().addSecs( length ) );
    ui.slider_Strictness->setValue( strictness );
}

void
ConstraintTypes::CheckpointEditWidget::on_timeEdit_Position_timeChanged( const QTime& t )
{
    emit positionChanged( QTime().secsTo( t ) );
    emit updated();
}

void
ConstraintTypes::CheckpointEditWidget::on_slider_Strictness_valueChanged( const int v )
{
    emit strictnessChanged( v );
    emit updated();
}

void
ConstraintTypes::CheckpointEditWidget::on_trackSelector_selectionChanged( const Meta::DataPtr& data )
{
    emit checkpointChanged( data );
    emit updated();
}
