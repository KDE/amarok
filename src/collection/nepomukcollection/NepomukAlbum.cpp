/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "NepomukAlbum.h"

#include "NepomukArtist.h"
#include "NepomukCollection.h"
#include "NepomukQueryMaker.h"
#include "NepomukRegistry.h"

#include "covermanager/CoverFetchingActions.h"
#include "meta/CustomActionsCapability.h"
#include "Debug.h"
#include "Meta.h"

#include <QAction>
#include <QDir>
#include <QFile>
#include <QPixmap>
#include <QString>

#include <KMD5>
#include <Nepomuk/ResourceManager>
#include <Nepomuk/Variant>
#include <Soprano/BindingSet>
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
        NepomukQueryMaker *qm = static_cast<NepomukQueryMaker*>( m_collection->queryMaker() );
        qm->setQueryType( QueryMaker::Track );
        addMatchTo( qm );
        qm->blocking( true );
        qm->run();
        m_tracks = qm->tracks( m_collection->collectionId() );
        delete qm;
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
    if( !m_hasImageChecked )
        m_hasImage = ! const_cast<NepomukAlbum*>( this )->image( size ).isNull();
    return m_hasImage;
}

QPixmap
NepomukAlbum::image( int size )
{
    if ( !m_hasImageChecked )
    {
        m_hasImageChecked = true;
        m_imagePath = findImage();
        if ( !m_imagePath.isEmpty() )
            m_hasImage = true;
    }
    if( !m_hasImage )
        return Meta::Album::image( size );

    if( size == 0) // fullsize image
        return QPixmap( m_imagePath );
    if( m_images.contains( size ) )
        return QPixmap( m_images.value( size ) );
    
    QString path = findOrCreateScaledImage( m_imagePath, size );
    if ( !path.isEmpty() )
    {
        m_images.insert( size, path );
        return QPixmap( path );
    }
 
    return Meta::Album::image( size );
}

void
NepomukAlbum::setImage( const QImage &image )
{
    if( image.isNull() )
        return;

    QString album = m_name;
    QString artist = hasAlbumArtist() ? albumArtist()->name() : QString();

    if( artist.isEmpty() && album.isEmpty() )
        return;

    // removeImage() will destroy all scaled cached versions of the artwork 
    // and remove references from the database if required.
    if( hasImage( 0 ) ) 
        removeImage();

    KMD5 context( artist.toLower().toLocal8Bit() + album.toLower().toLocal8Bit() );
    QByteArray key = context.hexDigest();
    QString path = Amarok::saveLocation( "albumcovers/large/" ) + "nepo" + key;
    image.save( path, "JPG" );

    QString query = QString("SELECT ?r WHERE {"
            "?r <%1> \"%2\"^^<%3> ."  // only for current artist
            "?r <%4> \"%5\"^^<%6> . }" ) // only for current album
            .arg( m_collection->getUrlForValue( Meta::valArtist ) )
            .arg( m_artist )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() )
            .arg( m_collection->getUrlForValue( Meta::valAlbum ) )
            .arg( m_name )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );

    Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel();
    Soprano::QueryResultIterator it
            = model->executeQuery( query,
                                   Soprano::Query::QueryLanguageSparql );
    while( it.next() )
    {
        Nepomuk::Resource res( it.binding( "r" ).uri() );
        
        m_collection->registry()->writeToNepomukAsync( res 
                                , QUrl("http://amarok.kde.org/metadata/1.0/track#coverUrl")
                                , Nepomuk::Variant( path ) );
    }

    m_hasImage = true;
    m_hasImageChecked = true;
    m_imagePath = path;
    notifyObservers();
}

void
NepomukAlbum::removeImage()
{
    QString album = m_name;
    QString artist = hasAlbumArtist() ? albumArtist()->name() : QString();

    if( artist.isEmpty() && album.isEmpty() )
        return;

    KMD5 context( artist.toLower().toLocal8Bit() + album.toLower().toLocal8Bit() );
    QByteArray key = context.hexDigest();

    // remove fullsize image if in amarok dir
    QFile::remove( Amarok::saveLocation( "albumcovers/large/" ) + "nepo" + key );
    // remove all cache images
    QDir        cacheDir( Amarok::saveLocation( "albumcovers/cache/" ) );
    QStringList cacheFilter;
    cacheFilter << QString( "*nepo" + key );
    QStringList cachedImages = cacheDir.entryList( cacheFilter );

    foreach( const QString &image, cachedImages )
    {
        QFile::remove( cacheDir.filePath( image ) );
    }

    // TODO: remove directory image ??

    // Update the image path in Nepomuk

    QString query = QString("SELECT ?r WHERE {"
            "?r <%1> \"%2\"^^<%3> ."  // only for current artist
            "?r <%4> \"%5\"^^<%6> . }" ) // only for current album
            .arg( m_collection->getUrlForValue( Meta::valArtist ) )
            .arg( m_artist )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() )
            .arg( m_collection->getUrlForValue( Meta::valAlbum ) )
            .arg( m_name )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );

    Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel();
    Soprano::QueryResultIterator it
            = model->executeQuery( query,
                                   Soprano::Query::QueryLanguageSparql );
    
    QList<Soprano::BindingSet> bindings = it.allBindings();

    foreach( const Soprano::BindingSet &binding, bindings )
    {
        Nepomuk::Resource res( binding.value( "r" ).uri() );
        QUrl url( "http://amarok.kde.org/metadata/1.0/track#coverUrl" );
        res.removeProperty( url );
    }

    m_hasImageChecked = true;
    m_hasImage = false;
    m_imagePath.clear();
    m_images.clear();
    notifyObservers();
}

bool
NepomukAlbum::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    switch( type )
    {
        case Meta::Capability::CustomActions:
            return true;

        default:
            return false;
    }
}

Meta::Capability*
NepomukAlbum::createCapabilityInterface( Meta::Capability::Type type )
{
    switch( type )
    {
        case Meta::Capability::CustomActions:
        {
            QList<QAction*> actions;
            //actions.append( new CopyToDeviceAction( m_collection, this ) );
            //actions.append( new CompilationAction( m_collection, this ) );
            //QAction* separator = new QAction( m_collection );
            //separator->setSeparator( true );
            //actions.append( separator );
            actions.append( new FetchCoverAction( m_collection, this ) );
            actions.append( new SetCustomCoverAction( m_collection, this ) );
            QAction *displayCoverAction = new DisplayCoverAction( m_collection, this );
            QAction *unsetCoverAction   = new UnsetCoverAction( m_collection, this );
            if( !hasImage() )
            {
                displayCoverAction->setEnabled( false );
                unsetCoverAction->setEnabled( false );
            }
            actions.append( displayCoverAction );
            actions.append( unsetCoverAction );
            return new CustomActionsCapability( actions );
        }

        default:
            return 0;
    }
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
    // first look for an cover set in Nepomuk, if that fails look for an image in the dir of the files
    QString path = findImageInNepomuk();
    if ( path.isEmpty() )
        path = findImageInDir();
    return path;
}

QString
NepomukAlbum::findOrCreateScaledImage( QString path, int size ) const
{
    if( size <= 1 )
        return QString();

    QByteArray widthKey = QString::number( size ).toLocal8Bit() + "@nepo"; // nepo
    QString album = m_name;
    QString artist = hasAlbumArtist() ? albumArtist()->name() : QString();

    if( artist.isEmpty() && album.isEmpty() )
        return QString();

    KMD5 context( artist.toLower().toLocal8Bit() + album.toLower().toLocal8Bit() );
    QByteArray key = context.hexDigest();

    QDir cacheCoverDir( Amarok::saveLocation( "albumcovers/cache/" ) );
    QString cachedImagePath = cacheCoverDir.filePath( widthKey + key );

    // create if it not already there
    if ( !QFile::exists( cachedImagePath ) )
    {
         if( QFile::exists( path ) )
        {
            QImage img( path );
            if( img.isNull() )
                return QString();
            
            // resize and save the image
            img.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation ).save( cachedImagePath, "JPG" );
        }
    }
    return cachedImagePath;
}

QString
NepomukAlbum::findImageInNepomuk() const
{
    QString path;
    QString query = QString("SELECT DISTINCT ?r ?path WHERE {"
            "?r <%1> \"%2\"^^<%3> ."  // only for current artist
            "?r <%4> \"%5\"^^<%6> ." // only for current album
            "?r <http://amarok.kde.org/metadata/1.0/track#coverUrl> ?path . " // we want to know the coverUrl
            "} LIMIT 1") // we need not more than 2
            .arg( m_collection->getUrlForValue( Meta::valArtist ) )
            .arg( m_artist )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() )
            .arg( m_collection->getUrlForValue( Meta::valAlbum ) )
            .arg( m_name )
            .arg( Soprano::Vocabulary::XMLSchema::string().toString() );
                        
    Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel();
    Soprano::QueryResultIterator it
            = model->executeQuery( query,
                                   Soprano::Query::QueryLanguageSparql );
    if ( it.next() )
    {
        path = it.binding( "path" ).toString();
        if ( !QFile::exists( path ) )
            path.clear();
    }
    return path;
}

QString
NepomukAlbum::findImageInDir() const
{
    // test if all files are in one directory
    QString path;
    QString query = QString("SELECT DISTINCT ?path WHERE {"
                "?r <%1> \"%2\"^^<%3> ."  // only for current artist
                "?r <%4> \"%5\"^^<%6> ." // only for current album
                "?r <http://strigi.sf.net/ontologies/0.9#parentUrl> ?path . " // we want to know the parenturl
                "} LIMIT 2") // we need not more than 2
                .arg( m_collection->getUrlForValue( Meta::valArtist ) )
                .arg( m_artist )
                .arg( Soprano::Vocabulary::XMLSchema::string().toString() )
                .arg( m_collection->getUrlForValue( Meta::valAlbum ) )
                .arg( m_name )
                .arg( Soprano::Vocabulary::XMLSchema::string().toString() );
                        
    Soprano::Model* model = Nepomuk::ResourceManager::instance()->mainModel();
    Soprano::QueryResultIterator it
            = model->executeQuery( query,
                                    Soprano::Query::QueryLanguageSparql );
    if ( it.next() )
    {
        path = it.binding( "path" ).toString();
        // if we have another result the files are in more than one dir 
        if ( it.next() )
            return QString();
    }
    else
    {
        // should not happen
        return QString();
    }
    query = QString("SELECT ?r WHERE {"
                  "?r <http://strigi.sf.net/ontologies/0.9#parentUrl> \"%1\"^^<%2> . " // only from path
                  "?r <%3> ?mime  FILTER regex(STR(?mime), '^image') } "  // only images
                  "LIMIT 1") // only one
                  .arg( path )
                  .arg( Soprano::Vocabulary::XMLSchema::string().toString() )
                  .arg( Soprano::Vocabulary::Xesam::mimeType().toString() );
    it = model->executeQuery( query,
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

