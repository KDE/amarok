/****************************************************************************
 * copyright            :(C) 2006 Roel Meeuws <r.j.meeuws+amarok@gmail.com  *
 *                       (C) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com> *
 *                       (C) 2005 Seb Ruiz <me@sebruiz.net>                 *
 *                                                                          *
 * With some code helpers from KIO_VFAT                                     *
 *                        (c) 2004 Thomas Loeber <vfat@loeber1.de>          *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#define DEBUG_PREFIX "GenericMediaDevice"

#include "genericmediadevice.h"

AMAROK_EXPORT_PLUGIN( GenericMediaDevice )

#include "amarok.h"
#include "debug.h"
#include "medium.h"
#include "metabundle.h"
#include "collectiondb.h"
#include "collectionbrowser.h"
#include "k3bexporter.h"
#include "playlist.h"
#include "podcastbundle.h"
#include "statusbar/statusbar.h"
#include "transferdialog.h"
#include "genericmediadeviceconfigdialog.h"

#include <kapplication.h>
#include <kconfig.h>           //download saveLocation
#include <kdiskfreesp.h>
#include <kiconloader.h>       //smallIcon
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kmountpoint.h>
#include <kpopupmenu.h>
#include <kurlrequester.h>     //downloadSelectedItems()
#include <kurlrequesterdlg.h>  //downloadSelectedItems()

#include <taglib/audioproperties.h>

#include <unistd.h>            //usleep()

#include <qcstring.h>
#include <qfile.h>
#include <qstringx.h>
#include <qstringlist.h>

#include <qcombobox.h>
#include <qlistbox.h>
#include <qlineedit.h>

typedef QPtrList<GenericMediaFile> MediaFileList;
typedef QPtrListIterator<GenericMediaFile> MediaFileListIterator;

/**
 * GenericMediaItem Class
 */

class GenericMediaItem : public MediaItem
{
    public:
        GenericMediaItem( QListView *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

        GenericMediaItem( QListViewItem *parent, QListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

        // List directories first, always
        int
        compare( QListViewItem *i, int col, bool ascending ) const
        {
            #define i static_cast<GenericMediaItem *>(i)
            switch( type() )
            {
                case MediaItem::DIRECTORY:
                    if( i->type() == MediaItem::DIRECTORY )
                        break;
                    return -1;

                default:
                    if( i->type() == MediaItem::DIRECTORY )
                        return 1;
            }
            #undef i

            return MediaItem::compare(i, col, ascending);
        }

    private:
        bool     m_dir;
};

class GenericMediaFile
{
    public:
        GenericMediaFile( GenericMediaFile *parent, QString basename, GenericMediaDevice *device )
        : m_parent( parent )
        , m_device( device )
        {
            m_listed = false;
            m_children = new MediaFileList();

            if( m_parent )
            {
                if( m_parent == m_device->getInitialFile() )
                    m_viewItem = new GenericMediaItem( m_device->view() );
                else
                    m_viewItem = new GenericMediaItem( m_parent->getViewItem() );
                setNamesFromBase( basename );
                m_viewItem->setText( 0, m_baseName );
                m_parent->getChildren()->append( this );
            }
            else
            {
                m_viewItem = 0;
                setNamesFromBase( basename );
            }

            m_device->getItemMap()[m_viewItem] = this;

            if( m_device->getFileMap()[m_fullName] )
            {
                debug() << "Trying to create two GenericMediaFile items with same fullName!" << endl;
                debug() << "name already existing: " << m_device->getFileMap()[m_fullName]->getFullName() << endl;
                delete this;
            }
            else
            {
                m_device->getFileMap()[m_fullName] = this;
            }

        }

        ~GenericMediaFile()
        {
            if( m_parent )
                m_parent->removeChild( this );
            m_device->getItemMap().erase( m_viewItem );
            m_device->getFileMap().erase( m_fullName );
            if ( m_children )
                delete m_children;
            if ( m_viewItem )
                delete m_viewItem;
        }

        GenericMediaFile*
        getParent() { return m_parent; }

        void
        setParent( GenericMediaFile* parent )
        {
            m_device->getFileMap().erase( m_fullName );
            m_parent->getChildren()->remove( this );
            m_parent = parent;
            if( m_parent )
                m_parent->getChildren()->append( this );
            setNamesFromBase( m_baseName );
            m_device->getFileMap()[m_fullName] = this;
        }

        void
        removeChild( GenericMediaFile* childToDelete ) { m_children->remove( childToDelete ); }

        GenericMediaItem*
        getViewItem() { return m_viewItem; }

        bool
        getListed() { return m_listed; }

        void
        setListed( bool listed ) { m_listed = listed; }

        QString
        getFullName() { return m_fullName; }

        QString
        getBaseName() { return m_baseName; }

        //always follow this function with setNamesFromBase()
        void
        setBaseName( QString &name ) { m_baseName = name; }

        void
        setNamesFromBase( const QString &name = QString::null )
        {
            if( name != QString::null )
                m_baseName = name;
            if( m_parent )
                m_fullName = m_parent->getFullName() + '/' + m_baseName;
            else
                m_fullName = m_baseName;
            if( m_viewItem )
                m_viewItem->setBundle( new MetaBundle( KURL::fromPathOrURL( m_fullName ), true, TagLib::AudioProperties::Fast ) );
        }

        MediaFileList*
        getChildren() { return m_children; }

        void
        deleteAll( bool onlyChildren )
        {
            GenericMediaFile *vmf;
            if( m_children && !m_children->isEmpty() )
            {
                MediaFileListIterator it( *m_children );
                while( ( vmf = it.current() ) != 0 )
                {
                    ++it;
                    vmf->deleteAll( true );
                }
            }
            if( onlyChildren )
                delete this;
        }

        void
        renameAllChildren()
        {
            GenericMediaFile *vmf;
            if( m_children && !m_children->isEmpty() )
            {
                for( vmf = m_children->first(); vmf; vmf = m_children->next() )
                    vmf->renameAllChildren();
            }
            setNamesFromBase();
        }

    private:
        QString m_fullName;
        QString m_baseName;
        GenericMediaFile *m_parent;
        MediaFileList *m_children;
        GenericMediaItem *m_viewItem;
        GenericMediaDevice* m_device;
        bool m_listed;
};

QString
GenericMediaDevice::fileName( const MetaBundle &bundle )
{
    QString result = cleanPath( bundle.artist() );

    if( !result.isEmpty() )
    {
        if( m_spacesToUnderscores )
            result += "_-_";
        else
            result += " - ";
    }

    if( bundle.track() )
    {
        result.sprintf( "%02d", bundle.track() );

        if( m_spacesToUnderscores )
            result += '_';
        else
            result += ' ';
    }

    result += cleanPath( bundle.title() + '.' + bundle.type() );

    return result;
}


/**
 * GenericMediaDevice Class
 */

GenericMediaDevice::GenericMediaDevice()
    : MediaDevice()
    , m_kBSize( 0 )
    , m_kBAvail( 0 )
    , m_connected( false )
{
    DEBUG_BLOCK
    m_name = i18n("Generic Audio Player");
    m_dirLister = new KDirLister();
    m_dirLister->setNameFilter( "*.mp3 *.wav *.asf *.flac *.wma *.ogg *.aac *.m4a *.mp4 *.mp2 *.ac3" );
    m_dirLister->setAutoUpdate( false );

    m_spacesToUnderscores = false;
    m_ignoreThePrefix     = false;
    m_asciiTextOnly       = false;

    m_songLocation = QString::null;
    m_podcastLocation = QString::null;

    m_supportedFileTypes.clear();

    m_configDialog = 0;

    connect( m_dirLister, SIGNAL( newItems(const KFileItemList &) ), this, SLOT( newItems(const KFileItemList &) ) );
    connect( m_dirLister, SIGNAL( completed() ), this, SLOT( dirListerCompleted() ) );
    connect( m_dirLister, SIGNAL( clear() ), this, SLOT( dirListerClear() ) );
    connect( m_dirLister, SIGNAL( clear(const KURL &) ), this, SLOT( dirListerClear(const KURL &) ) );
    connect( m_dirLister, SIGNAL( deleteItem(KFileItem *) ), this, SLOT( dirListerDeleteItem(KFileItem *) ) );
}

void
GenericMediaDevice::init( MediaBrowser* parent )
{
    MediaDevice::init( parent );
}

GenericMediaDevice::~GenericMediaDevice()
{
    closeDevice();
}

void
GenericMediaDevice::applyConfig()
{
    if( m_configDialog != 0)
    {
        m_supportedFileTypes.clear();
        for( uint i = 0; i < m_configDialog->m_supportedListBox->count(); i++ )
        {
            QString currentText = m_configDialog->m_supportedListBox->item( i )->text();

            if( currentText == m_configDialog->m_convertComboBox->currentText() )
                m_supportedFileTypes.prepend( currentText );
            else
                m_supportedFileTypes.append( currentText );
        }

        m_spacesToUnderscores = m_configDialog->m_spaceCheck->isChecked();
        m_asciiTextOnly = m_configDialog->m_asciiCheck->isChecked();
        m_vfatTextOnly = m_configDialog->m_vfatCheck->isChecked();
        m_ignoreThePrefix = m_configDialog->m_ignoreTheCheck->isChecked();

        m_songLocation = m_configDialog->m_songLocationBox->text();
        m_podcastLocation = m_configDialog->m_podcastLocationBox->text();
    }


    setConfigString( "songLocation"       , m_songLocation );
    setConfigString( "podcastLocation"    , m_podcastLocation );
    setConfigBool(   "spacesToUnderscores", m_spacesToUnderscores );
    setConfigBool(   "ignoreThePrefix"    , m_ignoreThePrefix );
    setConfigBool(   "asciiTextOnly"      , m_asciiTextOnly );
    setConfigBool(   "vfatTextOnly"       , m_vfatTextOnly );
    setConfigString( "supportedFiletypes" , m_supportedFileTypes.join( ", " ) );
}


void
GenericMediaDevice::loadConfig()
{
    MediaDevice::loadConfig();

    m_spacesToUnderscores = configBool( "spacesToUnderscores", false );
    m_ignoreThePrefix = configBool( "ignoreThePrefix", false);
    m_asciiTextOnly = configBool( "asciiTextOnly", false );
    m_vfatTextOnly = configBool( "vfatTextOnly", false );

    m_songLocation = configString( "songLocation", "/%artist/%album/%title.%filetype" );
    m_podcastLocation = configString( "podcastLocation", "/podcasts/" );

    m_supportedFileTypes = QStringList::split( ", ", configString( "supportedFiletypes", "mp3"), true);
}

bool
GenericMediaDevice::openDevice( bool /*silent*/ )
{
    DEBUG_BLOCK
    if( !m_medium.mountPoint() )
    {
        Amarok::StatusBar::instance()->longMessage( i18n( "Devices handled by this plugin must be mounted first.\n"
                                                          "Please mount the device and click \"Connect\" again." ),
                                                    KDE::StatusBar::Sorry );
        return false;
    }

    KMountPoint::List currentmountpoints = KMountPoint::currentMountPoints();
    KMountPoint::List::Iterator mountiter = currentmountpoints.begin();
    for(; mountiter != currentmountpoints.end(); ++mountiter)
    {
        if( m_medium.mountPoint() == (*mountiter)->mountPoint() )
            m_medium.setFsType( (*mountiter)->mountType() );
    }
    m_actuallyVfat = (m_medium.fsType() == "vfat" || m_medium.fsType() == "msdosfs") ? 
            true : false;
    m_connected = true;
    KURL tempurl = KURL::fromPathOrURL( m_medium.mountPoint() );
    QString newMountPoint = tempurl.isLocalFile() ? tempurl.path( -1 ) : tempurl.prettyURL( -1 ); //no trailing slash
    m_transferDir = newMountPoint;
    m_initialFile = new GenericMediaFile( 0, newMountPoint, this );
    listDir( newMountPoint );
    connect( this, SIGNAL( startTransfer() ), MediaBrowser::instance(), SLOT( transferClicked() ) );
    return true;
}

bool
GenericMediaDevice::closeDevice()  //SLOT
{
    if( m_connected )
    {
        m_initialFile->deleteAll( true );
        m_view->clear();
        m_connected = false;

    }
    //delete these?
    m_mfm.clear();
    m_mim.clear();
    return true;
}

/// Renaming

void
GenericMediaDevice::renameItem( QListViewItem *item ) // SLOT
{

    if( !item )
        return;

    #define item static_cast<GenericMediaItem*>(item)

    QString src = m_mim[item]->getFullName();
    QString dst = m_mim[item]->getParent()->getFullName() + '/' + item->text(0);

    debug() << "Renaming: " << src << " to: " << dst << endl;

    //do we want a progress dialog?  If so, set last false to true
    if( KIO::NetAccess::file_move( KURL::fromPathOrURL(src), KURL::fromPathOrURL(dst), -1, false, false, false ) )
    {
        m_mfm.erase( m_mim[item]->getFullName() );
        m_mim[item]->setNamesFromBase( item->text(0) );
        m_mfm[m_mim[item]->getFullName()] = m_mim[item];
    }
    else
    {
        debug() << "Renaming FAILED!" << endl;
        //failed, so set the item's text back to how it should be
        item->setText( 0, m_mim[item]->getBaseName() );
    }


    refreshDir( m_mim[item]->getParent()->getFullName() );
    m_mim[item]->renameAllChildren();

    #undef item

}

/// Creating a directory

MediaItem *
GenericMediaDevice::newDirectory( const QString &name, MediaItem *parent )
{
    if( !m_connected || name.isEmpty() ) return 0;

    #define parent static_cast<GenericMediaItem*>(parent)

    QString fullName = m_mim[parent]->getFullName();
    QString cleanedName = cleanPath( name );
    QString fullPath = fullName + '/' + cleanedName;
    debug() << "Creating directory: " << fullPath << endl;    
    const KURL url( fullPath );

    if( !KIO::NetAccess::mkdir( url, m_parent ) ) //failed
    {
        debug() << "Failed to create directory " << fullPath << endl;
        return 0;
    }

    refreshDir( m_mim[parent]->getFullName() );

    #undef parent

    return 0;
}

void
GenericMediaDevice::addToDirectory( MediaItem *directory, QPtrList<MediaItem> items )
{
    if( items.isEmpty() ) return;

    GenericMediaFile *dropDir;
    if( !directory )
        dropDir = m_initialFile;
    else
    {
        if( directory->type() == MediaItem::TRACK )
        #define directory static_cast<GenericMediaItem *>(directory)
            dropDir = m_mim[directory]->getParent();
        else
            dropDir = m_mim[directory];
    }

    for( QPtrListIterator<MediaItem> it(items); *it; ++it )
    {
        GenericMediaItem *currItem = static_cast<GenericMediaItem *>(*it);
        QString src  = m_mim[currItem]->getFullName();
        QString dst = dropDir->getFullName() + '/' + currItem->text(0);
        debug() << "Moving: " << src << " to: " << dst << endl;

        const KURL srcurl(src);
        const KURL dsturl(dst);

        if ( !KIO::NetAccess::file_move( srcurl, dsturl, -1, false, false, m_parent ) )
            debug() << "Failed moving " << src << " to " << dst << endl;
        else
        {
            refreshDir( m_mim[currItem]->getParent()->getFullName() );
            refreshDir( dropDir->getFullName() );
            //smb: urls don't seem to refresh correctly, but this seems to be a samba issue?
        }
    }
    #undef directory
}

/// Uploading

QString
GenericMediaDevice::buildDestination( const QString &format, const MetaBundle &mb )
{
    bool isCompilation = mb.compilation() == MetaBundle::CompilationYes;
    QMap<QString, QString> args;
    QString artist = mb.artist();
    QString albumartist = artist;
    if( isCompilation )
        albumartist = i18n( "Various Artists" );
    args["theartist"] = cleanPath( artist );
    args["thealbumartist"] = cleanPath( albumartist );
    if( m_ignoreThePrefix && artist.startsWith( "The " ) )
        CollectionView::instance()->manipulateThe( artist, true );
    artist = cleanPath( artist );
    if( m_ignoreThePrefix && albumartist.startsWith( "The " ) )
        CollectionView::instance()->manipulateThe( albumartist, true );

    albumartist = cleanPath( albumartist );
    for( int i = 0; i < MetaBundle::NUM_COLUMNS; i++ )
    {
        if( i == MetaBundle::Score || i == MetaBundle::PlayCount || i == MetaBundle::LastPlayed )
            continue;
        args[mb.exactColumnName( i ).lower()] = cleanPath( mb.prettyText( i ) );
    }
    args["artist"] = artist;
    args["albumartist"] = albumartist;
    args["initial"] = albumartist.mid( 0, 1 ).upper();
    args["filetype"] = mb.url().pathOrURL().section( ".", -1 ).lower();
    QString track;
    if ( mb.track() )
        track.sprintf( "%02d", mb.track() );
    args["track"] = track;

    Amarok::QStringx formatx( format );
    QString result = formatx.namedOptArgs( args );
    if( !result.startsWith( "/" ) )
        result.prepend( "/" );

   return result.replace( QRegExp( "/\\.*" ), "/" );
}

void
GenericMediaDevice::checkAndBuildLocation( const QString& location )
{
    // check for every directory from the mount point to the location
    // whether they exist or not.
    int mountPointDepth = m_medium.mountPoint().contains( '/', false );
    int locationDepth = location.contains( '/', false );

    if( m_medium.mountPoint().endsWith( "/" ) )
        mountPointDepth--;

    if( location.endsWith( "/") )
        locationDepth--;

    // the locationDepth indicates the filename, in the following loop
    // however, we only look at the direcories. hence i < locationDepth
    // instead of '<='
    for( int i = mountPointDepth;
         i < locationDepth;
         i++ )
    {

        QString firstpart = location.section( '/', 0, i-1 );
        QString secondpart = cleanPath( location.section( '/', i, i ) );
        KURL url = KURL::fromPathOrURL( firstpart + '/' + secondpart );

        if( !KIO::NetAccess::exists( url, false, m_parent ) )
        {
            debug() << "directory does not exist, creating..." << url << endl;
            if( !KIO::NetAccess::mkdir(url, m_view ) ) //failed
            {
                debug() << "Failed to create directory " << url << endl;
                return;
            }
        }
    }
}

QString
GenericMediaDevice::buildPodcastDestination( const PodcastEpisodeBundle *bundle )
{
    QString location = m_podcastLocation.endsWith("/") ? m_podcastLocation : m_podcastLocation + '/';
    // get info about the PodcastChannel
    QString parentUrl = bundle->parent().url();
    QString sql = "SELECT title,parent FROM podcastchannels WHERE url='" + CollectionDB::instance()->escapeString( parentUrl ) + "';";
    QStringList values = CollectionDB::instance()->query( sql );
    QString channelTitle;
    int parent = 0;
    channelTitle = values.first();
    parent = values.last().toInt();
    // Put the file in a directory tree like in the playlistbrowser
    sql = "SELECT name,parent FROM podcastfolders WHERE id=%1;";
    QString name;
    while ( parent > 0 )
    {
        values = CollectionDB::instance()->query( sql.arg( parent ) );
        name    =    values.first();
        parent  =   values.last().toInt();
        location += cleanPath( name ) + '/';
    }
    location += cleanPath( channelTitle ) + '/' + cleanPath( bundle->localUrl().filename() );
    return location;
}


MediaItem *
GenericMediaDevice::copyTrackToDevice( const MetaBundle& bundle )
{
    if( !m_connected ) return 0;

    // use different naming schemes for differen kinds of tracks
    QString path = m_transferDir;
    debug() << "bundle exists: " << bundle.podcastBundle() << endl;
    if( bundle.podcastBundle() )
        path += buildPodcastDestination( bundle.podcastBundle() );
    else
        path += buildDestination( m_songLocation, bundle );

    checkAndBuildLocation( path );

    const KURL desturl = KURL::fromPathOrURL( path );

    //kapp->processEvents( 100 );

    if( !kioCopyTrack( bundle.url(), desturl ) )
    {
        debug() << "Failed to copy track: " << bundle.url().pathOrURL() << " to " << desturl.pathOrURL() << endl;
        return 0;
    }

    refreshDir( m_transferDir );

    //the return value just can't be null, as nothing is done with it
    //other than to see if it is NULL or not
    //if we're here the transfer shouldn't have failed, so we shouldn't get into a loop by waiting...
    while( !m_view->firstChild() )
        kapp->processEvents( 100 );
    return static_cast<MediaItem*>(m_view->firstChild());
}

//Somewhat related...

MediaItem *
GenericMediaDevice::trackExists( const MetaBundle& bundle )
{
    QString key;
    QString path = buildDestination( m_songLocation, bundle);
    KURL url( path );
    QStringList directories = QStringList::split( "/", url.directory(1,1), false );

    QListViewItem *it = view()->firstChild();
    for( QStringList::Iterator directory = directories.begin();
         directory != directories.end();
         directory++ )
    {
        key = *directory;
        while( it && it->text( 0 ) != key )
            it = it->nextSibling();
        if( !it )
            return 0;
        if( !it->childCount() )
            expandItem( it );
        it = it->firstChild();
    }

    key = url.fileName( true );
    key = key.isEmpty() ? fileName( bundle ) : key;
    while( it && it->text( 0 ) != key )
        it = it->nextSibling();

    return dynamic_cast<MediaItem *>( it );
}

/// File transfer methods


void
GenericMediaDevice::downloadSelectedItems()
{
    KURL::List urls = getSelectedItems();

    CollectionView::instance()->organizeFiles( urls, i18n("Copy Files to Collection"), true );

    hideProgress();
}

KURL::List
GenericMediaDevice::getSelectedItems()
{
    return m_view->nodeBuildDragList( static_cast<MediaItem*>(m_view->firstChild()), true );
}

/// Deleting

int
GenericMediaDevice::deleteItemFromDevice( MediaItem *item, int /*flags*/ )
{
    if( !item || !m_connected ) return -1;

    #define item static_cast<GenericMediaItem*>(item)

    QString path = m_mim[item]->getFullName();
    debug() << "Deleting path: " << path << endl;

    if ( !KIO::NetAccess::del( KURL::fromPathOrURL(path), m_view ))
    {
        debug() << "Could not delete!" << endl;
        return -1;
    }

    if( m_mim[item] == m_initialFile )
    {
        m_mim[item]->deleteAll( false );
        debug() << "Not deleting root directory of mount!" << endl;
        path = m_initialFile->getFullName();
    }
    else
    {
        path = m_mim[item]->getParent()->getFullName();
        m_mim[item]->deleteAll( true );
    }
    refreshDir( path );

    setProgress( progress() + 1 );

    #undef item
    return 1;
}

/// Directory Reading

void
GenericMediaDevice::expandItem( QListViewItem *item ) // SLOT
{
    if( !item || !item->isExpandable() ) return;

    #define item static_cast<GenericMediaItem *>(item)
    m_dirListerComplete = false;
    listDir( m_mim[item]->getFullName() );
    #undef item

    while( !m_dirListerComplete )
    {
        kapp->processEvents( 100 );
        usleep(10000);
    }
}

void
GenericMediaDevice::listDir( const QString &dir )
{
    m_dirListerComplete = false;
    if( m_mfm[dir]->getListed() )
        m_dirLister->updateDirectory( KURL::fromPathOrURL(dir) );
    else
    {
        m_dirLister->openURL( KURL::fromPathOrURL(dir), true, true );
        m_mfm[dir]->setListed( true );
    }
}

void
GenericMediaDevice::refreshDir( const QString &dir )
{
    m_dirListerComplete = false;
    m_dirLister->updateDirectory( KURL::fromPathOrURL(dir) );
}

void
GenericMediaDevice::newItems( const KFileItemList &items )
{
    QPtrListIterator<KFileItem> it( items );
    KFileItem *kfi;
    while ( (kfi = it.current()) != 0 ) {
        ++it;
        addTrackToList( kfi->isFile() ? MediaItem::TRACK : MediaItem::DIRECTORY, kfi->url(), 0 );
    }
}

void
GenericMediaDevice::dirListerCompleted()
{
    m_dirListerComplete = true;
}

void
GenericMediaDevice::dirListerClear()
{
    m_initialFile->deleteAll( true );

    m_view->clear();
    m_mfm.clear();
    m_mim.clear();

    KURL tempurl = KURL::fromPathOrURL( m_medium.mountPoint() );
    QString newMountPoint = tempurl.isLocalFile() ? tempurl.path( -1 ) : tempurl.prettyURL( -1 ); //no trailing slash
    m_initialFile = new GenericMediaFile( 0, newMountPoint, this );
}

void
GenericMediaDevice::dirListerClear( const KURL &url )
{
    QString directory = url.pathOrURL();
    GenericMediaFile *vmf = m_mfm[directory];
    if( vmf )
        vmf->deleteAll( false );
}

void
GenericMediaDevice::dirListerDeleteItem( KFileItem *fileitem )
{
    QString filename = fileitem->url().pathOrURL();
    GenericMediaFile *vmf = m_mfm[filename];
    if( vmf )
        vmf->deleteAll( true );
}

int
GenericMediaDevice::addTrackToList( int type, KURL url, int /*size*/ )
{
    QString path = url.isLocalFile() ? url.path( -1 ) : url.prettyURL( -1 ); //no trailing slash
    int index = path.findRev( '/', -1 );
    QString baseName = path.right( path.length() - index - 1 );
    QString parentName = path.left( index );

    GenericMediaFile* parent = m_mfm[parentName];
    GenericMediaFile* newItem = new GenericMediaFile( parent, baseName, this );

    if( type == MediaItem::DIRECTORY ) //directory
        newItem->getViewItem()->setType( MediaItem::DIRECTORY );
    //TODO: this logic could maybe be taken out later...or the dirlister shouldn't
    //filter, one or the other...depends if we want to allow viewing any files
    //or just update the list in the plugin as appropriate
    else if( type == MediaItem::TRACK ) //file
    {
        if( baseName.endsWith( "mp3", false ) || baseName.endsWith( "wma", false ) ||
            baseName.endsWith( "wav", false ) || baseName.endsWith( "ogg", false ) ||
            baseName.endsWith( "asf", false ) || baseName.endsWith( "flac", false ) ||
            baseName.endsWith( "aac", false ) || baseName.endsWith( "m4a", false ) )

            newItem->getViewItem()->setType( MediaItem::TRACK );

        else
            newItem->getViewItem()->setType( MediaItem::UNKNOWN );
    }

    refreshDir( parent->getFullName() );

    return 0;
}

/// Capacity, in kB

bool
GenericMediaDevice::getCapacity( KIO::filesize_t *total, KIO::filesize_t *available )
{
    if( !m_connected || !KURL::fromPathOrURL( m_medium.mountPoint() ).isLocalFile() ) return false;

    KDiskFreeSp* kdf = new KDiskFreeSp( m_parent, "generic_kdf" );
    kdf->readDF( m_medium.mountPoint() );
    connect(kdf, SIGNAL(foundMountPoint( const QString &, unsigned long, unsigned long, unsigned long )),
                 SLOT(foundMountPoint( const QString &, unsigned long, unsigned long, unsigned long )));

    int count = 0;

    while( m_kBSize == 0 && m_kBAvail == 0){
        usleep( 10000 );
        kapp->processEvents( 100 );
        count++;
        if (count > 120){
            debug() << "KDiskFreeSp taking too long.  Returning false from getCapacity()" << endl;
            return false;
        }
    }

    *total = m_kBSize*1024;
    *available = m_kBAvail*1024;
    unsigned long localsize = m_kBSize;
    m_kBSize = 0;
    m_kBAvail = 0;

    return localsize > 0;
}

void
GenericMediaDevice::foundMountPoint( const QString & mountPoint, unsigned long kBSize, unsigned long /*kBUsed*/, unsigned long kBAvail )
{
    if ( mountPoint == m_medium.mountPoint() ){
        m_kBSize = kBSize;
        m_kBAvail = kBAvail;
    }
}

/// Helper functions

void
GenericMediaDevice::rmbPressed( QListViewItem* qitem, const QPoint& point, int )
{
    enum Actions { APPEND, LOAD, QUEUE,
        DOWNLOAD,
        BURN_DATACD, BURN_AUDIOCD,
        DIRECTORY, RENAME,
        DELETE, TRANSFER_HERE };

    MediaItem *item = static_cast<MediaItem *>(qitem);
    if ( item )
    {
        KPopupMenu menu( m_view );
        menu.insertItem( SmallIconSet( Amarok::icon( "playlist" ) ), i18n( "&Load" ), LOAD );
        menu.insertItem( SmallIconSet( Amarok::icon( "1downarrow" ) ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( SmallIconSet( Amarok::icon( "fastforward" ) ), i18n( "&Queue Tracks" ), QUEUE );
        menu.insertSeparator();
        menu.insertItem( SmallIconSet( Amarok::icon( "collection" ) ), i18n( "&Copy Files to Collection..." ), DOWNLOAD );
        menu.insertItem( SmallIconSet( Amarok::icon( "cdrom_unmount" ) ), i18n( "Burn to CD as Data" ), BURN_DATACD );
        menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
        menu.insertItem( SmallIconSet( Amarok::icon( "cdaudio_unmount" ) ), i18n( "Burn to CD as Audio" ), BURN_AUDIOCD );
        menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );
        menu.insertSeparator();
        menu.insertItem( SmallIconSet( Amarok::icon( "folder" ) ), i18n( "Add Directory" ), DIRECTORY );
        menu.insertItem( SmallIconSet( Amarok::icon( "edit" ) ), i18n( "Rename" ), RENAME );
        menu.insertItem( SmallIconSet( Amarok::icon( "remove" ) ), i18n( "Delete" ), DELETE );
        menu.insertSeparator();
        // NOTE: need better icon
        menu.insertItem( SmallIconSet( Amarok::icon( "add_playlist" ) ), i18n( "Transfer Queue to Here..." ), TRANSFER_HERE );
        menu.setItemEnabled( TRANSFER_HERE, MediaBrowser::queue()->childCount() );

        int id =  menu.exec( point );
        switch( id )
        {
            case LOAD:
                Playlist::instance()->insertMedia( getSelectedItems(), Playlist::Replace );
                break;
            case APPEND:
                Playlist::instance()->insertMedia( getSelectedItems(), Playlist::Append );
                break;
            case QUEUE:
                Playlist::instance()->insertMedia( getSelectedItems(), Playlist::Queue );
                break;
            case DOWNLOAD:
                downloadSelectedItems();
                break;
            case BURN_DATACD:
                K3bExporter::instance()->exportTracks( getSelectedItems(), K3bExporter::DataCD );
                break;
            case BURN_AUDIOCD:
                K3bExporter::instance()->exportTracks( getSelectedItems(), K3bExporter::AudioCD );
                break;

            case DIRECTORY:
                if( item->type() == MediaItem::DIRECTORY )
                    m_view->newDirectory( static_cast<MediaItem*>(item) );
                else
                    m_view->newDirectory( static_cast<MediaItem*>(item->parent()) );
                break;

            case RENAME:
                m_view->rename( item, 0 );
                break;

            case DELETE:
                deleteFromDevice();
                break;

            case TRANSFER_HERE:
                #define item static_cast<GenericMediaItem*>(item)
                if( item->type() == MediaItem::DIRECTORY )
                    m_transferDir = m_mim[item]->getFullName();
                else
                    m_transferDir = m_mim[item]->getParent()->getFullName();
                #undef item
                emit startTransfer();
                break;
        }
        return;
    }

    if( isConnected() )
    {
        KPopupMenu menu( m_view );
        menu.insertItem( SmallIconSet( Amarok::icon( "folder" ) ), i18n("Add Directory" ), DIRECTORY );
        if ( MediaBrowser::queue()->childCount())
        {
            menu.insertSeparator();
            menu.insertItem( SmallIconSet( Amarok::icon( "add_playlist" ) ), i18n(" Transfer queue to here..." ), TRANSFER_HERE );
        }
        int id =  menu.exec( point );
        switch( id )
        {
            case DIRECTORY:
                m_view->newDirectory( 0 );
                break;

            case TRANSFER_HERE:
                m_transferDir = m_medium.mountPoint();
                emit startTransfer();
                break;

        }
    }
}


QString GenericMediaDevice::cleanPath( const QString &component )
{
    QString result = Amarok::cleanPath( component );

    if( m_asciiTextOnly )
        result = Amarok::asciiPath( result );

    result.simplifyWhiteSpace();
    if( m_spacesToUnderscores )
        result.replace( QRegExp( "\\s" ), "_" );
    if( m_actuallyVfat || m_vfatTextOnly )
        result = Amarok::vfatPath( result );

    result.replace( "/", "-" );

    return result;
}

/// File Information functions

bool GenericMediaDevice::isPlayable( const MetaBundle& bundle )
{
    for( QStringList::Iterator it = m_supportedFileTypes.begin(); it != m_supportedFileTypes.end() ; it++ )
    {
        if( bundle.type().lower() == (*it).lower() )
            return true;
    }

    return false;
}


bool GenericMediaDevice::isPreferredFormat( const MetaBundle &bundle )
{
    return m_supportedFileTypes.first().lower() == bundle.type().lower();
}

/// Configuration Dialog Extension

void GenericMediaDevice::addConfigElements( QWidget * parent )
{
    m_configDialog = new GenericMediaDeviceConfigDialog( parent );

    m_configDialog->setDevice( this );
}


void GenericMediaDevice::removeConfigElements( QWidget * /* parent */ )
{
    if( m_configDialog != 0 )
        delete m_configDialog;

    m_configDialog = 0;

}


#include "genericmediadevice.moc"
