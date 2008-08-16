/****************************************************************************
 * copyright            :(C) 2006 Roel Meeuws <r.j.meeuws+amarok@gmail.com  *
 *                       (C) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com> *
 *                       (C) 2005 Seb Ruiz <ruiz@kde.org>                   *
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
//Added by qt3to4:

AMAROK_EXPORT_PLUGIN( GenericMediaDevice )

#include "Amarok.h"
#include "Debug.h"
#include "meta/file/File.h"
#include "collectionbrowser/CollectionTreeItemModel.h"
#include "collection/CollectionManager.h"
#include "collection/SqlStorage.h"
#include "k3bexporter.h"
#include "playlist/PlaylistModel.h"
#include "statusbar/StatusBar.h"
#include "transferdialog.h"
#include "genericmediadeviceconfigdialog.h"

#include <k3popupmenu.h>
#include <kapplication.h>
#include <kconfig.h>           //download saveLocation
#include <kdiskfreespace.h>
#include <kiconloader.h>       //smallIcon
#include <kio/copyjob.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kmountpoint.h>
#include <kmenu.h>
#include <kurlrequester.h>     //downloadSelectedItems()
#include <kurlrequesterdlg.h>  //downloadSelectedItems()

#include <taglib/audioproperties.h>

#include <unistd.h>            //usleep()

#include <QFile>
#include <qstringx.h>
#include <QStringList>

#include <QComboBox>
#include <q3listbox.h>
#include <QLineEdit>

/**
 * GenericMediaItem Class
 */

class GenericMediaItem : public MediaItem
{
    public:
        GenericMediaItem( Q3ListView *parent, Q3ListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

        GenericMediaItem( Q3ListViewItem *parent, Q3ListViewItem *after = 0 )
            : MediaItem( parent, after )
        { }

        // List directories first, always
        int
        compare( Q3ListViewItem *i, int col, bool ascending ) const
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

            if( m_parent )
            {
                if( m_parent == m_device->getInitialFile() )
                    m_viewItem = new GenericMediaItem( m_device->view() );
                else
                    m_viewItem = new GenericMediaItem( m_parent->getViewItem() );
                setNamesFromBase( basename );
                m_viewItem->setText( 0, m_baseName );
                m_parent->getChildren().append( this );
            }
            else
            {
                m_viewItem = 0;
                setNamesFromBase( basename );
            }

            m_device->getItemMap()[m_viewItem] = this;

            if( m_device->getFileMap()[m_fullName] )
            {
                debug() << "Trying to create two GenericMediaFile items with same fullName!";
                debug() << "name already existing: " << m_device->getFileMap()[m_fullName]->getFullName();
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
            m_device->getItemMap().remove( m_viewItem );
            m_device->getFileMap().remove( m_fullName );
            if ( m_viewItem )
                delete m_viewItem;
        }

        GenericMediaFile*
        getParent() { return m_parent; }

        void
        setParent( GenericMediaFile* parent )
        {
            m_device->getFileMap().remove( m_fullName );
            m_parent->getChildren().removeAll( this );
            m_parent = parent;
            if( m_parent )
                m_parent->getChildren().append( this );
            setNamesFromBase( m_baseName );
            m_device->getFileMap()[m_fullName] = this;
        }

        void
        removeChild( GenericMediaFile* childToDelete ) { m_children.removeAll( childToDelete ); }

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
        setNamesFromBase( const QString &name = QString() )
        {
            if( name != QString() )
                m_baseName = name;
            if( m_parent )
                m_fullName = m_parent->getFullName() + '/' + m_baseName;
            else
                m_fullName = m_baseName;
            if( m_viewItem )
                m_viewItem->setMeta( Meta::DataPtr(new MetaFile::Track( KUrl( m_fullName ))) );
        }

        QList<GenericMediaFile*>
        getChildren() { return m_children; }

        void
        deleteAll( bool onlyChildren )
        {
            foreach( GenericMediaFile *vmf, m_children )
                    vmf->deleteAll( true );
            if( onlyChildren )
                delete this;
        }

        void
        renameAllChildren()
        {
            foreach( GenericMediaFile *vmf, m_children )
                vmf->renameAllChildren();
            setNamesFromBase();
        }

    private:
        QString m_fullName;
        QString m_baseName;
        GenericMediaFile *m_parent;
        QList<GenericMediaFile*> m_children;
        GenericMediaItem *m_viewItem;
        GenericMediaDevice* m_device;
        bool m_listed;
};

QString
GenericMediaDevice::fileName( const Meta::TrackPtr track )
{
    QString result = cleanPath( track->artist()->name() );

    if( !result.isEmpty() )
    {
        if( m_spacesToUnderscores )
            result += "_-_";
        else
            result += " - ";
    }

    if( track->trackNumber() )
    {
        result += QString("%1").arg( track->trackNumber(), 2, 10, QChar('0') );

        if( m_spacesToUnderscores )
            result += '_';
        else
            result += ' ';
    }

    result += cleanPath( track->name() + '.' + track->type() );

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

    m_songLocation = QString();
    m_podcastLocation = QString();

    m_supportedFileTypes.clear();

    m_configDialog = 0;

    connect( m_dirLister, SIGNAL( newItems(const KFileItemList &) ), this, SLOT( newItems(const KFileItemList &) ) );
    connect( m_dirLister, SIGNAL( completed() ), this, SLOT( dirListerCompleted() ) );
    connect( m_dirLister, SIGNAL( clear() ), this, SLOT( dirListerClear() ) );
    connect( m_dirLister, SIGNAL( clear(const KUrl &) ), this, SLOT( dirListerClear(const KUrl &) ) );
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

    m_supportedFileTypes = configString( "supportedFiletypes", "mp3" ).split( ", " );
}

bool
GenericMediaDevice::openDevice( bool /*silent*/ )
{
    DEBUG_BLOCK
    if( m_mountPoint.isEmpty() )
    {
        The::statusBar()->longMessage( i18n( "Devices handled by this plugin must be mounted first.\n"
                                                          "Please mount the device and click \"Connect\" again." ),
                                                    KDE::StatusBar::Sorry );
        return false;
    }

    KMountPoint::List currentmountpoints = KMountPoint::currentMountPoints();
    KMountPoint::List::Iterator mountiter = currentmountpoints.begin();
    for(; mountiter != currentmountpoints.end(); ++mountiter)
    {
        if( m_mountPoint == (*mountiter)->mountPoint() )
            m_fsType = (*mountiter)->mountType();
    }
    m_actuallyVfat = ( m_fsType == "msdosfs" || m_fsType =="vfat" )
        ? true : false;
    m_connected = true;
    KUrl tempurl = KUrl( m_mountPoint );
    QString newMountPoint = tempurl.isLocalFile() ? tempurl.path( KUrl::RemoveTrailingSlash ) : tempurl.prettyUrl( KUrl::RemoveTrailingSlash ); //no trailing slash
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
GenericMediaDevice::renameItem( Q3ListViewItem *item ) // SLOT
{

    if( !item )
        return;

    #define item static_cast<GenericMediaItem*>(item)

    QString src = m_mim[item]->getFullName();
    QString dst = m_mim[item]->getParent()->getFullName() + '/' + QFile::encodeName( item->text(0) );

    debug() << "Renaming: " << src << " to: " << dst;

    KIO::CopyJob* job = KIO::move( KUrl(src), KUrl(dst), KIO::HideProgressInfo );
    if( KIO::NetAccess::synchronousRun( job, Amarok::mainWindow() ) )
    {
        m_mfm.remove( m_mim[item]->getFullName() );
        m_mim[item]->setNamesFromBase( item->text(0) );
        m_mfm[m_mim[item]->getFullName()] = m_mim[item];
    }
    else
    {
        debug() << "Renaming FAILED!";
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
    QString dirPath = QFile::encodeName( fullPath );
    debug() << "Creating directory: " << dirPath;
    const KUrl url( dirPath );

    if( !KIO::NetAccess::mkdir( url, m_parent ) ) //failed
    {
        debug() << "Failed to create directory " << dirPath;
        return 0;
    }

    refreshDir( m_mim[parent]->getFullName() );

    #undef parent

    return 0;
}

void
GenericMediaDevice::addToDirectory( MediaItem *directory, QList<MediaItem*> items )
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

    foreach( MediaItem* item, items )
    {
        GenericMediaItem *currItem = static_cast<GenericMediaItem *>(item);
        QString src  = m_mim[currItem]->getFullName();
        QString dst = dropDir->getFullName() + '/' + QFile::encodeName( currItem->text(0) );
        debug() << "Moving: " << src << " to: " << dst;

        KIO::CopyJob* job = KIO::move( KUrl( src ), KUrl( dst ), KIO::HideProgressInfo );
        if( !KIO::NetAccess::synchronousRun( job, Amarok::mainWindow() ) )
             debug() << "Failed moving " << src << " to " << dst;
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
GenericMediaDevice::buildDestination( const QString &format, const Meta::TrackPtr track )
{
    bool isCompilation = track->album()->isCompilation();
    QMap<QString, QString> args;
    QString artist = track->artist()->name();
    QString albumartist = artist;
    if( isCompilation )
        albumartist = i18n( "Various Artists" );
    args["theartist"] = cleanPath( artist );
    args["thealbumartist"] = cleanPath( albumartist );
    if( m_ignoreThePrefix && artist.startsWith( "The " ) )
        Amarok::manipulateThe( artist, true );
    artist = cleanPath( artist );
    if( m_ignoreThePrefix && albumartist.startsWith( "The " ) )
        Amarok::manipulateThe( albumartist, true );

    albumartist = cleanPath( albumartist );

    //these additional columns from MetaBundle were used before but haven't
    //been ported yet. Do they need to be?
    //Bpm,Directory,Bitrate,SampleRate,Mood
    args["title"] = cleanPath( track->prettyName() );
    args["composer"] = cleanPath( track->composer()->prettyName() );
    args["year"] = cleanPath( track->year()->prettyName() );
    args["album"] = cleanPath( track->album()->prettyName() );
    args["discnumber"] = QString::number( track->discNumber() );
    args["genre"] = cleanPath( track->genre()->prettyName() );
    args["comment"] = cleanPath( track->comment() );
    args["artist"] = artist;
    args["albumartist"] = albumartist;
    args["initial"] = albumartist.mid( 0, 1 ).toUpper();
    args["filetype"] = track->type();
    args["rating"] = track->rating();
    args["filesize"] = track->filesize();
    args["length"] = track->length();
    QString trackNum;
    if ( track->trackNumber() )
        trackNum = QString("%1").arg( track->trackNumber(), 2, 10, QChar('0') );
    args["track"] = trackNum;

    Amarok::QStringx formatx( format );
    QString result = formatx.namedOptArgs( args );
    if( !result.startsWith( '/' ) )
        result.prepend( "/" );

   return result.replace( QRegExp( "/\\.*" ), "/" );
}

void
GenericMediaDevice::checkAndBuildLocation( const QString& location )
{
    // check for every directory from the mount point to the location
    // whether they exist or not.
    int mountPointDepth = m_mountPoint.count( '/' );
    int locationDepth = location.count( '/' );

    if( m_mountPoint.endsWith( '/' ) )
        mountPointDepth--;

    if( location.endsWith( '/' ) )
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
        KUrl url = KUrl( QFile::encodeName( firstpart + '/' + secondpart ) );

        KIO::UDSEntry udsentry;
        if( !KIO::NetAccess::stat( url, udsentry, Amarok::mainWindow() ) )
        {
            debug() << "directory does not exist, creating..." << url;
            if( !KIO::NetAccess::mkdir(url, m_view ) ) //failed
            {
                debug() << "Failed to create directory " << url;
                return;
            }
        }
    }
}
/* //PORT to meta
QString
GenericMediaDevice::buildPodcastDestination( const PodcastEpisodeBundle *bundle )
{
    QString location = m_podcastLocation.endsWith('/') ? m_podcastLocation : m_podcastLocation + '/';
    // get info about the PodcastChannel
    QString parentUrl = bundle->parent().url();
    QString sql = "SELECT title,parent FROM podcastchannels WHERE url='" + CollectionManager::instance()->sqlStorage()->escape( parentUrl ) + "';";
    QStringList values = CollectionManager::instance()->sqlStorage()->query( sql );
    QString channelTitle;
    int parent = 0;
    channelTitle = values.first();
    parent = values.last().toInt();
    // Put the file in a directory tree like in the playlistbrowser
    sql = "SELECT name,parent FROM podcastfolders WHERE id=%1;";
    QString name;
    while ( parent > 0 )
    {
        values = CollectionManager::instance()->sqlStorage()->query( sql.arg( parent ) );
        name    =    values.first();
        parent  =   values.last().toInt();
        location += cleanPath( name ) + '/';
    }
    location += cleanPath( channelTitle ) + '/' + cleanPath( bundle->localUrl().fileName() );
    return location;
}
*/

MediaItem *
GenericMediaDevice::copyTrackToDevice( const Meta::TrackPtr track )
{
    if( !m_connected ) return 0;

    // use different naming schemes for differen kinds of tracks
    QString path = m_transferDir;
    //TODO podcast port to meta
    //debug() << "bundle exists: " << bundle.podcastBundle();
    //if( bundle.podcastBundle() )
    //    path += buildPodcastDestination( bundle.podcastBundle() );
    //else
        path += buildDestination( m_songLocation, track );

    checkAndBuildLocation( path );

    const KUrl desturl = KUrl( path );

    //kapp->processEvents();

    if( !kioCopyTrack( track->url(), desturl ) )
    {
        debug() << "Failed to copy track: " << track->url() << " to " << desturl.pathOrUrl();
        return 0;
    }

    refreshDir( m_transferDir );

    //the return value just can't be null, as nothing is done with it
    //other than to see if it is NULL or not
    //if we're here the transfer shouldn't have failed, so we shouldn't get into a loop by waiting...
    while( !m_view->firstChild() )
        kapp->processEvents();
    return static_cast<MediaItem*>(m_view->firstChild());
}

//Somewhat related...

MediaItem *
GenericMediaDevice::trackExists( const Meta::TrackPtr track )
{
    QString key;
    QString path = buildDestination( m_songLocation, track);
    KUrl url( path );
    url.adjustPath( KUrl::RemoveTrailingSlash );
    QStringList directories = url.directory().split( '/', QString::SkipEmptyParts );

    Q3ListViewItem *it = view()->firstChild();
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

    key = url.fileName();
    key = key.isEmpty() ? fileName( track ) : key;
    while( it && it->text( 0 ) != key )
        it = it->nextSibling();

    return dynamic_cast<MediaItem *>( it );
}

/// File transfer methods


void
GenericMediaDevice::downloadSelectedItems()
{
    KUrl::List urls = getSelectedItems();

//PORT 2.0    CollectionView::instance()->organizeFiles( urls, i18n("Copy Files to Collection"), true );

    hideProgress();
}

KUrl::List
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
    debug() << "Deleting path: " << path;

    if ( !KIO::NetAccess::del( KUrl(path), m_view ))
    {
        debug() << "Could not delete!";
        return -1;
    }

    if( m_mim[item] == m_initialFile )
    {
        m_mim[item]->deleteAll( false );
        debug() << "Not deleting root directory of mount!";
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
GenericMediaDevice::expandItem( Q3ListViewItem *item ) // SLOT
{
    if( !item || !item->isExpandable() ) return;

    #define item static_cast<GenericMediaItem *>(item)
    m_dirListerComplete = false;
    listDir( m_mim[item]->getFullName() );
    #undef item

    while( !m_dirListerComplete )
    {
        kapp->processEvents();
        usleep(10000);
    }
}

void
GenericMediaDevice::listDir( const QString &dir )
{
    m_dirListerComplete = false;
    if( m_mfm[dir]->getListed() )
        m_dirLister->updateDirectory( KUrl(dir) );
    else
    {
        m_dirLister->openUrl( KUrl(dir), KDirLister::Keep | KDirLister::Reload );
        m_mfm[dir]->setListed( true );
    }
}

void
GenericMediaDevice::refreshDir( const QString &dir )
{
    m_dirListerComplete = false;
    m_dirLister->updateDirectory( KUrl(dir) );
}

void
GenericMediaDevice::newItems( const KFileItemList &items )
{
    foreach( const KFileItem &kfi, items )
        addTrackToList( kfi.isFile() ? MediaItem::TRACK : MediaItem::DIRECTORY, kfi.url(), 0 );
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

    KUrl tempurl = KUrl( m_mountPoint );
    QString newMountPoint = tempurl.isLocalFile() ? tempurl.path( KUrl::RemoveTrailingSlash ) : tempurl.prettyUrl( KUrl::RemoveTrailingSlash  ); //no trailing slash
    m_initialFile = new GenericMediaFile( 0, newMountPoint, this );
}

void
GenericMediaDevice::dirListerClear( const KUrl &url )
{
    QString directory = url.pathOrUrl();
    GenericMediaFile *vmf = m_mfm[directory];
    if( vmf )
        vmf->deleteAll( false );
}

void
GenericMediaDevice::dirListerDeleteItem( KFileItem *fileitem )
{
    QString filename = fileitem->url().pathOrUrl();
    GenericMediaFile *vmf = m_mfm[filename];
    if( vmf )
        vmf->deleteAll( true );
}

int
GenericMediaDevice::addTrackToList( int type, KUrl url, int /*size*/ )
{
    QString path = url.isLocalFile() ? url.path( KUrl::RemoveTrailingSlash ) : url.prettyUrl( KUrl::RemoveTrailingSlash ); //no trailing slash
    int index = path.lastIndexOf( '/', -1 );
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
        if( baseName.endsWith( "mp3", Qt::CaseInsensitive ) || baseName.endsWith( "wma", Qt::CaseInsensitive ) ||
            baseName.endsWith( "wav", Qt::CaseInsensitive ) || baseName.endsWith( "ogg", Qt::CaseInsensitive ) ||
            baseName.endsWith( "asf", Qt::CaseInsensitive ) || baseName.endsWith( "flac", Qt::CaseInsensitive ) ||
            baseName.endsWith( "aac", Qt::CaseInsensitive ) || baseName.endsWith( "m4a", Qt::CaseInsensitive ) ||
            baseName.endsWith( "oga", Qt::CaseInsensitive ) )

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
    if( !m_connected || !KUrl( m_mountPoint ).isLocalFile() ) return false;

    KDiskFreeSpace* kdf = new KDiskFreeSpace( m_parent );
    kdf->readDF( m_mountPoint );
    connect(kdf, SIGNAL(foundMountPoint( const QString &, quint64, quint64, quint64 )),
                 SLOT(foundMountPoint( const QString &, quint64, quint64, quint64 )));

    int count = 0;

    while( m_kBSize == 0 && m_kBAvail == 0){
        usleep( 10000 );
        kapp->processEvents();
        count++;
        if (count > 120){
            debug() << "KDiskFreeSpace taking too long.  Returning false from getCapacity()";
            return false;
        }
    }

    *total = m_kBSize*1024;
    *available = m_kBAvail*1024;
    quint64 localsize = m_kBSize;
    m_kBSize = 0;
    m_kBAvail = 0;

    return localsize > 0;
}

void
GenericMediaDevice::foundMountPoint( const QString & mountPoint, quint64 kBSize, quint64 /*kBUsed*/, quint64 kBAvail )
{
    if ( mountPoint == m_mountPoint ){
        m_kBSize = kBSize;
        m_kBAvail = kBAvail;
    }
}

/// Helper functions

void
GenericMediaDevice::rmbPressed( Q3ListViewItem* qitem, const QPoint& point, int )
{
#ifdef DELETE
#undef DELETE
#endif
    enum Actions { APPEND, LOAD, QUEUE,
        DOWNLOAD,
        BURN_DATACD, BURN_AUDIOCD,
        DIRECTORY, RENAME,
        DELETE, TRANSFER_HERE };

    MediaItem *item = static_cast<MediaItem *>(qitem);
    if ( item )
    {
        K3PopupMenu menu( m_view );
        menu.insertItem( KIcon( "view-media-playlist-amarok" ), i18nc( "Replace the current playlist with this","&Load" ), LOAD );
        menu.insertItem( KIcon( "1downarrow" ), i18n( "&Append to Playlist" ), APPEND );
        menu.insertItem( KIcon( "media-seek-forward-amarok" ), i18n( "&Queue Tracks" ), QUEUE );
        menu.insertSeparator();
        menu.insertItem( KIcon( "collection-amarok" ), i18n( "&Copy Files to Collection..." ), DOWNLOAD );
        menu.insertItem( KIcon( "cdrom_unmount" ), i18n( "Burn to CD as Data" ), BURN_DATACD );
        menu.setItemEnabled( BURN_DATACD, K3bExporter::isAvailable() );
        menu.insertItem( KIcon( "cdaudio_unmount" ), i18n( "Burn to CD as Audio" ), BURN_AUDIOCD );
        menu.setItemEnabled( BURN_AUDIOCD, K3bExporter::isAvailable() );
        menu.insertSeparator();
        menu.insertItem( KIcon( "folder" ), i18n( "Add Directory" ), DIRECTORY );
        menu.insertItem( KIcon( "edit-rename-amarok" ), i18n( "Rename" ), RENAME );
        menu.insertItem( KIcon( "edit-delete-amarok" ), i18n( "Delete" ), DELETE );
        menu.insertSeparator();
        // NOTE: need better icon
        menu.insertItem( KIcon( "list-add-amarok" ), i18n( "Transfer Queue to Here..." ), TRANSFER_HERE );
        menu.setItemEnabled( TRANSFER_HERE, MediaBrowser::queue()->childCount() );

        int id =  menu.exec( point );
        Meta::TrackList tracks = CollectionManager::instance()->tracksForUrls( getSelectedItems() );
        switch( id )
        {
            case LOAD:
                The::playlistModel()->insertOptioned( tracks, Playlist::Replace );
                break;
            case APPEND:
                The::playlistModel()->insertOptioned( tracks, Playlist::Append );
                break;
            case QUEUE:
                The::playlistModel()->insertOptioned( tracks, Playlist::Queue );
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
        K3PopupMenu menu( m_view );
        menu.insertItem( KIcon( "folder" ), i18n("Add Directory" ), DIRECTORY );
        if ( MediaBrowser::queue()->childCount())
        {
            menu.insertSeparator();
            menu.insertItem( KIcon( "list-add-amarok" ), i18n(" Transfer queue to here..." ), TRANSFER_HERE );
        }
        int id =  menu.exec( point );
        switch( id )
        {
            case DIRECTORY:
                m_view->newDirectory( 0 );
                break;

            case TRANSFER_HERE:
                m_transferDir = m_mountPoint;
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

    result.simplified();
    if( m_spacesToUnderscores )
        result.replace( QRegExp( "\\s" ), "_" );
    if( m_actuallyVfat || m_vfatTextOnly )
        result = Amarok::vfatPath( result );

    result.replace( '/', '-' );

    return result;
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
