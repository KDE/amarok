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

#define DEBUG_PREFIX "Constraint::Checkpoint"

#include "Checkpoint.h"

#include "playlistgenerator/Constraint.h"
#include "playlistgenerator/ConstraintFactory.h"

#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <QUrl>

#include <algorithm>
#include <climits>
#include <cmath>

Constraint*
ConstraintTypes::Checkpoint::createFromXml( QDomElement& xmlelem, ConstraintNode* p )
{
    if ( p ) {
        return new Checkpoint( xmlelem, p );
    } else {
        return nullptr;
    }
}

Constraint*
ConstraintTypes::Checkpoint::createNew( ConstraintNode* p )
{
    if ( p ) {
        return new Checkpoint( p );
    } else {
        return nullptr;
    }
}

ConstraintFactoryEntry*
ConstraintTypes::Checkpoint::registerMe()
{
    return new ConstraintFactoryEntry( QStringLiteral("Checkpoint"),
                                       i18n("Checkpoint"),
                                       i18n("Fixes a track, album, or artist to a certain position in the playlist"),
                                       &Checkpoint::createFromXml, &Checkpoint::createNew );
}

ConstraintTypes::Checkpoint::Checkpoint( QDomElement& xmlelem, ConstraintNode* p )
        : Constraint( p )
        , m_position( 0 )
        , m_strictness( 1.0 )
        , m_checkpointType( CheckpointTrack )
        , m_matcher( nullptr )
{
    QDomAttr a;

    a = xmlelem.attributeNode( QStringLiteral("position") );
    if ( !a.isNull() )
        m_position = a.value().toInt();


    a = xmlelem.attributeNode( QStringLiteral("checkpointtype") );
    if ( !a.isNull() )
        m_checkpointType = static_cast<CheckpointType>( a.value().toInt() );

    a = xmlelem.attributeNode( QStringLiteral("trackurl") );
    if ( !a.isNull() ) {
        Meta::TrackPtr trk = CollectionManager::instance()->trackForUrl( QUrl( a.value() ) );
        if ( trk ) {
            if ( m_checkpointType == CheckpointAlbum ) {
                m_checkpointObject = Meta::DataPtr::dynamicCast( trk->album() );
            } else if ( m_checkpointType == CheckpointArtist ) {
                m_checkpointObject = Meta::DataPtr::dynamicCast( trk->artist() );
            } else {
                m_checkpointObject = Meta::DataPtr::dynamicCast( trk );
            }
        }
    }

    setCheckpoint( m_checkpointObject );

    a = xmlelem.attributeNode( QStringLiteral("strictness") );
    if ( !a.isNull() )
        m_strictness = a.value().toDouble();
    debug() << getName();
}

ConstraintTypes::Checkpoint::Checkpoint( ConstraintNode* p )
        : Constraint( p )
        , m_position( 0 )
        , m_strictness( 1.0 )
        , m_checkpointType( CheckpointTrack )
        , m_matcher( nullptr )
{
}

ConstraintTypes::Checkpoint::~Checkpoint()
{
    delete m_matcher;
}


QWidget*
ConstraintTypes::Checkpoint::editWidget() const
{
    CheckpointEditWidget* e = new CheckpointEditWidget( m_position, static_cast<int>( 10*m_strictness ), m_checkpointObject );
    connect( e, &CheckpointEditWidget::positionChanged, this, &Checkpoint::setPosition );
    connect( e, &CheckpointEditWidget::strictnessChanged, this, &Checkpoint::setStrictness );
    connect( e, &CheckpointEditWidget::checkpointChanged, this, &Checkpoint::setCheckpoint );
    return e;
}

void
ConstraintTypes::Checkpoint::toXml( QDomDocument& doc, QDomElement& elem ) const
{
    if( !m_checkpointObject )
        return;

    QDomElement c = doc.createElement( QStringLiteral("constraint") );
    QDomText t = doc.createTextNode( getName() );
    c.appendChild( t );
    c.setAttribute( QStringLiteral("type"), QStringLiteral("Checkpoint") );
    c.setAttribute( QStringLiteral("position"), m_position );
    c.setAttribute( QStringLiteral("checkpointtype"), m_checkpointType );
    Meta::TrackPtr r;
    Meta::ArtistPtr a;
    Meta::AlbumPtr l;
    switch ( m_checkpointType ) {
        case CheckpointTrack:
            r = Meta::TrackPtr::dynamicCast( m_checkpointObject );
            c.setAttribute( QStringLiteral("trackurl"), r->uidUrl() );
            break;
        case CheckpointAlbum:
            l = Meta::AlbumPtr::dynamicCast( m_checkpointObject );
            if ( l->tracks().length() > 0 ) {
                r = l->tracks().first();
                c.setAttribute( QStringLiteral("trackurl"), r->uidUrl() );
            }
            break;
        case CheckpointArtist:
            a = Meta::ArtistPtr::dynamicCast( m_checkpointObject );
            if ( a->tracks().length() > 0 ) {
                r = a->tracks().first();
                c.setAttribute( QStringLiteral("trackurl"), r->uidUrl() );
            }
            break;
    }
    c.setAttribute( QStringLiteral("strictness"), QString::number( m_strictness ) );
    elem.appendChild( c );
}

QString
ConstraintTypes::Checkpoint::getName() const
{
    KLocalizedString name( ki18n("Checkpoint: %1") );
    Meta::TrackPtr t;
    Meta::AlbumPtr l;
    Meta::ArtistPtr r;
    switch ( m_checkpointType ) {
        case CheckpointTrack:
            t = Meta::TrackPtr::dynamicCast( m_checkpointObject );
            if ( t == Meta::TrackPtr() ) {
                name = name.subs( i18n("unassigned") );
            } else {
                name = name.subs( i18n("\"%1\" (track) by %2", t->prettyName(), t->artist()->prettyName() ) );
            }
            break;
        case CheckpointAlbum:
            l = Meta::AlbumPtr::dynamicCast( m_checkpointObject );
            if ( l == Meta::AlbumPtr() ) {
                name = name.subs( i18n("unassigned") );
            } else {
                if ( l->hasAlbumArtist() ) {
                    name = name.subs( i18n("\"%1\" (album) by %2", l->prettyName(), l->albumArtist()->prettyName() ) );
                } else {
                    name = name.subs( i18n("\"%1\" (album)", l->prettyName() ) );
                }
            }
            break;
        case CheckpointArtist:
            r = Meta::ArtistPtr::dynamicCast( m_checkpointObject );
            if ( r == Meta::ArtistPtr() ) {
                name = name.subs( i18n("unassigned") );
            } else {
                name = name.subs( i18n("\"%1\" (artist)", r->prettyName() ) );
            }
            break;
    }

    return name.toString();
}

double
ConstraintTypes::Checkpoint::satisfaction( const Meta::TrackList& tl ) const
{
    if( !m_matcher ) // Incomplete condition, skip and avoid a crash
        return 0.0;
    // What are the ending time boundaries of each track in this playlist?
    qint64 start = 0;
    QList< qint64 > boundaries;
    foreach ( const Meta::TrackPtr t, tl ) {
        boundaries << ( start += t->length() );
    }

    // Is the playlist long enough to contain the checkpoint?
    if ( boundaries.last() < m_position ) {
        return 0.0; // no, it does not
    }
    
    // Where are the appropriate tracks in this playlist?
    QList<int> locs = m_matcher->find( tl );
    if ( locs.size() < 1 ) {
        return 0.0; // none found
    } else {
        qint64 best = boundaries.last(); // the length of the playlist is the upper bound for distances
        foreach ( int i, locs ) {
            qint64 start = ( i>0 )?boundaries.at( i-1 ):0;
            qint64 end = boundaries.at( i );
            if ( (start <= m_position) && ( end >= m_position ) ) {
                // checkpoint position has a match flanking it
                return 1.0;
            } else if ( end < m_position ) {
                // appropriate track is before the checkpoint
                best = (best < (m_position - end))?best:(m_position - end);
            } else if ( start > m_position ) {
                // appropriate track is after the checkpoint
                best = (best < (start - m_position))?best:(start - m_position);
            } else {
                warning() << "WTF JUST HAPPENED?" << m_position << "(" << start << "," << end << ")";
            }
        }
        return penalty( best );
    }
    
    warning() << "Improper exit condition";
    return 0.0;
}

double
ConstraintTypes::Checkpoint::penalty( const qint64 d ) const
{
    return exp( d / ( -( 120000.0 * ( 1.0 + ( 8.0 * m_strictness ) ) ) ) );
}

quint32
ConstraintTypes::Checkpoint::suggestPlaylistSize() const
{
    return static_cast<quint32>( m_position / 300000 ) + 1;
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

    delete m_matcher;
    if ( Meta::TrackPtr track = Meta::TrackPtr::dynamicCast( data ) ) {
        m_checkpointType = CheckpointTrack;
        m_matcher = new TrackMatcher( track );
        debug() << "setting checkpoint track:" << track->prettyName();
    } else if ( Meta::AlbumPtr album = Meta::AlbumPtr::dynamicCast( data ) ) {
        m_checkpointType = CheckpointAlbum;
        m_matcher = new AlbumMatcher( album );
        debug() << "setting checkpoint album:" << album->prettyName();
    } else if ( Meta::ArtistPtr artist = Meta::ArtistPtr::dynamicCast( data ) ) {
        debug() << "setting checkpoint artist:" << artist->prettyName();
        m_matcher = new ArtistMatcher( artist );
        m_checkpointType = CheckpointArtist;
    }

    m_checkpointObject = data;
    Q_EMIT dataChanged();
}

/******************************
 * Track Matcher              *
 ******************************/
ConstraintTypes::Checkpoint::TrackMatcher::TrackMatcher( const Meta::TrackPtr& t )
    : m_trackToMatch( t )
{
}

ConstraintTypes::Checkpoint::TrackMatcher::~TrackMatcher()
{
}

QList<int>
ConstraintTypes::Checkpoint::TrackMatcher::find( const Meta::TrackList& tl ) const
{
    QList<int> positions;
    for ( int i = 0; i < tl.length(); i++ ) {
        if ( tl.at( i ) == m_trackToMatch ) {
            positions << i;
        }
    }

    return positions;
}

bool
ConstraintTypes::Checkpoint::TrackMatcher::match( const Meta::TrackPtr& t ) const
{
    return ( t == m_trackToMatch );
}

/******************************
 * Artist Matcher             *
 ******************************/
ConstraintTypes::Checkpoint::ArtistMatcher::ArtistMatcher( const Meta::ArtistPtr& a )
    : m_artistToMatch( a )
{
}

ConstraintTypes::Checkpoint::ArtistMatcher::~ArtistMatcher()
{
}

QList<int>
ConstraintTypes::Checkpoint::ArtistMatcher::find( const Meta::TrackList& tl ) const
{
    QList<int> positions;
    for ( int i = 0; i < tl.length(); i++ ) {
        if ( tl.at( i )->artist() == m_artistToMatch ) {
            positions << i;
        }
    }

    return positions;
}

bool
ConstraintTypes::Checkpoint::ArtistMatcher::match( const Meta::TrackPtr& t ) const
{
    return ( t->artist() == m_artistToMatch );
}

/******************************
 * Album Matcher              *
 ******************************/
ConstraintTypes::Checkpoint::AlbumMatcher::AlbumMatcher( const Meta::AlbumPtr& l )
    : m_albumToMatch( l )
{
}

ConstraintTypes::Checkpoint::AlbumMatcher::~AlbumMatcher()
{
}

QList<int>
ConstraintTypes::Checkpoint::AlbumMatcher::find( const Meta::TrackList& tl ) const
{
    QList<int> positions;
    for ( int i = 0; i < tl.length(); i++ ) {
        if ( tl.at( i )->album() == m_albumToMatch ) {
            positions << i;
        }
    }

    return positions;
}

bool
ConstraintTypes::Checkpoint::AlbumMatcher::match( const Meta::TrackPtr& t ) const
{
    return ( t->album() == m_albumToMatch );
}

/******************************
 * Edit Widget                *
 ******************************/

ConstraintTypes::CheckpointEditWidget::CheckpointEditWidget( const qint64 length,
                                                             const int strictness,
                                                             const Meta::DataPtr& data ) : QWidget( nullptr )
{
    ui.setupUi( this );

    ui.timeEdit_Position->setTime( QTime(0, 0, 0).addMSecs( length ) );
    ui.slider_Strictness->setValue( strictness );
    ui.trackSelector->setData( data );
}

void
ConstraintTypes::CheckpointEditWidget::on_timeEdit_Position_timeChanged( const QTime& t )
{
    Q_EMIT positionChanged( QTime(0, 0, 0).msecsTo( t ) );
    Q_EMIT updated();
}

void
ConstraintTypes::CheckpointEditWidget::on_slider_Strictness_valueChanged( const int v )
{
    Q_EMIT strictnessChanged( v );
    Q_EMIT updated();
}

void
ConstraintTypes::CheckpointEditWidget::on_trackSelector_selectionChanged( const Meta::DataPtr& data )
{
    Q_EMIT checkpointChanged( data );
    Q_EMIT updated();
}
