/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "LastFmBias.h"

#include "core/collections/Collection.h"
#include "CollectionManager.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "core/meta/Meta.h"
#include "core/collections/QueryMaker.h"

#include "lastfm/Artist"
#include "lastfm/ws.h"
#include "lastfm/XmlQuery"

#include <QDomDocument>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPair>

// CUSTOM BIAS FACTORY
#include <KComboBox>

Dynamic::LastFmBiasFactory::LastFmBiasFactory()
    : CustomBiasEntryFactory()
{

}

Dynamic::LastFmBiasFactory::~LastFmBiasFactory()
{

}

QString
Dynamic::LastFmBiasFactory::name() const
{

    return i18n( "Last.Fm Similar Artists" );
}

QString
Dynamic::LastFmBiasFactory::pluginName() const
{
    return "lastfm_similarartists";
}



Dynamic::CustomBiasEntry*
Dynamic::LastFmBiasFactory::newCustomBiasEntry()
{
    debug() << "CREATING SIMILAR BIAS";
    return new LastFmBias( true );
}

Dynamic::CustomBiasEntry*
Dynamic::LastFmBiasFactory::newCustomBiasEntry( QDomElement e )
{
    DEBUG_BLOCK
    debug() << "lastfm bias created with:" << e.attribute("value");
    bool sim = e.attribute( "value" ).toInt() == 0;
    return new LastFmBias( sim );
}


/// class LastFmBias

Dynamic::LastFmBias::LastFmBias( bool similarArtists )
    : Dynamic::CustomBiasEntry()
    , Engine::EngineObserver( The::engineController() )
    , m_similarArtists( similarArtists )
    , m_artistQuery( 0 )
    , m_qm( 0 )
{
    DEBUG_BLOCK

    connect( this, SIGNAL( doneFetching() ), this, SLOT( saveDataToFile() ) );
    
    loadFromFile();
    engineNewTrackPlaying(); // kick it into gear if a track is already playnig. if not, it's harmless
}

Dynamic::LastFmBias::~LastFmBias()
{
    delete m_qm;
}


QString
Dynamic::LastFmBias::pluginName() const
{
    return "lastfm_similarartists";
}

QWidget*
Dynamic::LastFmBias::configWidget( QWidget* parent )
{
    DEBUG_BLOCK
    QFrame * frame = new QFrame( parent );
    QVBoxLayout* layout = new QVBoxLayout( frame );

    QLabel * label = new QLabel( i18n( "Adds songs related to currently playing track, recommended by Last.Fm" ), frame );
    label->setWordWrap( true );
    label->setAlignment( Qt::AlignCenter );
    QLabel* typeLabel = new QLabel( i18n( "Add tracks based on recommended:" ), frame );
    m_combo = new KComboBox( frame );
    m_combo->addItem( i18n("Artists"), 1);
    m_combo->addItem( i18n("Tracks"), 2);
    
    QHBoxLayout* comboSelect = new QHBoxLayout( frame );
    comboSelect->addWidget( typeLabel );
    comboSelect->addWidget( m_combo );
    layout->addLayout( comboSelect, Qt::AlignCenter );
    layout->addWidget( label, Qt::AlignCenter );

    if( m_similarArtists )
        m_combo->setCurrentIndex( 0 );
    else
        m_combo->setCurrentIndex( 1 );
        

    connect( m_combo, SIGNAL( currentIndexChanged(int)), this, SLOT( activated(int) ) );
    return frame;
}

void
Dynamic::LastFmBias::activated(int index)
{
    if( m_combo->itemData( index ).toInt() == 1 ) // artists
    {
        m_similarArtists = true;
    } else if( m_combo->itemData( index ).toInt() == 2 ) // tracks
    {
        m_similarArtists = false;
    }
    updateBias();
    emit biasChanged();
}

void
Dynamic::LastFmBias::engineNewTrackPlaying()
{
    updateBias();
}

void Dynamic::LastFmBias::updateBias()
{
    DEBUG_BLOCK
    Meta::TrackPtr track = The::engineController()->currentTrack();

    if( track )
    {
        if( m_similarArtists && track->artist() && !track->artist()->name().isEmpty() )
        {
            m_currentArtist = track->artist()->name(); // save for sure
            // if already saved, don't re-fetch
            if( !m_savedArtists.contains( track->artist()->name() ) )
            {
                QMap< QString, QString > params;
                params[ "method" ] = "artist.getSimilar";
                //   params[ "limit" ] = "70";
                params[ "artist" ] = m_currentArtist;

                m_artistQuery = lastfm::ws::get( params );

                connect( m_artistQuery, SIGNAL( finished() ), this, SLOT( artistQueryDone() ) );
            }
        } else if( !m_similarArtists && !track->name().isEmpty() )
        {
            m_currentArtist = track->artist()->name(); // save for sure
            m_currentTrack = track->name();
            if( !m_savedTracks.contains( m_currentTrack ) )
            {
                QMap< QString, QString > params;
                params[ "method" ] = "track.getSimilar";
                //   params[ "limit" ] = "70";
                params[ "artist" ] = m_currentArtist;
                params[ "track" ] = m_currentTrack;
//                 if( track->mb
// TODO add mbid if the track has one

                m_trackQuery = lastfm::ws::get( params );

                connect( m_trackQuery, SIGNAL( finished() ), this, SLOT( trackQueryDone() ) );
            }

        }

    }
}


void
Dynamic::LastFmBias::update()
{
    DEBUG_BLOCK
    if( !m_needsUpdating )
        return;

    m_qm->run();
}

void
Dynamic::LastFmBias::artistQueryDone() // slot
{
    DEBUG_BLOCK

    if( !m_artistQuery )
    {
        debug() << "job was deleted from under us...wtf! blame the gerbils.";
        return;
    }

    QMutexLocker locker( &m_mutex );
    
    QMap< int, QString > similar;
    QByteArray data = m_artistQuery->readAll();
//     debug() << "artistQuery has data:" << data;
    QDomDocument d;
    if( !d.setContent( data ) )
    {
        debug() << "Got invalid XML data from last.fm!";
        return;
    }
    QDomNodeList nodes = d.elementsByTagName( "artist" );
    for( int i =0; i < nodes.size(); ++i )
    {
        QDomElement n = nodes.at( i ).toElement();
        similar.insert( n.firstChildElement( "match" ).text().toFloat() * 100,
                        n.firstChildElement( "name" ).text() );
    }


    // ok we have the list, now figure out what we've got in the collection

    m_collection = CollectionManager::instance()->primaryCollection();
    if( !m_collection )
        return;
    m_qm = m_collection->queryMaker();
    
    if( !m_qm ) // maybe this is during startup and we don't have a QM for some reason yet
        return;

  //  debug() << "got similar artists:" << similar.values();

    m_qm->beginOr();
    foreach( const QString &artist, similar.values() )
    {
        m_qm->addFilter( Meta::valArtist, artist, true, true );
    }
    m_qm->endAndOr();
    
    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );
    m_qm->orderByRandom(); // as to not affect the amortized time

    connect( m_qm, SIGNAL( newResultReady( QString, QStringList ) ),
            SLOT( artistUpdateReady( QString, QStringList ) ), Qt::DirectConnection );
    connect( m_qm, SIGNAL( queryDone() ), SLOT( updateFinished() ), Qt::DirectConnection );

    collectionUpdated(); // force an update
    emit doneFetching();
    m_qm->run();
    m_artistQuery->deleteLater();
}

void Dynamic::LastFmBias::trackQueryDone()
{
    DEBUG_BLOCK

    if( !m_trackQuery )
    {
        debug() << "track job was deleted from under us...wtf! blame the gerbils.";
        return;
    }

    QMutexLocker locker( &m_mutex );

    // double match value, qpair title - artist
    QMap< int, QPair<QString,QString> > similar;
    QByteArray data = m_trackQuery->readAll();
//     debug() << "trackQuery has data:" << data;
    QDomDocument d;
    if( !d.setContent( data ) )
    {
        debug() << "Got invalid XML data from last.fm!";
        return;
    }
    QDomNodeList nodes = d.elementsByTagName( "track" );
    for( int i =0; i < nodes.size(); ++i )
    {
        QDomElement n = nodes.at( i ).toElement();
//         debug() << "parsing track:" << n.text();
//         debug() << "adding data:" << n.firstChildElement( "match" ).text().toDouble() << QPair< QString, QString >( n.firstChildElement( "name" ).text(), n.firstChildElement( "artist" ).firstChildElement( "name" ).text() );
        similar.insert( n.firstChildElement( "match" ).text().toDouble(),
                                             QPair< QString, QString >( n.firstChildElement( "name" ).text(), n.firstChildElement( "artist" ).firstChildElement( "name" ).text() ) );
    }


//     debug() << "found similar tracks:" << similar;
    // ok we have the list, now figure out what we've got in the collection

    m_collection = CollectionManager::instance()->primaryCollection();
    if( !m_collection )
        return;
    m_qm = m_collection->queryMaker();

    if( !m_qm ) // maybe this is during startup and we don't have a QM for some reason yet
        return;

  //  debug() << "got similar artists:" << similar.values();

    m_qm->beginOr();
    for( int i = 0; i < similar.values().size(); i++ )
    {
        QPair< QString, QString > t = similar.values().at( i );
        m_qm->beginAnd();
        m_qm->addFilter( Meta::valTitle, t.first, true, true );
        m_qm->addFilter( Meta::valArtist, t.second, true, true );
        m_qm->endAndOr();
    }
    m_qm->endAndOr();

    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );
    m_qm->orderByRandom(); // as to not affect the amortized time

    connect( m_qm, SIGNAL( newResultReady( QString, QStringList ) ),
            SLOT( trackUpdateReady( QString, QStringList ) ), Qt::DirectConnection );
    connect( m_qm, SIGNAL( queryDone() ), SLOT( updateFinished() ), Qt::DirectConnection );

    collectionUpdated(); // force an update
    m_qm->run();
    emit doneFetching();
    m_trackQuery->deleteLater();
}


void
Dynamic::LastFmBias::updateFinished()
{
    DEBUG_BLOCK

    m_needsUpdating = false;
}

void
Dynamic::LastFmBias::collectionUpdated()
{
    m_needsUpdating = true;
}

void
Dynamic::LastFmBias::artistUpdateReady(QString collectionId, QStringList uids )
{
    addData( collectionId, uids, m_currentArtist, m_savedArtists );
}

void
Dynamic::LastFmBias::trackUpdateReady(QString collectionId, QStringList uids )
{
    addData( collectionId, uids, m_currentTrack,  m_savedTracks );
}


void
Dynamic::LastFmBias::addData( QString collectionId, QStringList uids, const QString& index, QMap< QString, QSet< QByteArray > >& data )
{
    DEBUG_BLOCK

    Q_UNUSED(collectionId)
    QMutexLocker locker( &m_mutex );

    int protocolLength =
        ( QString( m_collection->uidUrlProtocol() ) + "://" ).length();

  //  debug() << "setting cache of related artist UIDs for artist:" << m_currentArtist << "to:" << uids;
    data[ index ].clear();
    data[ index ].reserve( uids.size() );
    QByteArray uid;
    foreach( const QString &uidString, uids )
    {
        uid = uidString.mid( protocolLength ).toAscii();
        data[ index ].insert( uid );
    }
}


bool
Dynamic::LastFmBias::trackSatisfies( const Meta::TrackPtr track )
{
    //DEBUG_BLOCK
    QMutexLocker locker( &m_mutex );

    //debug() << "checking if " << track->name() << "by" << track->artist()->name() << "is in suggested:" << m_savedArtists[ m_currentArtist ] << "of" << m_currentArtist;
    const QString uidString = track->uidUrl().mid( track->uidUrl().lastIndexOf( '/' ) );
    const QByteArray uid = uidString.toAscii();
    
    if( m_similarArtists && m_savedArtists.keys().contains( m_currentArtist ) )
    {
        debug() << "saying has similar artist:" <<  m_savedArtists[ m_currentArtist ].contains( uid ) << "for" << track->artist()->name();
        return m_savedArtists[ m_currentArtist ].contains( uid );
    } else if( !m_similarArtists && m_savedTracks.contains( m_currentTrack ) )
    {
        debug() << "saying has similar track:" <<  m_savedTracks[ m_currentTrack ].contains( uid ) << "for" << track->name();
        return m_savedTracks[ m_currentTrack ].contains( uid );
    } else
    {
        debug() << "DIDN'T HAVE ARTIST OR TRACK SUGGESTIONS SAVED FOR THIS ARTIST:" << m_currentArtist << " AND THIS TRACK:" << m_currentTrack << "getting similar artists?" << m_similarArtists;
    }
    
    return false;
    
}

double
Dynamic::LastFmBias::numTracksThatSatisfy( const Meta::TrackList& tracks )
{
    DEBUG_BLOCK
    QMutexLocker locker( &m_mutex );

    int satisfy = 0;
    if( m_similarArtists && m_savedArtists.keys().contains( m_currentArtist ) )
    {
        foreach( const Meta::TrackPtr track, tracks )
        {
             const QString uidString = track->uidUrl().mid( track->uidUrl().lastIndexOf( '/' ) );
             const QByteArray uid = uidString.toAscii();
    
            if( m_savedArtists[ m_currentArtist ].contains( uid ) )
                satisfy++;

        }
    } else if( !m_similarArtists && m_savedTracks.keys().contains( m_currentTrack ) )
    {
        foreach( const Meta::TrackPtr track, tracks )
        {
             const QString uidString = track->uidUrl().mid( track->uidUrl().lastIndexOf( '/' ) );
             const QByteArray uid = uidString.toAscii();

            if( m_savedTracks[ m_currentTrack ].contains( uid ) )
                satisfy++;
        }
    } else
    {
        debug() << "AGAIN, didn't have artist suggestions saved for these multiple artists";
    }
    
    return satisfy;
}

QDomElement
Dynamic::LastFmBias::xml( QDomDocument doc ) const
{
    DEBUG_BLOCK
    QDomElement from = doc.createElement( "similarArtists" );
    from.setAttribute( "value", m_similarArtists ? "0" : "1" );

    debug() << "returning lastfmbias with xml:" << from.text();

    return from;
}

void
Dynamic::LastFmBias::saveDataToFile()
{
    QFile file( Amarok::saveLocation() + "dynamic_lastfm_similarartists.xml" );
    file.open( QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text );
    QTextStream out( &file );
    foreach( const QString& key, m_savedArtists.keys() )
    {
        out << key << "#";
        foreach( const QByteArray& uid, m_savedArtists[ key ] )
        {
            out << uid << "^";
        }
        out << endl;
    }
    file.close();

    QFile file2( Amarok::saveLocation() + "dynamic_lastfm_similartracks.xml" );
    file2.open( QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text );
    QTextStream out2( &file2 );
    foreach( const QString& key, m_savedTracks.keys() )
    {
        out2 << key << "#";
        foreach( const QByteArray& uid, m_savedTracks[ key ] )
        {
            out2 << uid << "^";
        }
        out2 << endl;
    }
    file2.close();

}

void
Dynamic::LastFmBias::loadFromFile()
{
    QFile file( Amarok::saveLocation() + "dynamic_lastfm_similarartists.xml" );
    file.open( QIODevice::ReadOnly | QIODevice::Text );
    QTextStream in( &file );
    while( !in.atEnd() )
    {
        QString line = in.readLine();
        QString key = line.split( "#" )[ 0 ];
        QStringList uids = line.split( "#" )[ 1 ].split( "^" );
        QSet<QByteArray> u;
        foreach( const QString& uid, uids )
        {
            if( !uid.isEmpty() )
                u.insert( uid.toUtf8() );
        }
        m_savedArtists.insert( key, u );
    }
    file.close();

    QFile file2( Amarok::saveLocation() + "dynamic_lastfm_similartracks.xml" );
    file2.open( QIODevice::ReadOnly | QIODevice::Text );
    QTextStream in2( &file2 );
    while( !in2.atEnd() )
    {
        QString line = in2.readLine();
        QString key = line.split( "#" )[ 0 ];
        QStringList uids = line.split( "#" )[ 1 ].split( "^" );
        QSet<QByteArray> u;
        foreach( const QString& uid, uids )
        {
            if( !uid.isEmpty() )
                u.insert( uid.toUtf8() );
        }
        m_savedTracks.insert( key, u );
    }
    file2.close();
}

bool
Dynamic::LastFmBias::hasCollectionFilterCapability()
{
    return true;
}

Dynamic::CollectionFilterCapability*
Dynamic::LastFmBias::collectionFilterCapability( double weight )
{
    DEBUG_BLOCK
    debug() << "returning new cfb with weight:" << weight;
    return new Dynamic::LastFmBiasCollectionFilterCapability( this, weight );
}

const QSet< QByteArray >&
Dynamic::LastFmBiasCollectionFilterCapability::propertySet()
{
    if( m_bias->m_similarArtists )
    {
        debug() << "returning matching set for artist: " << m_bias->m_currentArtist << "of size:" << m_bias->m_savedArtists[ m_bias->m_currentArtist ].size();
        return m_bias->m_savedArtists[ m_bias->m_currentArtist ];
    } else
    {
        debug() << "returning matching set for track: " << m_bias->m_currentTrack << "of size:" << m_bias->m_savedTracks[ m_bias->m_currentTrack ].size();
        return m_bias->m_savedTracks[ m_bias->m_currentTrack ];
    }
}

double Dynamic::LastFmBiasCollectionFilterCapability::weight() const
{
    return m_weight;
}

#include "LastFmBias.moc"
