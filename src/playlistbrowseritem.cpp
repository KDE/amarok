/***************************************************************************
 * copyright            : (c) 2004 Pierpaolo Di Panfilo                    *
 *                        (c) 2004 Mark Kretschmann <markey@web.de>        *
 *                        (c) 2005-2006 Seb Ruiz <me@sebruiz.net>          *
 *                        (c) 2005 Christian Muehlhaeuser <chris@chris.de> *
 * See COPYING file for licensing information                              *
 ***************************************************************************/

#include "amarok.h"
#include "collectiondb.h"
#include "debug.h"
#include "dynamicmode.h"
#include "playlistbrowser.h"
#include "playlistbrowseritem.h"
#include "playlistloader.h"    //load()
#include "podcastbundle.h"
#include "podcastsettings.h"
#include "metabundle.h"
#include "statusbar.h"
#include "threadweaver.h"
#include "mediabrowser.h"

#include <qlabel.h>
#include <qpainter.h>          //paintCell()
#include <qpixmap.h>           //paintCell()
#include <qregexp.h>

#include <kdeversion.h>        //KDE_VERSION ifndefs.  Remove this once we reach a kde 4 dep
#include <kiconloader.h>       //smallIcon
#include <kio/jobclasses.h>    //podcast retrieval
#include <kio/job.h>           //podcast retrieval
#include <klocale.h>
#include <kmessagebox.h>       //podcast info box
#include <kmimetype.h>
#include <kstandarddirs.h>     //podcast loading icons
#include <kstringhandler.h>

/////////////////////////////////////////////////////////////////////////////
///    CLASS PlaylistReader
////////////////////////////////////////////////////////////////////////////

class PlaylistReader : public ThreadWeaver::DependentJob
{
    public:
        PlaylistReader( QObject *recipient, const QString &path )
                : ThreadWeaver::DependentJob( recipient, "PlaylistReader" )
                , m_path( path ) {}

        virtual bool doJob() {
            bundles = PlaylistFile( m_path ).bundles();
            return true;
        }

        BundleList bundles;

    private:
        const QString m_path;
};

/////////////////////////////////////////////////////////////////////////////
///    CLASS PlaylistBrowserEntry
////////////////////////////////////////////////////////////////////////////

int
PlaylistBrowserEntry::compare( QListViewItem* item, int col, bool ascending ) const
{
    bool i1 = rtti() == PlaylistCategory::RTTI;
    bool i2 = item->rtti() == PlaylistCategory::RTTI;

    // If only one of them is a category, make it show up before
    if ( i1 != i2 )
        return i1 ? -1 : 1;
    else if ( i1 ) //both are categories
    {
        PlaylistBrowser * const pb = PlaylistBrowser::instance();

        QValueList<PlaylistCategory*> toplevels; //define a static order for the toplevel categories
        toplevels << pb->m_playlistCategory
                  << pb->m_smartCategory
                  << pb->m_dynamicCategory
                  << pb->m_streamsCategory
                  << pb->m_podcastCategory;

        for( int i = 0, n = toplevels.count(); i < n; ++i )
        {
            if( this == toplevels[i] )
                return ascending ? -1 : 1; //same order whether or not it's ascending
            if( item == toplevels[i] )
                return ascending ? 1 : -1;
        }
    }

    return KListViewItem::compare(item, col, ascending);
}



/////////////////////////////////////////////////////////////////////////////
///    CLASS PlaylistCategory
////////////////////////////////////////////////////////////////////////////

PlaylistCategory::PlaylistCategory( QListView *parent, QListViewItem *after, const QString &t, bool isFolder )
    : PlaylistBrowserEntry( parent, after )
    , m_title( t )
    , m_id( -1 )
    , m_folder( isFolder )
{
    setDragEnabled( false );
    setRenameEnabled( 0, isFolder );
    setPixmap( 0, SmallIcon("folder_red") );
    setText( 0, t );
}


PlaylistCategory::PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, const QString &t, bool isFolder )
    : PlaylistBrowserEntry( parent, after )
    , m_title( t )
    , m_id( -1 )
    , m_folder( isFolder )
{
    setDragEnabled( false );
    setRenameEnabled( 0, isFolder );
    setPixmap( 0, SmallIcon("folder") );
    setText( 0, t );
}


PlaylistCategory::PlaylistCategory( QListView *parent, QListViewItem *after, const QDomElement &xmlDefinition, bool isFolder )
    : PlaylistBrowserEntry( parent, after )
    , m_id( -1 )
    , m_folder( isFolder )
{
    setXml( xmlDefinition );
    setDragEnabled( false );
    setRenameEnabled( 0, isFolder );
    setPixmap( 0, SmallIcon("folder_red") );
}


PlaylistCategory::PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, const QDomElement &xmlDefinition )
    : PlaylistBrowserEntry( parent, after )
    , m_id( -1 )
    , m_folder( true )
{
    setXml( xmlDefinition );
    setDragEnabled( false );
    setRenameEnabled( 0, true );
    setPixmap( 0, SmallIcon("folder") );
}

PlaylistCategory::PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, const QString &t, const int id )
    : PlaylistBrowserEntry( parent, after )
    , m_title( t )
    , m_id( id )
    , m_folder( true )
{
    setDragEnabled( false );
    setRenameEnabled( 0, true );
    setPixmap( 0, SmallIcon("folder") );
    setText( 0, t );
}

void PlaylistCategory::okRename( int col )
{
    QListViewItem::okRename( col );

    if( m_id < 0 )  return;

    // update the database entry to have the correct name
    const int parentId = parent() ? static_cast<PlaylistCategory*>(parent())->id() : 0;
    CollectionDB::instance()->updatePodcastFolder( m_id, text(0), parentId, isOpen() );
}

void PlaylistCategory::setXml( const QDomElement &xml )
{
    PlaylistBrowser *pb = PlaylistBrowser::instance();
    QString tname = xml.tagName();
    if ( tname == "category" )
    {
        setOpen( xml.attribute( "isOpen" ) == "true" );
        m_title = xml.attribute( "name" );
        setText( 0, m_title );
        QListViewItem *last = 0;
        for( QDomNode n = xml.firstChild() ; !n.isNull(); n = n.nextSibling() )
        {
            QDomElement e = n.toElement();
            if ( e.tagName() == "category" ) {
                last = new PlaylistCategory( this, last, e);
            }
            else if ( e.tagName() == "default" ) {
                if( e.attribute( "type" ) == "stream" )
                    pb->m_coolStreamsOpen   = (e.attribute( "isOpen" ) == "true");
                if( e.attribute( "type" ) == "smartplaylist" )
                    pb->m_smartDefaultsOpen = (e.attribute( "isOpen" ) == "true");
                continue;
            }
            else if ( e.tagName() == "stream" ) {
                last = new StreamEntry( this, last, e );
            }
            else if ( e.tagName() == "smartplaylist" ) {
                last = new SmartPlaylist( this, last, e );
            }
            else if ( e.tagName() == "playlist" ) {
                last = new PlaylistEntry( this, last, e );
            }
            else if ( e.tagName() == "dynamic" ) {
                if ( e.attribute( "name" ) == i18n("Random Mix") || e.attribute( "name" ) == i18n("Suggested Songs" ) )
                    continue;
                last = new DynamicEntry( this, last, e );
            }
            else if ( e.tagName() == "podcast" )
            {
                const KURL url( n.namedItem( "url").toElement().text() );
                QString xmlLocation = amaroK::saveLocation( "podcasts/" );
                xmlLocation        += n.namedItem( "cache" ).toElement().text();

                QDomDocument xml;
                QFile xmlFile( xmlLocation );
                QTextStream stream( &xmlFile );
                stream.setEncoding( QTextStream::UnicodeUTF8 );

                if( !xmlFile.open( IO_ReadOnly ) || !xml.setContent( stream.read() ) )
                {
                    // Invalid podcasts should still be added to the browser, which means there is no cached xml.
                    last = new PodcastChannel( this, last, url, n );
                }
                else
                    last = new PodcastChannel( this, last, url, n, xml );

                #define item static_cast<PodcastChannel*>(last)
                if( item->autoscan() )
                    pb->m_podcastItemsToScan.append( item );
                #undef  item
            }
            else if ( e.tagName() == "settings" )
            {
                PlaylistBrowser::instance()->registerPodcastSettings(  title(), new PodcastSettings( e, title() ) );
            }
            if( !e.attribute( "isOpen" ).isNull() ) last->setOpen( e.attribute( "isOpen" ) == "true" ); //settings doesn't have an attribute "isOpen"
        }
        setText( 0, xml.attribute("name") );
    }
}


QDomElement PlaylistCategory::xml()
{
        QDomDocument d;
        QDomElement i = d.createElement("category");
        i.setAttribute( "name", text(0) );
        if( isOpen() )
            i.setAttribute( "isOpen", "true" );
        for( PlaylistBrowserEntry *it = static_cast<PlaylistBrowserEntry*>( firstChild() ); it; it = static_cast<PlaylistBrowserEntry*>( it->nextSibling() ) )
        {
            if( it == PlaylistBrowser::instance()->m_coolStreams )
            {
                QDomDocument doc;
                QDomElement e = doc.createElement("default");
                e.setAttribute( "type", "stream" );
                if( it->isOpen() )
                    e.setAttribute( "isOpen", "true" );
                i.appendChild( d.importNode( e, true ) );
            }
            else if( it == PlaylistBrowser::instance()->m_smartDefaults )
            {
                QDomDocument doc;
                QDomElement e = doc.createElement("default");
                e.setAttribute( "type", "smartplaylist" );
                if( it->isOpen() )
                    e.setAttribute( "isOpen", "true" );
                i.appendChild( d.importNode( e, true ) );
            }
            else if( it != PlaylistBrowser::instance()->m_randomDynamic &&
                     it != PlaylistBrowser::instance()->m_suggestedDynamic )
                i.appendChild( d.importNode( it->xml(), true ) );
        }
        return i;
}

void
PlaylistCategory::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    QFont font( p->font() );

    if( !m_folder ) {
        font.setBold( true );
    }

    p->setFont( font );

    KListViewItem::paintCell( p, cg, column, width, align );
}


/////////////////////////////////////////////////////////////////////////////
///    CLASS PlaylistEntry
////////////////////////////////////////////////////////////////////////////

PlaylistEntry::PlaylistEntry( QListViewItem *parent, QListViewItem *after, const KURL &url, int tracks, int length )
    : PlaylistBrowserEntry( parent, after )
    , m_url( url )
    , m_length( length )
    , m_trackCount( tracks )
    , m_loading( false )
    , m_loaded( false )
    , m_dynamic( false )
    , m_dynamicPix( 0 )
    , m_loadingPix( 0 )
    , m_lastTrack( 0 )
{
    m_trackList.setAutoDelete( true );
    tmp_droppedTracks.setAutoDelete( false );

    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setExpandable(true);

    setText(0, fileBaseName( url.path() ) );
    setPixmap( 0, SmallIcon( amaroK::icon( "playlist" ) ) );

    if( !m_trackCount )
        load();   //load the playlist file
}


PlaylistEntry::PlaylistEntry( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition )
    : PlaylistBrowserEntry( parent, after )
    , m_loading( false )
    , m_loaded( false )
    , m_dynamic( false )
    , m_dynamicPix( 0 )
    , m_loadingPix( 0 )
    , m_lastTrack( 0 )
{
    m_url.setPath( xmlDefinition.attribute( "file" ) );
    m_trackCount = xmlDefinition.namedItem( "tracks" ).toElement().text().toInt();
    m_length = xmlDefinition.namedItem( "length" ).toElement().text().toInt();

    m_trackList.setAutoDelete( true );
    tmp_droppedTracks.setAutoDelete( false );

    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setExpandable(true);

    setText(0, fileBaseName( m_url.path() ) );
    setPixmap( 0, SmallIcon( amaroK::icon( "playlist" ) ) );


    if( !m_trackCount )
        load();   //load the playlist file
}


PlaylistEntry::~PlaylistEntry()
{
    m_trackList.clear();
    tmp_droppedTracks.setAutoDelete( true );
    tmp_droppedTracks.clear();
}

void PlaylistEntry::load()
{
    if ( m_loading )
	    return;
    m_trackList.clear();
    m_length = 0;
    m_loaded = false;
    m_loading = true;
    //starts loading animation
    static_cast<PlaylistBrowserView *>(listView())->startAnimation( this );

    //delete all children, so that we don't duplicate things
    while( firstChild() )
        delete firstChild();

     //read the playlist file in a thread
    ThreadWeaver::instance()->queueJob( new PlaylistReader( this, m_url.path() ) );
}

void PlaylistEntry::insertTracks( QListViewItem *after, KURL::List list )
{
    QValueList<MetaBundle> bundles;

    foreachType( KURL::List, list )
        bundles += MetaBundle( *it );

    insertTracks( after, bundles );
}

void PlaylistEntry::insertTracks( QListViewItem *after, QValueList<MetaBundle> bundles )
{
    int pos = 0;
    if( after ) {
        pos = m_trackList.find( static_cast<PlaylistTrackItem*>(after)->trackInfo() ) + 1;
        if( pos == -1 )
            return;
    }

    uint k = 0;
    foreachType( QValueList<MetaBundle>, bundles )
    {
        TrackItemInfo *newInfo = new TrackItemInfo( (*it).url(), (*it).title(), (*it).length() );
        m_length += newInfo->length();
        m_trackCount++;

        if( after ) {
            m_trackList.insert( pos+k, newInfo );
            if( isOpen() )
                after = new PlaylistTrackItem( this, after, newInfo );
        }
        else {
            if( m_loaded && !m_loading ) {
                m_trackList.append( newInfo );
                if( isOpen() )  //append the track item to the playlist
                    m_lastTrack = new PlaylistTrackItem( this, m_lastTrack, newInfo );
            }
            else
                tmp_droppedTracks.append( newInfo );
        }
        ++k;
    }

    if( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW && !isOpen() )
        listView()->repaintItem( this ); //update the info count, don't repaing if open, since new PlaylistTrackItem will do this.

    if ( !m_loading ) {
        PlaylistBrowser::instance()->savePlaylist( this );
        if ( !m_loaded )
            tmp_droppedTracks.clear(); // after saving, dropped tracks are on the file
    }
}


void PlaylistEntry::removeTrack( QListViewItem *item, bool isLast )
{
    #define item static_cast<PlaylistTrackItem*>(item)
    //remove a track and update playlist stats
    TrackItemInfo *info = item->trackInfo();
    m_length -= info->length();
    m_trackCount--;
    m_trackList.remove( info );
    if( item == m_lastTrack ) {
        QListViewItem *above = item->itemAbove();
        m_lastTrack = above ? static_cast<PlaylistTrackItem *>( above ) : 0;
    }
    delete item;

    #undef item

    if( isLast )
        PlaylistBrowser::instance()->savePlaylist( this );
}


void PlaylistEntry::customEvent( QCustomEvent *e )
{
    if( e->type() == (int)PlaylistReader::JobFinishedEvent )
    {
        foreachType( BundleList, static_cast<PlaylistReader*>(e)->bundles ) {
           const MetaBundle &b = *it;
           TrackItemInfo *info = new TrackItemInfo( b.url(), b.title(), b.length() );
           m_trackList.append( info );
           m_length += info->length();
           if( isOpen() )
               m_lastTrack = new PlaylistTrackItem( this, m_lastTrack, info );
        }

        //the tracks dropped on the playlist while it wasn't loaded are added to the track list
        if( tmp_droppedTracks.count() ) {

            for ( TrackItemInfo *info = tmp_droppedTracks.first(); info; info = tmp_droppedTracks.next() ) {
                m_trackList.append( info );
            }
            tmp_droppedTracks.clear();
        }

        m_loading = false;
        m_loaded = true;
        static_cast<PlaylistBrowserView *>(listView())->stopAnimation( this );  //stops the loading animation

        if( m_trackCount && !m_dynamic && !isDynamic() ) setOpen( true );
        else listView()->repaintItem( this );

        m_trackCount = m_trackList.count();

        PlaylistBrowser::instance()->savePlaylist( this );
    }
}

/**
 *  We destroy the tracks on collapsing the entry.  However, if we are using dynamic mode, then we leave them
 *  because adding from a custom list is problematic if the entry has no children.  Using load() is not effective
 *  since this is a threaded operation and would require pulling apart the entire class to make it work.
 */

void PlaylistEntry::setOpen( bool open )
{
    if( open == isOpen())
        return;

    if( open ) {    //expand

        if( m_loaded ) {
            //create track items
            for ( TrackItemInfo *info = m_trackList.first(); info; info = m_trackList.next() )
                m_lastTrack = new PlaylistTrackItem( this, m_lastTrack, info );
        }
        else if( !isDynamic() || !m_dynamic ) {
            load();
            return;
        }
    }
    else if( !isDynamic() || !m_dynamic ) {    //collapse

        //delete all children
        while( firstChild() )
            delete firstChild();

        m_lastTrack = 0;
    }

    QListViewItem::setOpen( open );
}


int PlaylistEntry::compare( QListViewItem* i, int /*col*/ ) const
{
    PlaylistEntry* item = static_cast<PlaylistEntry*>(i);

    // Compare case-insensitive
    return QString::localeAwareCompare( text( 0 ).lower(), item->text( 0 ).lower() );
}


KURL::List PlaylistEntry::tracksURL()
{
    KURL::List list;

    if( m_loaded )  { //playlist loaded
        for( TrackItemInfo *info = m_trackList.first(); info; info = m_trackList.next() )
            list += info->url();
    }
    else
        list = m_url;    //playlist url

    return list;
}

void PlaylistEntry::updateInfo()
{
    const QString body = "<tr><td><b>%1</b></td><td>%2</td></tr>";

    QString str  = "<html><body><table width=\"100%\" border=\"0\">";

    str += body.arg( i18n( "Playlist" ),         text(0) );
    str += body.arg( i18n( "Number of tracks" ), QString::number(m_trackCount) );
    str += body.arg( i18n( "Length" ),           MetaBundle::prettyTime( m_length ) );
    str += body.arg( i18n( "Location" ),         m_url.prettyURL() );
    str += "</table></body></html>";

    PlaylistBrowser::instance()->setInfo( str );
}

void PlaylistEntry::setDynamic( bool enable )
{
    if( enable != m_dynamic )
    {
        if( enable )
        {
            m_dynamicPix = new QPixmap( KGlobal::iconLoader()->loadIcon( "favorites", KIcon::NoGroup, 16 ) );
            if( !m_loaded ) load();
        }
        else {
            delete m_dynamicPix;
            m_dynamicPix = 0;
        }
        m_dynamic = enable;
    }

    listView()->repaintItem( this );
}

void PlaylistEntry::setup()
{
    QFontMetrics fm( listView()->font() );
    int margin = listView()->itemMargin()*2;
    int h = fm.lineSpacing();
    if ( h % 2 > 0 )
        h++;
    if( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW )
        setHeight( h + fm.lineSpacing() + margin );
    else
        setHeight( h + margin );
}


void PlaylistEntry::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    bool detailedView = PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW;

    //flicker-free drawing
    static QPixmap buffer;
    buffer.resize( width, height() );

    if( buffer.isNull() )
    {
        KListViewItem::paintCell( p, cg, column, width, align );
        return;
    }

    QPainter pBuf( &buffer, true );
    // use alternate background
#if KDE_VERSION < KDE_MAKE_VERSION(3,3,91)
    pBuf.fillRect( buffer.rect(), isSelected() ? cg.highlight() : backgroundColor() );
#else
    pBuf.fillRect( buffer.rect(), isSelected() ? cg.highlight() : backgroundColor(0) );
#endif

    KListView *lv = static_cast<KListView *>( listView() );

    if( m_loading && m_loadingPix ) {
        pBuf.drawPixmap( (lv->treeStepSize() - m_loadingPix->width())/2,
                         (height() - m_loadingPix->height())/2,
                         *m_loadingPix );
    }

    QFont font( p->font() );
    QFontMetrics fm( p->fontMetrics() );

    int text_x = 0;// lv->treeStepSize() + 3;
    int textHeight;

    if( detailedView )
        textHeight = fm.lineSpacing() + lv->itemMargin() + 1;
    else
        textHeight = height();

    pBuf.setPen( isSelected() ? cg.highlightedText() : cg.text() );

    if( m_dynamic && m_dynamicPix && amaroK::dynamicMode() )
    {
        pBuf.drawPixmap( text_x, (textHeight - m_dynamicPix->height())/2, *m_dynamicPix );
        text_x += m_dynamicPix->width()+4;
    }
    else if( pixmap( column ) )
    {
        int y = (textHeight - pixmap(column)->height())/2;
        if( detailedView ) y++;
        pBuf.drawPixmap( text_x, y, *pixmap(column) );
        text_x += pixmap(column)->width()+4;
    }

    // draw the playlist name in italics
    font.setBold( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW );
    font.setItalic( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW );
    pBuf.setFont( font );
    QFontMetrics fmName( font );

    QString name = text(column);
    const int _width = width - text_x - lv->itemMargin()*2;
    if( fmName.width( name ) > _width )
    {
        name = KStringHandler::rPixelSqueeze( name, pBuf.fontMetrics(), _width );
    }

    pBuf.drawText( text_x, 0, width, textHeight, AlignVCenter, name );

    if( detailedView ) {
        QString info;

        text_x = lv->treeStepSize() + 3;
        font.setBold( false );
        pBuf.setFont( font );

        if( m_loading )
            info = i18n( "Loading..." );
        else
        {    //playlist loaded
            // draw the number of tracks and the total length of the playlist
            info += i18n("1 Track", "%n Tracks", m_trackCount);
            if( m_length )
        info += QString(i18n(" - [%2]")).arg( MetaBundle::prettyTime( m_length ) );
        }

        pBuf.drawText( text_x, textHeight, width, fm.lineSpacing(), AlignVCenter, info);
    }

    pBuf.end();
    p->drawPixmap( 0, 0, buffer );
}


QDomElement PlaylistEntry::xml() {
        QDomDocument doc;
        QDomElement i = doc.createElement("playlist");
        i.setAttribute( "file", url().path() );
        if( isOpen() )
            i.setAttribute( "isOpen", "true" );

        QDomElement attr = doc.createElement( "tracks" );
        QDomText t = doc.createTextNode( QString::number( trackCount() ) );
        attr.appendChild( t );
        i.appendChild( attr );

        attr = doc.createElement( "length" );
        t = doc.createTextNode( QString::number( length() ) );
        attr.appendChild( t );
        i.appendChild( attr );

        QFileInfo fi( url().path() );
        attr = doc.createElement( "modified" );
        t = doc.createTextNode( QString::number( fi.lastModified().toTime_t() ) );
        attr.appendChild( t );
        i.appendChild( attr );

        return i;
}


//////////////////////////////////////////////////////////////////////////////////
///    CLASS PlaylistTrackItem
////////////////////////////////////////////////////////////////////////////////

PlaylistTrackItem::PlaylistTrackItem( QListViewItem *parent, QListViewItem *after, TrackItemInfo *info )
    : PlaylistBrowserEntry( parent, after )
    , m_trackInfo( info )
{
    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setText( 0, info->title() );
}

const KURL &PlaylistTrackItem::url()
{
    return m_trackInfo->url();
}


//////////////////////////////////////////////////////////////////////////////////
///    CLASS TrackItemInfo
////////////////////////////////////////////////////////////////////////////////

TrackItemInfo::TrackItemInfo( const KURL &u, const QString &t, const int l )
        : m_url( u )
        , m_title( t )
        , m_length( l )
{
    if( title().isEmpty() )
    {
        MetaBundle *mb = new MetaBundle( u );
        if( mb->isValidMedia() )
        {
            m_title  = mb->prettyTitle();
            m_length = mb->length();
        }
        else
        {
            m_title = MetaBundle::prettyTitle( fileBaseName( m_url.path() ) );
        }
    }

    if( m_length < 0 )
        m_length = 0;
}

/////////////////////////////////////////////////////////////////////////////
///    CLASS StreamEntry
////////////////////////////////////////////////////////////////////////////

StreamEntry::StreamEntry( QListViewItem *parent, QListViewItem *after, const KURL &u, const QString &t )
    : PlaylistBrowserEntry( parent, after )
    , m_title( t )
    , m_url( u )
{
    setDragEnabled( true );
    setRenameEnabled( 0, true );
    setExpandable( false );

    if( m_title.isEmpty() )
        m_title = fileBaseName( m_url.prettyURL() );

    setPixmap( 0, SmallIcon( amaroK::icon( "playlist" ) ) );

    setText( 0, m_title );
}

StreamEntry::StreamEntry( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition )
    : PlaylistBrowserEntry( parent, after )
{
    setDragEnabled( true );
    setRenameEnabled( 0, true );
    setExpandable( false );

    m_title = xmlDefinition.attribute( "name" );
    QDomElement e = xmlDefinition.namedItem( "url" ).toElement();
    m_url  = KURL::fromPathOrURL( e.text() );


    if( m_title.isEmpty() )
        m_title = fileBaseName( m_url.prettyURL() );

    setPixmap( 0, SmallIcon( amaroK::icon( "playlist" ) ) );

    setText( 0, m_title );
}


QDomElement StreamEntry::xml() {
        QDomDocument doc;
        QDomElement i = doc.createElement("stream");
        i.setAttribute( "name", title() );
        if( isOpen() )
            i.setAttribute( "isOpen", "true" );
        QDomElement url = doc.createElement( "url" );
        url.appendChild( doc.createTextNode( m_url.prettyURL() ));
        i.appendChild( url );
        return i;
}

void StreamEntry::updateInfo()
{
    const QString body = "<tr><td><b>%1</b></td><td>%2</td></tr>";

    QString str = "<html><body><table width=\"100%\" border=\"0\">";

    str += body.arg( i18n( "Name" ), text(0) );
    str += body.arg( i18n( "URL" ),  m_url.prettyURL() );
    str += "</table></body></html>";

    PlaylistBrowser::instance()->setInfo( str );
}

void StreamEntry::setup()
{
    QFontMetrics fm( listView()->font() );
    int margin = listView()->itemMargin()*2;
    int h = fm.lineSpacing();
    if ( h % 2 > 0 )
        h++;
    if( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW )
        setHeight( h + fm.lineSpacing() + margin );
    else
        setHeight( h + margin );
}

void StreamEntry::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    bool detailedView = PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW;

    //flicker-free drawing
    static QPixmap buffer;
    buffer.resize( width, height() );

    if( buffer.isNull() )
    {
        KListViewItem::paintCell( p, cg, column, width, align );
        return;
    }

    QPainter pBuf( &buffer, true );
    // use alternate background
#if KDE_VERSION < KDE_MAKE_VERSION(3,3,91)
    pBuf.fillRect( buffer.rect(), isSelected() ? cg.highlight() : backgroundColor() );
#else
    pBuf.fillRect( buffer.rect(), isSelected() ? cg.highlight() : backgroundColor(0) );
#endif

    KListView *lv = static_cast<KListView *>( listView() );

    QFont font( p->font() );
    QFontMetrics fm( p->fontMetrics() );

    int text_x = 0;// lv->treeStepSize() + 3;
    int textHeight;

    if( detailedView )
        textHeight = fm.lineSpacing() + lv->itemMargin() + 1;
    else
        textHeight = height();

    pBuf.setPen( isSelected() ? cg.highlightedText() : cg.text() );

    if( pixmap(column) ) {
        int y = (textHeight - pixmap(column)->height())/2;
        if( detailedView ) y++;
        pBuf.drawPixmap( text_x, y, *pixmap(column) );
        text_x += pixmap(column)->width()+4;
    }

    font.setBold( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW );
    font.setItalic( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW );
    pBuf.setFont( font );
    QFontMetrics fmName( font );

    QString name = text(column);
    const int _width = width - text_x - lv->itemMargin()*2;
    if( fmName.width( name ) > _width )
    {
        name = KStringHandler::rPixelSqueeze( name, pBuf.fontMetrics(), _width );
    }

    pBuf.drawText( text_x, 0, width, textHeight, AlignVCenter, name );

    if( detailedView ) {
        QString info;

        text_x = lv->treeStepSize() + 3;
        font.setBold( false );
        font.setItalic( true );
        pBuf.setFont( font );

        info += m_url.prettyURL();

        pBuf.drawText( text_x, textHeight, width, fm.lineSpacing(), AlignVCenter, info);
    }

    pBuf.end();
    p->drawPixmap( 0, 0, buffer );
}

/////////////////////////////////////////////////////////////////////////////
///    CLASS StreamEditor
////////////////////////////////////////////////////////////////////////////

StreamEditor::StreamEditor( QWidget *parent, const QString &title, const QString &url, bool readonly )
    : KDialogBase( parent, "StreamEditor", true, QString::null, Ok|Cancel)
{
    makeGridMainWidget( 2, Qt::Horizontal );

    QLabel *nameLabel = new QLabel( i18n("&Name:"), mainWidget() );
    m_nameLineEdit = new KLineEdit( title, mainWidget() );
    m_nameLineEdit->setReadOnly( readonly );
    nameLabel->setBuddy( m_nameLineEdit );

    QLabel *urlLabel = new QLabel( i18n("&Url:"), mainWidget() );
    m_urlLineEdit = new KLineEdit( url, mainWidget() );
    m_urlLineEdit->setReadOnly( readonly );
    urlLabel->setBuddy( m_urlLineEdit );

    if( !readonly )
        m_nameLineEdit->setFocus();

    QSize min( 480, 110 );
    setInitialSize( min );
}


/////////////////////////////////////////////////////////////////////////////
///    CLASS DynamicEntry
////////////////////////////////////////////////////////////////////////////
DynamicEntry::DynamicEntry( QListViewItem *parent, QListViewItem *after, const QString &name )
        : PlaylistBrowserEntry( parent, after, name )
        , DynamicMode( name )
{
    setPixmap( 0, SmallIcon("dynamic") );
    setDragEnabled( true );
}

DynamicEntry::DynamicEntry( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition )
        : PlaylistBrowserEntry( parent, after )
        , DynamicMode( xmlDefinition.attribute( "name" ) )
{
    setPixmap( 0, SmallIcon( "dynamic" ) );
    setDragEnabled( true );

    QDomElement e;

    setCycleTracks( xmlDefinition.namedItem( "cycleTracks" ).toElement().text() == "true" );
    setMarkHistory( xmlDefinition.namedItem( "markHistory" ).toElement().text() == "true" );

    setUpcomingCount( xmlDefinition.namedItem( "upcoming" ).toElement().text().toInt() );
    setPreviousCount( xmlDefinition.namedItem( "previous" ).toElement().text().toInt() );

    setAppendType( xmlDefinition.namedItem( "appendType" ).toElement().text().toInt() );
    setAppendCount( xmlDefinition.namedItem( "appendCount" ).toElement().text().toInt() );

    if ( appendType() == 2 ) {
        setItems( QStringList::split( ',', xmlDefinition.namedItem( "items" ).toElement().text() ) );
    }
}

QString DynamicEntry::text( int column ) const
{
    if( column == 0 )
        return title();
    return PlaylistBrowserEntry::text( column );
}

QDomElement DynamicEntry::xml()
{
    QDomDocument doc;
    QDomElement i;

    i = doc.createElement("dynamic");
    i.setAttribute( "name", title() );
    if( isOpen() )
        i.setAttribute( "isOpen", "true" );

    QDomElement attr = doc.createElement( "cycleTracks" );
    QDomText t = doc.createTextNode( cycleTracks() ? "true" : "false" );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "markHistory" );
    t = doc.createTextNode( markHistory() ? "true" : "false" );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "upcoming" );
    t = doc.createTextNode( QString::number( upcomingCount() ) );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "previous" );
    t = doc.createTextNode( QString::number( previousCount() ) );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "appendCount" );
    t = doc.createTextNode( QString::number( appendCount() ) );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "appendType" );
    t = doc.createTextNode( QString::number( appendType() ) );
    attr.appendChild( t );
    i.appendChild( attr );

    QString list;
    if( appendType() == 2 ) {
        QStringList itemsl = items();
        for( uint c = 0; c < itemsl.count(); c = c + 2 ) {
            list.append( itemsl[c] );
            list.append( ',' );
            list.append( itemsl[c+1] );
            if ( c < itemsl.count()-1 )
                list.append( ',' );
        }
    }

    attr = doc.createElement( "items" );
    t = doc.createTextNode( list );
    attr.appendChild( t );
    i.appendChild( attr );
    return i;
}


/////////////////////////////////////////////////////////////////////////////
///    CLASS PodcastChannel
////////////////////////////////////////////////////////////////////////////

PodcastChannel::PodcastChannel( QListViewItem *parent, QListViewItem *after, const KURL &url )
    : PlaylistBrowserEntry( parent, after )
        , m_url( url )
        , m_fetching( false )
        , m_updating( false )
        , m_new( false )
        , m_hasProblem( false )
        , m_parent( static_cast<PlaylistCategory*>(parent) )
        , m_settingsValid( false )
{
    setDragEnabled( true );
    setRenameEnabled( 0, false );

    setText(0, i18n("Retrieving Podcast...") ); //HACK to fill loading time space
    setPixmap( 0, SmallIcon( amaroK::icon( "podcast" ) ) );

    fetch();
}

PodcastChannel::PodcastChannel( QListViewItem *parent, QListViewItem *after, const KURL &url, const QDomNode &channelSettings )
    : PlaylistBrowserEntry( parent, after )
    , m_url( url )
    , m_fetching( false )
    , m_updating( false )
    , m_new( false )
    , m_hasProblem( false )
    , m_parent( static_cast<PlaylistCategory*>(parent) )
    , m_settingsValid( true )
{
    setDragEnabled( true );
    setRenameEnabled( 0, false );

    setDOMSettings( channelSettings );

    setText(0, i18n("Retrieving Podcast...") ); //HACK to fill loading time space
    setPixmap( 0, SmallIcon( amaroK::icon( "podcast" ) ) );

    fetch();
}

PodcastChannel::PodcastChannel( QListViewItem *parent, QListViewItem *after,
                                const KURL &url, const QDomNode &channelSettings, const QDomDocument &xmlDefinition )
    : PlaylistBrowserEntry( parent, after )
    , m_url( url )
    , m_fetching( false )
    , m_updating( false )
    , m_new( false )
    , m_hasProblem( false )
    , m_parent( static_cast<PlaylistCategory*>(parent) )
    , m_settingsValid( true )
{
    QDomNode type = xmlDefinition.namedItem("rss");
    if( !type.isNull() )
        setXml( type.namedItem("channel"), RSS );
    else
        setXml( type, ATOM );

    setDOMSettings( channelSettings );

    setDragEnabled( true );
    setRenameEnabled( 0, false );

    setPixmap( 0, SmallIcon( amaroK::icon( "podcast" ) ) );
}

PodcastChannel::PodcastChannel( QListViewItem *parent, QListViewItem *after, const PodcastChannelBundle &pcb )
    : PlaylistBrowserEntry( parent, after )
    , m_bundle( pcb )
    , m_url( pcb.url() )
    , m_fetching( false )
    , m_updating( false )
    , m_new( false )
    , m_hasProblem( false )
    , m_parent( static_cast<PlaylistCategory*>(parent) )
    , m_settingsValid( true )
{
    setText( 0, title() );
    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setPixmap( 0, SmallIcon( amaroK::icon( "podcast" ) ) );
}

void
PodcastChannel::setDOMSettings( const QDomNode &channelSettings )
{
    QString save   = channelSettings.namedItem("savelocation").toElement().text();
    bool scan      = channelSettings.namedItem("autoscan").toElement().text() == "true";
    bool hasPurge  = channelSettings.namedItem("purge").toElement().text() == "true";
    int purgeCount = channelSettings.namedItem("purgecount").toElement().text().toInt();
    int fetchType  = STREAM;

    if( channelSettings.namedItem( "fetch").toElement().text() == "automatic" )
        fetchType  = AUTOMATIC;

    KURL saveURL;
    QString t = title();
    if( save.isEmpty() )
        save = amaroK::saveLocation( "podcasts/" + amaroK::vfatPath( t ) );

    PodcastSettings *settings = new PodcastSettings( t, save, scan, fetchType, false/*transfer*/, hasPurge, purgeCount );
    m_bundle.setSettings( settings );
}

void
PodcastChannel::configure()
{
    PodcastSettingsDialog *dialog = new PodcastSettingsDialog( m_bundle.getSettings() );

    if( dialog->configure() )
    {
        PodcastSettings *newSettings = dialog->getSettings();

        bool downloadMedia = ( (fetchType() != newSettings->fetchType()) && (newSettings->fetchType() == AUTOMATIC) );

        /**
         * Rewrite local url
         * Move any downloaded media to the new location
         */
        if( saveLocation() != newSettings->saveLocation() )
        {
            KURL::List copyList;

            PodcastEpisode *item = static_cast<PodcastEpisode*>( firstChild() );
            // get a list of the urls of already downloaded items
            while( item )
            {
                if( item->isOnDisk() )
                    copyList << item->localUrl();

                item->setLocalUrlBase( newSettings->saveLocation().prettyURL() );
                item = static_cast<PodcastEpisode*>( item->nextSibling() );
            }
            // move the items
            if( !copyList.isEmpty() )
            {
                //create the local directory first
                PodcastEpisode::createLocalDir( newSettings->saveLocation().path() );
                KIO::CopyJob* m_podcastMoveJob = KIO::move( copyList, newSettings->saveLocation(), false );
                amaroK::StatusBar::instance()->newProgressOperation( m_podcastMoveJob )
                                              .setDescription( i18n( "Moving Podcasts" ) );
            }
        }

        if( newSettings->autoscan() != autoscan() )
        {
            if( autoscan() )
                PlaylistBrowser::instance()->m_podcastItemsToScan.append( this );
            else
                PlaylistBrowser::instance()->m_podcastItemsToScan.remove( this );
        }

        m_bundle.setSettings( newSettings );
        CollectionDB::instance()->updatePodcastChannel( m_bundle );

        if( hasPurge() && purgeCount() != 0 )
            purge();

        if( downloadMedia )
            downloadChildren();
    }

    delete dialog->getSettings();
    delete dialog;
}

void
PodcastChannel::downloadChildren()
{
    QListViewItem *item = firstChild();
    while( item )
    {
        #define item static_cast<PodcastEpisode*>(item)
        if( item->isNew() )
            m_podcastDownloadQueue.append( item );
        #undef  item

        item = item->nextSibling();
    }
    downloadChildQueue();
}

void
PodcastChannel::downloadChildQueue()
{
    if( m_podcastDownloadQueue.isEmpty() ) return;

    PodcastEpisode *first = m_podcastDownloadQueue.first();
    first->downloadMedia();
    m_podcastDownloadQueue.removeFirst();

    connect( first, SIGNAL( downloadFinished() ), this, SLOT( downloadChildQueue() ) );
}

void
PodcastChannel::fetch()
{
    setText(0, i18n( "Retrieving Podcast..." ) );

    m_iconCounter = 1;
    startAnimation();
    connect( &m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );

    m_podcastJob = KIO::storedGet( m_url, false, false );

    amaroK::StatusBar::instance()->newProgressOperation( m_podcastJob )
        .setDescription( i18n( "Fetching Podcast" ) )
        .setAbortSlot( this, SLOT( abortFetch() ) );

    connect( m_podcastJob, SIGNAL( result( KIO::Job* ) ), SLOT( fetchResult( KIO::Job* ) ) );
}

void
PodcastChannel::abortFetch()
{
    m_podcastJob->kill();

    stopAnimation();
    title().isEmpty() ?
        setText( 0, m_url.prettyURL() ) :
        setText( 0, title() );
}

void
PodcastChannel::fetchResult( KIO::Job* job ) //SLOT
{
    stopAnimation();
    if ( !job->error() == 0 ) {
        amaroK::StatusBar::instance()->shortMessage( i18n( "Unable to connect to Podcast server." ) );
        debug() << "Unable to retrieve podcast information. KIO Error: " << job->error() << endl;

        if( title().isEmpty() )
            setText( 0, m_url.prettyURL() );

        setPixmap( 0, SmallIcon("cancel") );

        return;
    }

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );

    QDomDocument d;

    QString error;
    int errorline, errorcolumn;
    if( !d.setContent( storedJob->data(), false /* disable namespace processing */,
                &error, &errorline, &errorcolumn ) )
    {
        amaroK::StatusBar::instance()->shortMessage( i18n("Podcast returned invalid data.") );
        debug() << "Podcast DOM failure in line " << errorline << ", column " << errorcolumn << ": " << error << endl;

        if( title().isEmpty() )
            setText( 0, m_url.prettyURL() );

        setPixmap( 0, SmallIcon("cancel") );
        return;
    }

    QDomNode type = d.namedItem("rss");
    if( type.isNull() || type.toElement().attribute( "version" ) != "2.0" )
    {
        type = d.namedItem("feed");
        if( type.isNull() )
        {
            amaroK::StatusBar::instance()->shortMessage( i18n("Sorry, only RSS 2.0 or Atom feeds for podcasts!") );

            if( title().isEmpty() )
                setText( 0, m_url.prettyURL() );

            setPixmap( 0, SmallIcon("cancel") );
            return;
        }
        // feed is ATOM
        else
        {
            setXml( type, ATOM );
        }
    }
    // feed is rss 2.0
    else
        setXml( type.namedItem("channel"), RSS );
}

void
PodcastChannel::removeChildren()
{
    QListViewItem *child, *next;
    if ( (child = firstChild()) )
    {
        while ( (next = child->nextSibling()) )
        {
            delete child;
            child=next;
        }
        delete child;
    }
}

void
PodcastChannel::rescan()
{
    m_updating = true;
    fetch();
}

void
PodcastChannel::setNew( bool n )
{
    if( n )
        setPixmap( 0, SmallIcon( amaroK::icon( "podcast2" ) ) );
    else if( m_hasProblem )
        setPixmap( 0, SmallIcon("cancel") );
    else
        setPixmap( 0, SmallIcon( amaroK::icon( "podcast" ) ) );

    m_new = n;
}

/// DONT TOUCH m_url!!!  The podcast has no mention to the location of the xml file, idiots.
void
PodcastChannel::setXml( const QDomNode &xml, const int feedType )
{
    /// Podcast Channel information
    const bool isAtom = ( feedType == ATOM );

    QString t = xml.namedItem( "title" ).toElement().text();

    QString a = xml.namedItem( "author" ).toElement().text();

    setText( 0, t );

    QString l = QString::null;

    if( isAtom )
        l = xml.namedItem( "link" ).toElement().attribute( "rel" );
    else
        l = xml.namedItem( "link" ).toElement().text();

    QString d = xml.namedItem( "description" ).toElement().text();
    QString id = xml.namedItem( "itunes:summary" ).toElement().text();
    if( id.length() > d.length() )
       d = id;
    QString c = xml.namedItem( "copyright" ).toElement().text();
    QString img = xml.namedItem( "image" ).toElement().namedItem( "url" ).toElement().text();
    if( img.isEmpty() )
        img = xml.namedItem( "itunes:image" ).toElement().namedItem( "url" ).toElement().text();
    if( img.isEmpty() )
        img = xml.namedItem( "itunes:image" ).toElement().attribute( "href" );
    if( img.isEmpty() )
        img = xml.namedItem( "itunes:image" ).toElement().text();

    PodcastSettings * settings = 0;
    if( m_settingsValid )
    {
        settings = m_bundle.getSettings();
    }
    else
    {
        settings = new PodcastSettings( t );
        m_settingsValid = true;
    }

    m_bundle = PodcastChannelBundle( m_url, t, a, l, d, c, settings );
    delete settings;
    m_bundle.setImageURL( KURL::fromPathOrURL( img ) );

    m_bundle.setParentId( m_parent->id() );
    if( !m_updating )
    { // don't reinsert on a refresh
        debug() << "Adding podcast to database" << endl;
        CollectionDB::instance()->addPodcastChannel( m_bundle );
    }
    else
    {
        debug() << "Updating podcast in database" << endl;
        CollectionDB::instance()->updatePodcastChannel( m_bundle );
    }

    /// Podcast Episodes information

    QDomNode n;
    if( isAtom )
        n = xml.namedItem( "entry" );
    else
        n = xml.namedItem( "item" );

    int  children = 0;
    int  episode_limit = (m_bundle.purgeCount() > 0 ? m_bundle.purgeCount() : 0 );
    bool downloadMedia = ( fetchType() == AUTOMATIC );
    QDomNode node;

    // We use an auto-increment id in the database, so we must insert podcasts in the reverse order
    // to ensure we can pull them out reliably.
    QValueList<QDomElement> eList;

    for( ; !n.isNull(); n = n.nextSibling() )
    {
        if( m_updating )
        {
            if( episodeExists( n, feedType ) )
                break;

            if( !n.namedItem( "enclosure" ).toElement().attribute( "url" ).isEmpty() )
            {
                eList.prepend( n.toElement() );
            }
            else if( isAtom )
            {
                // Atom feeds have multiple nodes called link, only one which has an enclosure.
                QDomNode nodes = n.namedItem("link");
                for( ; !nodes.isNull(); nodes = nodes.nextSibling() )
                {
                    if( nodes.toElement().attribute("rel") == "enclosure" )
                    {
                        eList.prepend( n.toElement() );
                        break;
                    }
                }
            }

        }
        else // Freshly added podcast
        {
            if( episode_limit != 0 && children > episode_limit-1 )
                break;

            if( !n.namedItem( "enclosure" ).toElement().attribute( "url" ).isEmpty() )
            {
                eList.prepend( n.toElement() );
                children++;
            }
            else if( isAtom )
            {
                // Atom feeds have multiple nodes called link, only one which has an enclosure.
                QDomNode node = n.namedItem("link");
                for( ; !node.isNull(); node = node.nextSibling() )
                {
                    if( node.toElement().attribute("rel") == "enclosure" )
                    {
                        eList.prepend( n.toElement() );
                        children++;
                        break;
                    }
                }
            }
        }
    }

    foreachType( QValueList<QDomElement>, eList )
    {
        PodcastEpisode *ep = new PodcastEpisode( this, 0 /*adding in reverse!*/, *it, feedType, m_updating/*new*/ );
        if( m_updating )
            ep->setNew( true );
    }

    if( hasPurge() && purgeCount() != 0 && childCount() > purgeCount() )
        purge();

    if( downloadMedia )
        downloadChildren();

    if( m_updating && firstChild() && static_cast<PodcastEpisode *>( firstChild() )->isNew() )
    {
        setNew();
        amaroK::StatusBar::instance()->shortMessage( i18n("New podcasts have been retrieved!") );
    }
}

const bool
PodcastChannel::episodeExists( const QDomNode &xml, const int feedType )
{
    QString command;
    if( feedType == RSS )
    {
        //check id
        QString guid = xml.namedItem( "guid" ).toElement().text();
        if( !guid.isEmpty() )
        {
            command = QString("SELECT id FROM podcastepisodes WHERE parent='%1' AND guid='%2';")
                              .arg( CollectionDB::instance()->escapeString( url().url() ),
                                    CollectionDB::instance()->escapeString( guid ) );
            QStringList values = CollectionDB::instance()->query( command );
            return !values.isEmpty();
        }

        QString episodeTitle = xml.namedItem( "title" ).toElement().text();
        QString episodeURL   = xml.namedItem( "enclosure" ).toElement().attribute( "url" );
        command = QString("SELECT id FROM podcastepisodes WHERE parent='%1' AND url='%2' AND title='%3';")
                          .arg( CollectionDB::instance()->escapeString( url().url() ),
                                CollectionDB::instance()->escapeString( episodeURL ),
                                CollectionDB::instance()->escapeString( episodeTitle ) );
        QStringList values = CollectionDB::instance()->query( command );

        return !values.isEmpty();
    }

    else if( feedType == ATOM )
    {
        //check id
        QString guid = xml.namedItem( "id" ).toElement().text();
        if( !guid.isEmpty() )
        {
            command = QString("SELECT id FROM podcastepisodes WHERE parent='%1' AND guid='%2';")
                              .arg( CollectionDB::instance()->escapeString( url().url() ),
                                    CollectionDB::instance()->escapeString( guid ) );
            QStringList values = CollectionDB::instance()->query( command );
            return !values.isEmpty();
        }

        QString episodeTitle = xml.namedItem("title").toElement().text();
        QString episodeURL = QString::null;
        QDomNode n = xml.namedItem("link");
        for( ; !n.isNull(); n = n.nextSibling() )
        {
            if( n.nodeName() == "link" && n.toElement().attribute("rel") == "enclosure" )
            {
                episodeURL = n.toElement().attribute( "href" );
                break;
            }
        }

        command = QString("SELECT id FROM podcastepisodes WHERE parent='%1' AND url='%2' AND title='%3';")
                          .arg( CollectionDB::instance()->escapeString( url().url() ),
                                CollectionDB::instance()->escapeString( episodeURL ),
                                CollectionDB::instance()->escapeString( episodeTitle ) );
        QStringList values = CollectionDB::instance()->query( command );

        return !values.isEmpty();
    }

    return false;
}

void
PodcastChannel::setParent( PlaylistCategory *newParent )
{
    if( newParent != m_parent )
    {
        m_parent->takeItem( this );
        newParent->insertItem( this );
        newParent->sortChildItems( 0, true );
    
        m_parent = newParent;
    }
    m_bundle.setParentId( m_parent->id() );

    CollectionDB::instance()->updatePodcastChannel( m_bundle );
}

void
PodcastChannel::updateInfo()
{
    const QString body = "<tr><td><b>%1</b></td><td>%2</td></tr>";

    QString str  = "<html><body><table width=\"100%\" border=\"0\">";

    str += body.arg( i18n( "Title" ),       title() );
    str += body.arg( i18n( "URL" ),         m_url.prettyURL() );
    str += body.arg( i18n( "Website" ),     link().prettyURL() );
    str += body.arg( i18n( "Copyright" ),   copyright() );
    str += body.arg( i18n( "Description" ), description() );
    str += "</table>";
    str += i18n( "<p>&nbsp;<b>Episodes</b></p><ul>" );
    for( QListViewItem *c = firstChild(); c; c = c->nextSibling() )
    {
        str += QString("<li>%1</li>").arg( static_cast<PodcastEpisode*>(c)->title() );
    }

    str += "</ul></body></html>";

    PlaylistBrowser::instance()->setInfo( str );
}

//maintain max items property
void
PodcastChannel::purge()
{
    if( childCount() - purgeCount() <= 0 )
        return;

    KURL::List urlsToDelete;
    QValueList<QListViewItem*> purgedItems;

    QListViewItem *current = firstChild();
    for( int i=0; current && i < childCount(); current = current->nextSibling(), i++ )
    {
        if( i < purgeCount() )
            continue;

        purgedItems.append( current );
    }

    foreachType( QValueList<QListViewItem*>, purgedItems )
    {
        QListViewItem *item = *it;

    #define item static_cast<PodcastEpisode*>(item)
        if( item->isOnDisk() )
            urlsToDelete.append( item->localUrl() );

        CollectionDB::instance()->removePodcastEpisode( item->dBId() );
    #undef  item
        delete item;
    }

    if( !urlsToDelete.isEmpty() )
        KIO::del( urlsToDelete );
}

void
PodcastChannel::startAnimation()
{
    if( !m_animationTimer.isActive() )
        m_animationTimer.start( ANIMATION_INTERVAL );
}

void
PodcastChannel::stopAnimation()
{
    m_animationTimer.stop();
    setPixmap( 0, SmallIcon( amaroK::icon( "podcast" ) ) );
}

void
PodcastChannel::slotAnimation()
{
    m_iconCounter % 2 ?
        setPixmap( 0, SmallIcon( amaroK::icon( "podcast" ) ) ):
        setPixmap( 0, SmallIcon( amaroK::icon( "podcast2" ) ) );

    m_iconCounter++;
}

/////////////////////////////////////////////////////////////////////////////
///    CLASS PodcastEpisode
///    @note we fucking hate itunes for taking over podcasts and inserting
///          their own attributes.
////////////////////////////////////////////////////////////////////////////
PodcastEpisode::PodcastEpisode( QListViewItem *parent, QListViewItem *after,
                                const QDomElement &xml, const int feedType, const bool &isNew )
    : PlaylistBrowserEntry( parent, after )
      , m_parent( parent )
      , m_localUrl( 0 )
      , m_fetching( false )
      , m_onDisk( false )
{
    const bool isAtom = ( feedType == ATOM );
    QString title = xml.namedItem( "title" ).toElement().text();
    QString subtitle;

    QString description, author, date, guid, type;
    int duration = 0;
    uint size = 0;
    KURL link;
    QString mimetype;

    if( isAtom )
    {
        for( QDomNode n = xml.firstChild(); !n.isNull(); n = n.nextSibling() )
        {
            if      ( n.nodeName() == "summary" )   description = n.toElement().text();
            else if ( n.nodeName() == "author" )    author      = n.toElement().text();
            else if ( n.nodeName() == "published" ) date        = n.toElement().text();
            else if ( n.nodeName() == "id" )        guid        = n.toElement().text();
            else if ( n.nodeName() == "link" )
            {
                if( n.toElement().attribute( "rel" ) == "enclosure" )
                {
                    const QString weblink = n.toElement().attribute( "href" );
                    link = KURL::fromPathOrURL( weblink );
                }
            }
        }
    }
    else
    {
        description = xml.namedItem( "description" ).toElement().text();
        QString idescription = xml.namedItem( "itunes:summary" ).toElement().text();
        if( idescription.length() > description.length() )
           description = idescription;

        if( subtitle.isEmpty() )
            subtitle = xml.namedItem( "itunes:subtitle" ).toElement().text();

        author   = xml.namedItem( "author" ).toElement().text();
        if( author.isEmpty() )
            author = xml.namedItem( "itunes:author" ).toElement().text();

        date     = xml.namedItem( "pubDate" ).toElement().text();

        QString ds = xml.namedItem( "itunes:duration" ).toElement().text();
        QString secs = ds.section( ":", -1, -1 );
        duration = secs.toInt();
        QString min = ds.section( ":", -2, -2 );
        duration += min.toInt() * 60;
        QString h = ds.section( ":", -3, -3 );
        duration += h.toInt() * 3600;

        size     = xml.namedItem( "enclosure" ).toElement().attribute( "length" ).toInt();
        type     = xml.namedItem( "enclosure" ).toElement().attribute( "type" );
        guid     = xml.namedItem( "guid" ).toElement().text();

        const QString weblink = xml.namedItem( "enclosure" ).toElement().attribute( "url" );
        mimetype = xml.namedItem( "enclosure" ).toElement().attribute( "type" );

        link     = KURL::fromPathOrURL( weblink );
    }

    if( title.isEmpty() )
        title = link.fileName();

    PodcastChannel *channel = dynamic_cast<PodcastChannel*>(m_parent);
    if( channel )
        m_localUrl = saveURL( channel->saveLocation(), mimetype, link );
    else
        m_localUrl = saveURL( PodcastSettings( "Podcasts" ).saveLocation(), mimetype, link );

    if( QFile::exists( m_localUrl.path() ) )
    {
        m_onDisk = true;
    }

    KURL parentUrl = static_cast<PodcastChannel*>(parent)->url();
    m_bundle.setDBId( -1 );
    m_bundle.setURL( link );
    if( m_onDisk )
        m_bundle.setLocalURL( m_localUrl );
    m_bundle.setParent( parentUrl );
    m_bundle.setTitle( title );
    m_bundle.setSubtitle( subtitle );
    m_bundle.setAuthor( author );
    m_bundle.setDescription( description );
    m_bundle.setDate( date );
    m_bundle.setType( type );
    m_bundle.setDuration( duration );
    m_bundle.setSize( size );
    m_bundle.setGuid( guid );
    m_bundle.setNew( isNew );

    int id = CollectionDB::instance()->addPodcastEpisode( m_bundle );
    m_bundle.setDBId( id );

    setText( 0, title );
    updatePixmap();
    setDragEnabled( true );
    setRenameEnabled( 0, false );
}

PodcastEpisode::PodcastEpisode( QListViewItem *parent, QListViewItem *after, PodcastEpisodeBundle &bundle )
    : PlaylistBrowserEntry( parent, after )
      , m_parent( parent )
      , m_bundle( bundle )
      , m_localUrl( 0 )
      , m_fetching( false )
      , m_onDisk( false )
{
    m_localUrl    =  m_bundle.localUrl();
    bool dbOnDisk = !m_localUrl.isEmpty();
    QString _url  =  m_localUrl.path();

    if( m_localUrl.isEmpty() )
    {
        PodcastChannel *channel = dynamic_cast<PodcastChannel*>(m_parent);
        if( channel )
           m_localUrl = saveURL( channel->saveLocation(), m_bundle.type(), m_bundle.url() );
        else
           m_localUrl = saveURL( PodcastSettings("Podcasts").saveLocation(), m_bundle.type(), m_bundle.url() );
    }

    if( QFile::exists( m_localUrl.path() ) )
    {
        m_onDisk = true;
    }

    if( m_onDisk )
    {
        m_bundle.setLocalURL( m_localUrl );
    }

    if( _url != m_bundle.localUrl().path() && dbOnDisk != m_onDisk && m_bundle.dBId() )
       CollectionDB::instance()->updatePodcastEpisode( m_bundle.dBId(), m_bundle );


    setText( 0, bundle.title() );
    updatePixmap();
    setDragEnabled( true );
    setRenameEnabled( 0, false );
}

KURL
PodcastEpisode::saveURL( const KURL &base, const QString &mimetype, const KURL &link )
{
    KURL url = base;

    QString filename = amaroK::vfatPath( KURL::encode_string_no_slash( link.url() ) );
    QString ext = filename.section( ".", -1 );
    QStringList acceptExtensions, rejectExtensions;
    acceptExtensions << "mp3" << "ogg" << "m4a" << "m4a" << "m4v";
    rejectExtensions << "cgi" << "php";
    QStringList patterns;
    if( !mimetype.isEmpty() )
        patterns = KMimeType::mimeType( mimetype )->patterns();
    if( !patterns.isEmpty() )
    {
        if( ext.isEmpty() || !patterns.contains( ext ) )
            filename += patterns[0];
    }
    else if( ext.isEmpty() || !acceptExtensions.contains( ext ) )
    {
        QString path = link.path();
        QString newext = path.section( ".", -1 );
        if( !newext.isEmpty() && acceptExtensions.contains( newext ) )
            filename += "." + newext;
        else if( ext.isEmpty() || rejectExtensions.contains( ext ) )
        {
            if( !newext.isEmpty() && !rejectExtensions.contains( newext ) )
                filename += "." + newext;
            else
                filename += ".mp3";
        }
    }
    url.setFileName( filename );

    return url;
}

void
PodcastEpisode::updatePixmap()
{
    if( isNew() )
        setPixmap( 0, SmallIcon( amaroK::icon( "podcast2" ) ) );
    else if( m_onDisk )
        setPixmap( 0, SmallIcon( "down" ) );
    else
        setPixmap( 0, SmallIcon( amaroK::icon( "podcast" ) ) );
}

const bool
PodcastEpisode::isOnDisk()
{
    bool oldOnDisk = m_onDisk;
    m_onDisk = QFile::exists( m_localUrl.path() );
    updatePixmap();
    m_bundle.setLocalURL( m_onDisk ? m_localUrl : KURL() );
    if( oldOnDisk != m_onDisk && dBId() )
        CollectionDB::instance()->updatePodcastEpisode( dBId(), m_bundle );
    return m_onDisk;
}

void
PodcastEpisode::downloadMedia()
{
    if( isOnDisk() )
        return;

    setText(0, i18n( "Downloading Media..." ) );

    m_iconCounter = 1;
    startAnimation();
    connect( &m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );
    KURL::List list( url() );

    KURL m_localDir = KURL::fromPathOrURL( m_localUrl.directory(true, true) );
    createLocalDir( m_localDir );

    m_podcastEpisodeJob = KIO::copy( list, m_localUrl, false );

    amaroK::StatusBar::instance()->newProgressOperation( m_podcastEpisodeJob )
            .setDescription( title().isEmpty()
                    ? i18n( "Downloading Podcast Media" )
                    : i18n( "Downloading Podcast \"%1\"" ).arg( title() ) )
            .setAbortSlot( this, SLOT(abortDownload()) );

    connect( m_podcastEpisodeJob, SIGNAL( result( KIO::Job* ) ), SLOT( downloadResult( KIO::Job* ) ) );
}

void PodcastEpisode::createLocalDir( const KURL &localDir )
{
    if( localDir.isEmpty() ) return;

    QString localDirString = localDir.path();
    if( !QFile::exists( localDirString ) )
    {
        QString parentDirString = localDir.directory( true, true );
        createLocalDir( parentDirString );
        QDir dir( localDirString );
        dir.mkdir( localDirString );
    }
}

void
PodcastEpisode::abortDownload() //SLOT
{
    emit downloadAborted();
    m_podcastEpisodeJob->kill();

    KURL part = m_localUrl;
    part.addPath( ".part" );
    KIO::del( part );

    stopAnimation();
    setText( 0, title() );
    m_onDisk = false;
    updatePixmap();
}

void
PodcastEpisode::downloadResult( KIO::Job* job ) //SLOT
{
    emit downloadFinished();

    stopAnimation();
    setText( 0, title() );
    if ( !job->error() == 0 ) {
        amaroK::StatusBar::instance()->shortMessage( i18n( "Media download aborted, unable to connect to server." ) );
        debug() << "Unable to retrieve podcast media. KIO Error: " << job->error() << endl;

        setPixmap( 0, SmallIcon("cancel") );

        return;
    }

    m_onDisk = true;

    m_bundle.setLocalURL( m_localUrl );
    CollectionDB::instance()->updatePodcastEpisode( dBId(), m_bundle );

    PodcastChannel *channel = dynamic_cast<PodcastChannel *>( m_parent );
    if( channel && channel->autotransfer() && MediaBrowser::isAvailable() )
    {
        addToMediaDevice();
        MediaBrowser::queue()->URLsAdded();
    }

    updatePixmap();
}

void
PodcastEpisode::addToMediaDevice()
{
    MetaBundle *bundle = new MetaBundle( localUrl() );
    PodcastChannel *channel = dynamic_cast<PodcastChannel *>( m_parent );
    if(channel && !channel->title().isEmpty())
        bundle->setAlbum(channel->title());
    if(!title().isEmpty())
        bundle->setTitle(title());

    MediaBrowser::queue()->addURL( localUrl(), bundle );
}

void
PodcastEpisode::setLocalUrlBase( const QString &s )
{
    QString filename = m_localUrl.filename();
    QString newL = s + filename;
    m_localUrl = KURL::fromPathOrURL( newL );
}

void
PodcastEpisode::setNew( const bool &n )
{
    if( n == isNew() ) return;
    m_bundle.setNew( n );
    updatePixmap();
    CollectionDB::instance()->updatePodcastEpisode( dBId(), m_bundle );
}

void PodcastEpisode::setListened( const bool &n )
{
    setNew( !n );
}

void
PodcastEpisode::startAnimation()
{
    if( !m_animationTimer.isActive() )
        m_animationTimer.start( ANIMATION_INTERVAL );
}

void
PodcastEpisode::stopAnimation()
{
    m_animationTimer.stop();
    updatePixmap();
}

void
PodcastEpisode::slotAnimation()
{
    m_iconCounter % 2 ?
        setPixmap( 0, SmallIcon( amaroK::icon( "podcast") ) ):
        setPixmap( 0, SmallIcon( amaroK::icon( "podcast2") ) );

    m_iconCounter++;
}

void
PodcastEpisode::setup()
{
    QFontMetrics fm( listView()->font() );
    int margin = listView()->itemMargin()*2;
    int h = fm.lineSpacing();
    if ( h % 2 > 0 )
        h++;
    if( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW )
        setHeight( h + fm.lineSpacing() + margin );
    else
        setHeight( h + margin );
}

void
PodcastEpisode::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    bool detailedView = PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW;

    //flicker-free drawing
    static QPixmap buffer;
    buffer.resize( width, height() );

    if( buffer.isNull() )
    {
        KListViewItem::paintCell( p, cg, column, width, align );
        return;
    }

    QPainter pBuf( &buffer, true );
    // use alternate background
#if KDE_VERSION < KDE_MAKE_VERSION(3,3,91)
    pBuf.fillRect( buffer.rect(), isSelected() ? cg.highlight() : backgroundColor() );
#else
    pBuf.fillRect( buffer.rect(), isSelected() ? cg.highlight() : backgroundColor(0) );
#endif

    KListView *lv = static_cast<KListView *>( listView() );

    QFont font( p->font() );
    QFontMetrics fm( p->fontMetrics() );

    int text_x = 0;// lv->treeStepSize() + 3;
    int textHeight;

    if( detailedView )
        textHeight = fm.lineSpacing() + lv->itemMargin() + 1;
    else
        textHeight = height();

    pBuf.setPen( isSelected() ? cg.highlightedText() : cg.text() );

    if( pixmap( column ) )
    {
        int y = (textHeight - pixmap(column)->height())/2;
        if( detailedView ) y++;
        pBuf.drawPixmap( text_x, y, *pixmap(column) );
        text_x += pixmap(column)->width()+4;
    }

    // draw the podcast name in italics
    font.setBold( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW );
    font.setItalic( PlaylistBrowser::instance()->viewMode() == PlaylistBrowser::DETAILEDVIEW );
    pBuf.setFont( font );
    QFontMetrics fmName( font );

    QString name = text(column);
    const int _width = width - text_x - lv->itemMargin()*2;
    if( fmName.width( name ) > _width )
    {
        name = KStringHandler::rPixelSqueeze( name, pBuf.fontMetrics(), _width );
    }

    pBuf.drawText( text_x, 0, width, textHeight, AlignVCenter, name );

    if( detailedView )
    {
        text_x = lv->treeStepSize() + 3;
        font.setBold( false );
        pBuf.setFont( font );
        QFontMetrics fmInfo( font );
        QString info = description();
        // remove unwanted text
        info.replace( "\n", " " );
        info.replace( QRegExp("<[^>]*>"), "" ); //html tags

        const int _width = width - text_x - lv->itemMargin()*2;
        if( fmInfo.width( info ) > _width )
        {
            info = KStringHandler::rPixelSqueeze( info, pBuf.fontMetrics(), _width );
        }

        pBuf.drawText( text_x, textHeight, width, fm.lineSpacing(), AlignVCenter, info );
    }

    pBuf.end();
    p->drawPixmap( 0, 0, buffer );
}

void
PodcastEpisode::updateInfo()
{
    const QString body = "<tr><td><b>%1</b></td><td>%2</td></tr>";

    QString str  = "<html><body><table width=\"100%\" border=\"0\">";

    str += body.arg( i18n( "Title" ),       m_bundle.title() );
    str += body.arg( i18n( "Author" ),      m_bundle.author() );
    str += body.arg( i18n( "Date" ),        m_bundle.date() );
    str += body.arg( i18n( "Type" ),        m_bundle.type() );
    str += body.arg( i18n( "Description" ), m_bundle.description() );
    str += body.arg( i18n( "URL" ),         m_bundle.url().prettyURL() );
    str += "</table></body></html>";

    PlaylistBrowser::instance()->setInfo( str );
}


/////////////////////////////////////////////////////////////////////////////
///    CLASS SmartPlaylist
////////////////////////////////////////////////////////////////////////////

SmartPlaylist::SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QString &name, const QString &query )
        : PlaylistBrowserEntry( parent, after, name )
        , m_sqlForTags( query )
        , m_title( name )
        , m_dynamic( false )
{
    setPixmap( 0, SmallIcon( amaroK::icon( "playlist" ) ) );
    setDragEnabled( query.isEmpty() ? false : true );

    setText( 0, name );
}

SmartPlaylist::SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QString &name, const QString &urls, const QString &tags )
        : PlaylistBrowserEntry( parent, after, name )
        , m_sqlForTags( tags )
        , m_title( name )
        , m_dynamic( false )
{
    setPixmap( 0, SmallIcon( amaroK::icon( "playlist" ) ) );
    setDragEnabled( !urls.isEmpty() && !tags.isEmpty() );

    setText( 0, name );
}


SmartPlaylist::SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QDomElement &xmlDefinition )
        : PlaylistBrowserEntry( parent, after )
        , m_after( after )
        , m_dynamic( false )
{
    setPixmap( 0, SmallIcon( amaroK::icon( "playlist" ) ) );
    setXml( xmlDefinition );
    setDragEnabled( !m_sqlForTags.isEmpty() );
}

void SmartPlaylist::setXml( const QDomElement &xml )
{
    m_xml = xml;
    m_title = xml.attribute( "name" );
    setText( 0, m_title );
    m_sqlForTags = xml.namedItem( "sqlquery" ).toElement().text();
    static QStringList genres;
    static QStringList artists;
    static QStringList albums;
    static QStringList years;

    //Delete all children before
    QListViewItem *child, *next;
    if ( (child = firstChild()) ) {
        while ( (next = child->nextSibling()) ) {
            delete child;
            child=next;
        }
        delete child;
    }

    QDomNode expandN = xml.namedItem( "expandby" );
    if ( !expandN.isNull() ) {
        QDomElement expand = expandN.toElement();

        QString field = expand.attribute( "field" );
        SmartPlaylist *item = this;
        if ( field == i18n("Genre") ) {
            if ( genres.isEmpty() ) {
                genres = CollectionDB::instance()->genreList();
            }
            foreach( genres ) {
                m_after = new SmartPlaylist( item, m_after, i18n( "%1" ).arg( *it ), expand.text().replace("(*ExpandString*)", *it)  );
            }
        }
        if ( field == i18n("Artist") ) {
            if ( artists.isEmpty() ) {
                artists = CollectionDB::instance()->artistList();
            }
            foreach( artists ) {
                m_after = new SmartPlaylist( item, m_after, i18n( "By %1" ).arg( *it ), expand.text().replace("(*ExpandString*)", *it)  );
            }
        }
        if ( field == i18n("Album") ) {
            if ( albums.isEmpty() ) {
                albums = CollectionDB::instance()->albumList();
            }
            foreach( albums ) {
                m_after = new SmartPlaylist( item, m_after, i18n( "%1" ).arg( *it ), expand.text().replace("(*ExpandString*)", *it)  );
            }
        }
        if ( field == i18n("Year") ) {
            if ( years.isEmpty() ) {
                years = CollectionDB::instance()->yearList();
            }
            foreach( years ) {
                m_after = new SmartPlaylist( item, m_after, i18n( "%1" ).arg( *it ), expand.text().replace("(*ExpandString*)", *it)  );
            }
        }
    }

}


QString SmartPlaylist::query()
{
    return QString( m_sqlForTags ).replace("(*CurrentTimeT*)" , QString::number(QDateTime::currentDateTime().toTime_t()) );
}


void SmartPlaylist::setDynamic( bool enable )
{
    if( enable != m_dynamic )
    {
        enable ?
            setPixmap( 0, SmallIcon( "favorites" ) ) :
            setPixmap( 0, SmallIcon( amaroK::icon( "playlist" ) ) );
        m_dynamic = enable;
    }
}


#include "playlistbrowseritem.moc"
