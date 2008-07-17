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

#include "NepomukAlbum.h"

#include "NepomukArtist.h"
#include "NepomukCollection.h"
#include "NepomukRegistry.h"

#include "BlockingQuery.h"
#include "Debug.h"
#include "Meta.h"

#include <QFile>
#include <QPixmap>
#include <QString>

#include <KUrl>
#include <Nepomuk/ResourceManager>
#include <Soprano/Model>
#include <Soprano/QueryResultIterator>
#include <Soprano/Vocabulary/Xesam>
#include <Soprano/Vocabulary/XMLSchema>

using namespace Meta;

NepomukAlbum::NepomukAlbum( NepomukCollection *collection, const QString &name, const QString &artist )
        : Album()
        , m_collection( collection )
        , m_name( name )
        , m_artist( artist )
        , m_tracksLoaded( false )
        , m_hasImage( false )
        , m_hasImageChecked( false )
{
}

QString
NepomukAlbum::name() const
{
    return m_name;
}

QString
NepomukAlbum::prettyName() const
{
    return m_name;
}

TrackList
NepomukAlbum::tracks()
{
    if( m_tracksLoaded )
    {
        return m_tracks;
    }
    else if( m_collection )
    {
        QueryMaker *qm = m_collection->queryMaker();
        qm->setQueryType( QueryMaker::Track );
        addMatchTo( qm );
        BlockingQuery bq( qm );
        bq.startQuery();
        m_tracks = bq.tracks( m_collection->collectionId() );
        m_tracksLoaded = true;
        return m_tracks;
    }
    else
        return TrackList(); 
}

bool
NepomukAlbum::isCompilation() const
{
    return false;
}

bool
NepomukAlbum::hasAlbumArtist() const
{
    return true;
}

ArtistPtr
NepomukAlbum::albumArtist() const
{
    return m_collection->registry()->artistForArtistName( m_artist );
}

bool
NepomukAlbum::hasImage( int size ) const
{
    DEBUG_BLOCK
    Q_UNUSED( size )
    if ( m_hasImageChecked )
        return m_hasImage;
    else
    {
        m_hasImageChecked = true;
        m_imagePath = findImage();
        if ( !m_imagePath.isEmpty() )
        {
            m_hasImage = true;
            return true;
        }
        else
            return false;
    }
}

QPixmap
NepomukAlbum::image( int size, bool withShadow )
{
    if( !hasImage( size) )
        return Meta::Album::image( size, withShadow );
   DEBUG_BLOCK
   QPixmap img( m_imagePath );
    if ( !img.isNull() )
    {
       // debug() << "nepo image size " << img.size() << endl;
       return img.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    }
    

    return Meta::Album::image( size, withShadow );
}

void
NepomukAlbum::emptyCache()
{
    // FIXME: Add proper locks

    m_tracks.clear();
    m_tracksLoaded = false;
}

QString
NepomukAlbum::findImage() const
{
    // TODO: Query for Image set in Nepomuk
    return findImageInDir();
}

QString
NepomukAlbum::findImageInDir() const
{
    if ( !m_tracksLoaded )
        const_cast<NepomukAlbum*>(this)-> tracks();
    
    // test if all files are in one directory ( we could ask nepomuk that, but I believe that will not be faster )
    QString path;
    foreach( Meta::TrackPtr tp, m_tracks )
    {
        Meta::NepomukTrackPtr ntp = Meta::NepomukTrackPtr::staticCast( tp );
        KUrl url( ntp->resourceUri() );
        if ( path.isEmpty() )
            path = url.directory();
        else if ( path != url.directory() )
        {
            // files are in different dirs
            return QString();
        }
    }
    
    QString query = QString("SELECT ?r WHERE {"
                  "?r <http://strigi.sf.net/ontologies/0.9#parentUrl> \"%1\"^^<%2> . " // only from path
                  "?r <%3> ?mime  FILTER regex(STR(?mime), '^image') } "  // only images
                  "LIMIT 1") // only one
                  .arg( path )
                  .arg( Soprano::Vocabulary::XMLSchema::string().toString() )
                  .arg( Soprano::Vocabulary::Xesam::mimeType() );
    Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel();
    Soprano::QueryResultIterator it
            = model->executeQuery( query,
                                     Soprano::Query::QueryLanguageSparql );
    if( it.next() )
    {
        Soprano::Node node = it.binding( "r" ) ;
        QUrl url( node.toString() );
        debug() << "nepo image found: " << url << endl;
        if ( QFile::exists( url.toLocalFile() ) )
            return url.toLocalFile();
    }
   return QString();
}
