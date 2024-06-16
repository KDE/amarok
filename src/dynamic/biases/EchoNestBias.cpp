/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2010, 2011, 2013 Ralf Engels <ralf-engels@gmx.de>                      *
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

#define DEBUG_PREFIX "EchoNestBias"

#include "EchoNestBias.h"

#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KIO/Job>
#include <KLocalizedString>

#include <QDomDocument>
#include <QDomNode>
#include <QFile>
#include <QLabel>
#include <QPixmap>
#include <QRadioButton>
#include <QStandardPaths>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

QString
Dynamic::EchoNestBiasFactory::i18nName() const
{ return i18nc("Name of the \"EchoNest\" bias", "EchoNest similar artist"); }

QString
Dynamic::EchoNestBiasFactory::name() const
{ return Dynamic::EchoNestBias::sName(); }

QString
Dynamic::EchoNestBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"EchoNest\" bias",
                   "The \"EchoNest\" bias looks up tracks on echo nest and only adds similar tracks."); }

Dynamic::BiasPtr
Dynamic::EchoNestBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::EchoNestBias() ); }


// ----- EchoNestBias --------

Dynamic::EchoNestBias::EchoNestBias()
    : SimpleMatchBias()
    , m_artistSuggestedQuery( nullptr )
    , m_match( PreviousTrack )
{
    loadDataFromFile();
}

Dynamic::EchoNestBias::~EchoNestBias()
{
    // TODO: kill all running queries
}

void
Dynamic::EchoNestBias::fromXml( QXmlStreamReader *reader )
{
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

void
Dynamic::EchoNestBias::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( QStringLiteral("match"), nameForMatch( m_match ) );
}

QString
Dynamic::EchoNestBias::sName()
{
    return QStringLiteral( "echoNestBias" );
}

QString
Dynamic::EchoNestBias::name() const
{
    return Dynamic::EchoNestBias::sName();
}

QString
Dynamic::EchoNestBias::toString() const
{
    switch( m_match )
    {
    case PreviousTrack:
        return i18nc("EchoNest bias representation",
                     "Similar to the previous artist (as reported by EchoNest)");
    case Playlist:
        return i18nc("EchoNest bias representation",
                     "Similar to any artist in the current playlist (as reported by EchoNest)");
    }
    return QString();
}

QWidget*
Dynamic::EchoNestBias::widget( QWidget* parent )
{
    QWidget *widget = new QWidget( parent );
    QVBoxLayout *layout = new QVBoxLayout( widget );

    QLabel *imageLabel = new QLabel();
    imageLabel->setPixmap( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/echonest.png") ) ) );
    QLabel *label = new QLabel( i18n( "<a href=\"http://the.echonest.com/\">the echonest</a> thinks the artist is similar to" ) );

    QRadioButton *rb1 = new QRadioButton( i18n( "the previous track's artist" ) );
    QRadioButton *rb2 = new QRadioButton( i18n( "one of the artist in the current playlist" ) );

    rb1->setChecked( m_match == PreviousTrack );
    rb2->setChecked( m_match == Playlist );

    connect( rb2, &QRadioButton::toggled,
             this, &Dynamic::EchoNestBias::setMatchTypePlaylist );

    layout->addWidget( imageLabel );
    layout->addWidget( label );
    layout->addWidget( rb1 );
    layout->addWidget( rb2 );

    return widget;
}

Dynamic::TrackSet
Dynamic::EchoNestBias::matchingTracks( const Meta::TrackList& playlist,
                                       int contextCount, int finalCount,
                                       const Dynamic::TrackCollectionPtr &universe ) const
{
    Q_UNUSED( contextCount );
    Q_UNUSED( finalCount );

    // collect the artist
    QStringList artists = currentArtists( playlist.count() - 1, playlist );
    if( artists.isEmpty() )
        return Dynamic::TrackSet( universe, true );

    {
        QMutexLocker locker( &m_mutex );
        QString key = tracksMapKey( artists );
        // debug() << "searching in cache for"<<key
            // <<"have tracks"<<m_tracksMap.contains( key )
            // <<"have artists"<<m_similarArtistMap.contains( key );
        if( m_tracksMap.contains( key ) )
            return m_tracksMap.value( key );
    }

    m_tracks = Dynamic::TrackSet( universe, false );
    m_currentArtists = artists;
    QTimer::singleShot(0,
                       const_cast<EchoNestBias*>(this),
                       &EchoNestBias::newQuery); // create the new query from my parent thread

    return Dynamic::TrackSet();
}


bool
Dynamic::EchoNestBias::trackMatches( int position,
                                     const Meta::TrackList& playlist,
                                     int contextCount ) const
{
    Q_UNUSED( contextCount );

    // collect the artist
    QStringList artists = currentArtists( position, playlist );
    if( artists.isEmpty() )
        return true;

    // the artist of this track
    if( position < 0 || position >= playlist.count() )
        return false;

    Meta::TrackPtr track = playlist[position];
    Meta::ArtistPtr artist = track->artist();
    if( !artist || artist->name().isEmpty() )
        return false;

    {
        QMutexLocker locker( &m_mutex );
        QString key = tracksMapKey( artists );
        if( m_similarArtistMap.contains( key ) )
            return m_similarArtistMap.value( key ).contains( artist->name() );
    }
    debug() << "didn't have artist suggestions saved for this artist:" << artist->name();
    return false;
}


void
Dynamic::EchoNestBias::invalidate()
{
    SimpleMatchBias::invalidate();
    m_tracksMap.clear();
}

void
Dynamic::EchoNestBias::newQuery()
{
    // - get the similar artists
    QStringList similar;
    {
        QMutexLocker locker( &m_mutex );
        QString key = tracksMapKey( m_currentArtists );
        if( m_similarArtistMap.contains( key ) )
        {
            similar = m_similarArtistMap.value( key );
            debug() << "got similar artists:" << similar.join(QStringLiteral(", "));
        }
        else
        {
            newSimilarArtistQuery();
            return; // not yet ready to do construct a query maker
        }
    }

    // ok, I need a new query maker
    m_qm.reset( CollectionManager::instance()->queryMaker() );

    // - construct the query
    m_qm->beginOr();
    for( const QString &artistName : similar )
    {
        m_qm->addFilter( Meta::valArtist, artistName, true, true );

    }
    m_qm->endAndOr();

    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );

    connect( m_qm.data(), &Collections::QueryMaker::newResultReady,
             this, &EchoNestBias::updateReady );
    connect( m_qm.data(), &Collections::QueryMaker::queryDone,
             this, &EchoNestBias::updateFinished );

    // - run the query
    m_qm->run();
}

void
Dynamic::EchoNestBias::newSimilarArtistQuery()
{
    QMultiMap< QString, QString > params;

    // -- start the query
    params.insert( QStringLiteral("results"), QStringLiteral("30") );
    params.insert( QStringLiteral("name"), m_currentArtists.join(QStringLiteral(", ")) );
    m_artistSuggestedQuery = KIO::storedGet( createUrl( QStringLiteral("artist/similar"), params ), KIO::NoReload, KIO::HideProgressInfo );
    connect( m_artistSuggestedQuery, &KJob::result,
             this, &EchoNestBias::similarArtistQueryDone );
}

void
Dynamic::EchoNestBias::similarArtistQueryDone( KJob* job ) // slot
{
    job->deleteLater();
    if( job != m_artistSuggestedQuery )
    {
        debug() << "job was deleted from under us...wtf! blame the gerbils.";
        m_tracks.reset( false );
        Q_EMIT resultReady( m_tracks );
        return;
    }

    QDomDocument doc;
    if( !doc.setContent( m_artistSuggestedQuery->data() ) )
    {
        debug() << "got invalid XML from EchoNest::get_similar!";
        m_tracks.reset( false );
        Q_EMIT resultReady( m_tracks );
        return;
    }

    // -- decode the result
    QDomNodeList artists = doc.elementsByTagName( QStringLiteral("artist") );
    if( artists.isEmpty() )
    {
        debug() << "Got no similar artists! Bailing!";
        m_tracks.reset( false );
        Q_EMIT resultReady( m_tracks );
        return;
    }

    QStringList similarArtists;
    for( int i = 0; i < artists.count(); i++ )
    {
        similarArtists.append( artists.at(i).firstChildElement( QStringLiteral("name") ).text() );
    }

    // -- commit the result
    {
        QMutexLocker locker( &m_mutex );
        QString key = tracksMapKey( m_currentArtists );
        m_similarArtistMap.insert( key, similarArtists );
        saveDataToFile();
    }

    newQuery();
}

void
Dynamic::EchoNestBias::updateFinished()
{
    // -- store away the result for future reference
    QString key = tracksMapKey( m_currentArtists );
    m_tracksMap.insert( key, m_tracks );
    debug() << "saving found similar tracks to key:" << key;

    SimpleMatchBias::updateFinished();
}

QStringList
Dynamic::EchoNestBias::currentArtists( int position, const Meta::TrackList& playlist ) const
{
    QStringList result;

    if( m_match == PreviousTrack )
    {
        if( position >= 0 && position < playlist.count() )
        {
            Meta::ArtistPtr artist = playlist[ position ]->artist();
            if( artist && !artist->name().isEmpty() )
                result.append( artist->name() );
        }
    }
    else if( m_match == Playlist )
    {
        for( int i=0; i < position && i < playlist.count(); i++ )
        {
            Meta::ArtistPtr artist = playlist[i]->artist();
            if( artist && !artist->name().isEmpty() )
                result.append( artist->name() );
        }
    }

    return result;
}


// this method shamelessly inspired by liblastfm/src/ws/ws.cpp
QUrl Dynamic::EchoNestBias::createUrl( const QString &method, QMultiMap< QString, QString > params )
{
    params.insert( QStringLiteral("api_key"), QStringLiteral("DD9P0OV9OYFH1LCAE") );
    params.insert( QStringLiteral("format"), QStringLiteral("xml") );

    QUrl url;
    QUrlQuery query;
    url.setScheme( QStringLiteral("http") );
    url.setHost( QStringLiteral("developer.echonest.com") );
    url.setPath( "/api/v4/" + method );

    // take care of the ID possibility  manually
    // Qt setQueryItems doesn't encode a bunch of stuff, so we do it manually
    QMapIterator<QString, QString> i( params );
    while ( i.hasNext() ) {
        i.next();
        QByteArray const key = QUrl::toPercentEncoding( i.key() );
        QByteArray const value = QUrl::toPercentEncoding( i.value() );
        query.addQueryItem( key, value );
    }
    url.setQuery( query );

    return url;
}

void
Dynamic::EchoNestBias::saveDataToFile() const
{
    QFile file( Amarok::saveLocation() + "dynamic_echonest_similar.xml" );
    if( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
        return;

    QXmlStreamWriter writer( &file );
    writer.setAutoFormatting( true );

    writer.writeStartDocument();
    writer.writeStartElement( QStringLiteral("echonestSimilar") );

    // -- write the similar artists
    for( const QString& key : m_similarArtistMap.keys() )
    {
        writer.writeStartElement( QStringLiteral("similarArtist") );
        writer.writeTextElement( QStringLiteral("artist"), key );
        for( const QString& name : m_similarArtistMap.value( key ) )
        {
            writer.writeTextElement( QStringLiteral("similar"), name );
        }
        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();
}

void
Dynamic::EchoNestBias::readSimilarArtists( QXmlStreamReader *reader )
{
    QString key;
    QList<QString> artists;

    while (!reader->atEnd()) {
        reader->readNext();
        QStringRef name = reader->name();

        if( reader->isStartElement() )
        {
            if( name == QLatin1String("artist") )
                key = reader->readElementText(QXmlStreamReader::SkipChildElements);
            else if( name == QLatin1String("similar") )
                artists.append( reader->readElementText(QXmlStreamReader::SkipChildElements) );
            else
                reader->skipCurrentElement();
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }

    m_similarArtistMap.insert( key, artists );
}

void
Dynamic::EchoNestBias::loadDataFromFile()
{
    m_similarArtistMap.clear();

    QFile file( Amarok::saveLocation() + "dynamic_echonest_similar.xml" );

    if( !file.exists() ||
        !file.open( QIODevice::ReadOnly ) )
        return;

    QXmlStreamReader reader( &file );

    while (!reader.atEnd()) {
        reader.readNext();

        QStringRef name = reader.name();
        if( reader.isStartElement() )
        {
            if( name == QLatin1String("lastfmSimilar") )
            {
                ; // just recurse into the element
            }
            else if( name == QLatin1String("similarArtist") )
            {
                readSimilarArtists( &reader );
            }
            else
            {
                reader.skipCurrentElement();
            }
        }
        else if( reader.isEndElement() )
        {
            break;
        }
    }
}

Dynamic::EchoNestBias::MatchType
Dynamic::EchoNestBias::match() const
{ return m_match; }

void
Dynamic::EchoNestBias::setMatch( Dynamic::EchoNestBias::MatchType value )
{
    m_match = value;
    invalidate();
    Q_EMIT changed( BiasPtr(this) );
}


void
Dynamic::EchoNestBias::setMatchTypePlaylist( bool playlist )
{
    setMatch( playlist ? Playlist : PreviousTrack );
}


QString
Dynamic::EchoNestBias::nameForMatch( Dynamic::EchoNestBias::MatchType match )
{
    switch( match )
    {
    case Dynamic::EchoNestBias::PreviousTrack: return QStringLiteral("previous");
    case Dynamic::EchoNestBias::Playlist:      return QStringLiteral("playlist");
    }
    return QString();
}

Dynamic::EchoNestBias::MatchType
Dynamic::EchoNestBias::matchForName( const QString &name )
{
    if( name == QLatin1String("previous") )      return PreviousTrack;
    else if( name == QLatin1String("playlist") ) return Playlist;
    else return PreviousTrack;
}

QString
Dynamic::EchoNestBias::tracksMapKey( const QStringList &artists )
{
    return artists.join(QStringLiteral("|"));
}

