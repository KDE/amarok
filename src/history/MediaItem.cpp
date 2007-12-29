/*  Copyright (C) 2005-2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
    (c) 2004 Christian Muehlhaeuser <chris@chris.de>
    (c) 2005-2006 Martin Aumueller <aumuell@reserv.at>
    (c) 2005 Seb Ruiz <ruiz@kde.org>  
    (c) 2006 T.R.Shashwath <trshash84@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "amarok.h"
#include "mediabrowser.h"
#include "MediaItem.h"

#include "kiconloader.h"

#include <QPainter>

QPixmap *MediaItem::s_pixUnknown = 0;
QPixmap *MediaItem::s_pixArtist = 0;
QPixmap *MediaItem::s_pixAlbum = 0;
QPixmap *MediaItem::s_pixFile = 0;
QPixmap *MediaItem::s_pixTrack = 0;
QPixmap *MediaItem::s_pixPodcast = 0;
QPixmap *MediaItem::s_pixPlaylist = 0;
QPixmap *MediaItem::s_pixInvisible = 0;
QPixmap *MediaItem::s_pixStale = 0;
QPixmap *MediaItem::s_pixOrphaned = 0;
QPixmap *MediaItem::s_pixDirectory = 0;
QPixmap *MediaItem::s_pixRootItem = 0;
QPixmap *MediaItem::s_pixTransferFailed = 0;
QPixmap *MediaItem::s_pixTransferBegin = 0;
QPixmap *MediaItem::s_pixTransferEnd = 0;

MediaItem::MediaItem( Q3ListView* parent )
: K3ListViewItem( parent )
{
    init();
}

MediaItem::MediaItem( Q3ListViewItem* parent )
: K3ListViewItem( parent )
{
    init();
}

MediaItem::MediaItem( Q3ListView* parent, Q3ListViewItem* after )
: K3ListViewItem( parent, after )
{
    init();
}

MediaItem::MediaItem( Q3ListViewItem* parent, Q3ListViewItem* after )
: K3ListViewItem( parent, after )
{
    init();
}

MediaItem::~MediaItem()
{
    setBundle( 0 );
}

void
MediaItem::init()
{
    // preload pixmaps used in browser
    KIconLoader iconLoader;
    MediaItem::s_pixUnknown = new QPixmap(iconLoader.loadIcon( Amarok::icon( "unknown" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ));
    MediaItem::s_pixTrack = new QPixmap(iconLoader.loadIcon( Amarok::icon( "view-media-playlist" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ));
    MediaItem::s_pixFile = new QPixmap(iconLoader.loadIcon( Amarok::icon( "audio-x-generic" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    MediaItem::s_pixPodcast = new QPixmap(iconLoader.loadIcon( Amarok::icon( "podcast" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    MediaItem::s_pixPlaylist = new QPixmap(iconLoader.loadIcon( Amarok::icon( "view-media-playlist" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    MediaItem::s_pixRootItem = new QPixmap(iconLoader.loadIcon( Amarok::icon( "files2" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    // history
    // favorites
    // collection
    // folder
    // folder_red
    // player_playlist_2
    // cancel
    // sound
    MediaItem::s_pixArtist = new QPixmap(iconLoader.loadIcon( Amarok::icon( "view-media-artist" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    MediaItem::s_pixAlbum = new QPixmap(iconLoader.loadIcon( Amarok::icon( "media-optical-audio" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    MediaItem::s_pixInvisible = new QPixmap(iconLoader.loadIcon( Amarok::icon( "dialog-cancel" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    MediaItem::s_pixStale = new QPixmap(iconLoader.loadIcon( Amarok::icon( "dialog-cancel" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    MediaItem::s_pixOrphaned = new QPixmap(iconLoader.loadIcon( Amarok::icon( "dialog-cancel" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    MediaItem::s_pixDirectory = new QPixmap(iconLoader.loadIcon( Amarok::icon( "folder" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    MediaItem::s_pixTransferBegin = new QPixmap(iconLoader.loadIcon( Amarok::icon( "play" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    MediaItem::s_pixTransferEnd = new QPixmap(iconLoader.loadIcon( Amarok::icon( "process-stop" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    MediaItem::s_pixTransferFailed = new QPixmap(iconLoader.loadIcon( Amarok::icon( "dialog-cancel" ), KIconLoader::Toolbar, KIconLoader::SizeSmall ) );
    
    m_bundle=0;
    m_order=0;
    m_type=UNKNOWN;
    m_playlistName.clear();
    m_device=0;
    m_flags=0;
    setExpandable( false );
    setDragEnabled( true );
    setDropEnabled( true );
}

void
MediaItem::setBundle( MetaBundle *bundle )
{
    MediaBrowser::instance()->m_itemMapMutex.lock();
    if( m_bundle )
    {
    QString itemUrl = url().url();
        MediaBrowser::ItemMap::iterator it = MediaBrowser::instance()->m_itemMap.find( itemUrl );
        if( it != MediaBrowser::instance()->m_itemMap.end() && *it == this )
            MediaBrowser::instance()->m_itemMap.remove( itemUrl );
    }
    delete m_bundle;
    m_bundle = bundle;

    if( m_bundle )
    {
    QString itemUrl = url().url();
        MediaBrowser::ItemMap::iterator it = MediaBrowser::instance()->m_itemMap.find( itemUrl );
        if( it == MediaBrowser::instance()->m_itemMap.end() )
            MediaBrowser::instance()->m_itemMap[itemUrl] = this;
    }
    MediaBrowser::instance()->m_itemMapMutex.unlock();
    createToolTip();
}

void MediaItem::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    switch( type() )
    {
    case INVISIBLE:
    case PODCASTSROOT:
    case PLAYLISTSROOT:
    case ORPHANEDROOT:
    case STALEROOT:
        {
            QFont font( p->font() );
            font.setBold( true );
            p->setFont( font );
        }
    default:
        break;
    }

    K3ListViewItem::paintCell( p, cg, column, width, align );
}

const MetaBundle *
MediaItem::bundle() const
{
    return m_bundle;
}

KUrl
MediaItem::url() const
{
    if( bundle() )
        return bundle()->url();
    else
        return KUrl();
}

bool
MediaItem::isFileBacked() const
{
    switch( type() )
    {
    case ARTIST:
    case ALBUM:
    case PODCASTSROOT:
    case PODCASTCHANNEL:
    case PLAYLISTSROOT:
    case PLAYLIST:
    case PLAYLISTITEM:
    case INVISIBLEROOT:
    case STALEROOT:
    case STALE:
    case ORPHANEDROOT:
        return false;

    case UNKNOWN:
    case TRACK:
    case ORPHANED:
    case INVISIBLE:
    case PODCASTITEM:
    case DIRECTORY:
        return true;
    }

    return false;
}

long
MediaItem::size() const
{
    if( !isFileBacked() )
        return 0;

    if( bundle() )
        return bundle()->filesize();

    return 0;
}

void
MediaItem::setType( Type type )
{
    m_type=type;

    setDragEnabled(true);
    setDropEnabled(false);

    switch(m_type)
    {
        case UNKNOWN:
            setPixmap(0, *s_pixUnknown);
            break;
        case INVISIBLE:
        case TRACK:
            setPixmap(0, *s_pixFile);
            break;
        case PLAYLISTITEM:
            setPixmap(0, *s_pixTrack);
            setDropEnabled(true);
            break;
        case ARTIST:
            setPixmap(0, *s_pixArtist);
            break;
        case ALBUM:
            setPixmap(0, *s_pixAlbum);
            break;
        case PODCASTSROOT:
            setPixmap(0, *s_pixRootItem);
            break;
        case PODCASTITEM:
        case PODCASTCHANNEL:
            setPixmap(0, *s_pixPodcast);
            break;
        case PLAYLIST:
            setPixmap(0, *s_pixPlaylist);
            setDropEnabled(true);
            break;
        case PLAYLISTSROOT:
            setPixmap(0, *s_pixRootItem);
            setDropEnabled( true );
            break;
        case INVISIBLEROOT:
            setPixmap(0, *s_pixInvisible);
            break;
        case STALEROOT:
        case STALE:
            setPixmap(0, *s_pixStale);
            break;
        case ORPHANEDROOT:
        case ORPHANED:
            setPixmap(0, *s_pixOrphaned);
            break;
        case DIRECTORY:
            setExpandable( true );
            setDropEnabled( true );
            setPixmap(0, *s_pixDirectory);
            break;
    }
    createToolTip();
}

void
MediaItem::setFailed( bool failed )
{
    if( failed )
    {
        m_flags &= ~MediaItem::Transferring;
        m_flags |= MediaItem::Failed;
        setPixmap(0, *MediaItem::s_pixTransferFailed);
    }
    else
    {
        m_flags &= ~MediaItem::Failed;
        if( m_type == PODCASTITEM )
            setPixmap(0, *s_pixPodcast);
        else if( m_type == PLAYLIST )
            setPixmap(0, *s_pixPlaylist);
        else
            setPixmap(0, QPixmap() );
    }
}

MediaItem *
MediaItem::lastChild() const
{
    Q3ListViewItem *last = 0;
    for( Q3ListViewItem *it = firstChild();
            it;
            it = it->nextSibling() )
    {
        last = it;
    }

    return dynamic_cast<MediaItem *>(last);
}

bool
MediaItem::isLeafItem() const
{
    switch(type())
    {
        case UNKNOWN:
            return false;

        case INVISIBLE:
        case TRACK:
        case PODCASTITEM:
        case PLAYLISTITEM:
        case STALE:
        case ORPHANED:
            return true;

        case ARTIST:
        case ALBUM:
        case PODCASTSROOT:
        case PODCASTCHANNEL:
        case PLAYLISTSROOT:
        case PLAYLIST:
        case INVISIBLEROOT:
        case STALEROOT:
        case ORPHANEDROOT:
        case DIRECTORY:
            return false;
    }

    return false;
}

MediaItem *
MediaItem::findItem( const QString &key, const MediaItem *after ) const
{
    MediaItem *it = 0;
    if( after )
        it = dynamic_cast<MediaItem *>( after->nextSibling() );
    else
        it = dynamic_cast<MediaItem *>( firstChild() );

    for( ; it; it = dynamic_cast<MediaItem *>(it->nextSibling()))
    {
        if(key == it->text(0))
            return it;
        if(key.isEmpty() && it->text(0).isEmpty())
            return it;
    }
    return 0;
}

int
MediaItem::compare( Q3ListViewItem *i, int col, bool ascending ) const
{
    MediaItem *item = dynamic_cast<MediaItem *>(i);
    if(item && col==0 && item->m_order != m_order)
        return m_order-item->m_order;
    else if( item && item->type() == MediaItem::ARTIST )
    {
        QString key1 = key( col, ascending );
        if( key1.startsWith( "the ", Qt::CaseInsensitive ) )
            key1 = key1.mid( 4 );
        QString key2 = i->key( col, ascending );
        if( key2.startsWith( "the ", Qt::CaseInsensitive ) )
            key2 = key2.mid( 4 );

       return key1.localeAwareCompare( key2 );
    }

    return K3ListViewItem::compare(i, col, ascending);
}

void
MediaItem::createToolTip()
{
    QString text;
    switch( type() )
    {
        case MediaItem::TRACK:
            {
                const MetaBundle *b = bundle();
                if( b )
                {
                    if( b->track() )
                        text = QString( "%1 - %2 (%3)" )
                            .arg( QString::number(b->track()), b->title(), b->prettyLength() );
                    if( !b->genre().isEmpty() )
                    {
                        if( !text.isEmpty() )
                            text += "<br>";
                        text += QString( "<i>Genre: %1</i>" )
                            .arg( b->genre() );
                    }
                }
            }
            break;
        case MediaItem::PLAYLISTSROOT:
            text = i18n( "Drag items here to create new playlist" );
            break;
        case MediaItem::PLAYLIST:
            text = i18n( "Drag items here to append to this playlist" );
            break;
        case MediaItem::PLAYLISTITEM:
            text = i18n( "Drag items here to insert before this item" );
            break;
        case MediaItem::INVISIBLEROOT:
        case MediaItem::INVISIBLE:
            text = i18n( "Not visible on media device" );
            break;
        case MediaItem::STALEROOT:
        case MediaItem::STALE:
            text = i18n( "In device database, but file is missing" );
            break;
        case MediaItem::ORPHANEDROOT:
        case MediaItem::ORPHANED:
            text = i18n( "File on device, but not in device database" );
            break;
        default:
            break;
    }

    //if( !text.isEmpty() ) setToolTip( text );
}


using namespace Meta;


MediaDeviceTrack::MediaDeviceTrack(const QString & name)
    : Meta::Track()
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
    , m_name( name )
    , m_trackNumber( 0 )
    , m_length( 0 )
    , m_displayUrl( 0 )
    , m_playableUrl( 0 )
    , m_albumName( 0 )
    , m_artistName( 0 )
    , m_type( 0 )
{
}

MediaDeviceTrack::~MediaDeviceTrack()
{
    //nothing to do
}

QString
MediaDeviceTrack::name() const
{
    return m_name;
}

QString
MediaDeviceTrack::prettyName() const
{
    return m_name;
}

KUrl
MediaDeviceTrack::playableUrl() const
{
    KUrl url( m_playableUrl );
    return url;
}

QString
MediaDeviceTrack::url() const
{
    return m_playableUrl;
}

QString
MediaDeviceTrack::prettyUrl() const
{
    return m_displayUrl;
}

bool
MediaDeviceTrack::isPlayable() const
{
    return true;
}

bool
MediaDeviceTrack::isEditable() const
{
    return false;
}

AlbumPtr
MediaDeviceTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
MediaDeviceTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
MediaDeviceTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
MediaDeviceTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
MediaDeviceTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

void
MediaDeviceTrack::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void
MediaDeviceTrack::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist )
}

void
MediaDeviceTrack::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer )
}

void
MediaDeviceTrack::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre )
}

void
MediaDeviceTrack::setYear( const QString &newYear )
{
    Q_UNUSED( newYear )
}

QString
MediaDeviceTrack::comment() const
{
    return QString();
}

void
MediaDeviceTrack::setComment( const QString &newComment )
{
    Q_UNUSED( newComment )
}

double
MediaDeviceTrack::score() const
{
    return 0.0;
}

void
MediaDeviceTrack::setScore( double newScore )
{
    Q_UNUSED( newScore )
}

int
MediaDeviceTrack::rating() const
{
    return 0;
}

void
MediaDeviceTrack::setRating( int newRating )
{
    Q_UNUSED( newRating )
}

int
MediaDeviceTrack::length() const
{
    return m_length;
}

int
MediaDeviceTrack::filesize() const
{
    return 0;
}

int
MediaDeviceTrack::sampleRate() const
{
    return 0;
}

int
MediaDeviceTrack::bitrate() const
{
    return 0;
}

int
MediaDeviceTrack::trackNumber() const
{
    return m_trackNumber;
}

void
MediaDeviceTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
MediaDeviceTrack::discNumber() const
{
    return 0;
}

void
MediaDeviceTrack::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

int
MediaDeviceTrack::playCount() const
{
    return 0;
}

uint
MediaDeviceTrack::lastPlayed() const
{
    return 0;
}

QString
MediaDeviceTrack::filename() const
{
    return m_filename;
}

QString
MediaDeviceTrack::type() const
{
    return m_type;
}

void
MediaDeviceTrack::setAlbum( AlbumPtr album )
{
    m_album = album;
}

void
MediaDeviceTrack::setArtist( ArtistPtr artist )
{
    m_artist = artist;
}

void
MediaDeviceTrack::setGenre( GenrePtr genre )
{
    m_genre = genre;
}

void
MediaDeviceTrack::setComposer( ComposerPtr composer )
{
    m_composer = composer;
}

void
MediaDeviceTrack::setYear( YearPtr year )
{
    m_year = year;
}

void
MediaDeviceTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
MediaDeviceTrack::setLength( int length )
{
    m_length = length;
}

void
MediaDeviceTrack::setFilename( const QString &filename )
{
    m_filename = filename;
}


//MediaDeviceArtist



MediaDeviceArtist::MediaDeviceArtist( const QString & name )
    : Meta::Artist()
    , m_name( name )
    , m_description( 0 )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceArtist::~MediaDeviceArtist()
{
    //nothing to do
}

QString
MediaDeviceArtist::name() const
{
    return m_name;
}

QString
MediaDeviceArtist::prettyName() const
{
    return m_name;
}

void MediaDeviceArtist::setTitle(const QString & title)
{
    m_name = title;
}

TrackList
MediaDeviceArtist::tracks()
{
    return m_tracks;
}

void
MediaDeviceArtist::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}

MediaDeviceAlbum::MediaDeviceAlbum( const QString & name )
    : Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( 0 )
    , m_artistName( 0 )
{
    //nothing to do
}

MediaDeviceAlbum::~MediaDeviceAlbum()
{
    //nothing to do
}

void MediaDeviceAlbum::setArtistName(const QString & name)
{
    m_artistName = name;
}

QString MediaDeviceAlbum::artistName() const
{
    return m_artistName;
}

QString
MediaDeviceAlbum::name() const
{
    return m_name;
}

QString
MediaDeviceAlbum::prettyName() const
{
    return m_name;
}

void MediaDeviceAlbum::setTitle(const QString & title)
{
    m_name = title;
}

bool
MediaDeviceAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
MediaDeviceAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
MediaDeviceAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
MediaDeviceAlbum::tracks()
{
    return m_tracks;
}

void
MediaDeviceAlbum::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}

void
MediaDeviceAlbum::setAlbumArtist( ArtistPtr artist )
{
    m_albumArtist = artist;
}

void
MediaDeviceAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

void MediaDeviceAlbum::setDescription(const QString &description)
{
    m_description = description;
}

QString MediaDeviceAlbum::description() const
{
    return m_description;
}



//MediaDeviceGenre

MediaDeviceGenre::MediaDeviceGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

MediaDeviceGenre::~MediaDeviceGenre()
{
    //nothing to do
}

QString
MediaDeviceGenre::name() const
{
    return m_name;
}

QString
MediaDeviceGenre::prettyName() const
{
    return m_name;
}

void MediaDeviceGenre::setName(const QString & name)
{
    m_name = name;
}


int MediaDeviceGenre::albumId()
{
    return m_albumId;
}

void MediaDeviceGenre::setAlbumId(int albumId)
{
    m_albumId = albumId;
}


TrackList
MediaDeviceGenre::tracks()
{
    return m_tracks;
}

void
MediaDeviceGenre::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}


