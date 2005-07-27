// (c) 2004 Pierpaolo Di Panfilo
// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2005 Seb Ruiz <me@sebruiz.net>
// License: GPL V2. See COPYING file for information.

#include "amarok.h"
#include "collectiondb.h"
#include "debug.h"
#include "playlistbrowser.h"
#include "playlistbrowseritem.h"
#include "playlistloader.h"    //load()
#include "podcastsettings.h"
#include "metabundle.h"
#include "statusbar.h"
#include "threadweaver.h"

#include <qfile.h>             //loadPlaylists(), renamePlaylist()
#include <qlabel.h>
#include <qpainter.h>          //paintCell()
#include <qpixmap.h>           //paintCell()

#include <kiconloader.h>       //smallIcon
#include <kio/job.h>           //podcast retrieval
#include <klocale.h>
#include <kmdcodec.h>          //podcast media saving
#include <kstandarddirs.h>     //podcast loading icons

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
///    CLASS PlaylistCategory
////////////////////////////////////////////////////////////////////////////

PlaylistCategory::PlaylistCategory( QListView *parent, QListViewItem *after, const QString &t, bool isFolder )
    : PlaylistBrowserEntry( parent, after )
    , m_title( t )
    , m_folder( isFolder )
{
    setDragEnabled( false );
    setRenameEnabled( 0, isFolder );
    setExpandable( true );

    setPixmap( 0, SmallIcon("folder_red") );

    setText( 0, t );
    setText( 1, "" );
}


PlaylistCategory::PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, const QString &t, bool isFolder )
    : PlaylistBrowserEntry( parent, after )
    , m_title( t )
    , m_folder( isFolder )
{
    setDragEnabled( false );
    setRenameEnabled( 0, isFolder );
    setExpandable( true );

    setPixmap( 0, SmallIcon("folder") );

    setText( 0, t );
    setText( 1, "" );
}


PlaylistCategory::PlaylistCategory( QListView *parent, QListViewItem *after, QDomElement xmlDefinition, bool isFolder )
    : PlaylistBrowserEntry( parent, after )
    , m_folder( isFolder )
{
    setXml( xmlDefinition );
    setDragEnabled( false );
    setRenameEnabled( 0, isFolder );
    setExpandable( true );

    setPixmap( 0, SmallIcon("folder_red") );
    setText( 1, "" );
}


PlaylistCategory::PlaylistCategory( PlaylistCategory *parent, QListViewItem *after, QDomElement xmlDefinition )
    : PlaylistBrowserEntry( parent, after )
    , m_folder( true )
{
    setXml( xmlDefinition );
    setDragEnabled( false );
    setRenameEnabled( 0, true );
    setExpandable( true );

    setPixmap( 0, SmallIcon("folder") );
}


void PlaylistCategory::setXml( QDomElement xml )
{
    QString tname = xml.tagName();
    if ( tname == "category" ) {
        QListViewItem *last = 0;
        for( QDomNode n = xml.firstChild() ; !n.isNull(); n = n.nextSibling() )
        {
            QDomElement e = n.toElement();
            if ( e.tagName() == "category" ) {
                last = new PlaylistCategory( this, last, e);
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
            else if ( e.tagName() == "party" ) {
                last = new PartyEntry( this, last, e );
            }
        }
        setText( 0, xml.attribute("name") );
    }
}


QDomElement PlaylistCategory::xml()
{
        QDomDocument d;
        QDomElement i = d.createElement("category");
        i.setAttribute( "name", text(0) );
        for( PlaylistBrowserEntry *it = (PlaylistBrowserEntry*)firstChild(); it; it = (PlaylistBrowserEntry*)it->nextSibling() ) {
          //FIXME: this is a very ugly and bad hack not to save the default smart and stream lists.
            if ( it->text(0) == i18n("Cool-Streams") || it->text(0) == i18n("Collection") )
                continue;
            i.appendChild( it->xml() );
        }
        return i;
}

void
PlaylistCategory::paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int align )
{
    QFont font( p->font() );

    if( !m_folder ) { // increase font size for base categories
        font.setBold( true );
        font.setPointSize( font.pointSize() + 1 );
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
    , m_modified( false )
    , m_dynamic( false )
    , m_dynamicPix( 0 )
    , m_savePix( 0 )
    , m_loadingPix( 0 )
    , m_lastTrack( 0 )
{
    m_trackList.setAutoDelete( true );
    tmp_droppedTracks.setAutoDelete( false );

    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setExpandable(true);

    setText(0, fileBaseName( url.path() ) );
    setPixmap( 0, SmallIcon("player_playlist_2") );

    if( !m_trackCount )
        load();   //load the playlist file
}


PlaylistEntry::PlaylistEntry( QListViewItem *parent, QListViewItem *after, QDomElement xmlDefinition )
    : PlaylistBrowserEntry( parent, after )
    , m_loading( false )
    , m_loaded( false )
    , m_modified( false )
    , m_dynamic( false )
    , m_dynamicPix( 0 )
    , m_savePix( 0 )
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
    setPixmap( 0, SmallIcon("player_playlist_2") );


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
    debug() << "Loading playlist" << endl;
    m_trackList.clear();
    m_length = 0;
    m_loaded = false;
    m_loading = true;
    //starts loading animation
    ((PlaylistBrowserView *)listView())->startAnimation( this );

     //read the playlist file in a thread
    ThreadWeaver::instance()->queueJob( new PlaylistReader( this, m_url.path() ) );
}


void PlaylistEntry::restore()
{
    setOpen( false );

    if( !m_loaded ) {
        TrackItemInfo *info = tmp_droppedTracks.first();
        while( info ) {
            m_length -= info->length();
            m_trackCount--;
            tmp_droppedTracks.remove();    //remove current item
            delete info;
            info = tmp_droppedTracks.current();    //the new current item
        }
    }
    else
        load();    //reload the playlist

    setModified( false );
}


void PlaylistEntry::insertTracks( QListViewItem *after, KURL::List list, QMap<QString,QString> map )
{
    int pos = 0;
    if( after ) {
        pos = m_trackList.find( ((PlaylistTrackItem*)after)->trackInfo() ) + 1;
        if( pos == -1 )
            return;
    }

    uint k = 0;
    const KURL::List::ConstIterator end = list.end();
    for ( KURL::List::ConstIterator it = list.begin(); it != end; ++it, ++k ) {
        QString key = (*it).isLocalFile() ? (*it).path() : (*it).url();
        QString str = map[ key ];
        QString title = str.section(';',0,0);
        int length = str.section(';',1,1).toUInt();

        TrackItemInfo *newInfo = new TrackItemInfo( *it, title.isEmpty() ? key : title, length );
        m_length += newInfo->length();
        m_trackCount++;

        if( after ) {
            m_trackList.insert( pos+k, newInfo );
            if( isOpen() )
                after = new PlaylistTrackItem( this, after, newInfo );
        }
        else {
            if( m_loaded ) {
                m_trackList.append( newInfo );
                if( isOpen() )  //append the track item to the playlist
                    m_lastTrack = new PlaylistTrackItem( this, m_lastTrack, newInfo );
            }
            else
                tmp_droppedTracks.append( newInfo );
        }

    }

    setModified( true );    //show a save icon to save changes
}


void PlaylistEntry::removeTrack( QListViewItem *item )
{
    #define item static_cast<PlaylistTrackItem*>(item)
    //remove a track and update playlist stats
    TrackItemInfo *info = item->trackInfo();
    m_length -= info->length();
    m_trackCount--;
    m_trackList.remove( info );
    if( item == m_lastTrack ) {
        QListViewItem *above = item->itemAbove();
        m_lastTrack = above ? (PlaylistTrackItem *)above : 0;
    }
    delete item;

    #undef item

    setModified( true );    //show a save icon to save changes
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
                m_length += info->length();
            }
            tmp_droppedTracks.clear();
        }

        m_loading = false;
        m_loaded = true;
        ((PlaylistBrowserView *)listView())->stopAnimation( this );  //stops the loading animation

        if( m_trackCount && !m_dynamic && !isDynamic() ) setOpen( true );
        else repaint();

        m_trackCount = m_trackList.count();
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

        QListViewItem* child = firstChild();
        QListViewItem* childTmp;
        //delete all children
        while ( child ) {
            childTmp = child;
            child = child->nextSibling();
            delete childTmp;
        }
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

    repaint();
}

void PlaylistEntry::setModified( bool chg )
{
    if( chg != m_modified ) {
        if( chg )
            m_savePix = new QPixmap( KGlobal::iconLoader()->loadIcon( "filesave", KIcon::NoGroup, 16 ) );
        else {
            delete m_savePix;
            m_savePix = 0;
        }

        m_modified = chg;
    }
    //this function is also called every time a track is inserted or removed
    //we repaint the item to update playlist info
    repaint();
}


void PlaylistEntry::setup()
{
    QFontMetrics fm( listView()->font() );
    int margin = listView()->itemMargin()*2;
    int h = m_savePix ? QMAX( m_savePix->height(), fm.lineSpacing() ) : fm.lineSpacing();
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
    pBuf.fillRect( buffer.rect(), isSelected() ? cg.highlight() : backgroundColor() );

    KListView *lv = (KListView *)listView();

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

    if( m_modified && m_savePix )
    {
        pBuf.drawPixmap( text_x, (textHeight - m_savePix->height())/2, *m_savePix );
        text_x += m_savePix->width()+4;
    }
    else if( m_dynamic && m_dynamicPix && AmarokConfig::dynamicMode() )
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
    if( fmName.width( name ) + text_x + lv->itemMargin()*2 > width )
    {
        int ellWidth = fmName.width( i18n("...") );
        QString text = QString::fromLatin1("");
        int i = 0;
        int len = name.length();
        while ( i < len && fmName.width( text + name[ i ] ) + ellWidth < width - text_x - lv->itemMargin()*2  ) {
            text += name[ i ];
            i++;
        }
        name = text + i18n("...");
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
    if( m_title.isEmpty() )
        m_title = MetaBundle::prettyTitle( fileBaseName( m_url.path() ) );

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

    setPixmap( 0, SmallIcon("player_playlist_2") );

    setText( 0, m_title );
}

StreamEntry::StreamEntry( QListViewItem *parent, QListViewItem *after, QDomElement xmlDefinition )
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

    setPixmap( 0, SmallIcon("player_playlist_2") );

    setText( 0, m_title );
}


QDomElement StreamEntry::xml() {
        QDomDocument doc;
        QDomElement i = doc.createElement("stream");
        i.setAttribute( "name", m_title );
        QDomElement url = doc.createElement( "url" );
        url.appendChild( doc.createTextNode( escapeHTML( m_url.prettyURL() ) ));
        i.appendChild( url );
        return i;
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
    pBuf.fillRect( buffer.rect(), isSelected() ? cg.highlight() : backgroundColor() );

    KListView *lv = (KListView *)listView();

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
    if( fmName.width( name ) + text_x + lv->itemMargin()*2 > width )
    {
        int ellWidth = fmName.width( i18n("...") );
        QString text = QString::fromLatin1("");
        int i = 0;
        int len = name.length();
        while ( i < len && fmName.width( text + name[ i ] ) + ellWidth < width - text_x - lv->itemMargin()*2  ) {
            text += name[ i ];
            i++;
        }
        name = text + i18n("...");
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

// For creating
StreamEditor::StreamEditor( QWidget *parent, const char *name )
    : KDialogBase( parent, name, true, i18n("Add Radio Stream"), Ok|Cancel)
{
    makeGridMainWidget( 2, Qt::Horizontal );

    QLabel *nameLabel = new QLabel( i18n("&Name:"), mainWidget() );
    m_nameLineEdit = new KLineEdit( i18n("Radio Stream"), mainWidget() );
    nameLabel->setBuddy( m_nameLineEdit );

    QLabel *urlLabel = new QLabel( i18n("&Url:"), mainWidget() );
    m_urlLineEdit = new KLineEdit( "", mainWidget() );
    urlLabel->setBuddy( m_urlLineEdit );

    QSize min( 480, 110 );
    setInitialSize( min );

    m_nameLineEdit->setFocus();

}

// For editing
StreamEditor::StreamEditor( QWidget *parent, QString title, QString url, const char *name )
    : KDialogBase( parent, name, true, i18n("Edit Radio Stream"), Ok|Cancel)
{
    makeGridMainWidget( 2, Qt::Horizontal );

    QLabel *nameLabel = new QLabel( i18n("&Name:"), mainWidget() );
    m_nameLineEdit = new KLineEdit( title, mainWidget() );
    nameLabel->setBuddy( m_nameLineEdit );

    QLabel *urlLabel = new QLabel( i18n("&Url:"), mainWidget() );
    m_urlLineEdit = new KLineEdit( url, mainWidget() );
    urlLabel->setBuddy( m_urlLineEdit );

    QSize min( 480, 110 );
    setInitialSize( min );

    m_nameLineEdit->setFocus();

}

/////////////////////////////////////////////////////////////////////////////
///    CLASS PartyEntry
////////////////////////////////////////////////////////////////////////////
PartyEntry::PartyEntry( QListViewItem *parent, QListViewItem *after, const QString &name )
        : PlaylistBrowserEntry( parent, after, name )
        , m_title( name )
        , m_items( NULL )
        , m_cycled( true )
        , m_marked( true )
        , m_upcoming( 20 )
        , m_previous( 5 )
        , m_appendCount( 1 )
        , m_appendType( RANDOM )
{
    setPixmap( 0, SmallIcon("party") );
    setDragEnabled( false );

    setText( 0, name );
}

PartyEntry::PartyEntry( QListViewItem *parent, QListViewItem *after, QDomElement xmlDefinition )
        : PlaylistBrowserEntry( parent, after )
{
    setPixmap( 0, SmallIcon( "party" ) );
    setDragEnabled( false );

    m_title  = xmlDefinition.attribute( "name" );

    QDomElement e;

    setCycled( xmlDefinition.namedItem( "cycleTracks" ).toElement().text() == "true" );
    setMarked( xmlDefinition.namedItem( "markHistory" ).toElement().text() == "true" );

    setUpcoming( xmlDefinition.namedItem( "upcoming" ).toElement().text().toInt() );
    setPrevious( xmlDefinition.namedItem( "previous" ).toElement().text().toInt() );

    setAppendType( xmlDefinition.namedItem( "appendType" ).toElement().text().toInt() );
    setAppendCount( xmlDefinition.namedItem( "appendCount" ).toElement().text().toInt() );

    if ( m_appendType == 2 ) {
        setItems( QStringList::split( ',', xmlDefinition.namedItem( "items" ).toElement().text() ) );
    }
    setText( 0, m_title );
}


QDomElement PartyEntry::xml() {
    QDomDocument doc;
    QDomElement i;

    i = doc.createElement("party");
    i.setAttribute( "name", text(0) );

    QDomElement attr = doc.createElement( "cycleTracks" );
    QDomText t = doc.createTextNode( isCycled() ? "true" : "false" );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "markHistory" );
    t = doc.createTextNode( isMarked() ? "true" : "false" );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "upcoming" );
    t = doc.createTextNode( QString::number( upcoming() ) );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "previous" );
    t = doc.createTextNode( QString::number( previous() ) );
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
    , m_loading1( 0 )
    , m_loading2( 0 )
    , m_availablePix( 0 )
    , m_fetching( false )
    , m_updating( false )
    , m_new( false )
    , m_hasProblem( false )
    , m_dirtyFeed( false )
    , m_autoScan( true )
    , m_interval( 4 )
    , m_mediaFetch( DOWNLOAD )
    , m_purgeItems( false )
    , m_purgeCount( 20 )
    , m_last( 0 )
{
    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setExpandable(true);

    setText(0, i18n("Retrieving Podcast...") ); //HACK to fill loading time space
    setPixmap( 0, SmallIcon("player_playlist_2") );

    fetch();
}


PodcastChannel::PodcastChannel( QListViewItem *parent, QListViewItem *after,
                                const KURL &url, QDomNode channelSettings, QDomDocument xmlDefinition )
    : PlaylistBrowserEntry( parent, after )
    , m_url( url )
    , m_loading1( 0 )
    , m_loading2( 0 )
    , m_availablePix( 0 )
    , m_fetching( false )
    , m_updating( false )
    , m_new( false )
    , m_hasProblem( false )
    , m_dirtyFeed( false )
    , m_last( 0 )
{
    setXml( xmlDefinition.namedItem("rss").namedItem("channel") );

    m_autoScan   = ( channelSettings.namedItem( "autoscan").toElement().text() == "true" );
    m_interval   =   channelSettings.namedItem( "scaninterval").toElement().text().toInt();

    if( channelSettings.namedItem( "fetch").toElement().text() == "download" )
        m_mediaFetch = DOWNLOAD;
    else
        m_mediaFetch = STREAM;

    m_purgeItems = ( channelSettings.namedItem( "purge").toElement().text() == "true" );
    m_purgeCount =   channelSettings.namedItem( "purgecount").toElement().text().toInt();

    setDragEnabled( true );
    setRenameEnabled( 0, false );
    setExpandable(true);

    setPixmap( 0, SmallIcon("player_playlist_2") );
}

void
PodcastChannel::configure()
{
    QString url    = m_url.prettyURL();

    PodcastSettings *settings = new PodcastSettings( m_url, m_autoScan, m_interval,
                                                     m_mediaFetch, m_purgeItems, m_purgeCount );

    settings->show();

    if( url != m_url.prettyURL() )
    {
        m_dirtyFeed = true;
        fetch();
    }
}

void
PodcastChannel::fetch()
{
    setText(0, i18n( "Retrieving Podcast..." ) );
    m_loading1 = new QPixmap( locate("data", "amarok/images/loading1.png" ) );
    m_loading2 = new QPixmap( locate("data", "amarok/images/loading2.png" ) );
    m_animationTimer = new QTimer();

    startAnimation();
    connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );

    m_podcastJob = KIO::storedGet( m_url, false, false );

    amaroK::StatusBar::instance()->newProgressOperation( m_podcastJob )
            .setDescription( i18n( "Fetching Podcast" ) );

    connect( m_podcastJob, SIGNAL( result( KIO::Job* ) ), SLOT( fetchResult( KIO::Job* ) ) );

}


void
PodcastChannel::fetchResult( KIO::Job* job ) //SLOT
{
    stopAnimation();
    if ( !job->error() == 0 ) {
        amaroK::StatusBar::instance()->shortMessage( i18n( "Unable to connect to Podcast server." ) );
        debug() << "Unable to retrieve podcast information. KIO Error: " << job->error() << endl;

        if( !m_title )
            setText( 0, m_url.prettyURL() );

        setPixmap( 0, SmallIcon("cancel") );

        return;
    }

    QString xml = QString( static_cast<KIO::StoredTransferJob*>( job )->data() );

    QDomDocument d;

    if( !d.setContent( xml ) )
    {
        amaroK::StatusBar::instance()->shortMessage( i18n("Podcast returned invalid data.") );

        if( !m_title )
            setText( 0, m_url.prettyURL() );

        setPixmap( 0, SmallIcon("cancel") );
        return;
    }

    setXml( d.namedItem("rss").namedItem("channel") );

    ///BEGIN Cache the xml
    QFile file( amaroK::saveLocation( "podcasts/" ) + m_cache );

    QTextStream stream( &file );

    if( !file.open( IO_WriteOnly ) ) return;

    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << d.toString();
    ///END Cache the xml
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
        setPixmap( 0, SmallIcon("favorites") );
    else if( m_hasProblem )
        setPixmap( 0, SmallIcon("cancel") );
    else
        setPixmap( 0, SmallIcon("player_playlist_2") );

    m_new = n;
}

/// DONT TOUCH m_url!!!  The podcast has no mention to the location of the xml file, idiots.
void
PodcastChannel::setXml( QDomNode xml )
{
    m_title = xml.namedItem( "title" ).toElement().text();
    setText( 0, m_title );

    m_cache = m_title;
    m_cache.replace( " ", "_" );
    m_cache += "_" + m_url.fileName();

    QString weblink = xml.namedItem( "link" ).toElement().text();
    KURL m_link( weblink );

    PodcastItem *updatingLast = 0;
    PodcastItem *first = (PodcastItem *)firstChild();

    if( m_dirtyFeed )
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
        m_dirtyFeed = false;
        first = 0;
    }

    QDomNode n = xml.namedItem( "item" );
    uint children = 0;
    bool downloadMedia = ( m_mediaFetch == AVAILABLE );
    for( ; !n.isNull(); n = n.nextSibling() )
    {
        if( m_updating )
        {
            // podcasts get inserted in a chronological order,
            // no need to continue traversing, we must have them already
            if( first && first->hasXml( n ) )
                break;

            if( !n.namedItem( "title" ).toElement().text().isEmpty() &&
                !n.namedItem( "enclosure" ).toElement().attribute( "url" ).isEmpty() )
            {
                updatingLast = new PodcastItem( this, updatingLast, n.toElement() );
                if( downloadMedia )
                    updatingLast->downloadMedia();

                updatingLast->setNew();
            }
        }
        else
        {
            //only download two episodes initially -> current and 1 back issue
            //Make configurable?
            if( children > 2 )
                break;

            if( !n.namedItem( "title" ).toElement().text().isEmpty() &&
                !n.namedItem( "enclosure" ).toElement().attribute( "url" ).isEmpty() )
            {
                m_last = new PodcastItem( this, m_last, n.toElement() );
                if( downloadMedia )
                    m_last->downloadMedia();
                children++;
            }
        }

    }

    if( childCount() > m_purgeCount )
        purge();

    if( firstChild() && static_cast<PodcastItem *>( firstChild() )->isNew() && m_updating )
    {
        setNew();
        amaroK::StatusBar::instance()->longMessage( i18n("New podcasts have been retrieved!") );
    }
}

//maintain max items property
void
PodcastChannel::purge()
{
    uint removeCount = childCount() - m_purgeCount;

    for( uint i=0; i < removeCount; i++ )
    {
        PodcastItem *newLast = 0;

        if( m_last && m_last != firstChild() )
            newLast = (PodcastItem *)m_last->itemAbove();


        delete m_last;
        m_last = newLast;
    }
}

QDomElement
PodcastChannel::xml()
{
        QDomDocument doc;
        QDomElement i = doc.createElement("podcast");
        i.setAttribute( "title", m_title );

        QDomElement attr = doc.createElement( "url" );
        QDomText t = doc.createTextNode( m_url.prettyURL() );
        attr.appendChild( t );
        i.appendChild( attr );

        attr = doc.createElement( "cache" );
        t = doc.createTextNode( m_cache );
        attr.appendChild( t );
        i.appendChild( attr );

        attr = doc.createElement( "autoscan" );
        t = doc.createTextNode( m_autoScan ? "true" : "false" );
        attr.appendChild( t );
        i.appendChild( attr );

        attr = doc.createElement( "scaninterval" );
        t = doc.createTextNode( QString::number( m_interval ) );
        attr.appendChild( t );
        i.appendChild( attr );

        attr = doc.createElement( "fetch" );
        t = doc.createTextNode( ( m_mediaFetch == DOWNLOAD ) ? "download" : "stream" );
        attr.appendChild( t );
        i.appendChild( attr );

        attr = doc.createElement( "purge" );
        t = doc.createTextNode( m_purgeItems ? "true" : "false" );
        attr.appendChild( t );
        i.appendChild( attr );

        attr = doc.createElement( "purgecount" );
        t = doc.createTextNode( QString::number( m_purgeCount ) );
        attr.appendChild( t );
        i.appendChild( attr );

        return i;
}

void
PodcastChannel::startAnimation()
{
    if( !m_animationTimer->isActive() )
        m_animationTimer->start( 100 );
}

void
PodcastChannel::stopAnimation()
{
    m_animationTimer->stop();
    setPixmap( 0, SmallIcon("player_playlist_2") );
}

void
PodcastChannel::slotAnimation()
{
    static uint iconCounter = 1;

    iconCounter % 2 ?
        setPixmap( 0, *m_loading1 ):
        setPixmap( 0, *m_loading2 );

    iconCounter++;
}

/////////////////////////////////////////////////////////////////////////////
///    CLASS PodcastItem
////////////////////////////////////////////////////////////////////////////
PodcastItem::PodcastItem( QListViewItem *parent, QListViewItem *after, QDomElement xml )
    : PlaylistBrowserEntry( parent, after )
      , m_localUrl( 0 )
      , m_loading1( 0 )
      , m_loading2( 0 )
      , m_fetching( false )
      , m_downloaded( false )
      , m_new( false )
{
    m_title       = xml.namedItem( "title" ).toElement().text();
    m_description = xml.namedItem( "description" ).toElement().text();
    m_author      = xml.namedItem( "author" ).toElement().text();
    m_date        = xml.namedItem( "pubDate" ).toElement().text();
    m_duration    = xml.namedItem( "enclosure" ).toElement().attribute( "length" ).toInt();
    m_type        = xml.namedItem( "enclosure" ).toElement().attribute( "type" );
    const QString url   = xml.namedItem( "enclosure" ).toElement().attribute( "url" );

    m_url         = KURL::fromPathOrURL( url );

    m_localUrlString = amaroK::saveLocation( "podcasts/data/" );

    m_localUrlString += m_title;
    m_localUrlString.replace( " ", "_" );
    m_localUrlString += "_" + m_url.fileName();

    m_localUrl = KURL::fromPathOrURL( m_localUrlString );

    setText( 0, m_title );
    setPixmap( 0, SmallIcon("player_playlist_2") );
    setDragEnabled( true );
    setRenameEnabled( 0, false );
}


void
PodcastItem::downloadMedia()
{
    if( QFile::exists( m_localUrlString ) )
        return;

    setText(0, i18n( "Downloading Media..." ) );
    m_loading1 = new QPixmap( locate("data", "amarok/images/loading1.png" ) );
    m_loading2 = new QPixmap( locate("data", "amarok/images/loading2.png" ) );
    m_animationTimer = new QTimer();

    startAnimation();
    connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );

    m_podcastItemJob = KIO::storedGet( m_url, false, false );

    amaroK::StatusBar::instance()->newProgressOperation( m_podcastItemJob )
            .setDescription( i18n( "Downloading Podcast Media" ) );

    connect( m_podcastItemJob, SIGNAL( result( KIO::Job* ) ), SLOT( downloadResult( KIO::Job* ) ) );
}

void
PodcastItem::downloadResult( KIO::Job* job ) //SLOT
{
    stopAnimation();
    setText( 0, m_title );
    if ( !job->error() == 0 ) {
        amaroK::StatusBar::instance()->shortMessage( i18n( "Media download aborted, unable to connect to server." ) );
        debug() << "Unable to retrieve podcast media. KIO Error: " << job->error() << endl;

        setPixmap( 0, SmallIcon("cancel") );

        return;
    }

    m_downloaded = true;

    ///BEGIN store the file
    QByteArray ba = m_podcastItemJob->data();

    QFile file( m_localUrlString );

    QTextStream stream( &file );

    if( !file.open( IO_WriteOnly ) ) return;

    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream.writeRawBytes( ba, ba.size() );

    file.close();
    ///END store the file
}


bool
PodcastItem::hasXml( const QDomNode& xml )
{
    return m_title           == xml.namedItem( "title" ).toElement().text() &&
           m_description     == xml.namedItem( "description" ).toElement().text() &&
           m_author          == xml.namedItem( "author" ).toElement().text() &&
           m_date            == xml.namedItem( "pubDate" ).toElement().text() &&
           m_duration        == xml.namedItem( "enclosure" ).toElement().attribute( "length" ).toInt() &&
           m_type            == xml.namedItem( "enclosure" ).toElement().attribute( "type" ) &&
           m_url.prettyURL() == xml.namedItem( "enclosure" ).toElement().attribute( "url" );
}

void
PodcastItem::setNew( bool n )
{
    if( n )
        setPixmap( 0, SmallIcon("favorites") );
    else
        setPixmap( 0, SmallIcon("player_playlist_2") );

    m_new = n;
}

void
PodcastItem::startAnimation()
{
    if( !m_animationTimer->isActive() )
        m_animationTimer->start( 100 );
}

void
PodcastItem::stopAnimation()
{
    m_animationTimer->stop();
    setPixmap( 0, SmallIcon("player_playlist_2") );
}

void
PodcastItem::slotAnimation()
{
    static uint iconCounter = 1;

    iconCounter % 2 ?
        setPixmap( 0, *m_loading1 ):
        setPixmap( 0, *m_loading2 );

    iconCounter++;
}

/////////////////////////////////////////////////////////////////////////////
///    CLASS SmartPlaylist
////////////////////////////////////////////////////////////////////////////

SmartPlaylist::SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QString &name, const QString &query )
        : PlaylistBrowserEntry( parent, after, name )
        , sqlForTags( query )
        , m_title( name )
        , m_dynamic( false )
{
    setPixmap( 0, SmallIcon( "player_playlist_2" ) );
    setDragEnabled( query.isEmpty() ? false : true );

    setText( 0, name );
}

SmartPlaylist::SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QString &name, const QString &urls, const QString &tags )
        : PlaylistBrowserEntry( parent, after, name )
        , sqlForTags( tags )
        , m_title( name )
        , m_dynamic( false )
{
    setPixmap( 0, SmallIcon( "player_playlist_2" ) );
    setDragEnabled( !urls.isEmpty() && !tags.isEmpty() );

    setText( 0, name );
}


SmartPlaylist::SmartPlaylist( QListViewItem *parent, QListViewItem *after, QDomElement xmlDefinition )
        : PlaylistBrowserEntry( parent, after )
        , m_after ( after )
        , m_dynamic( false )
{
    setPixmap( 0, SmallIcon( "player_playlist_2" ) );
    setXml( xmlDefinition );
    setDragEnabled( !sqlForTags.isEmpty() );
}

void SmartPlaylist::setXml( QDomElement xml ) {
    m_xml = xml;
    m_title = xml.attribute( "name" );
    setText( 0, m_title );
    sqlForTags = xml.namedItem( "sqlquery" ).toElement().text();
    static QStringList genres;
    static QStringList artists;
    static QStringList albums;
    static QStringList years;


    debug() << "Removing old children from smartplaylist..." << endl;
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

void SmartPlaylist::setDynamic( bool enable )
{
    if( enable != m_dynamic )
    {
        enable ?
            setPixmap( 0, SmallIcon( "favorites" ) ) :
            setPixmap( 0, SmallIcon( "player_playlist_2" ) );
        m_dynamic = enable;
    }

}


#include "playlistbrowseritem.moc"
