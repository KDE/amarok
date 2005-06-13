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
#include "metabundle.h"
#include "threadweaver.h"

#include <qfile.h>             //loadPlaylists(), renamePlaylist()
#include <qlabel.h>
#include <qpainter.h>          //paintCell()
#include <qpixmap.h>           //paintCell()

#include <kiconloader.h>       //smallIcon
#include <klocale.h>

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
        font.setPointSize( font.pointSize() + 2 );
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

        if( m_trackCount ) setOpen( true );
        else repaint();

        m_trackCount = m_trackList.count();
    }
}


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
        else {
            load();
            return;
        }

    }
    else {    //collapse

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

    //if the playlist has been modified a save icon is shown
    if( m_modified && m_savePix )
    {
        pBuf.drawPixmap( text_x, (textHeight - m_savePix->height())/2, *m_savePix );
        text_x += m_savePix->width()+4;
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
///    CLASS ItemSaver
////////////////////////////////////////////////////////////////////////////////

ItemSaver::ItemSaver(  QString title, QWidget *parent, const char *name )
    : KDialogBase( parent, name, true, i18n("Save Current Playlist"), Ok|Cancel)
{
    makeVBoxMainWidget();

    QHBox *nameBox = new QHBox( mainWidget() );
    nameBox->setSpacing( 5 );
    new QLabel( i18n("Name:"), nameBox );
    m_nameLineEdit = new KLineEdit( title, nameBox );

    m_nameLineEdit->setFocus();
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
    m_url  = KURL::KURL( e.text() );


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
StreamEditor::StreamEditor( QString defaultName, QWidget *parent, const char *name )
    : KDialogBase( parent, name, true, i18n("Stream Editor"), Ok|Cancel)
{
    makeVBoxMainWidget();

    QHBox *nameBox = new QHBox( mainWidget() );
    nameBox->setSpacing( 5 );
    new QLabel( i18n("Name:"), nameBox );
    m_nameLineEdit = new KLineEdit( defaultName, nameBox );

    QHBox *urlBox = new QHBox( mainWidget() );
    urlBox->setSpacing( 5 );
    new QLabel( i18n("Url:"), urlBox );
    m_urlLineEdit = new KLineEdit( defaultName, urlBox );

    QSize min( 480, 110 );
    setInitialSize( min );

    m_nameLineEdit->setFocus();

}

// For editing
StreamEditor::StreamEditor( QWidget *parent, QString title, QString url, const char *name )
    : KDialogBase( parent, name, true, i18n("Stream Editor"), Ok|Cancel)
{
    makeVBoxMainWidget();

    QHBox *nameBox = new QHBox( mainWidget() );
    nameBox->setSpacing( 5 );
    new QLabel( i18n("Name:"), nameBox );
    m_nameLineEdit = new KLineEdit( title, nameBox );

    QHBox *urlBox = new QHBox( mainWidget() );
    urlBox->setSpacing( 5 );
    new QLabel( i18n("Url:"), urlBox );
    m_urlLineEdit = new KLineEdit( url, urlBox );

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
///    CLASS SmartPlaylist
////////////////////////////////////////////////////////////////////////////

SmartPlaylist::SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QString &name, const QString &query )
        : PlaylistBrowserEntry( parent, after, name )
        , sqlForTags( query )
        , m_title( name )
{
    setPixmap( 0, SmallIcon( "player_playlist_2" ) );
    setDragEnabled( query.isEmpty() ? false : true );

    setText( 0, name );
}

SmartPlaylist::SmartPlaylist( QListViewItem *parent, QListViewItem *after, const QString &name, const QString &urls, const QString &tags )
        : PlaylistBrowserEntry( parent, after, name )
        , sqlForTags( tags )
        , m_title( name )
{
    setPixmap( 0, SmallIcon( "player_playlist_2" ) );
    setDragEnabled( !urls.isEmpty() && !tags.isEmpty() );

    setText( 0, name );
}


SmartPlaylist::SmartPlaylist( QListViewItem *parent, QListViewItem *after, QDomElement xmlDefinition )
        : PlaylistBrowserEntry( parent, after )
        , m_after ( after )
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

#include "playlistbrowseritem.moc"
