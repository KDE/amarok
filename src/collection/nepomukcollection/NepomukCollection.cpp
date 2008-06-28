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
#include "NepomukMeta.h"

#include "Debug.h"
#include "QueryMaker.h"

#include <QHash>
#include <QString>
#include <QTime>

#include <klocale.h>
#include <KUrl>
#include <Nepomuk/Resource>
#include <Nepomuk/ResourceManager>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/NAO>
#include <Soprano/Vocabulary/Xesam>


AMAROK_EXPORT_PLUGIN( NepomukCollectionFactory )

// CollectionFactory

void
NepomukCollectionFactory::init()
{
    Soprano::Client::DBusClient* client = new Soprano::Client::DBusClient( "org.kde.NepomukStorage" );

    // TODO: use QLocalSocket 
    //if ( Nepomuk::ResourceManager::instance()->init() == 0 )
    if (client->isValid())
    {
        // find out if Nepomuk is fast enough
        // (if sesame2 is used or not, it makes no sense to use it with redland
        // doesn't work and is terrible slow, slows down amarok start when 
        // songs in playlist)
        
        QTime t;
        t.start();
        Nepomuk::Resource::Resource( "file://home/" ).exists();
        int elapsed = t.elapsed();
        debug() << "Nepomuk Resource.exists() took " << elapsed <<  " ms" << endl;
        
        Collection* collection;
        if ( elapsed < 50 )
        {
            collection = new NepomukCollection( client, true );
            debug() << "fast  enough full nepomuk collection enabled" << endl;
        }
        else
        {
            collection = new NepomukCollection( client, false );
            debug() << "too slow, trackForUrl() disabled" << endl;
        }
        emit newCollection( collection );
    }
    else
    {
        warning() << "Nepomuk is not running, can not init Nepomuk Collection" << endl;
        delete client;
    }
}

// NepomukCollection

NepomukCollection::NepomukCollection(Soprano::Client::DBusClient *client, bool isFast )
    :   Collection() 
    ,   m_client( client )
    ,   m_isFast( isFast )
{
    initHashMaps();
}

NepomukCollection::~NepomukCollection()
{
    delete m_client;
}

QueryMaker*
NepomukCollection::queryMaker()
{
	return new NepomukQueryMaker(this, m_client);
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
        return i18n("Nepomuk Collection (fast)");
    else
        return i18n("Nepomuk Collection (slow, redland?)");
}

bool
NepomukCollection::possiblyContainsTrack( const KUrl &url ) const
{
    return url.protocol() == "file";
}

Meta::TrackPtr
NepomukCollection::trackForUrl( const KUrl &url )
{
    // it is too slow with redland, makes start of amarok slow
    // so just return 
    
    if ( !m_isFast )
        return Meta::TrackPtr();
    
    DEBUG_BLOCK
    if ( Nepomuk::Resource::Resource( url ).exists() )
    {
        debug() << "Track: " << url.prettyUrl() << " is in NepomukCollection" << endl;
        NepomukQueryMaker qm (this, m_client);
        qm.startTrackQuery();
        qm.addMatch( url );
        QString query = qm.buildQuery();
        Soprano::Model* model = (Soprano::Model*)m_client->createModel( "main" );
        Soprano::QueryResultIterator it
                              = model->executeQuery( query, 
                                                     Soprano::Query::QueryLanguageSparql );
        
        // assuming that there is only one result, should never be more, if so giving
        // the first is the best to do anyway
        if ( it.next() )
        {
            Soprano::BindingSet bindingSet = it.currentBindings();
            Meta::TrackPtr tp ( new Meta::NepomukTrack( this, bindingSet ) );
            delete model;
            return tp;
        }
    }
    return Meta::TrackPtr();
}

void
NepomukCollection::lostDBusConnection()
{
    debug() << "removing NepomukCollection, lost dbus connection" << endl;
    emit remove();
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

const QStringList&
NepomukCollection::getAllNamesAndUrls() const
{
    return m_allNamesAndUrls;
}


#include "NepomukCollection.moc"
