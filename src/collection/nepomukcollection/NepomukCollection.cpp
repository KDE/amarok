/* 
   Copyright (C) 2008 Daniel Winter <dw@danielwinter.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define DEBUG_PREFIX "NepomukCollection"

#include "NepomukCollection.h"
#include "NepomukQueryMaker.h"
#include "NepomukRegistry.h"

#include "Debug.h"
#include "proxy/MetaProxy.h"
#include "QueryMaker.h"

#include <QHash>
#include <QString>
#include <QTime>
#include <QUuid>

#include <klocale.h>
#include <KUrl>
#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Soprano/LiteralValue>
#include <Soprano/Model>
#include <Soprano/PluginManager>
#include <Soprano/QueryResultIterator>
#include <Soprano/Statement>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/RDF>
#include <Soprano/Vocabulary/Xesam>


AMAROK_EXPORT_PLUGIN( NepomukCollectionFactory )

// CollectionFactory

void
NepomukCollectionFactory::init()
{
    if ( Nepomuk::ResourceManager::instance()->init() == 0 )
    {
        Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel();

        // find out if Nepomuk is fast enough
        // (if sesame2 is used or not, it makes no sense to use it with redland
        // doesn't work and is terrible slow, slows down amarok start when 
        // songs in playlist)

        // FIXME; Find a better wy to do this
        QTime t;
        t.start();
        Nepomuk::Resource::Resource( "file://home/" ).exists();
        int elapsed = t.elapsed();
        debug() << "Nepomuk Resource.exists() took " << elapsed <<  " ms" << endl;
          
        Collection* collection;
        if ( elapsed < 50 )
        {
            collection = new NepomukCollection( model, true );
            debug() << "fast  enough full nepomuk collection enabled" << endl;
        }
        else
        {
            collection = new NepomukCollection( model, false );
            debug() << "too slow, trackForUrl() disabled" << endl;
        }
        if ( !static_cast<NepomukCollection*>(collection)->isEmpty() )
            emit newCollection( collection );
        else
            delete collection;
    }
    else
    {
        warning() << "Nepomuk is not running, can not init Nepomuk Collection" << endl;
    }
}

// NepomukCollection

NepomukCollection::NepomukCollection( Soprano::Model* model, bool isFast )
    :   Collection() 
    ,   m_model( model )
    ,   m_isFast( isFast )
{
    initHashMaps();
    m_registry = new NepomukRegistry( this, model );
}

NepomukCollection::~NepomukCollection()
{
    delete m_model;
    delete m_registry;
}

QueryMaker*
NepomukCollection::queryMaker()
{
	return new NepomukQueryMaker(this, m_model);
}

QString
NepomukCollection::collectionId() const
{
	return "nepomukCollection";
}

QString
NepomukCollection::prettyName() const
{
    if ( m_isFast )
        return i18n( "Nepomuk Collection" );
    else
        return i18n( "Nepomuk Collection (slow, Redland)" );
}

bool
NepomukCollection::possiblyContainsTrack( const KUrl &url ) const
{
    QString proto = url.protocol();
    if ( proto == "file" || proto == "amarokcollnepomukuid" )
        return true;
    else
        return false;
}

Meta::TrackPtr
NepomukCollection::trackForUrl( const KUrl &url )
{
    DEBUG_BLOCK
    // it is too slow with redland, makes start of amarok slow
    // so just return 
    if ( !m_isFast )
        return Meta::TrackPtr();

    QString proto = url.protocol();
    
    if ( proto == "file" && !Nepomuk::Resource::Resource( url ).exists() )
        return Meta::TrackPtr();

    NepomukQueryMaker *qm = new NepomukQueryMaker( this, m_model );
    qm->setQueryType( QueryMaker::Track );
    if ( proto == "file" )
    {
        qm->addMatch( url );
    }
    else
    {
        qm->addMatchId ( url.host() );
    }
    
    qm->blocking( true );
    qm->run();
    Meta::TrackList tracks = qm->tracks( this->collectionId() );
    delete qm;
    
    // assuming that there is only one result, should never be more, if so giving
    // the first is the best to do anyway
    if ( !tracks.isEmpty() )
    {
        return tracks.first();
    }
    
    return Meta::TrackPtr();
}

bool
NepomukCollection::isEmpty() const
{
   // FIXME: why doesn't it work with the first one?
   // Soprano::Node predicate( Soprano::Vocabulary::RDF::type() );
    Soprano::Node object ( Soprano::LiteralValue( Soprano::Vocabulary::Xesam::Music().toString() ) );
    Soprano::Statement statement( Soprano::Node(), Soprano::Node() , object );
    return !m_model->containsAnyStatement( statement );
}

void
NepomukCollection::initHashMaps()
{
    // this "v =" works around a linker error 
    // (undefined reference to QueryMaker::valXYZ)
    // does anyone know why?
    
    qint64 v;
    
    m_nameForValue[ v = QueryMaker::valAlbum ] = "album";
    m_nameForValue[ v = QueryMaker::valArtist ] = "artist";
    m_nameForValue[ v = QueryMaker::valBitrate ] = "bitrate";
    m_nameForValue[ v = QueryMaker::valComment ] = "comment";
    m_nameForValue[ v = QueryMaker::valComposer ] = "composer";
    m_nameForValue[ v = QueryMaker::valCreateDate ] = "createdate";
    m_nameForValue[ v = QueryMaker::valDiscNr ] = "discnr";
    m_nameForValue[ v = QueryMaker::valFilesize ] = "filesize";
    m_nameForValue[ v = QueryMaker::valFirstPlayed ] = "firstplayed";
    m_nameForValue[ v = QueryMaker::valFormat ] = "type";
    m_nameForValue[ v = QueryMaker::valGenre] = "genre";
    m_nameForValue[ v = QueryMaker::valLastPlayed ] = "lastplayed";
    m_nameForValue[ v = QueryMaker::valLength ] = "length";
    m_nameForValue[ v = QueryMaker::valPlaycount ] = "playcount";
    m_nameForValue[ v = QueryMaker::valRating ] = "rating";
    m_nameForValue[ v = QueryMaker::valSamplerate ] = "samplerate";
    m_nameForValue[ v = QueryMaker::valScore] = "score";
    m_nameForValue[ v = QueryMaker::valTitle ] = "title";
    m_nameForValue[ v = QueryMaker::valTrackNr ] = "tracknr";
    m_nameForValue[ v = QueryMaker::valUrl ] = "url";
    m_nameForValue[ v = QueryMaker::valYear ] = "year";   
    
    m_urlForValue[ v = QueryMaker::valAlbum ] = Soprano::Vocabulary::Xesam::album().toString();
    m_urlForValue[ v = QueryMaker::valArtist ] = Soprano::Vocabulary::Xesam::artist().toString();
    m_urlForValue[ v = QueryMaker::valBitrate ] = Soprano::Vocabulary::Xesam::audioBitrate().toString();
    m_urlForValue[ v = QueryMaker::valComment ] = Soprano::Vocabulary::Xesam::comment().toString();
    m_urlForValue[ v = QueryMaker::valComposer ] = Soprano::Vocabulary::Xesam::composer().toString();
    m_urlForValue[ v = QueryMaker::valCreateDate ] = Soprano::Vocabulary::Xesam::contentCreated().toString();
    m_urlForValue[ v = QueryMaker::valDiscNr ] = Soprano::Vocabulary::Xesam::discNumber().toString();
    m_urlForValue[ v = QueryMaker::valFilesize ] = Soprano::Vocabulary::Xesam::size().toString();
    // FirstUsed = FirstPlayed?
    m_urlForValue[ v = QueryMaker::valFirstPlayed ] = Soprano::Vocabulary::Xesam::firstUsed().toString();
    m_urlForValue[ v = QueryMaker::valFormat ] = Soprano::Vocabulary::Xesam::fileExtension().toString();
    m_urlForValue[ v = QueryMaker::valGenre] = Soprano::Vocabulary::Xesam::genre().toString();
    // LastUsed = LastPlayed?
    m_urlForValue[ v = QueryMaker::valLastPlayed ] = Soprano::Vocabulary::Xesam::lastUsed().toString();
    m_urlForValue[ v = QueryMaker::valLength ] = Soprano::Vocabulary::Xesam::mediaDuration().toString();
    // useCount = Playcount?
    m_urlForValue[ v = QueryMaker::valPlaycount ] = Soprano::Vocabulary::Xesam::useCount().toString();
    
    // there is a Xesam Value (userRating) but using Nepomuk one as Dolphin uses it (ok?)
    m_urlForValue[ v = QueryMaker::valRating ] = Soprano::Vocabulary::NAO::numericRating().toString();
    m_urlForValue[ v = QueryMaker::valSamplerate ] = Soprano::Vocabulary::Xesam::audioSampleRate().toString();
    m_urlForValue[ v = QueryMaker::valScore] = Soprano::Vocabulary::Xesam::autoRating().toString();
    m_urlForValue[ v = QueryMaker::valTitle ] = Soprano::Vocabulary::Xesam::title().toString();
    m_urlForValue[ v = QueryMaker::valTrackNr ] = Soprano::Vocabulary::Xesam::trackNumber().toString();
    m_urlForValue[ v = QueryMaker::valUrl ] = Soprano::Vocabulary::Xesam::url().toString();
    // Amarok seems to use Xesam contentCreated for year in SQL-Collection, so I assume it is ok here  
    m_urlForValue[ v = QueryMaker::valYear ] = Soprano::Vocabulary::Xesam::contentCreated().toString();
    
    QHashIterator<qint64, QString> it( m_nameForValue );
    while ( it.hasNext() ) 
    {
        it.next();
        m_allNamesAndUrls.append( it.value() );
        m_allNamesAndUrls.append( m_urlForValue[ it.key() ] );
    }
    m_allNamesAndUrls.append( "trackuid" );
    m_allNamesAndUrls.append( "http://amarok.kde.org/metadata/1.0/track#uid" );

    QHashIterator<qint64, QString> it2( m_urlForValue );
    while ( it2.hasNext() )
    {
        it2.next();
        m_valueForUrl[ it2.value( ) ] = it2.key();
    }
    
}

QString
NepomukCollection::getNameForValue( const qint64 value ) const
{
    return m_nameForValue[value];
}

QString
NepomukCollection::getUrlForValue( const qint64 value ) const
{
    return m_urlForValue[value];
}

qint64
NepomukCollection::valueForUrl( const QString& url ) const
{
    if ( m_valueForUrl.contains( url ) )
        return m_valueForUrl[ url ];
    else
        return 0;
}

const QStringList&
NepomukCollection::getAllNamesAndUrls() const
{
    return m_allNamesAndUrls;
}

NepomukRegistry*
NepomukCollection::registry() const
{
    return m_registry;
}

#include "NepomukCollection.moc"
