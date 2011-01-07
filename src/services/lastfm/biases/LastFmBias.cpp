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
#include "core-impl/collections/support/CollectionManager.h"
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

QString
Dynamic::LastFmBiasFactory::i18nName() const
{ return i18nc("Name of the \"LastFm\" bias", "LastFm similar artist"); }

QString
Dynamic::LastFmBiasFactory::name() const
{ return Dynamic::LastFmBias::sName(); }

QString
Dynamic::LastFmBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"LastFm\" bias",
                   "The \"LastFm\" bias looks up tracks on echo nest and only adds similar tracks."); }

Dynamic::BiasPtr
Dynamic::LastFmBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::LastFmBias() ); }

Dynamic::BiasPtr
Dynamic::LastFmBiasFactory:: createBias( QXmlStreamReader *reader )
{ return Dynamic::BiasPtr( new Dynamic::LastFmBias( reader ) ); }



// ----- LastFmBias --------

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

Dynamic::LastFmBias::LastFmBias()
    : SimpleMatchBias()
    , m_artistQuery( 0 )
    , m_trackQuery( 0 )
    , m_match( SimilarArtist )
{
    loadFromFile();
}

Dynamic::LastFmBias::LastFmBias( QXmlStreamReader *reader )
    : SimpleMatchBias()
    , m_artistQuery( 0 )
    , m_trackQuery( 0 )
    , m_match( SimilarArtist )
{
    loadFromFile();
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "match" )
                m_match = matchForName( reader->readElementText(QXmlStreamReader::SkipChildElements) );
            else
            {
                debug()<<"Unexpected xml start element"<<reader->name()<<"in input";
                reader->skipCurrentElement();
            }
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }
}

Dynamic::LastFmBias::~LastFmBias()
{
    // TODO: kill all running queries
}


void
Dynamic::LastFmBias::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( "match", nameForMatch( m_match ) );
}

QString
Dynamic::LastFmBias::sName() const
{
    return "lastfm_similarartists";
}

QString
Dynamic::LastFmBias::name() const
{
    return Dynamic::LastFmBias::sName();
}

QWidget*
Dynamic::LastFmBias::widget( QWidget* parent )
{
    PlaylistBrowserNS::BiasWidget *bw = new PlaylistBrowserNS::BiasWidget( BiasPtr(this), parent );
    QComboBox *combo = new QComboBox();
    combo->addItem( i18n( "Similar to previous artist" ),
                    nameForMatch( SimilarArtist ) );
    combo->addItem( i18n( "Similar to previous track" ),
                    nameForMatch( SimilarTrack ) );
    switch( match )
    {
    case SimilarArtist: combo->setCurrentIndex( 0 ); break;
    case SimilarTrack:  combo->setCurrentIndex( 1 ); break;
    }
    connect( combo, SIGNAL( currentIndexChanged(int) ),
             this, SLOT( selectionChanged( int ) ) );


    bw->formLayout()->addRow( i18n( "Match:" ), combo );

    return bw;
}

Dynamic::TrackSet
Dynamic::LastFmBias::matchingTracks( int position,
                                       const Meta::TrackList& playlist,
                                       int contextCount,
                                       Dynamic::TrackCollectionPtr universe ) const
{
    Q_UNUSED( contextCount );

    if( position <= 0 || position > playlist.count())
        return Dynamic::TrackSet( universe, true );

    // determine the last track and artist
    Meta::TrackPtr lastTrack = playlist[position-1];
    Meta::ArtistPrt lastArtist = lastTrack->artist();

    m_currentTrack = lastTrack->name();
    m_currentArtist = lastArtist ? lastArtist->name() : QString();


    {
        QMutexLocker locker( &m_mutex );

        if( artists.isEmpty() )
            return Dynamic::TrackSet( universe, true );
        if( m_tracksValid && m_tracksMap.contains( key ) )
            return m_tracksMap.value( key );
    }

    m_tracks = Dynamic::TrackSet( universe, false );
    m_currentArtists = artists;
    QTimer::singleShot(0,
                       const_cast<LastFmBias*>(this),
                       SLOT(newQuery())); // create the new query from my parent thread

    return Dynamic::TrackSet();
}


bool
Dynamic::LastFmBias::trackMatches( int position,
                                     const Meta::TrackList& playlist,
                                     int contextCount ) const
{
    Q_UNUSED( contextCount );

    // collect the artist
    QStringList artists = currentArtists( position, playlist );
    if( artists.isEmpty() )
        return false;

    // the artist of this track
    if( position < 0 || position > playlist.count() )
        return false;

    Meta::TrackPtr track = playlist[position];
    Meta::ArtistPtr artist = track->artist();
    if( !artist || artist->name().isEmpty() )
        return false;

    {
        QMutexLocker locker( &m_mutex );
        QString key = artists.join("|");
        if( m_similarArtistMap.contains( key ) )
            return m_similarArtistMap.value( key ).contains( artist->name() );
        else
            warning() << "didn't have artist suggestions saved for this artist:" << artist->name();
    }
    return false;
}


void
Dynamic::LastFmBias::invalidate()
{
    SimpleMatchBias::invalidate();
    m_tracksMap.clear();
}

void
Dynamic::LastFmBias::newQuery()
{
    DEBUG_BLOCK;

    // ok, I need a new query maker
    m_qm.reset( CollectionManager::instance()->queryMaker() );

    // - get the similar artists
    QStringList similar;
    {
        QMutexLocker locker( &m_mutex );
        QString key = m_currentArtists.join("|");
        if( m_similarArtistMap.contains( key ) )
        {
            similar = m_similarArtistMap.value( key );
            debug() << "got similar artists:" << similar.join(", ");
        }
        else
        {
            newSimilarArtistQuery();
            return; // not yet ready to do construct a query maker
        }
    }

    // - construct the query
    m_qm->beginOr();
    foreach( const QString &artistName, similar )
    {
        m_qm->addFilter( Meta::valArtist, artistName, true, true );

    }
    m_qm->endAndOr();

    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );

    connect( m_qm.data(), SIGNAL(newResultReady( QString, QStringList )),
             this, SLOT(updateReady( QString, QStringList )) );
    connect( m_qm.data(), SIGNAL(queryDone()),
             this, SLOT(updateFinished()) );

    // - run the query
    m_qm.data()->run();
}


void Dynamic::LastFmBias::newSimilarQuery()
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
Dynamic::LastFmBias::similarArtistQueryDone() // slot
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

void Dynamic::LastFmBias::similarTrackQueryDone()
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
    // note: how is that an xml file when it contains lines split by hash?
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

#include "LastFmBias.moc"
