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
#include "core-impl/collections/support/CollectionManager.h"

#include <KRandom>
#include <KUrl>

#include <QtGlobal>

#include <climits>

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
                                i18n("Fixes a track, album, or artist to a certain position in the playlist"),
                                &Checkpoint::createFromXml, &Checkpoint::createNew );
}

ConstraintTypes::Checkpoint::Checkpoint( QDomElement& xmlelem, ConstraintNode* p )
        : Constraint( p )
        , m_position( 0 )
        , m_strictness( 1.0 )
        , m_checkpointType( CheckpointTrack )
        , m_handler( 0 )
        , m_tracker( 0 )
{
    QDomAttr a;

    a = xmlelem.attributeNode( "position" );
    if ( !a.isNull() )
        m_position = a.value().toInt();


    a = xmlelem.attributeNode( "checkpointtype" );
    if ( !a.isNull() )
        m_checkpointType = static_cast<CheckpointType>( a.value().toInt() );

    a = xmlelem.attributeNode( "trackurl" );
    if ( !a.isNull() ) {
        Meta::TrackPtr trk = CollectionManager::instance()->trackForUrl( KUrl( a.value() ) );
        if ( trk != Meta::TrackPtr() ) {
            if ( m_checkpointType == CheckpointAlbum ) {
                m_checkpointObject = Meta::DataPtr::dynamicCast( trk->album() );
            } else if ( m_checkpointType == CheckpointArtist ) {
                m_checkpointObject = Meta::DataPtr::dynamicCast( trk->artist() );
            } else {
                m_checkpointObject = Meta::DataPtr::dynamicCast( trk );
            }
        }
        debug() << "loaded" << m_checkpointObject->prettyName() << "from XML";
    }

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
        , m_tracker( 0 )
{
}

ConstraintTypes::Checkpoint::~Checkpoint()
{
    delete m_handler;
    delete m_tracker;
}


QWidget*
ConstraintTypes::Checkpoint::editWidget() const
{
    CheckpointEditWidget* e = new CheckpointEditWidget( m_position, static_cast<int>( 10*m_strictness ), m_checkpointObject );
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
    c.setAttribute( "checkpointtype", m_checkpointType );
    Meta::TrackPtr r;
    Meta::ArtistPtr a;
    Meta::AlbumPtr l;
    switch ( m_checkpointType ) {
        case CheckpointTrack:
            r = Meta::TrackPtr::dynamicCast( m_checkpointObject );
            c.setAttribute( "trackurl", r->uidUrl() );
            break;
        case CheckpointAlbum:
            l = Meta::AlbumPtr::dynamicCast( m_checkpointObject );
            if ( l->tracks().length() > 0 ) {
                r = l->tracks().first();
                c.setAttribute( "trackurl", r->uidUrl() );
            }
            break;
        case CheckpointArtist:
            a = Meta::ArtistPtr::dynamicCast( m_checkpointObject );
            if ( a->tracks().length() > 0 ) {
                r = a->tracks().first();
                c.setAttribute( "trackurl", r->uidUrl() );
            }
            break;
    }
    c.setAttribute( "strictness", QString::number( m_strictness ) );
    elem.appendChild( c );
}

QString
ConstraintTypes::Checkpoint::getName() const
{
    QString name( "Checkpoint: %1" );
    Meta::TrackPtr t;
    Meta::AlbumPtr l;
    Meta::ArtistPtr r;
    switch ( m_checkpointType ) {
        case CheckpointTrack:
            t = Meta::TrackPtr::dynamicCast( m_checkpointObject );
            name = name.arg( "\"%1\" (track) by %2" ).arg( t->prettyName() ).arg( t->artist()->prettyName() );
            break;
        case CheckpointAlbum:
            l = Meta::AlbumPtr::dynamicCast( m_checkpointObject );
            if ( l->hasAlbumArtist() ) {
                name = name.arg( "\"%1\" (album) by %2" ).arg( l->prettyName() ).arg( l->albumArtist()->prettyName() );
            } else {
                name = name.arg( "\"%1\" (album)" ).arg( l->prettyName() );
            }
            break;
        case CheckpointArtist:
            r = Meta::ArtistPtr::dynamicCast( m_checkpointObject );
            name = name.arg("\"%1\" (artist)").arg( r->prettyName() );
            break;
    }

    return name;
}

Collections::QueryMaker*
ConstraintTypes::Checkpoint::initQueryMaker( Collections::QueryMaker* qm ) const
{
    return qm;
}

double
ConstraintTypes::Checkpoint::satisfaction( const Meta::TrackList& tl )
{
    delete m_handler;
    delete m_tracker;

    Meta::TrackPtr t;
    Meta::ArtistPtr a;
    Meta::AlbumPtr l;
    switch ( m_checkpointType ) {
        case CheckpointTrack:
            t = Meta::TrackPtr::dynamicCast( m_checkpointObject );
            m_handler = new TrackMatcher( t );
            break;
        case CheckpointArtist:
            a = Meta::ArtistPtr::dynamicCast( m_checkpointObject );
            m_handler = new ArtistMatcher( a );
            break;
        case CheckpointAlbum:
            l = Meta::AlbumPtr::dynamicCast( m_checkpointObject );
            m_handler = new AlbumMatcher( l );
            break;
    }
    m_tracker = new BoundaryTracker( tl );

    m_distance = findDistanceFor( tl, m_tracker );

    return penalty( m_distance );
}

double
ConstraintTypes::Checkpoint::deltaS_insert( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i ) const
{
    BoundaryTracker* newBT = m_tracker->cloneAndInsert( t, i );
    qint64 newDist = findDistanceFor( tl, newBT );
    delete newBT;
    return penalty( newDist ) - penalty( m_distance );
}

double
ConstraintTypes::Checkpoint::deltaS_replace( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i ) const
{
    BoundaryTracker* newBT = m_tracker->cloneAndReplace( t, i );
    qint64 newDist = findDistanceFor( tl, newBT );
    delete newBT;
    return penalty( newDist ) - penalty( m_distance );
}

double
ConstraintTypes::Checkpoint::deltaS_delete( const Meta::TrackList& tl, const int i ) const
{
    BoundaryTracker* newBT = m_tracker->cloneAndDelete( i );
    qint64 newDist = findDistanceFor( tl, newBT );
    delete newBT;
    return penalty( newDist ) - penalty( m_distance );
}

double
ConstraintTypes::Checkpoint::deltaS_swap( const Meta::TrackList& tl, const int i, const int j ) const
{
    BoundaryTracker* newBT = m_tracker->cloneAndSwap( i, j );
    qint64 newDist = findDistanceFor( tl, newBT );
    delete newBT;
    return penalty( newDist ) - penalty( m_distance );
}

void
ConstraintTypes::Checkpoint::insertTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i )
{
    m_tracker->insertTrack( t, i );
    m_distance = findDistanceFor( tl, m_tracker );
}

void
ConstraintTypes::Checkpoint::replaceTrack( const Meta::TrackList& tl, const Meta::TrackPtr t, const int i )
{
    m_tracker->replaceTrack( t, i );
    m_distance = findDistanceFor( tl, m_tracker );
}

void
ConstraintTypes::Checkpoint::deleteTrack( const Meta::TrackList& tl, const int i )
{
    m_tracker->deleteTrack( i );
    m_distance = findDistanceFor( tl, m_tracker );
}

void
ConstraintTypes::Checkpoint::swapTracks( const Meta::TrackList& tl, const int i, const int j )
{
    m_tracker->swapTracks( i, j );
    m_distance = findDistanceFor( tl, m_tracker );
}

int
ConstraintTypes::Checkpoint::suggestInitialPlaylistSize() const
{
    return static_cast<int>( m_position / 300000 ) + 1;
}

ConstraintNode::Vote*
ConstraintTypes::Checkpoint::vote( const Meta::TrackList& playlist, const Meta::TrackList& domain ) const
{
    Q_UNUSED( playlist )
    if ( m_distance == 0 )
        return 0;

    ConstraintNode::Vote* v = new ConstraintNode::Vote();

    // TODO: possible future optimization
    Checkpoint::BoundaryTracker* tracker = new Checkpoint::BoundaryTracker( playlist );
    v->place = tracker->indexAtTime( m_position );

    if ( v->place == playlist.length() ) {
        v->operation = OperationInsert;
        v->track = m_handler->suggest( domain );
        return v;
    }

    QList<int> possibilities = m_handler->find( playlist );
    if ( possibilities.length() > 0 ) {
        v->operation = OperationSwap;
        v->other = possibilities.at( KRandom::random() % possibilities.length() );

        // not really sure why this is necessary -- the m_distance check above seems not to work
        if ( v->place == v->other )
            return 0;
    } else {
        v->operation = OperationReplace;
        v->track = m_handler->suggest( domain );
    }

    delete tracker;

    return v;
}

void
ConstraintTypes::Checkpoint::audit(const Meta::TrackList& tl ) const
{
    m_tracker->audit( tl );
}

void
ConstraintTypes::Checkpoint::setPosition( const int v )
{
    m_position = static_cast<qint64>( v );
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
        debug() << "setting checkpoint track:" << track->prettyName();
    } else if ( Meta::AlbumPtr album = Meta::AlbumPtr::dynamicCast( data ) ) {
        m_checkpointType = CheckpointAlbum;
        debug() << "setting checkpoint album:" << album->prettyName();
    } else if ( Meta::ArtistPtr artist = Meta::ArtistPtr::dynamicCast( data ) ) {
        debug() << "setting checkpoint artist:" << artist->prettyName();
        m_checkpointType = CheckpointArtist;
    }
    m_checkpointObject = data;
    emit dataChanged();
}

qint64
ConstraintTypes::Checkpoint::findDistanceFor( const Meta::TrackList& tl, const BoundaryTracker* const tracker ) const
{
    QList<int> matchPostitions = m_handler->find( tl );

    // case: playlist does not contain the necessary track; return "infinite" distance
    if ( matchPostitions.length() == 0 ) {
        return static_cast<qint64>(LLONG_MAX);
    }

    int targetPosition = tracker->indexAtTime( m_position );

    // case: track is in the correct position
    if ( matchPostitions.contains( targetPosition ) ) {
        return 0;
    }

    // case: only one appropriate track is in the playlist, but it's in the wrong place
    if ( matchPostitions.length() == 1 ) {
        int pos = matchPostitions.first();
        QPair<qint64,qint64> bounds = tracker->getBoundariesAt( pos );
        if ( pos > targetPosition ) {
            return bounds.first - m_position;
        } else if ( pos < targetPosition ) {
            return m_position - bounds.second;
        } else { // this really shouldn't happen, but worth double-checking
            return 0;
        }
    }

    // cases: target position is before or after all of the appropriate tracks
    if ( matchPostitions.first() > targetPosition ) {
        QPair<qint64,qint64> bounds = tracker->getBoundariesAt( matchPostitions.first() );
        return bounds.first - m_position;
    }

    if ( matchPostitions.last() < targetPosition ) {
        QPair<qint64,qint64> bounds = tracker->getBoundariesAt( matchPostitions.last() );
        return m_position - bounds.second;
    }

    // case: target position is between two of the possible matches
    for ( int i = 1; i < matchPostitions.length(); i++ ) {
        int below = matchPostitions.at( i - 1 );
        int above = matchPostitions.at( i );
        if ( ( below < targetPosition ) && ( above > targetPosition ) ) {
            QPair<qint64, qint64> lowBounds = tracker->getBoundariesAt( below );
            qint64 lowDelta = m_position - lowBounds.second;

            QPair<qint64, qint64> hiBounds = tracker->getBoundariesAt( above );
            qint64 hiDelta = hiBounds.first - m_position;

            return ( lowDelta < hiDelta ) ? lowDelta : hiDelta;
        }
    }

    warning() << "the satisfaction routine failed to handle its input correctly";
    return static_cast<qint64>(LLONG_MAX);
}

double
ConstraintTypes::Checkpoint::penalty( const qint64 d ) const
{
    if ( d == static_cast<qint64>(LLONG_MAX) )
        return 0.0;
    else
        return exp( d / ( -( 120000.0 * ( 1.0 + ( 8.0 * m_strictness ) ) ) ) );
}

/******************************
 * Edit Widget                *
 ******************************/

ConstraintTypes::CheckpointEditWidget::CheckpointEditWidget( const qint64 length,
                                                             const int strictness,
                                                             const Meta::DataPtr& data ) : QWidget( 0 )
{
    ui.setupUi( this );

    ui.timeEdit_Position->setTime( QTime().addMSecs( length ) );
    ui.slider_Strictness->setValue( strictness );
    ui.trackSelector->setData( data );
}

void
ConstraintTypes::CheckpointEditWidget::on_timeEdit_Position_timeChanged( const QTime& t )
{
    emit positionChanged( QTime().msecsTo( t ) );
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
