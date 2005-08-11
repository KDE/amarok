/* Copyright 2002-2004 Mark Kretschmann, Max Howell
 * Copyright 2005 Seb Ruiz, Mike Diehl, Ian Monroe, Gábor Lehel
 * Licensed as described in the COPYING file found in the root of this distribution
 * Maintainer: Max Howell <max.howell@methylblue.com>

 * NOTES
 *
 * The PlaylistWindow handles some Playlist events. Thanks!
 * This class has a QOBJECT but it's private so you can only connect via PlaylistWindow::PlaylistWindow
 * Mostly it's sensible to implement playlist functionality in this class
 * TODO Obtaining information about the playlist is currently hard, we need the playlist to be globally
 *      available and have some more useful public functions
 */

#define DEBUG_PREFIX "Playlist"

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "collectiondb.h"
#include "collectionbrowser.h"
#include "enginecontroller.h"
#include "k3bexporter.h"
#include "metabundle.h"
#include "osd.h"
#include "playlist.h"
#include "playlistitem.h"
#include "playlistbrowser.h"
#include "playlistloader.h"
#include "queuemanager.h"
#include "scriptmanager.h"
#include "statusbar.h"       //for status messages
#include "tagdialog.h"
#include "threadweaver.h"

#include <qclipboard.h>      //copyToClipboard(), slotMouseButtonPressed()
#include <qcolor.h>
#include <qevent.h>
#include <qfile.h>           //undo system
#include <qheader.h>         //eventFilter()
#include <qlabel.h>           //showUsageMessage()
#include <qmap.h>            //dragObject()
#include <qpainter.h>
#include <qpen.h>            //slotGlowTimer()
#include <qsortedlist.h>
#include <qtimer.h>
#include <qvaluelist.h>      //addHybridTracks()
#include <qvaluevector.h>    //playNextTrack()
#include <qlayout.h>

#include <kaction.h>
#include <kapplication.h>
#include <kcursor.h>         //setOverrideCursor()
#include <kdialogbase.h>
#include <kglobalsettings.h> //rename()
#include <kiconloader.h>     //slotShowContextMenu()
#include <kio/job.h>         //deleteSelectedFiles()
#include <klineedit.h>       //setCurrentTrack()
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <krandomsequence.h> //random Mode
#include <kstandarddirs.h>   //KGlobal::dirs()
#include <kstdaction.h>
#include <kstringhandler.h>  //::showContextMenu()
#include <kurldrag.h>

extern "C"
{
    #if KDE_VERSION < KDE_MAKE_VERSION(3,3,91)
    #include <X11/Xlib.h>    //ControlMask in contentsDragMoveEvent()
    #endif
}


/**
 * Iterator class that only edits visible items! Preferentially always use
 * this! Invisible items should not be operated on! To iterate over all
 * items use MyIt::All as the flags parameter. MyIt::All cannot be OR'd,
 * sorry.
 */

class MyIterator : public QListViewItemIterator
{
public:
    MyIterator( QListViewItem *item, int flags = 0 )
        //QListViewItemIterator is not great and doesn't allow you to see everything if you
        //mask both Visible and Invisible :( instead just visible items are returned
        : QListViewItemIterator( item, flags == All ? 0 : flags | Visible  )
    {}

    MyIterator( QListView *view, int flags = 0 )
        : QListViewItemIterator( view, flags == All ? 0 : flags | Visible )
    {}

    //FIXME! Dirty hack for enabled/disabled items.
    enum IteratorFlag {
        Visible = QListViewItemIterator::Visible,
        All = QListViewItemIterator::Invisible
    };

    PlaylistItem *operator*() { return (PlaylistItem*)QListViewItemIterator::operator*(); }

    /// @return the next visible PlaylistItem after item
    static PlaylistItem *nextVisible( PlaylistItem *item )
    {
        MyIterator it( item );
        return (*it == item) ? *(MyIterator&)(++it) : *it;
    }

};

typedef MyIterator MyIt;


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS TagWriter : Threaded tag-updating
//////////////////////////////////////////////////////////////////////////////////////////

class TagWriter : public ThreadWeaver::Job
{ //TODO make this do all tags at once when you split playlist.cpp up
public:
    TagWriter( PlaylistItem*, const QString &oldTag, const QString &newTag, const int, const bool updateView = true );
   ~TagWriter();
    bool doJob();
    void completeJob();
private:
    PlaylistItem* const m_item;
    bool m_failed;

    QString m_oldTagString;
    QString m_newTagString;
    int     m_tagType;
    bool    m_updateView;
};

//////////////////////////////////////////////////////////////////////////////////////////
/// Glow
//////////////////////////////////////////////////////////////////////////////////////////

namespace Glow
{
    namespace Text
    {
        static float dr, dg, db;
        static int    r, g, b;
    }
    namespace Base
    {
        static float dr, dg, db;
        static int    r, g, b;
    }

    static const uint STEPS = 13;
    static uint counter;
    static QTimer timer;

    inline void startTimer()
    {
        counter = 0;
        timer.start( 40 );
    }

    inline void reset()
    {
        counter = 0;
        timer.stop();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS Playlist
//////////////////////////////////////////////////////////////////////////////////////////

static inline bool isDynamic() { return AmarokConfig::dynamicMode(); }

Playlist *Playlist::s_instance = 0;

Playlist::Playlist( QWidget *parent )
        : KListView( parent, "ThePlaylist" )
        , EngineObserver( EngineController::instance() )
        , m_currentTrack( 0 )
        , m_marker( 0 )
        , m_firstColumn( 0 )
        , m_totalLength( 0 )
        , m_selCount( 0 )
        , m_selLength( 0 )
        , m_visCount( 0 )
        , m_visLength( 0 )
        , m_undoDir( amaroK::saveLocation( "undo/" ) )
        , m_undoCounter( 0 )
        , m_stopAfterTrack( 0 )
        , m_showHelp( true )
        , m_stateSwitched( false )
        , m_partyDirt( false )
        , m_queueDirt( false )
        , m_undoDirt( false )
        , m_lockStack( 0 )
        , m_columnFraction( 14, 0 )
{
    s_instance = this;

    EngineController* const ec = EngineController::instance();
    connect( ec, SIGNAL(orderPrevious()), SLOT(playPrevTrack()) );
    connect( ec, SIGNAL(orderNext( const bool )),     SLOT(playNextTrack( const bool )) );
    connect( ec, SIGNAL(orderCurrent()),  SLOT(playCurrentTrack()) );


    setShowSortIndicator( true );
    setDropVisualizer( false );   //we handle the drawing for ourselves
    setDropVisualizerWidth( 3 );
    setItemsRenameable( true );
    KListView::setSorting( NO_SORT ); //use base so we don't saveUndoState() too
    setAcceptDrops( true );
    setSelectionMode( QListView::Extended );
    setAllColumnsShowFocus( true );
    //setItemMargin( 1 ); //aesthetics

    #if KDE_IS_VERSION( 3, 3, 91 )
    setShadeSortColumn( true );
    #endif

    //NOTE order is critical because we can't set indexes or ids
    addColumn( i18n( "Filename" ),   0 );
    addColumn( i18n( "Title"      ), 200 ); //displays filename if no title tag
    addColumn( i18n( "Artist"     ), 100 );
    addColumn( i18n( "Album"      ), 100 );
    addColumn( i18n( "Year"       ),   0 ); //0 means hidden
    addColumn( i18n( "Comment"    ),   0 );
    addColumn( i18n( "Genre"      ),   0 );
    addColumn( i18n( "Track"      ),   0 );
    addColumn( i18n( "Directory"  ),   0 );
    addColumn( i18n( "Length"     ),  80 );
    addColumn( i18n( "Bitrate"    ),   0 );
    addColumn( i18n( "Score"      ),   0 );
    addColumn( i18n( "Type"       ),   0 );
    addColumn( i18n( "Playcount"  ),   0 );

    setRenameable( 0, false ); //TODO allow renaming of the filename
    setRenameable( 1 );
    setRenameable( 2 );
    setRenameable( 3 );
    setRenameable( 4 );
    setRenameable( 5 );
    setRenameable( 6 );
    setRenameable( 7 );
    setRenameable( 11 );
    setRenameable( 12, false );
    setRenameable( 13, false );
    setColumnAlignment(  7, Qt::AlignCenter ); //track
    setColumnAlignment(  9, Qt::AlignRight );  //length
    setColumnAlignment( 10, Qt::AlignCenter ); //bitrate
    setColumnAlignment( 11, Qt::AlignCenter ); //score
    setColumnAlignment( 12, Qt::AlignCenter ); //extension
    setColumnAlignment( 13, Qt::AlignCenter ); //playcount


    connect( this,     SIGNAL( doubleClicked( QListViewItem* ) ),
             this,       SLOT( doubleClicked( QListViewItem* ) ) );
    connect( this,     SIGNAL( returnPressed( QListViewItem* ) ),
             this,       SLOT( activate( QListViewItem* ) ) );
    connect( this,     SIGNAL( mouseButtonPressed( int, QListViewItem*, const QPoint&, int ) ),
             this,       SLOT( slotMouseButtonPressed( int, QListViewItem*, const QPoint&, int ) ) );
    connect( this,     SIGNAL( selectionChanged() ),
             this,       SLOT( slotSelectionChanged() ) );
    connect( this,     SIGNAL( itemRenamed( QListViewItem*, const QString&, int ) ),
             this,       SLOT( writeTag( QListViewItem*, const QString&, int ) ) );
    connect( this,     SIGNAL( aboutToClear() ),
             this,       SLOT( saveUndoState() ) );
    connect( CollectionDB::instance(), SIGNAL( scoreChanged( const QString&, int ) ),
             this,       SLOT( scoreChanged( const QString&, int ) ) );
    connect( CollectionDB::instance(), SIGNAL( scoreChanged( const QString&, int ) ),
             this,       SLOT( countChanged( const QString& ) ) );
    connect( header(), SIGNAL( indexChange( int, int, int ) ),
             this,       SLOT( columnOrderChanged() ) ),


    connect( &Glow::timer, SIGNAL(timeout()), SLOT(slotGlowTimer()) );


    KActionCollection* const ac = amaroK::actionCollection();
    KStdAction::copy( this, SLOT( copyToClipboard() ), ac, "playlist_copy" );
    KStdAction::selectAll( this, SLOT( selectAll() ), ac, "playlist_select_all" );
    m_clearButton = KStdAction::clear( this, SLOT( clearAndSave() ), ac, "playlist_clear" );
    m_undoButton  = KStdAction::undo( this, SLOT( undo() ), ac, "playlist_undo" );
    m_redoButton  = KStdAction::redo( this, SLOT( redo() ), ac, "playlist_redo" );
    new KAction( i18n( "S&huffle" ), "rebuild", CTRL+Key_H, this, SLOT( shuffle() ), ac, "playlist_shuffle" );
    new KAction( i18n( "&Goto Current Track" ), "today", CTRL+Key_Enter, this, SLOT( showCurrentTrack() ), ac, "playlist_show" );
    new KAction( i18n( "&Remove Duplicate && Dead Entries" ), 0, this, SLOT( removeDuplicates() ), ac, "playlist_remove_duplicates" );
    new KAction( i18n( "&Queue Selected Tracks" ), CTRL+Key_D, this, SLOT( queueSelected() ), ac, "queue_selected" );

    //ensure we update action enabled states when repeat Playlist is toggled
    connect( ac->action( "repeat_playlist" ), SIGNAL(toggled( bool )), SLOT(updateNextPrev()) );

    m_clearButton->setIcon( "view_remove" );
    m_undoButton->setEnabled( false );
    m_redoButton->setEnabled( false );


    engineStateChanged( EngineController::engine()->state() ); //initialise state of UI
    paletteChange( palette() ); //sets up glowColors
    restoreLayout( KGlobal::config(), "PlaylistColumnsLayout" );
    columnOrderChanged();
    //cause the column fractions to be updated, but in a safe way, ie no specific column
    columnResizeEvent( header()->count(), 0, 0 );

    //do after you resize all the columns
    connect( header(), SIGNAL(sizeChange( int, int, int )), SLOT(columnResizeEvent( int, int, int )) );

    header()->installEventFilter( this );

    m_filtertimer = new QTimer( this );
    connect( m_filtertimer, SIGNAL(timeout()), this, SLOT(setDelayedFilter()) );
}

Playlist::~Playlist()
{
    saveLayout( KGlobal::config(), "PlaylistColumnsLayout" );

    if( AmarokConfig::savePlaylist() )
        saveXML( defaultPlaylistPath() );
    else
        QFile::remove( defaultPlaylistPath() );

    //clean undo directory
    QStringList list = m_undoDir.entryList();
    for( QStringList::ConstIterator it = list.constBegin(), end = list.constEnd(); it != end; ++it )
        m_undoDir.remove( *it );

    //speed up quit a little
    KListView::clear();   //our implementation is slow
    blockSignals( true ); //might help
}


////////////////////////////////////////////////////////////////////////////////
/// Media Handling
////////////////////////////////////////////////////////////////////////////////

void
Playlist::insertMedia( KURL::List list, int options )
{
    bool directPlay = options & DirectPlay;

    if( options & Unique ) {
        //passing by value is quick for QValueLists, though it is slow
        //if we change the list, but this is unlikely
        KURL::List::Iterator jt;

        for( MyIt it( this, MyIt::All ); *it; ++it ) {
            jt = list.find( (*it)->url() );

            if ( jt != list.end() ) {
                if ( directPlay && jt == list.begin() ) {
                    directPlay = false;
                    activate( *it );
                }

                list.remove( jt );
            }
        }
    }

    PlaylistItem *after = 0;

    if( options & Replace )
       clear();

    else if( options & Queue )
    {
        KURL::List addMe = list;
        KURL::List::Iterator jt;

        // add any songs not in the playlist to it.
        for( MyIt it( this, MyIt::All ); *it; ++it ) {
            jt = addMe.find( (*it)->url() );

            if ( jt != addMe.end() ) {
                addMe.remove( jt ); //dont want to add a track which is already present in the playlist
            }
        }

        if ( addMe.isEmpty() ) // all songs to be queued are already in the playlist
        {
            // find the songs and queue them.
            for (MyIt it( this, MyIt::All ); *it; ++it ) {
                jt = list.find( (*it)->url() );

                if ( jt != list.end() )
                {
                    queue( *it );
                    list.remove( jt );
                }
            }
        } else {
            // We add the track after the last track on queue, or after current if the queue is empty
            after =  m_nextTracks.isEmpty() ? currentTrack() : m_nextTracks.getLast();

            // wait until Playlist loader has finished its process, then go to customEvent() to start the queue process.
            m_queueList = list;
            insertMediaInternal( addMe, after, directPlay );
        }
        return;

    }
    else
        //we do this by default, even if we were passed some stupid flag combination
        after = lastItem();

    insertMediaInternal( list, after, directPlay );
}

void
Playlist::insertMediaInternal( const KURL::List &list, PlaylistItem *after, bool directPlay )
{
    if ( !list.isEmpty() ) {
        setSorting( NO_SORT );

        // prevent association with something that is about to be deleted
        // TODO improve the playlist with a list of items that are volatile or something
        while( after && after->exactText( 0 ) == "MARKERITEM" )
            after = (PlaylistItem*)after->itemAbove();

        ThreadWeaver::instance()->queueJob( new UrlLoader( list, after, directPlay ) );
    }
    else
        amaroK::StatusBar::instance()->shortMessage( i18n("Attempted to insert nothing into playlist.") );
}

void
Playlist::insertMediaSql( const QString& sql, int options )
{
    // TODO Implement more options
    PlaylistItem *after = 0;

    if ( options & Replace )
        clear();
    if ( options & Append )
        after = lastItem();

    setSorting( NO_SORT );
    ThreadWeaver::instance()->queueJob( new SqlLoader( sql, after ) );
}

void
Playlist::addSpecialTracks( uint songCount, const QString type )
{
    if( !songCount ) return;

    QueryBuilder qb;
    qb.setOptions( QueryBuilder::optRandomize | QueryBuilder::optRemoveDuplicates );
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valURL );

    int  currentPos = 0;
    for( MyIt it( this, MyIt::Visible ); *it; ++it )
    {
        if( m_currentTrack && *it == m_currentTrack )
            break;
        else if( !m_currentTrack && (*it)->isEnabled() )
            break;

        ++currentPos;
    }
    currentPos++;

    int required  = currentPos + AmarokConfig::dynamicUpcomingCount(); // currentPos handles currentTrack
    int remainder = childCount();

    if( required > remainder )
        songCount = required - remainder;


    QString text;
    if( type == "Random" )
    {
        songCount > 1 ?
            amaroK::StatusBar::instance()->shortMessage( i18n("Adding random tracks.") ):
            amaroK::StatusBar::instance()->shortMessage( i18n("Adding random track.") );

    }
    else if( type == "Suggestion" )
    {
        if( !m_currentTrack ) return;
        QStringList suggestions = CollectionDB::instance()->similarArtists( currentTrack()->artist(), 16 );
        qb.addMatches( QueryBuilder::tabArtist, suggestions );
        songCount > 1 ?
            amaroK::StatusBar::instance()->shortMessage( i18n("Adding suggested tracks.") ):
            amaroK::StatusBar::instance()->shortMessage( i18n("Adding suggested track.") );
    }
    else //we have playlists to choose from.
    {
        if( songCount > 1 )
            text = i18n("Adding tracks from custom filter.");
        else
            text = i18n("Adding track from custom filter.");

        songCount > 1 ?
            amaroK::StatusBar::instance()->shortMessage( i18n("Adding tracks from custom filter.") ):
            amaroK::StatusBar::instance()->shortMessage( i18n("Adding track from custom filter.") );

        addSpecialCustomTracks( songCount );
        return;
    }

    qb.setLimit( 0, songCount );
    QStringList url = qb.run();
    //FIXME: No items to add or if user wants non-unique entries!
    if( url.isEmpty() )
    {
        amaroK::StatusBar::instance()->shortMessage( i18n("No tracks were returned to be inserted.") );
        return;
    }
    insertMedia( KURL::List( url ), Playlist::Unique );
}

void
Playlist::addSpecialCustomTracks( uint songCount )
{
    if( !songCount )
        return;

    PlaylistBrowser *pb = PlaylistBrowser::instance() ? PlaylistBrowser::instance() : new PlaylistBrowser();
    QListViewItem *item = 0;

    QPtrList<QListViewItem> dynamicEntries = pb->dynamicEntries();

    //FIXME: What if the randomiser grabs the same playlist again and again?  Lets remove the playlist from the list.
    for( uint y=0; y < dynamicEntries.count(); y++ )
    {
        int x = KApplication::random() % dynamicEntries.count();

        item = dynamicEntries.at( x );

        if( item )
            break;
    }

    if ( !item ) {
        debug() << "[PARTY]: No valid source found." << endl;
        amaroK::StatusBar::instance()->shortMessage( i18n("No valid sources set for this Dynamic Playlist.") );
        return;
    }

    if( item->rtti() == PlaylistEntry::RTTI )
    {
        #define item static_cast<PlaylistEntry *>(item)

        KURL::List urls;
        KURL::List trackList;
        trackList = item->tracksURL();

        for( uint i=0; i < songCount; i++ )
        {
            int x = KApplication::random() % trackList.count();
            KURL::List::Iterator it = trackList.at( x );

            if( (*it).isValid() )
                urls << (*it).path();
        }

        if( urls.isEmpty() )
            amaroK::StatusBar::instance()->longMessage( i18n(
                "<div align=\"center\"><b>Warning</b></div>"
                "The playlist titled <i>%1</i> contains no tracks."
                "<br><br>"
                "Please modify your playlist or choose a different source." ).arg( item->text(0) ) );
        else
            insertMedia( urls );

        #undef item
    }
    else if( item->rtti() == SmartPlaylist::RTTI  )
    {
        #define sp static_cast<SmartPlaylist *>(item)
        QString query = sp->query();
        bool useDirect = false;

        QString sql = sp->sqlForTags;

        if ( sql.find( QString("ORDER BY"), FALSE ) == -1 ) {
            QRegExp limit( ";$" );
            sql.replace( limit, QString(" ORDER BY RAND() LIMIT 0, %1;").arg( songCount ) );
            useDirect = true;
        }

        QStringList queryResult = CollectionDB::instance()->query( sql );

        QStringList items;

        if ( !sp->sqlForTags.isEmpty() ) {
            //We have to filter all the un-needed results from query( sql )
            for (uint x=10; x < queryResult.count() ; x += 11)
                items << queryResult[x];
        } else {
            items = queryResult;
        }

        KURL::List urls = KURL::List( items );
        KURL::List addMe;

        for( uint i=0; !useDirect && i < songCount; i++ )
        {
            int x = KApplication::random() % urls.count();
            addMe << urls[x];
        }

        if( urls.isEmpty() )
            amaroK::StatusBar::instance()->longMessage( i18n(
                "<div align=\"center\"><b>Warning</b></div>"
                "The smart-playlist titled <i>%1</i> contains no tracks."
                "<br><br>"
                "Please modify your smart-playlist or choose a different source." ).arg( item->text(0) ) );
        else
            insertMedia( useDirect ? urls : addMe );

        #undef sp
    }
}

/**
 *  @param songCount : Number of tracks to be shown after the current track
 *  @param type      : Type of tracks to append if required (Random/Suggestion/Custom)
 */

void
Playlist::adjustPartyUpcoming( uint songCount, const QString type )
{
    bool requireTracks = false;
    int  currentPos = 0;
    int  x = 0;

    /**
     *  If m_currentTrack exists, we iterate until we find it
     *  Else, we iterate until we find an item which is enabled
     **/
    for( MyIt it( this, MyIt::Visible ); *it; ++it )
    {
        if( m_currentTrack && *it == m_currentTrack )
            break;
        else if( !m_currentTrack && (*it)->isEnabled() )
            break;

        ++currentPos;
    }
    currentPos++;

    if( (int)songCount > AmarokConfig::dynamicUpcomingCount() )
    {
        x = songCount - AmarokConfig::dynamicUpcomingCount();
        requireTracks = true;
    }
    else
    {
        x = childCount() - songCount - currentPos;
    }
    debug() << "x: " << x << "; Require tracks? " << requireTracks << endl;

    if ( requireTracks )
    {
        addSpecialTracks( x, type );
    }
    else
    {
        if( isLocked() ) return;

        //assemble a list of what needs removing
        //calling removeItem() iteratively is more efficient if they are in _reverse_ order, hence the prepend()
        QPtrList<QListViewItem> list;
        QListViewItem *item = lastItem();

        for( int y = x; y != 0; y-- )
        {
            list.append( item );

            if( !item->itemAbove() )
                break;
            item = item->itemAbove();
        }


        if( list.isEmpty() ) return;
        saveUndoState();

        //remove the items
        for( QListViewItem *item = list.first(); item; item = list.next() )
        {
            removeItem( (PlaylistItem*)item );
            delete item;
        }
        //NOTE no need to emit childCountChanged(), removeItem() does that for us
    }
}

/**
 *  @param songCount : Number of tracks to be shown before the current track
  */

void
Playlist::adjustPartyPrevious( uint songCount )
{
    int current = currentTrackIndex();
    int x = current - songCount;

    QPtrList<QListViewItem> list;
    int y=0;
    for( QListViewItemIterator it( firstChild() ); y < x ; list.prepend( *it ), ++it, y++ );

    if( list.isEmpty() ) return;
    saveUndoState();

    //remove the items
    for( QListViewItem *item = list.first(); item; item = list.next() )
    {
        removeItem( (PlaylistItem*)item );
        delete item;
    }
}

void
Playlist::alterHistoryItems( bool enable /*FALSE*/, bool entire /*FALSE*/ )
{
    //NOTE: we must make sure that partyMode works perfectly as we expect it to,
    //      for this functionality to be guarranteed. <sebr>

    if( !entire && !m_currentTrack )
        return;

    // Disable all items, since we should be leaving dynamic mode.
    for( MyIterator it( this, MyIterator::All ) ; *it ; ++it )
    {
        if( !entire )
        {
            if( *it == m_currentTrack )          break;
            if( !enable && !(*it)->isEnabled() ) break;
        }

        //avoid repainting if we can.
        if( (*it)->isEnabled() != enable )
        {
            (*it)->setEnabled( enable );
            repaintItem( *it );
        }
    }
}

QString
Playlist::defaultPlaylistPath() //static
{
    return amaroK::saveLocation() + "current.xml";
}

void
Playlist::restoreSession()
{
    KURL url;
    url.setPath( amaroK::saveLocation() + "current.xml" );

    // check it exists, because on the first ever run it doesn't and
    // it looks bad to show "some URLs were not suitable.." on the
    // first ever-run
    if( QFile::exists( url.path() ) )
    {
        //allows for history items to be re-enabled
        if( isDynamic() )
            m_stateSwitched = true;
        ThreadWeaver::instance()->queueJob( new UrlLoader( url, 0 ) );
    }
}



////////////////////////////////////////////////////////////////////////////////
/// Current Track Handling
////////////////////////////////////////////////////////////////////////////////

void
Playlist::playNextTrack( bool forceNext )
{
    PlaylistItem *item = currentTrack();

    if( isEmpty() || ( m_currentTrack && m_stopAfterTrack == m_currentTrack ) )
    {
        if( isDynamic() && !isEmpty() )
            advancePartyTrack( item );

        m_stopAfterTrack = 0;
        activate( 0 );

        return;
    }

    if( !AmarokConfig::repeatTrack() || forceNext )
    {
        if( !m_nextTracks.isEmpty() )
        {
            item = m_nextTracks.first();
            m_nextTracks.remove();
        }

        else if( AmarokConfig::randomMode() )
        {
            QValueVector<PlaylistItem*> tracks;

            //make a list of everything we can play
            for( MyIt it( this ); *it; ++it )
                if ( !m_prevTracks.containsRef( *it ) )
                    tracks.push_back( *it );

            if( tracks.isEmpty() )
            {
                //we have played everything

                if ( m_prevTracks.count() <= 80 ) {
                    m_prevTracks.clear();

                    // don't add it to previous tracks if we only have one file in the playlist
                    // would loop infinitely otherwise
                    int count = 0;
                    for( MyIterator it( this, MyIterator::Visible ); *it; ++it )
                        ++count;

                    if ( count > 1 )
                        m_prevTracks.append( m_currentTrack );
                }
                else {
                    m_prevTracks.first(); //set's current item to first item

                    //keep 80 tracks in the previous list so item time user pushes play
                    //we don't risk playing anything too recent
                    while( m_prevTracks.count() > 80 )
                        m_prevTracks.remove(); //removes current item
                }

                if( AmarokConfig::repeatPlaylist() )
                {
                    playNextTrack();
                    return;
                }
                //else we stop via activate( 0 ) below
            }
            else item = tracks.at( KApplication::random() % tracks.count() ); //is O(1)
        }
        else if( item )
        {
            item = MyIt::nextVisible( item );
            while( item && !item->isEnabled() )
                item = MyIt::nextVisible( item );
        }
        else
        {
            item = *MyIt( this ); //ie. first visible item
            while( item && !item->isEnabled() )
                item = item->nextSibling();
        }


        if ( isDynamic() && item != firstChild() )
            advancePartyTrack();

        if ( !item && AmarokConfig::repeatPlaylist() )
            item = *MyIt( this ); //ie. first visible item
    }


    if ( EngineController::engine()->loaded() )
        activate( item );
    else
        setCurrentTrack( item );
}

//This is called before setCurrentItem( item );
void
Playlist::advancePartyTrack( PlaylistItem *item )
{
    MyIterator it( this, MyIterator::Visible );

    if( !item ) item = currentTrack();

    int x;
    for( x=0 ; *it; ++it, x++ )
    {
        if( *it == item )
        {
            if( AmarokConfig::dynamicMarkHistory() ) (*it)->setEnabled( false );
            if( x < AmarokConfig::dynamicPreviousCount() )
                break;

            if( AmarokConfig::dynamicCycleTracks() )
            {
                PlaylistItem *first = firstChild();
                if( first )
                {
                    removeItem( first ); //first visible item
                    delete first;
                }
            }
            break;
        }
    }

    //keep upcomingTracks requirement, this seems to break StopAfterCurrent
    if( m_stopAfterTrack != m_currentTrack )
    {
        int appendNo = AmarokConfig::dynamicAppendCount();
        if( appendNo ) addSpecialTracks( appendNo, AmarokConfig::dynamicType() );
    }
    m_partyDirt = true;
}

void
Playlist::playPrevTrack()
{
    PlaylistItem *item = m_currentTrack;

    if ( !AmarokConfig::randomMode() || m_prevTracks.count() <= 1 )
        item = *(MyIt&)--MyIt( item ); //the previous track to item that is visible

    else {
        // if enough songs in buffer, jump to the previous one
        m_prevTracks.last();
        m_prevTracks.remove(); //remove the track playing now
        item = m_prevTracks.last();

        // we need to remove this item now, since it will be added in activate() again
        m_prevTracks.remove();
    }

    if ( !item && AmarokConfig::repeatPlaylist() )
        item = *MyIt( lastItem() ); //TODO check this works!

    if ( EngineController::engine()->loaded() )
        activate( item );
    else
        setCurrentTrack( item );
}

void
Playlist::playCurrentTrack()
{
    if ( !currentTrack() )
        playNextTrack( AmarokConfig::repeatTrack() ? true : false );

    //we must do this even if the above is correct
    //since the engine is not loaded the first time the user presses play
    //then calling the next() function wont play it
    activate( currentTrack() );
}

void
Playlist::queueSelected()
{
    for( MyIt it( this, MyIt::Selected ); *it; ++it )
        queue( *it );
}

void
Playlist::queue( QListViewItem *item )
{
    #define item static_cast<PlaylistItem*>(item)

    const int  queueIndex  = m_nextTracks.findRef( item );
    const bool isQueued    = queueIndex != -1;

    item->setSelected( false ); //for prettiness

    if( isQueued )
    {
        //remove the item, this is better way than remove( item )
        m_nextTracks.remove( queueIndex ); //sets current() to next item

        if( isDynamic() ) // we move the item after the last queued item to preserve the ordered 'queue'.
        {
            PlaylistItem *after = m_nextTracks.last();

            if( after )
                this->moveItem( item, 0, after );
        }
    }
    else if( !isDynamic() )
        m_nextTracks.append( item );

    else
    {
        PlaylistItem *after;
        m_nextTracks.isEmpty() ?
            after = m_currentTrack :
            after = m_nextTracks.last();

        if( !after )
        {
            after = firstChild();
            while( after && !after->isEnabled() )
            {
                if( after->nextSibling()->isEnabled() )
                    break;
                after = after->nextSibling();
            }
        }

        if( item->isEnabled() && item != m_currentTrack )
        {
            this->moveItem( item, 0, after );
            m_nextTracks.append( item );
        }
        else
        {
            m_queueDirt = true;
            insertMediaInternal( item->url(), after );
        }
    }

    if( QueueManager::instance() )
        emit queued( item );

    refreshNextTracks(); // from current()

    //NOTE "item" is repainted due to the setSelected() call
    // ^__ not if called in bulk by context/collection browsers

    updateNextPrev();
    #undef item
}

void
Playlist::sortQueuedItems()
{
    PlaylistItem *last = m_currentTrack;
    for( PlaylistItem *item = m_nextTracks.getFirst(); item; item = m_nextTracks.next() )
    {
        if( item->itemAbove() != last )
            this->moveItem( item, 0, last );

        last = item;
    }

}

void Playlist::setStopAfterCurrent( bool on )
{
    PlaylistItem *prev_stopafter = m_stopAfterTrack;

    if( on )
        m_stopAfterTrack = m_currentTrack;
    else
        m_stopAfterTrack = 0;

    if( m_stopAfterTrack )
        m_stopAfterTrack->repaint();
    if( prev_stopafter )
        prev_stopafter->repaint();
}

void Playlist::doubleClicked( QListViewItem *item )
{
    /* We have to check if the item exists before calling activate, otherwise clicking on an empty
    playlist space would stop playing (check BR #105106)*/
    if( item )
        if( item->isEnabled() )
            activate( item );
}

void
Playlist::activate( QListViewItem *item )
{
    ///item will be played if possible, the playback may be delayed
    ///so we start the glow anyway and hope

    //All internal requests for playback should come via
    //this function please!

    if( !item )
    {
        //we have reached the end of the playlist
        EngineController::instance()->stop();
        setCurrentTrack( 0 );
        amaroK::OSD::instance()->OSDWidget::show( i18n("Playlist finished"),
                                            QImage( KIconLoader().iconPath( "amarok", -KIcon::SizeHuge ) ) );
        return;
    }

    #define item static_cast<PlaylistItem*>(item)

    if( isDynamic() && !m_partyDirt )
    {
        if( m_currentTrack && item->isEnabled() )
            this->moveItem( item, 0, m_currentTrack );
        else
        {
            MyIt it( this, MyIt::Visible );
            bool hasHistory = false;
            if ( *it && !(*it)->isEnabled() )
            {
                hasHistory = true;
                for(  ; *it && !(*it)->isEnabled() ; ++it );
            }

            if( item->isEnabled() )
            {
                hasHistory ?
                    this->moveItem( item, *it, 0 ) :
                    this->moveItem( item, 0,   0 );
            }
            else // !item->isEnabled()
            {
                hasHistory ?
                    insertMediaInternal( item->url(), *it ):
                    insertMediaInternal( item->url(), 0 );
                m_partyDirt = true;
                return;
            }

        }
        advancePartyTrack();
    }


    if( !item->isEnabled() )
        return;

    m_prevTracks.append( item );

    //if we are playing something from the next tracks
    //list, remove it from the list
    if( m_nextTracks.removeRef( item ) )
        refreshNextTracks();

    //looks bad painting selected and glowing
    //only do when user explicitely activates an item though
    item->setSelected( false );

    setCurrentTrack( item );

    m_partyDirt = false;

    //use PlaylistItem::MetaBundle as it also updates the audioProps
    EngineController::instance()->play( item );
    #undef item
}

void
Playlist::activateByIndex( int index )
{
    QListViewItem* item = itemAtIndex( index );

    if ( item )
        activate(item);
}

void
Playlist::setCurrentTrack( PlaylistItem *item )
{
    ///mark item as the current track and make it glow

    PlaylistItem *prev = m_currentTrack;

    //FIXME best method would be to observe usage, especially don't shift if mouse is moving nearby
    if( item && AmarokConfig::playlistFollowActive() && !renameLineEdit()->isVisible() && selectedItems().count() < 2 )
    {
        if( !prev )
            //if nothing is current and then playback starts, we must show the currentTrack
            ensureItemVisible( item ); //handles 0 gracefully

        else {
            const int prevY = itemPos( prev );
            const int prevH = prev->height();

            // check if the previous track is visible
            if( prevY <= contentsY() + visibleHeight() && prevY + prevH >= contentsY() )
            {
                // in random mode always jump, if previous track is visible
                if( AmarokConfig::randomMode() )
                    ensureItemVisible( item );

                //FIXME would be better to just never be annoying
                // so if the user caused the track change, always show the new track
                // but if it is automatic be careful

                // if old item in view then try to keep the new one near the middle
                const int y = itemPos( item );
                const int h = item->height();
                const int vh = visibleHeight();
                const int amount = h * 3;

                int d = y - contentsY();

                if( d > 0 ) {
                    d += h;
                    d -= vh;

                    if( d > 0 && d <= amount )
                        // scroll down
                        setContentsPos( contentsX(), y - vh + amount );
                }
                else if( d >= -amount )
                    // scroll up
                    setContentsPos( contentsX(), y - amount );
            }
        }
    }

    m_currentTrack = item;

    if ( prev ) {
        //reset to normal height
        prev->invalidateHeight();
        prev->setup();
        //remove pixmap in first column
        prev->setPixmap( m_firstColumn, QPixmap() );
    }

    updateNextPrev();

    setCurrentTrackPixmap();

    Glow::reset();
    slotGlowTimer();
}

int
Playlist::currentTrackIndex()
{
    int index = 0;
    for( MyIt it( this, MyIt::Visible ); *it; ++it )
    {
        if ( *it == m_currentTrack )
            return index;
        ++index;
    }

    return -1;
}

int
Playlist::totalTrackCount()
{
    return childCount();
}

void
Playlist::setCurrentTrackPixmap( int state )
{
    if( !m_currentTrack )
        return;

    QString pixmap = QString::null;

    if( state < 0 )
        state = EngineController::engine()->state();

    if( state == Engine::Paused )
        pixmap = "currenttrack_pause";
    else if( state == Engine::Playing )
        pixmap = "currenttrack_play";

    m_currentTrack->setPixmap( m_firstColumn, pixmap.isNull() ? QPixmap() : amaroK::getPNG( pixmap ) );
    PlaylistItem::setPixmapChanged();
}

PlaylistItem*
Playlist::restoreCurrentTrack()
{
    ///It is always possible that the current track has been lost
    ///eg it was removed and then reinserted, here we check

    const KURL &url = EngineController::instance()->playingURL();

    if( !(m_currentTrack && m_currentTrack->url() == url) )
    {
        PlaylistItem* item;

        for( item = firstChild();
             item && item->url() != url;
             item = item->nextSibling() )
        {}

        setCurrentTrack( item ); //set even if NULL
    }

    return m_currentTrack;
}

bool
Playlist::isTrackAfter() const
{
    ///Is there a track after the current track?
    //order is carefully crafted, remember count() is O(n)
    //TODO randomMode will end if everything is in prevTracks

   return !currentTrack() && !isEmpty() ||
            !m_nextTracks.isEmpty() ||
            currentTrack() && currentTrack()->itemBelow() ||
            childCount() > 1 && ( AmarokConfig::randomMode() || AmarokConfig::repeatPlaylist() );
}

bool
Playlist::isTrackBefore() const
{
    //order is carefully crafted, remember count() is O(n)

    return !isEmpty() &&
           (
               currentTrack() && (currentTrack()->itemAbove() || AmarokConfig::repeatPlaylist() && childCount() > 1)
               ||
               AmarokConfig::randomMode() && childCount() > 1
           );
}

void
Playlist::updateNextPrev()
{
    amaroK::actionCollection()->action( "play" )->setEnabled( !isEmpty() );
    amaroK::actionCollection()->action( "prev" )->setEnabled( isTrackBefore() && !isDynamic() );
    amaroK::actionCollection()->action( "next" )->setEnabled( isTrackAfter() );
    amaroK::actionCollection()->action( "playlist_clear" )->setEnabled( !isEmpty() );

    if( m_currentTrack )
        // ensure currentTrack is shown at correct height
        m_currentTrack->setup();
}


////////////////////////////////////////////////////////////////////////////////
/// EngineObserver Reimplementation
////////////////////////////////////////////////////////////////////////////////

void
Playlist::engineNewMetaData( const MetaBundle &bundle, bool trackChanged )
{
    if ( m_currentTrack && !trackChanged ) {
         //if the track hasn't changed then this is a meta-data update

        //this is a hack, I repeat a hack! FIXME FIXME
        //we do it because often the stream title is from the pls file and is informative
        //we don't want to lose it when we get the meta data
        if ( m_currentTrack->exactText( PlaylistItem::Artist ).isEmpty() ) {
            QString comment = m_currentTrack->exactText( PlaylistItem::Title );
            m_currentTrack->setText( bundle );
            m_currentTrack->setText( PlaylistItem::Comment, comment );
        }
        else
            m_currentTrack->setText( bundle );
    }
    else
        //ensure the currentTrack is set correctly and highlight it
        restoreCurrentTrack();

    setFilterForItem( m_filter, m_currentTrack );
    emit itemCountChanged( childCount(), m_totalLength, m_visCount, m_visLength, m_selCount, m_selLength );
}

void
Playlist::engineStateChanged( Engine::State state, Engine::State /*oldState*/ )
{
    switch( state )
    {
    case Engine::Playing:
        amaroK::actionCollection()->action( "pause" )->setEnabled( true );
        amaroK::actionCollection()->action( "stop" )->setEnabled( true );
        amaroK::actionCollection()->action( "playlist_show" )->setEnabled( true );

        Glow::startTimer();

        break;

    case Engine::Paused:
        amaroK::actionCollection()->action( "pause" )->setEnabled( true );
        amaroK::actionCollection()->action( "stop" )->setEnabled( true );
        amaroK::actionCollection()->action( "playlist_show" )->setEnabled( true );

        Glow::reset();

        if( m_currentTrack )
            slotGlowTimer(); //update glow state

        break;

    case Engine::Empty:
        amaroK::actionCollection()->action( "pause" )->setEnabled( false );
        amaroK::actionCollection()->action( "stop" )->setEnabled( false );
        amaroK::actionCollection()->action( "playlist_show" )->setEnabled( false );

        //leave the glow state at full colour
        Glow::reset();

        if ( m_currentTrack )
        {
            //remove pixmap in all columns
            QPixmap null;
            for( int i = 0; i < header()->count(); i++ )
                m_currentTrack->setPixmap( i, null );

            PlaylistItem::setPixmapChanged();

            if( m_stopAfterTrack == m_currentTrack )
                m_stopAfterTrack = 0; //we just stopped

            //reset glow state
            slotGlowTimer();
        }

    case Engine::Idle:
        ;
    }

    //POSSIBLYAHACK
    //apparently you can't rely on EngineController::engine()->state() == state here, so pass it explicitly
    setCurrentTrackPixmap( state );
}


////////////////////////////////////////////////////////////////////////////////
/// KListView Reimplementation
////////////////////////////////////////////////////////////////////////////////

void
Playlist::appendMedia( const QString &path )
{
    appendMedia( KURL::fromPathOrURL( path ) );
}

void
Playlist::appendMedia( const KURL &url )
{
    insertMedia( KURL::List( url ) );
}

void
Playlist::clear() //SLOT
{
    if( isLocked() || renameLineEdit()->isVisible() ) return;

    emit aboutToClear(); //will saveUndoState()

    setCurrentTrack( 0 );
    m_prevTracks.clear();
    m_nextTracks.clear();
    m_totalLength = m_visLength = m_visCount = 0;

    // Update player button states
    amaroK::actionCollection()->action( "play" )->setEnabled( false );
    amaroK::actionCollection()->action( "prev" )->setEnabled( false );
    amaroK::actionCollection()->action( "next" )->setEnabled( false );
    amaroK::actionCollection()->action( "playlist_clear" )->setEnabled( false );

    ThreadWeaver::instance()->abortAllJobsNamed( "TagWriter" );

    // something to bear in mind, if there is any event in the loop
    // that depends on a PlaylistItem, we are about to crash amaroK
    // never unlock() the Playlist until it is safe!
    KListView::clear();

    emit itemCountChanged( childCount(), m_totalLength, 0, 0, 0, 0 );

    if( isDynamic() && !m_undoDirt )
        repopulate();
}

void
Playlist::clearAndSave() //SLOT
{
    clear();
    saveXML( defaultPlaylistPath() );
}

void
Playlist::setSorting( int col, bool b )
{
    saveUndoState();

    KListView::setSorting( col, b );
}

void
Playlist::setColumnWidth( int col, int width )
{
    KListView::setColumnWidth( col, width );

    //FIXME this is because Qt doesn't by default disable resizing width 0 columns. GRRR!
    //NOTE  default column sizes are stored in default amarokrc so that restoreLayout() in ctor will
    //      call this function. This is necessary because addColumn() doesn't call setColumnWidth() GRRR!
    header()->setResizeEnabled( width != 0, col );
}

void
Playlist::rename( QListViewItem *item, int column ) //SLOT
{
    switch( column )
    {
        case PlaylistItem::Artist:
            renameLineEdit()->completionObject()->setItems( CollectionDB::instance()->artistList() );
            break;

        case PlaylistItem::Album:
            renameLineEdit()->completionObject()->setItems( CollectionDB::instance()->albumList() );
            break;

        case PlaylistItem::Genre:
            renameLineEdit()->completionObject()->setItems( CollectionDB::instance()->genreList() );
            break;

        default:
            renameLineEdit()->completionObject()->clear();
            break;
    }

    renameLineEdit()->completionObject()->setCompletionMode( KGlobalSettings::CompletionPopupAuto );

    m_editOldTag = static_cast<PlaylistItem *>(item)->exactText( column );

    KListView::rename( item, column );
}

void
Playlist::writeTag( QListViewItem *qitem, const QString &newTag, int column ) //SLOT
{
    if( m_itemsToChangeTagsFor.isEmpty() )
        m_itemsToChangeTagsFor.append( (PlaylistItem*)qitem );

    for( PlaylistItem *item = m_itemsToChangeTagsFor.first(); item; item = m_itemsToChangeTagsFor.next() )
    {
        const QString &oldTag = item == qitem ? m_editOldTag : item->exactText(column);

        if( column == PlaylistItem::Score )
            CollectionDB::instance()->setSongPercentage( item->url().path(), newTag.toInt() );
        else
            if (oldTag != newTag)
                ThreadWeaver::instance()->queueJob( new TagWriter( item, oldTag, newTag, column ) );
    }

    m_itemsToChangeTagsFor.clear();
    m_editOldTag = QString::null;
}

void
Playlist::columnOrderChanged() //SLOT
{
    const uint prevColumn = m_firstColumn;

    //determine first visible column
    for ( m_firstColumn = 0; m_firstColumn < header()->count(); m_firstColumn++ )
        if ( header()->sectionSize( header()->mapToSection( m_firstColumn ) ) )
            break;

    //convert to logical column
    m_firstColumn = header()->mapToSection( m_firstColumn );

    //force redraw of currentTrack
    if( m_currentTrack )
    {
        m_currentTrack->setPixmap( prevColumn, QPixmap() );
        setCurrentTrackPixmap();
    }
}

void
Playlist::paletteChange( const QPalette &p )
{
    using namespace Glow;

    QColor fg;
    QColor bg;

    {
        using namespace Base;

        const uint steps = STEPS+5+5; //so we don't fade all the way to base, and all the way up to highlight either

        fg = colorGroup().highlight();
        bg = colorGroup().base();

        dr = double(bg.red() - fg.red()) / steps;
        dg = double(bg.green() - fg.green()) / steps;
        db = double(bg.blue() - fg.blue()) / steps;

        r = fg.red() + int(dr*5.0); //we add 5 steps so the default colour is slightly different to highlight
        g = fg.green() + int(dg*5.0);
        b = fg.blue() + int(db*5.0);
    }

    {
        using namespace Text;

        const uint steps = STEPS + 5; //so we don't fade all the way to base

        fg = colorGroup().highlightedText();
        bg = colorGroup().text();

        dr = double(bg.red() - fg.red()) / steps;
        dg = double(bg.green() - fg.green()) / steps;
        db = double(bg.blue() - fg.blue()) / steps;

        r = fg.red();
        g = fg.green();
        b = fg.blue();
    }

    KListView::paletteChange( p );

    counter = 0; // reset the counter or apparently the text lacks contrast
    slotGlowTimer(); // repaint currentTrack marker
}

void
Playlist::contentsDragEnterEvent( QDragEnterEvent *e )
{
    QString data;
    QCString subtype;
    QTextDrag::decode( e, data, subtype );

    e->accept(
            e->source() == viewport() ||
            subtype == "amarok-sql" ||
            subtype == "uri-list" || //this is to prevent DelayedUrlLists from performing their queries
            KURLDrag::canDecode( e ) );
}

void
Playlist::contentsDragMoveEvent( QDragMoveEvent* e )
{
    if( !e->isAccepted() ) return;

    #if KDE_IS_VERSION( 3, 3, 91 )
    const bool ctrlPressed = KApplication::keyboardMouseState() & Qt::ControlButton;
    #else
    const bool ctrlPressed= KApplication::keyboardModifiers() & ControlMask;
    #endif

    //Get the closest item _before_ the cursor
    const QPoint p = contentsToViewport( e->pos() );
    QListViewItem *item = itemAt( p );
    if( !item || ctrlPressed ) item = lastItem();
    else if( p.y() - itemRect( item ).top() < (item->height()/2) ) item = item->itemAbove();

    if( item != m_marker ) {
        //NOTE this if block prevents flicker
        slotEraseMarker();
        m_marker = item; //NOTE this is the correct place to set m_marker
        viewportPaintEvent( 0 );
    }
}

void
Playlist::contentsDragLeaveEvent( QDragLeaveEvent* )
{
    slotEraseMarker();
}

void
Playlist::contentsDropEvent( QDropEvent *e )
{
    DEBUG_BLOCK

    //NOTE parent is always 0 currently, but we support it in case we start using trees
    QListViewItem *parent = 0;
    QListViewItem *after  = m_marker;

    if( m_marker && !( static_cast<PlaylistItem *>(m_marker)->isEnabled() ) )
    {
        slotEraseMarker();
        return;
    }

    if( !after ) findDrop( e->pos(), parent, after ); //shouldn't happen, but you never know!

    slotEraseMarker();

    if ( e->source() == viewport() ) {
        setSorting( NO_SORT ); //disableSorting and saveState()
        movableDropEvent( parent, after );
    }

    else {
        QString data;
        QCString subtype;
        QTextDrag::decode( e, data, subtype );

        debug() << "QTextDrag::subtype(): " << subtype << endl;

        if( subtype == "amarok-sql" ) {
            setSorting( NO_SORT );
            ThreadWeaver::instance()->queueJob( new SqlLoader( data, after ) );
        }
        else if( KURLDrag::canDecode( e ) )
        {
            debug() << "KURLDrag::canDecode" << endl;

            KURL::List list;
            KURLDrag::decode( e, list );
            insertMediaInternal( list, (PlaylistItem*)after );
        }
        else
            e->ignore();
    }

    updateNextPrev();
}

QDragObject*
Playlist::dragObject()
{
    DEBUG_FUNC_INFO

    //TODO use of the map is pointless
    //TODO just get the tags with metabundle like every other part of amaroK does, the performance issues are negligible
    //     and this is just code-bloat

    KURL::List list;
    QMap<QString,QString> map;

    for( MyIt it( this, MyIt::Selected ); *it; ++it )
    {
        const PlaylistItem *item = (PlaylistItem*)*it;
        const KURL &url = item->url();
        list += url;
        const QString key = url.isLocalFile() ? url.path() : url.url();
        map[ key ] = QString("%1;%2").arg( item->title() ).arg( item->seconds() );
    }

    //it returns a KURLDrag with a QMap containing the title and the length of the track
    //this is used by the playlistbrowser to insert tracks in playlists without re-reading tags
    return new KURLDrag( list, map, viewport() );
}

#include <qsimplerichtext.h>
void
Playlist::viewportPaintEvent( QPaintEvent *e )
{
    if( e ) KListView::viewportPaintEvent( e ); //we call with 0 in contentsDropEvent()

    if ( m_marker ) {
        QPainter p( viewport() );
        p.fillRect(
                drawDropVisualizer( 0, 0, m_marker ),
                QBrush( colorGroup().highlight().dark(), QBrush::Dense4Pattern ) );
    }
    else if( m_showHelp && isEmpty() ) {
        QPainter p( viewport() );
        QString minimumText(i18n(
                "<div align=center>"
                  "<h3>The Playlist</h3>"
                    "This is the playlist. "
                    "To create a listing, "
                      "<b>drag</b> tracks from the browser-panels on the left, "
                      "<b>drop</b> them here and then <b>double-click</b> them to start playback."
                "</div>" ) );
        QSimpleRichText *t = new QSimpleRichText( minimumText +
                i18n( "<div align=center>"
                  "<h3>The Browsers</h3>"
                    "The browsers are the source of all your music. "
                    "The collection-browser holds your collection. "
                    "The playlist-browser holds your pre-set playlistings. "
                    "The file-browser shows a file-selector which you can use to access any music on your computer. "
                "</div>" ), QApplication::font() );

        if ( t->width()+30 >= viewport()->width() || t->height()+30 >= viewport()->height() ) {
            // too big for the window, so let's cut part of the text
            delete t;
            t = new QSimpleRichText( minimumText, QApplication::font());
            if ( t->width()+30 >= viewport()->width() || t->height()+30 >= viewport()->height() ) {
                //still too big, giving up
                return;
            }
        }

        const uint w = t->width();
        const uint h = t->height();
        const uint x = (viewport()->width() - w - 30) / 2 ;
        const uint y = (viewport()->height() - h - 30) / 2 ;

        p.setBrush( colorGroup().background() );
        p.drawRoundRect( x, y, w+30, h+30, (8*200)/w, (8*200)/h );
        t->draw( &p, x+15, y+15, QRect(), colorGroup() );
        delete t;
    }
}

static uint negativeWidth = 0;

void
Playlist::viewportResizeEvent( QResizeEvent *e )
{
    //only be clever with the sizing if there is not many items
    //TODO don't allow an item to be made too small (ie less than 50% of ideal width)

    //makes this much quicker
    header()->blockSignals( true );

    if( e->size().width() != e->oldSize().width() )
    {
        const double W = (double)e->size().width() - negativeWidth;

        for( uint c = 0; c < m_columnFraction.size(); ++c ) {
            switch( c ) {
            case PlaylistItem::Track:
            case PlaylistItem::Bitrate:
            case PlaylistItem::Score:
            case PlaylistItem::Length:
            case PlaylistItem::Year:
                break; //these columns retain their width - their items tend to have uniform size
            default:
                if( m_columnFraction[c] > 0 )
                   setColumnWidth( c, int(W * m_columnFraction[c]) );
            }
        }
    }

    header()->blockSignals( false );

    //ensure that the listview scrollbars are updated etc.
    triggerUpdate();
}

void
Playlist::columnResizeEvent( int col, int oldw, int neww )
{
    //prevent recursion
    header()->blockSignals( true );

    //qlistview is stupid sometimes
    if ( neww < 0 )
        setColumnWidth( col, 0 );

    if ( neww == 0 ) {
        //the column in question has been hidden
        //we need to adjust the other columns to fit

        const double W = (double)width() - negativeWidth;

        for( uint c = 0; c < m_columnFraction.size(); ++c ) {
            if( c == (uint)col )
               continue;
            switch( c ) {
            case PlaylistItem::Track:
            case PlaylistItem::Bitrate:
            case PlaylistItem::Score:
            case PlaylistItem::Length:
            case PlaylistItem::Year:
                break;
            default:
                if( m_columnFraction[c] > 0 )
                   setColumnWidth( c, int(W * m_columnFraction[c]) );
            }
        }
    }

    else if( oldw != 0 ) {
        //adjust the size of the column on the right side of this one

        for( int section = col, index = header()->mapToIndex( section ); index < header()->count(); ) {
            section = header()->mapToSection( ++index );

            if ( header()->sectionSize( section ) ) {
                int newSize = header()->sectionSize( section ) + oldw - neww;
                if ( newSize > 5 ) {
                    setColumnWidth( section, newSize );
                    //we only want to adjust one column!
                    break;
                }
            }
        }
    }

    header()->blockSignals( false );

    negativeWidth = 0;
    uint w = 0;

    //determine width excluding the columns that have static size
    for( uint x = 0; x < m_columnFraction.size(); ++x ) {
        switch( x ) {
        case PlaylistItem::Track:
        case PlaylistItem::Bitrate:
        case PlaylistItem::Score:
        case PlaylistItem::Length:
        case PlaylistItem::Year:
            break;
        default:
            w += columnWidth( x );
        }

        negativeWidth += columnWidth( x );
    }

    //determine the revised column fractions
    for( uint x = 0; x < m_columnFraction.size(); ++x )
        m_columnFraction[x] = (double)columnWidth( x ) / double(w);

    //negative width is an important property, honest!
    negativeWidth -= w;

    //we have to do this after we have established negativeWidth and set the columnFractions
    if( neww == 0 || oldw == 0 ) {
        //then this column has been inserted or removed, we need to update all the column widths
        QResizeEvent e( size(), QSize() );
        viewportResizeEvent( &e );
    }
}

bool
Playlist::eventFilter( QObject *o, QEvent *e )
{
    #define me static_cast<QMouseEvent*>(e)

    if( o == header() && e->type() == QEvent::MouseButtonPress && me->button() == Qt::RightButton )
    {
        enum { HIDE = 1000, CUSTOM };

        const int mouseOverColumn = header()->sectionAt( me->pos().x() );

        KPopupMenu popup;
        popup.setCheckable( true );
        popup.insertItem( i18n("&Hide This Column"), HIDE ); //TODO
        popup.setItemEnabled( HIDE, mouseOverColumn != -1 );

        for( int i = 0; i < columns(); ++i ) //columns() references a property
        {
            popup.insertItem( columnText( i ), i, i + 1 );
            popup.setItemChecked( i, columnWidth( i ) != 0 );
        }

        //TODO for 1.2.1
        //popup.insertSeparator();
        //popup.insertItem( i18n("&Add Custom Column..."), CUSTOM ); //TODO

        //do last so it doesn't get the first id
        popup.insertTitle( i18n( "Playlist Columns" ), /*id*/ -1, /*index*/ 1 );

        int col = popup.exec( static_cast<QMouseEvent *>(e)->globalPos() );

        switch( col ) {
        case HIDE:
            hideColumn( mouseOverColumn );
            break;

        case CUSTOM:
            addCustomColumn();
            break;

        default:
            if( col != -1 )
            {
                //TODO can result in massively wide column appearing!
                if( columnWidth( col ) == 0 )
                {
                    adjustColumn( col );
                    header()->setResizeEnabled( true, col );
                }
                else hideColumn( col );
            }
        }

        //determine first visible column again, since it has changed
        columnOrderChanged();
        //eat event
        return TRUE;
    }

    // not in slotMouseButtonPressed because we need to disable normal usage.
    if( o == viewport() && e->type() == QEvent::MouseButtonPress && me->state() == Qt::ControlButton && me->button() == RightButton )
    {
        PlaylistItem *item = (PlaylistItem*)itemAt( me->pos() );

        if( item )
            queue( item );

        return TRUE; //yum!
    }

    //allow the header to process this
    return KListView::eventFilter( o, e );

    #undef me
}

void
Playlist::customEvent( QCustomEvent *e )
{
    if( e->type() == (int)UrlLoader::JobFinishedEvent ) {
        refreshNextTracks( 0 );

        // Disable help if playlist is populated
        if ( !isEmpty() )
            m_showHelp = false;

        setFilter( m_filter );

        //necessary usually
        m_totalLength = m_visLength = m_visCount = 0;
        int itemCount = 0;
        QListViewItemIterator it( this );
        for( ; it.current(); ++it, itemCount++ ) {
            int length = static_cast<PlaylistItem *>(*it)->seconds().toInt();
            if( length > 0 )
                m_totalLength += length;
            if( (*it)->isVisible() )
            {
                if( length > 0 )
                    m_visLength += length;
                ++m_visCount;
            }
        }
        emit itemCountChanged( itemCount, m_totalLength, m_visCount, m_visLength, m_selCount, m_selLength );

        if ( !m_queueList.isEmpty() ) {
            KURL::List::Iterator jt;
            for( MyIt it( this, MyIt::All ); *it; ++it ) {
                jt = m_queueList.find( (*it)->url() );

                if ( jt != m_queueList.end() ) {
                    queue( *it );
                    m_queueList.remove( jt );
                }
            }
            m_queueList.clear();
        }
        //re-disable history items
        if( isDynamic() && m_stateSwitched )
        {
            alterHistoryItems( !AmarokConfig::dynamicMarkHistory() );
            m_stateSwitched = false;
        }

        if( m_partyDirt )
        {
            PlaylistItem *after = m_currentTrack;
            if( !after )
            {
                after = firstChild();
                while( after && !after->isEnabled() )
                    after = after->nextSibling();
            }
            else
                after = (PlaylistItem *)after->itemBelow();

            PlaylistItem *prev = (PlaylistItem *)after->itemAbove();
            if( prev )
                prev->setEnabled( false );

            activate( after );
        }

        if( m_queueDirt )
        {
            PlaylistItem *after = 0;

            m_nextTracks.isEmpty() ?
                after = m_currentTrack :
                after = m_nextTracks.last();

            if( !after )
            {
                after = firstChild();
                while( after && !after->isEnabled() )
                    after = after->nextSibling();
            }
            else
                after = (PlaylistItem *)after->itemBelow();

            if( after )
            {
                m_nextTracks.append( after );

                if( QueueManager::instance() )
                    emit queued( after );

                refreshNextTracks();
            }

            m_queueDirt = false;
        }

        //force redraw of currentTrack marker, play icon, etc.
        restoreCurrentTrack();

        saveXML( defaultPlaylistPath() );
    }

    updateNextPrev();
}



////////////////////////////////////////////////////////////////////////////////
/// Misc Public Methods
////////////////////////////////////////////////////////////////////////////////

bool
Playlist::saveM3U( const QString &path, bool relativePath ) const
{
    QFile file( path );

    if( !file.open( IO_WriteOnly ) ){
      return false;
    }

    QTextStream stream( &file );
    stream << "#EXTM3U\n";

    for( const PlaylistItem *item = firstChild(); item; item = item->nextSibling() )
    {
        const KURL url = item->url();

        stream << "#EXTINF:";
        stream << item->seconds();
        stream << ',';
        stream << item->title();
        stream << '\n';
        if (url.protocol() == "file" ) {
            if ( relativePath ) {
                const QFileInfo fi(file);
                stream << KURL::relativePath(fi.dirPath(), url.path());
            } else
                stream << url.path();
        } else {
            stream << url.url();
        }
        stream << "\n";
    }
    return true;
}

void
Playlist::saveXML( const QString &path )
{
    QFile file( path );

    if( !file.open( IO_WriteOnly ) ) return;

    QDomDocument newdoc;
    QDomElement playlist = newdoc.createElement( "playlist" );
    playlist.setAttribute( "product", "amaroK" );
    playlist.setAttribute( "version", APP_VERSION );
    newdoc.appendChild( playlist );

    for( const PlaylistItem *item = firstChild(); item; item = item->nextSibling() )
    {
        int queueIndex = m_nextTracks.findRef( item );
        bool isQueued = queueIndex != -1;
        QDomElement i = newdoc.createElement("item");
        i.setAttribute("url", item->url().url());
        if ( isQueued )
        {
            i.setAttribute( "queue_index", queueIndex + 1 );
        }
        else if ( item == currentTrack() )
        {
            i.setAttribute( "queue_index", 0 );
        }

        for( int x = 1; x < columns(); ++x )
        {
            if( !item->exactText(x).isEmpty() )
            {
                QDomElement attr = newdoc.createElement( item->columnName(x) );
                QDomText t = newdoc.createTextNode( item->exactText(x) );
                attr.appendChild( t );
                i.appendChild( attr );
            }
        }

        playlist.appendChild( i );
    }

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << newdoc.toString();
}

void
Playlist::burnPlaylist( int projectType )
{
    KURL::List list;

    QListViewItemIterator it( this );
    for( ; it.current(); ++it ) {
        PlaylistItem *item = static_cast<PlaylistItem*>(*it);
        KURL url = item->url();
        if( url.isLocalFile() )
            list << url;
    }

    K3bExporter::instance()->exportTracks( list, projectType );
}

void
Playlist::burnSelectedTracks( int projectType )
{
    KURL::List list;

    QListViewItemIterator it( this, QListViewItemIterator::Selected );
    for( ; it.current(); ++it ) {
        PlaylistItem *item = static_cast<PlaylistItem*>(*it);
        KURL url = item->url();
        if( url.isLocalFile() )
            list << url;
    }

    K3bExporter::instance()->exportTracks( list, projectType );
}

void
Playlist::addCustomMenuItem(QString submenu, QString itemTitle )  //for dcop
{
        m_customSubmenuItem[submenu] << itemTitle;
}

bool
Playlist::removeCustomMenuItem( QString submenu, QString itemTitle )  //for dcop
{
    if( !m_customSubmenuItem.contains(submenu) )
        return false;
    if( m_customSubmenuItem[submenu].remove( itemTitle ) != 0 )
    {
       if( m_customSubmenuItem[submenu].count() == 0 )
            m_customSubmenuItem.remove( submenu );
            return true;
        return true;
    }
    else
        return false;
}

void
Playlist::customMenuClicked(int id)  //adapted from burnSelectedTracks
{
    QString message = m_customIdItem[id];
    QListViewItemIterator it( this, QListViewItemIterator::Selected );
    for( ; it.current(); ++it ) {
        PlaylistItem *item = static_cast<PlaylistItem*>(*it);
        KURL url = item->url().url();
        if( url.isLocalFile() )
            message += " " + url.url();
    }
    ScriptManager::instance()->customMenuClicked( message );
}

void
Playlist::repopulate() //SLOT
{
    // Repopulate the upcoming tracks
    MyIt it( this, MyIt::All );
    QPtrList<QListViewItem> list;

    for( ; *it; ++it )
    {
        PlaylistItem *item = (PlaylistItem *)(*it);
        int     queueIndex = m_nextTracks.findRef( item );
        bool    isQueued   = queueIndex != -1;

        if( !item->isEnabled() || item == m_currentTrack || isQueued )
            continue;

        list.prepend( *it );
    }

    saveUndoState();

    //remove the items
    for( QListViewItem *item = list.first(); item; item = list.next() )
    {
        removeItem( (PlaylistItem*)item );
        delete item;
    }

    //calling advancePartyTrack will remove an item too, which is undesirable
    //block signals to avoid saveUndoState being called
    blockSignals( true );
    addSpecialTracks( AmarokConfig::dynamicUpcomingCount(), AmarokConfig::dynamicType() );
    blockSignals( false );
}

void
Playlist::shuffle() //SLOT
{
    if( isDynamic() )
        return;

    QPtrList<QListViewItem> list;

    setSorting( NO_SORT );

    //if there are nexttracks re-order them
    if( !m_nextTracks.isEmpty() ) {
        for( PlaylistItem *item = m_nextTracks.first(); item; item = m_nextTracks.next() ) {
            takeItem( item );
            insertItem( item );
        }

        updateNextPrev();

        return;
    }

    // shuffle only VISIBLE entries
    for( MyIt it( this ); *it; ++it )
        list.append( *it );

    // we do it in two steps because the iterator doesn't seem
    // to like it when we do takeItem and ++it in the same loop
    for( QListViewItem *item = list.first(); item; item = list.next() )
        takeItem( item );

    //shuffle
    KRandomSequence( (long)KApplication::random() ).randomize( &list );

    //reinsert in new order
    for( QListViewItem *item = list.first(); item; item = list.next() )
        insertItem( item );

    updateNextPrev();
}

void
Playlist::removeSelectedItems() //SLOT
{
    if( isLocked() ) return;

    setSelected( currentItem(), true );     //remove currentItem, no matter if selected or not

    //assemble a list of what needs removing
    //calling removeItem() iteratively is more efficient if they are in _reverse_ order, hence the prepend()
    QPtrList<QListViewItem> list;
    int dontReplaceDynamic = 0;

    for( MyIterator it( this, MyIt::Selected ); *it; ++it )
    {
        if( !(*it)->isEnabled() )
            dontReplaceDynamic++;
        list.prepend( *it );
    }

    if( list.isEmpty() ) return;
    saveUndoState();

    if( isDynamic() )
    {
        addSpecialTracks( list.count() - dontReplaceDynamic, AmarokConfig::dynamicType() );
    }

    //remove the items
    for( QListViewItem *item = list.first(); item; item = list.next() )
    {
        removeItem( (PlaylistItem*)item );
        delete item;
    }

    updateNextPrev();
    //NOTE no need to emit childCountChanged(), removeItem() does that for us
}

void
Playlist::deleteSelectedFiles() //SLOT
{
    if( isLocked() ) return;

    KURL::List urls;

    //assemble a list of what needs removing
    for( MyIt it( this, MyIt::Selected );
         it.current();
         urls << static_cast<PlaylistItem*>( *it )->url(), ++it );

    //NOTE we assume that currentItem is the main target
    const int count  = urls.count();
    QString text;
    if (count == 1)  // remember: there are languages that use singular also for 0 or 2
        text = i18n("<p>You have selected the file <i>'%1'</i> to be <b>irreversibly</b> deleted.")
                    .arg(static_cast<PlaylistItem*>( currentItem() )->url().fileName() );
    else
        text = i18n( "<p>You have selected one file to be <b>irreversibly</b> deleted.",
                     "<p>You have selected %n files to be <b>irreversibly</b> deleted.", count );

    int button = KMessageBox::warningContinueCancel( this,
                                                     text,
                                                     QString::null,
                                                     KGuiItem( i18n("&Delete"), "editdelete") );

    if ( button == KMessageBox::Continue )
    {
        // TODO We need to check which files have been deleted successfully
        KIO::DeleteJob* job = KIO::del( urls );
        connect( job, SIGNAL(result( KIO::Job* )), SLOT(removeSelectedItems()) );

        job->setAutoErrorHandlingEnabled( false );

        amaroK::StatusBar::instance()->newProgressOperation( job )
                .setDescription( i18n("Deleting files") );

        // we must handle delete errors somehow
        CollectionDB::instance()->removeSongs( urls );
        QTimer::singleShot( 0, CollectionView::instance(), SLOT( renderView() ) );
    }
}

void
Playlist::removeDuplicates() //SLOT
{
    // Remove dead entries:

    for( QListViewItemIterator it( this ); it.current(); ) {
        PlaylistItem* item = static_cast<PlaylistItem*>( *it );
        const KURL url = item->url();
        if ( url.isLocalFile() && !QFile::exists( url.path() ) ) {
            removeItem( item );
            ++it;
            delete item;
        }
        else ++it;
    }

    // Remove dupes:

    QSortedList<PlaylistItem> list;
    for( QListViewItemIterator it( this ); it.current(); ++it )
        list.prepend( (PlaylistItem*)it.current() );

    list.sort();

    QPtrListIterator<PlaylistItem> it( list );
    PlaylistItem *item;
    while( (item = it.current()) ) {
        const KURL &compare = item->url();
        ++it;
        if ( *it && compare == it.current()->url() ) {
            removeItem( item );
            delete item;
        }
    }
}

void
Playlist::copyToClipboard( const QListViewItem *item ) const //SLOT
{
    if( !item ) item = currentTrack();

    if( item )
    {
        const PlaylistItem* playlistItem = static_cast<const PlaylistItem*>( item );

        QString text = MetaBundle( (PlaylistItem*)playlistItem ).prettyTitle();
        // For streams add the streamtitle too
        //TODO make prettyTitle do this
        if ( playlistItem->url().protocol() == "http" )
            text.append( " :: " + playlistItem->url().url() );

        // Copy both to clipboard and X11-selection
        QApplication::clipboard()->setText( text, QClipboard::Clipboard );
        QApplication::clipboard()->setText( text, QClipboard::Selection );

        amaroK::OSD::instance()->OSDWidget::show( i18n( "Copied: %1" ).arg( text ),
                                 QImage(CollectionDB::instance()->albumImage(MetaBundle( (PlaylistItem*)playlistItem )) ) );
    }
}

void Playlist::undo()
{
    if( !isLocked() )
    {
        switchState( m_undoList, m_redoList );
        m_stateSwitched = true;
    }
} //SLOT

void Playlist::redo()
{
    if( !isLocked() )
    {
        switchState( m_redoList, m_undoList );
        m_stateSwitched = true;
    }
} //SLOT

void
Playlist::updateMetaData( const MetaBundle &mb ) //SLOT
{
    for( MyIt it( this, MyIt::All ); *it; ++it )
        if( mb.url() == (*it)->url() )
        {
            (*it)->setText( mb );
            setFilterForItem( m_filter, *it );
            emit itemCountChanged( childCount(), m_totalLength, m_visCount, m_visLength, m_selCount, m_selLength );
        }
}

void
Playlist::showQueueManager()
{
    DEBUG_BLOCK

    // Only show the dialog once
    if( QueueManager::instance() ) {
        QueueManager::instance()->raise();
        return;
    }

    QueueManager dialog;
    if( dialog.exec() == QDialog::Accepted )
    {
        QPtrList<PlaylistItem> oldQueue = m_nextTracks;
        m_nextTracks = dialog.newQueue();

        // make sure we repaint items no longer queued
        for( PlaylistItem* item = oldQueue.getFirst(); item; item = oldQueue.next() )
            repaintItem( item );

        // repaint newly queued or altered queue items
        if( isDynamic() )
            sortQueuedItems();
        else
            refreshNextTracks();
    }
}

bool
Playlist::googleMatch( QString query, const QStringMap &defaults, const QStringMap &all )
{
    if( query.contains( "\"" ) % 2 == 1 ) query += "\""; //make an even number of "s

    //something like thingy"bla"stuff -> thingy "bla" stuff
    bool odd = false;
    for( int pos = query.find( "\"" );
         pos >= 0 && pos <= (int)query.length();
         pos = query.find( "\"", pos + 1 ) )
    {
        query = query.insert( odd ? ++pos : pos++, " " );
        odd = !odd;
    }
    query = query.simplifyWhiteSpace();

    int x; //position in string of the end of the next element
    bool OR = false, minus = false; //whether the next element is to be OR, and/or negated
    QString tmp, s = "", field = ""; //the current element, a tempstring, and the field: of the next element
    QStringList tmpl; //list of elements of which at least one has to match (OR)
    QValueList<QStringList> allof; //list of all the tmpls, of which all have to match
    while( !query.isEmpty() )  //seperate query into parts which all have to match
    {
        if( query.startsWith( " " ) )
            query = query.mid( 1 ); //cuts off the first character
        if( query.startsWith( "\"" ) ) //take stuff in "s literally (basically just ends up ignoring spaces)
        {
            query = query.mid( 1 );
            x = query.find( "\"" );
        }
        else
            x = query.find( " " );
        if( x < 0 )
            x = query.length();
        s = query.left( x ); //get the element
        query = query.mid( x + 1 ); //move on

        if( !field.isEmpty() || ( s != "-" && s != "AND" && s != "OR" &&
                                  !s.endsWith( ":" ) && !s.endsWith( ":>" ) && !s.endsWith( ":<" ) ) )
        {
            if( !OR && !tmpl.isEmpty() ) //add the OR list to the AND list
            {
                allof += tmpl;
                tmpl.clear();
            }
            else
                OR = false;
            tmp = field + s;
            if( minus )
            {
                tmp = "-" + tmp;
                minus = false;
            }
            tmpl += tmp;
            tmp = field = "";
        }
        else if( s.endsWith( ":" ) || s.endsWith( ":>" ) || s.endsWith( ":<" ) )
            field = s;
        else if( s == "OR" )
            OR = true;
        else if( s == "-" )
            minus = true;
        else
            OR = false;
    }
    if( !tmpl.isEmpty() )
        allof += tmpl;

    const uint allofcount = allof.count();
    for( uint i = 0; i < allofcount; ++i ) //check each part for matchiness
    {
        uint count = allof[i].count();
        bool b = false; //whether at least one matches
        for( uint ii = 0; ii < count; ++ii )
        {
            s = allof[i][ii];
            bool neg = s.startsWith( "-" );
            if ( neg )
                s = s.mid( 1 ); //cut off the -
            x = s.find( ":" ); //where the field ends and the thing-to-match begins
            if( x > 0 && all.contains( s.left( x ).lower() ) ) //a field was specified and it exists
            {
                QString f = s.left(x).lower(), q = s.mid(x + 1), v = all[f].lower(), w = q.lower();
                //f = field, q = query, v = contents of the field, w = match against it
                bool condition; //whether it matches, not taking negation into account

                static const QString
                    Score     = PlaylistItem::columnName( PlaylistItem::Score     ).lower(),
                    Year      = PlaylistItem::columnName( PlaylistItem::Year      ).lower(),
                    Track     = PlaylistItem::columnName( PlaylistItem::Track     ).lower(),
                    Playcount = PlaylistItem::columnName( PlaylistItem::Playcount ).lower(),
                    Length    = PlaylistItem::columnName( PlaylistItem::Length    ).lower(),
                    Bitrate   = PlaylistItem::columnName( PlaylistItem::Bitrate   ).lower();

                if (q.startsWith(">"))
                {
                    w = w.mid( 1 );
                    if( f == Score || f == Year || f == Track || f == Playcount )
                        condition = v.toInt() > w.toInt();
                    else if( f == Length )
                    {
                        int g = v.find( ":" ), h = w.find( ":" );
                        condition = v.left( g ).toInt() > w.left( h ).toInt() ||
                                    ( v.left( g ).toInt() == w.left( h ).toInt() &&
                                      v.mid( g + 1 ).toInt() > w.mid( h + 1 ).toInt() );
                    }
                    else if( f == Bitrate )
                    {
                        if( v.contains( "?" ) )
                            condition = false;
                        else                      //cut off " kbps"
                            condition = v.left( v.length() - 5 ).toInt() > w.left( w.length() - 5 ).toInt();
                    }
                    else
                        condition = v > w; //compare the strings
                }
                else if( q.startsWith( "<" ) )
                {
                    w = w.mid(1);
                    if( f == Score || f == Year || f == Track || f == Playcount )
                        condition = v.toInt() < w.toInt();
                    else if( f == Length )
                    {
                        int g = v.find( ":" ), h = w.find( ":" );
                        condition = v.left( g ).toInt() < w.left( h ).toInt() ||
                                    ( v.left( g ).toInt() == w.left( h ).toInt() &&
                                      v.mid( g + 1 ).toInt() < w.mid( h + 1 ).toInt() );
                    }
                    else if( f == Bitrate )
                    {
                        if( v.contains( "?" ) )
                            condition = true;
                        else
                            condition = v.left( v.length() - 5 ).toInt() < w.left( w.length() - 5 ).toInt();
                    }
                    else
                        condition = v < w;
                }
                else
                    condition = v.contains( q, false );
                if( condition == ( neg ? false : true ) )
                {
                    b = true;
                    break;
                }
            }
            else //check just the default fields
            {
                QStringMap::ConstIterator end = defaults.constEnd();
                for( QStringMap::ConstIterator it = defaults.constBegin(); it != end; ++it )
                {
                    b = it.data().contains( s, false ) == ( neg ? false : true );
                    if( ( neg && !b ) || ( !neg && b ) )
                        break;
                }
                if( b )
                    break;
            }
        }
        if( !b )
            return false;
    }
    return true;
}

void
Playlist::setFilterSlot( const QString &query ) //SLOT
{
    m_filtertimer->stop();
    if( isAdvancedQuery( query ) )
    {
        m_filter = query;
        m_filtertimer->start( 50, true );
    }
    else
        setFilter( query );
}

void
Playlist::setDelayedFilter() //SLOT
{
    setFilter( m_filter );
}

void
Playlist::setFilter( const QString &query ) //SLOT
{
    MyIt it( this, ( !isAdvancedQuery( query ) && query.lower().startsWith( m_filter.lower() ) )
                   ? MyIt::Visible
                   : MyIt::All );

    for( ;*it; ++it )
        setFilterForItem( query, *it );

    m_filter = query;

    //to me it seems sensible to do this, BUT if it seems annoying to you, remove it
    showCurrentTrack();
    triggerUpdate();
    emit itemCountChanged( childCount(), m_totalLength, m_visCount, m_visLength, m_selCount, m_selLength );
}

void
Playlist::setFilterForItem( const QString &query, PlaylistItem *item )
{
    if( !item )
        return;

    bool visible = true;
    uint x, n = columns();
    if( isAdvancedQuery( query ) )
    {
        QStringMap defaults, all;
        for( x = 0; x < n; ++x ) {
            if ( columnWidth( x ) ) defaults[PlaylistItem::columnName( x ).lower()] = item->exactText( x );
            all[PlaylistItem::columnName( x ).lower()] = item->exactText( x );
        }

        visible = googleMatch( query, defaults, all );
    }
    else
    {
        const QStringList terms = QStringList::split( ' ', query.lower() );
        uint y;
        for( x = 0; visible && x < terms.count(); ++x )
        {
            for( y = 0; y < n; ++y )
                if ( columnWidth( y ) && item->exactText( y ).lower().contains( terms[x] ) )
                    break;
            visible = ( y < n );
        }
    }

    int length = item->seconds().toInt();
    if( visible && !item->isVisible() )
    {
        ++m_visCount;
        m_visLength += length;
        if( item->isSelected() )
        {
            ++m_selCount;
            m_selLength += length;
        }
    }
    else if( !visible && item->isVisible() )
    {
        --m_visCount;
        m_visLength -= length;
        if( item->isSelected() )
        {
            --m_selCount;
            m_selLength -= length;
        }
    }
    item->setVisible( visible );
}

bool
Playlist::isAdvancedQuery( const QString &query )
{
    if( query.contains( "\""  ) ||
        query.contains( ":"   ) ||
        query.contains( "-"   ) ||
        query.contains( "AND" ) ||
        query.contains( "OR"  ) )

        return true;

    return false;
}

void
Playlist::scoreChanged( const QString &path, int score )
{
    for( MyIt it( this, MyIt::All ); *it; ++it )
    {
        PlaylistItem *item = (PlaylistItem*)*it;
        if ( item->url().path() == path )
        {
            item->setText( PlaylistItem::Score, QString::number( score ) );
            setFilterForItem( m_filter, item );
            emit itemCountChanged( childCount(), m_totalLength, m_visCount, m_visLength, m_selCount, m_selLength );
        }
    }
}

void
Playlist::countChanged( const QString &path )
{
    for( MyIt it( this, MyIt::All ); *it; ++it )
    {
        PlaylistItem *item = (PlaylistItem*)*it;
        if ( item->url().path() == path )
            item->setText( PlaylistItem::Playcount, QString::number( CollectionDB::instance()->getPlayCount( path ) ) );
    }
}

void
Playlist::showContextMenu( QListViewItem *item, const QPoint &p, int col ) //SLOT
{
    #define item static_cast<PlaylistItem*>(item)

    enum {
        PLAY, PLAY_NEXT, STOP_DONE, VIEW, EDIT, FILL_DOWN, COPY, REMOVE, DELETE,
        BURN_MENU, BURN_SELECTION_DATA, BURN_SELECTION_AUDIO, BURN_ALBUM_DATA, BURN_ALBUM_AUDIO,
        BURN_ARTIST_DATA, BURN_ARTIST_AUDIO, LAST }; //keep LAST last

    if( item == 0 ) return; //technically we should show "Remove" but this is far neater

    const bool canRename   = isRenameable( col );
    const bool isCurrent   = (item == m_currentTrack);
    const bool isPlaying   = EngineController::engine()->state() == Engine::Playing;
    const bool trackColumn = col == PlaylistItem::Track;
    const QString tagName  = columnText( col );
    const QString tag      = item->exactText( col );

    uint itemCount = 0;
    for( MyIt it( this, MyIt::Selected ); *it; ++it )
        itemCount++;

    KPopupMenu popup;

    if(itemCount==1)
        popup.insertTitle( KStringHandler::rsqueeze( MetaBundle( item ).prettyTitle(), 50 ));
    else
        popup.insertTitle(i18n("1 Track", "%n Selected Tracks", itemCount));

    if( item->isEnabled() )
        popup.insertItem( SmallIconSet( "player_play" ), isCurrent && isPlaying
                ? i18n( "&Restart" )
                : i18n( "&Play" ), 0, 0, Key_Enter, PLAY );

    // Begin queue entry logic
    popup.insertItem( SmallIconSet( "2rightarrow" ), i18n("&Queue Selected Tracks"), PLAY_NEXT );

    bool queueToggle = false;
    MyIt it( this, MyIt::Selected );
    bool firstQueued = ( m_nextTracks.findRef( *it ) != -1 );

    for( ++it ; *it; ++it ) {
        if ( ( m_nextTracks.findRef( *it ) != -1 ) != firstQueued ) {
            queueToggle = true;
            break;
        }
    }
    if( itemCount == 1 )
    {
        if ( !firstQueued )
            popup.changeItem( PLAY_NEXT, i18n( "&Queue Track" ) );
        else
            popup.changeItem( PLAY_NEXT, SmallIconSet( "2leftarrow" ), i18n("&Dequeue Track") );
    } else {
        if ( queueToggle )
            popup.changeItem( PLAY_NEXT, i18n( "Toggle &Queue Status (1 track)", "Toggle &Queue Status (%n tracks)", itemCount ) );
        else
            // remember, queueToggled only gets set to false if there are items queued and not queued.
            // so, if queueToggled is false, all items have the same queue status as the first item.
            if ( !firstQueued )
                popup.changeItem( PLAY_NEXT, i18n( "&Queue Selected Tracks" ) );
            else
                popup.changeItem( PLAY_NEXT, SmallIconSet( "2leftarrow" ), i18n("&Dequeue Selected Tracks") );
    }
    // End queue entry logic

    if( isCurrent ) {
       amaroK::actionCollection()->action( "pause" )->plug( &popup );
    }
    if(itemCount == 1)
        popup.insertItem( SmallIconSet( "player_stop" ), i18n( "&Stop Playing After Track" ), STOP_DONE );
    popup.setItemChecked( STOP_DONE, m_stopAfterTrack == item );

    if( item->isEnabled() ) popup.insertSeparator();

    popup.insertItem( SmallIconSet( "edit" ), (itemCount == 1
            ? i18n( "&Edit Tag '%1'" )
            : i18n( "&Edit '%1' Tag for Selected Tracks" )).arg( tagName ), 0, 0, Key_F2, EDIT );
    popup.insertItem( trackColumn
            ? i18n("&Iteratively Assign Track Numbers")
  : i18n("Write '%1' for Selected Tracks").arg( KStringHandler::rsqueeze( tag, 30 ).replace( "&", "&&" ) ), FILL_DOWN );
    popup.insertItem( SmallIconSet( "editcopy" ), i18n( "&Copy Tags to Clipboard" ), 0, 0, CTRL+Key_C, COPY );

    popup.insertSeparator();

    KPopupMenu burnMenu;
    burnMenu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("Selected Tracks as Data CD"), BURN_SELECTION_DATA );
    burnMenu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n("Selected Tracks as Audio CD"), BURN_SELECTION_AUDIO );
    burnMenu.insertSeparator();
    burnMenu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("This Album as Data CD"), BURN_ALBUM_DATA );
    burnMenu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n("This Album as Audio CD"), BURN_ALBUM_AUDIO );
    burnMenu.insertSeparator();
    burnMenu.insertItem( SmallIconSet( "cdrom_unmount" ), i18n("All Tracks by This Artist as Data CD"), BURN_ARTIST_DATA );
    burnMenu.insertItem( SmallIconSet( "cdaudio_unmount" ), i18n("All Tracks by This Artist as Audio CD"), BURN_ARTIST_AUDIO );
    popup.insertItem( SmallIconSet( "cdwriter_unmount" ), i18n("Burn"), &burnMenu, BURN_MENU );
    popup.insertSeparator();

    popup.insertItem( SmallIconSet( "edittrash" ), i18n( "&Remove From Playlist" ), this, SLOT( removeSelectedItems() ), Key_Delete, REMOVE );
    popup.insertItem( SmallIconSet( "editdelete" ), itemCount == 1
            ? i18n("&Delete File")
            : i18n("&Delete Selected Files"), this, SLOT( deleteSelectedFiles() ), SHIFT+Key_Delete, DELETE );

    popup.insertSeparator();
    popup.insertItem( SmallIconSet( "info" )
        , i18n( "Edit Track &Information...",  "Edit &Information for %n Tracks...", itemCount)
        , VIEW );

    popup.setItemEnabled( EDIT, canRename ); //only enable for columns that have editable tags
    popup.setItemEnabled( FILL_DOWN, canRename && itemCount > 1 );
    popup.setItemEnabled( BURN_MENU, item->url().isLocalFile() && K3bExporter::isAvailable() );
    popup.setItemEnabled( REMOVE, !isLocked() ); // can't remove things when playlist is locked,
    popup.setItemEnabled( DELETE, !isLocked() ); // that's the whole point

    QValueList<QString> submenuTexts = m_customSubmenuItem.keys();
    for( QValueList<QString>::Iterator keyIt =submenuTexts.begin(); keyIt !=  submenuTexts.end(); ++keyIt )
    {
        KPopupMenu* menu;
        if( (*keyIt) == "root")
            menu = &popup;
        else
        {
            menu = new KPopupMenu();
            popup.insertItem( *keyIt, menu);
        }
        foreach(m_customSubmenuItem[*keyIt])
        {
            int id;
            if(m_customIdItem.isEmpty())
                id=LAST;
            else
                id=m_customIdItem.keys().last()+1;
            menu->insertItem( (*it), id );
            m_customIdItem[id]= (*keyIt) + ' ' + (*it);
        }
    }

    int menuItemId= popup.exec( p );
    PlaylistItem *prev_stopafter = m_stopAfterTrack;

    switch( menuItemId )
    {
    case PLAY:
        activate( item );
        break;

    case PLAY_NEXT:
        //NOTE: This forbids dequeuing of multiple tracks
       // if( itemCount > 1 )
            // we need to dequeue everything that is already queued when
            // the user is queueing a selection, otherwise it feels wrong
            // m_nextTracks.clear();

        for( MyIt it( this, MyIt::Selected ); *it; ++it )
            queue( *it );
        break;

    case STOP_DONE:
        if( m_stopAfterTrack == item )
            m_stopAfterTrack = 0;
        else
        {
            m_stopAfterTrack = item;
            item->repaint();
        }

        if( prev_stopafter )
            prev_stopafter->repaint();
        break;

    case VIEW:
        showTagDialog( selectedItems() );
        break;

    case EDIT:
        // do this because QListView sucks, if track change occurs during
        // an edit event, the rename operation ends, BUT, the list is not
        // cleared because writeTag is never called. Q/K ListView sucks
        m_itemsToChangeTagsFor.clear();

        if( !item->isSelected() )
            m_itemsToChangeTagsFor.append( item );
        else
            for( MyIt it( this, MyIt::Selected ); *it; ++it )
                m_itemsToChangeTagsFor.append( *it );

        rename( item, col );
        break;

    case FILL_DOWN:
        //Spreadsheet like fill-down
        {
            QString newTag = item->exactText( col );
            MyIt it( this, MyIt::Selected );

            //special handling for track column
            uint trackNo = (*it)->exactText( PlaylistItem::Track ).toInt(); //returns 0 if it is not a number

            //we should start at the next row if we are doing track number
            //and the first row has a number set
            if ( trackColumn && trackNo > 0 )
                ++it;

            ThreadWeaver::JobList jobs;
            bool updateView = true;
            for( ; *it; ++it ) {
                if ( trackColumn )
                    //special handling for track column
                    newTag = QString::number( ++trackNo );

                else if ( *it == item )
                    //skip the one we are copying
                    continue;

                //FIXME fix this hack!
                if ( (*it)->exactText( col ) != i18n("Writing tag...") )
                    jobs.prepend( new TagWriter( *it, (*it)->exactText( col ), newTag, col, updateView ) );

                updateView = false;
            }

            ThreadWeaver::instance()->queueJobs( jobs );
        }
        break;

    case COPY:
        copyToClipboard( item );
        break;

    case BURN_SELECTION_DATA:
        burnSelectedTracks( K3bExporter::DataCD );
        break;

    case BURN_SELECTION_AUDIO:
        burnSelectedTracks( K3bExporter::AudioCD );
        break;

    case BURN_ALBUM_DATA:
        K3bExporter::instance()->exportAlbum( item->album(), K3bExporter::DataCD );
        break;

    case BURN_ALBUM_AUDIO:
        K3bExporter::instance()->exportAlbum( item->album(), K3bExporter::AudioCD );
        break;

    case BURN_ARTIST_DATA:
        K3bExporter::instance()->exportArtist( item->artist(), K3bExporter::DataCD );
        break;

    case BURN_ARTIST_AUDIO:
        K3bExporter::instance()->exportArtist( item->artist(), K3bExporter::AudioCD );
        break;

    default:
        if(menuItemId < LAST)
            break;
        customMenuClicked(menuItemId);
        break;
    }

    #undef item
}



////////////////////////////////////////////////////////////////////////////////
/// Misc Private Methods
////////////////////////////////////////////////////////////////////////////////

void
Playlist::lock()
{
   if( m_lockStack == 0 ) {
      m_clearButton->setEnabled( false );
      m_undoButton->setEnabled( false );
      m_redoButton->setEnabled( false );
   }

   m_lockStack++;
}

void
Playlist::unlock()
{
   Q_ASSERT( m_lockStack > 0 );

   m_lockStack--;

   if( m_lockStack == 0 ) {
      m_clearButton->setEnabled( true );
      m_undoButton->setEnabled( !m_undoList.isEmpty() );
      m_redoButton->setEnabled( !m_redoList.isEmpty() );
   }
}

int
Playlist::visibleColumns() const
{
    int r = 0, i = 1;
    for( const int n = columns(); i <= n; ++i)
        if( columnWidth( i - 1 ) )
            ++r;
    return r;
}

int
Playlist::mapToLogicalColumn( int physical )
{
    int logical = header()->mapToSection( physical );

    //skip hidden columns
    int n = 0;
    for( int i = 0; i <= physical; ++i )
        if( !header()->sectionSize( header()->mapToSection( physical - i ) ) )
            ++n;
    while( n )
    {
        logical = header()->mapToSection( ++physical );
        if( logical < 0 )
        {
            logical = header()->mapToSection( physical - 1 );
            break;
        }
        if( header()->sectionSize( logical ) )
            --n;
    }

    return logical;
}

void
Playlist::removeItem( PlaylistItem *item )
{
    // NOTE we don't check isLocked() here as it is assumed that if you call this function you
    // really want to remove the item, there is no way the user can reach here without passing
    // a lock() check, (currently...)

    //this function ensures we don't have dangling pointers to items that are about to be removed
    //for some reason using QListView::takeItem() and QListViewItem::takeItem() was ineffective
    //NOTE we don't delete item for you! You must call delete item yourself :)

    //TODO there must be a way to do this without requiring notification from the item dtor!
    //NOTE orginally this was in ~PlaylistItem(), but that caused crashes due to clear() *shrug*
    //NOTE items already removed by takeItem() will crash if you call nextSibling() on them
    //     taken items return 0 from listView()
    //FIXME if you remove a series of items including the currentTrack and all the nextTracks
    //      then no new nextTrack will be selected and the playlist will resume from the begging
    //      next time

    if( m_currentTrack == item )
    {
        setCurrentTrack( 0 );

        //ensure the playlist doesn't start at the beginning after the track that's playing ends
        //we don't need to do that in random mode, it's getting randomly selected anyways
        if( m_nextTracks.isEmpty() && !AmarokConfig::randomMode() )
        {
            //*MyIt( item ) returns either "item" or if item is hidden, the next visible playlistitem
            PlaylistItem* const next = *MyIt( item );
            m_nextTracks.append( next );
            repaintItem( next );
        }
    }

    if( m_stopAfterTrack == item )
        m_stopAfterTrack = 0; //to be safe

    //keep m_nextTracks queue synchronised
    if( m_nextTracks.removeRef( item ) )
       refreshNextTracks();

    //keep recent buffer synchronised
    m_prevTracks.removeRef( item ); //removes all pointers to item

    int length = item->seconds().toInt();
    if( length > 0 ) m_totalLength -= length;
    if( m_selCount > 0 ) m_selCount--;
    if( m_selLength > 0 ) m_selLength -= length;
    if( item->isVisible() )
    {
        m_visCount--;
        if( length > 0 )
            m_visLength -= length;
    }

    emit itemCountChanged( childCount()-1, m_totalLength, m_visCount, m_visLength, m_selCount, m_selLength );
}

void
Playlist::refreshNextTracks( int from )
{
    // This function scans the m_nextTracks list starting from the 'from'
    // position and from there on updates the progressive numbering on related
    // items and repaints them. In short it performs an update subsequent to
    // a renumbering/order changing at some point of the m_nextTracks list.

    //start on the 'from'-th item of the list

    for( PlaylistItem* item = (from == -1) ? m_nextTracks.current() : m_nextTracks.at( from );
         item;
         item = m_nextTracks.next() )
    {
        repaintItem( item );
    }
}

void
Playlist::saveUndoState() //SLOT
{
   if( saveState( m_undoList ) )
   {
      m_redoList.clear();

      m_undoButton->setEnabled( true );
      m_redoButton->setEnabled( false );
   }
}

bool
Playlist::saveState( QStringList &list )
{
    //used by undo system, save state of playlist to undo/redo list

   //do not change this! It's required by the undo/redo system to work!
   //if you must change this, fix undo/redo first. Ask me what needs fixing <mxcl>
   if( !isEmpty() )
   {
      QString fileName;
      m_undoCounter %= AmarokConfig::undoLevels();
      fileName.setNum( m_undoCounter++ );
      fileName.prepend( m_undoDir.absPath() + "/" );
      fileName.append( ".xml" );

      if ( list.count() >= (uint)AmarokConfig::undoLevels() )
      {
         m_undoDir.remove( list.first() );
         list.pop_front();
      }

      // Copy current playlist to undo folder
      KIO::file_copy( KURL::fromPathOrURL( defaultPlaylistPath() ),
                      KURL::fromPathOrURL( fileName ),
                      -1, true, false, false );

      list.append( fileName );

      return true;
   }

   return false;
}

void
Playlist::switchState( QStringList &loadFromMe, QStringList &saveToMe )
{
    m_undoDirt = true;
    //switch to a previously saved state, remember current state
    KURL url; url.setPath( loadFromMe.last() );
    loadFromMe.pop_back();

    //save current state
    saveState( saveToMe );

    //blockSignals so that we don't cause a saveUndoState()
    //FIXME, but this will stop the search lineEdit from being cleared..
    blockSignals( true );
      clear();
    blockSignals( false );

    insertMediaInternal( url, 0 ); //because the listview is empty, undoState won't be forced

    m_undoButton->setEnabled( !m_undoList.isEmpty() );
    m_redoButton->setEnabled( !m_redoList.isEmpty() );

    if( isDynamic() ) alterHistoryItems( !AmarokConfig::dynamicMarkHistory() );
    m_undoDirt = false;
}

void
Playlist::slotMouseButtonPressed( int button, QListViewItem *after, const QPoint &p, int col ) //SLOT
{
    switch( button )
    {
    case Qt::MidButton:
    {
        const QString path = QApplication::clipboard()->text( QClipboard::Selection );
        const KURL url = KURL::fromPathOrURL( path );

        debug() << "X11 Paste: " << url << endl;

        if( url.isValid() )
            insertMediaInternal( url, (PlaylistItem*)(after ? after : lastItem()) );

        break;
    }

    case Qt::RightButton:
        showContextMenu( after, p, col );
        break;

    default:
        ;
    }
}

void
Playlist::slotSelectionChanged() //SLOT
{
    m_selCount = 0;
    m_selLength = 0;

    for( MyIt it( this, MyIt::Selected ); *it; ++it )
    {
        ++m_selCount;
        int length = static_cast<PlaylistItem *>(*it)->seconds().toInt();
        if( length > 0 )
            m_selLength += length;
    }

    emit itemCountChanged( childCount(), m_totalLength, m_visCount, m_visLength, m_selCount, m_selLength );
}

void
Playlist::slotGlowTimer() //SLOT
{
    if( !currentTrack() || currentTrack()->isSelected() ) return;

    using namespace Glow;

    if( counter <= STEPS*2 )
    {
        // 0 -> STEPS -> 0
        const double d = (counter > STEPS) ? 2*STEPS-counter : counter;

        {
            using namespace Base;
            PlaylistItem::glowBase = QColor( r + int(d*dr), g + int(d*dg), b + int(d*db) );
        }

        {
            using namespace Text;
            PlaylistItem::glowText = QColor( r + int(d*dr), g + int(d*dg), b + int(d*db) );
        }

        repaintItem( currentTrack() );
    }

    ++counter &= 63; //built in bounds checking with &=
}

void
Playlist::slotRepeatTrackToggled( bool /* enabled */ )
{
    if( m_currentTrack )
        m_currentTrack->repaint();
}

void
Playlist::slotEraseMarker() //SLOT
{
    if( m_marker )
    {
        const QRect spot = drawDropVisualizer( 0, 0, m_marker );
        m_marker = 0;
        viewport()->repaint( spot, false );
    }
}

void
Playlist::showTagDialog( QPtrList<QListViewItem> items )
{
    // despite being modal, the user can still modify the playlist
    // in a dangerous fashion, eg dcop clear() will get processed by
    // the TagDialog exec() loop. So we must lock the playlist
    Playlist::lock();

    if ( items.count() == 1 ) {
        PlaylistItem *item = static_cast<PlaylistItem*>( items.first() );

        if ( QFile::exists( item->url().path() ) ) {
            //NOTE we are modal because, eg, user clears playlist while
            //this dialog is shown, then the dialog operates on the playlistitem
            //TODO not perfect as dcop clear works for instance
            MetaBundle bundle( item );
            TagDialog( bundle, item, instance() ).exec();
        }
        else
            KMessageBox::sorry( this, i18n( "This file does not exist:" ) + " " + item->url().path() );
    }
    else {
        //edit multiple tracks in tag dialog
        KURL::List urls;
        for( QListViewItem *item = items.first(); item; item = items.next() )
            if ( item->isVisible() )
                urls << static_cast<PlaylistItem*>( item )->url();

        TagDialog( urls, instance() ).exec();
    }

    Playlist::unlock();
}


#include <kactivelabel.h>
#include <kdialog.h>
#include <kpushbutton.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qprocess.h>
#include <unistd.h>      //usleep()

// Moved outside the only function that uses it because
// gcc 2.95 doesn't like class declarations there.
    class CustomColumnDialog : public KDialog
    {
    public:
        CustomColumnDialog( QWidget *parent )
            : KDialog( parent )
        {
            QLabel *textLabel1, *textLabel2, *textLabel3;
            QLineEdit *lineEdit1, *lineEdit2;
            QGroupBox *groupBox1;

            textLabel1 = new QLabel( i18n(
                "<p>You can create a custom column that runs a shell command against each item in the playlist. "
                "The shell command is run as the user <b>nobody</b>, this is for security reasons.\n"
                "<p>You can only run the command against local files for the time being. "
                "The fullpath is inserted at the position <b>%f</b> in the string. "
                "If you do not specify <b>%f</b> it is appended." ), this );
            textLabel2 = new QLabel( i18n( "Column &name:" ), this );
            textLabel3 = new QLabel( i18n( "&Command:" ), this );

            lineEdit1  = new QLineEdit( this, "ColumnName" );
            lineEdit2  = new QLineEdit( this, "Command" );

            groupBox1 = new QGroupBox( 1, Qt::Vertical, i18n( "Examples" ), this );
            groupBox1->layout()->setMargin( 11 );
            new KActiveLabel( i18n( "file --brief %f\n" "ls -sh %f\n" "basename %f\n" "dirname %f" ), groupBox1 );

            // buddies
            textLabel2->setBuddy( lineEdit1 );
            textLabel3->setBuddy( lineEdit2 );

            // layouts
            QHBoxLayout *layout1 = new QHBoxLayout( 0, 0, 6 );
            layout1->addItem( new QSpacerItem( 181, 20, QSizePolicy::Expanding, QSizePolicy::Minimum ) );
            layout1->addWidget( new KPushButton( KStdGuiItem::ok(), this, "OkButton" ) );
            layout1->addWidget( new KPushButton( KStdGuiItem::cancel(), this, "CancelButton" ) );

            QGridLayout *layout2 = new QGridLayout( 0, 2, 2, 0, 6 );
            layout2->QLayout::add( textLabel2 );
            layout2->QLayout::add( lineEdit1 );
            layout2->QLayout::add( textLabel3 );
            layout2->QLayout::add( lineEdit2 );

            QVBoxLayout *Form1Layout = new QVBoxLayout( this, 11, 6, "Form1Layout");
            Form1Layout->addWidget( textLabel1 );
            Form1Layout->addWidget( groupBox1 );
            Form1Layout->addLayout( layout2 );
            Form1Layout->addLayout( layout1 );
            Form1Layout->addItem( new QSpacerItem( 20, 231, QSizePolicy::Minimum, QSizePolicy::Expanding ) );

            // properties
            setCaption( i18n("Add Custom Column") );

            // connects
            connect(
                child( "OkButton" ),
                SIGNAL(clicked()),
                SLOT(accept()) );
            connect(
                child( "CancelButton" ),
                SIGNAL(clicked()),
                SLOT(reject()) );
        }

        QString command() { return static_cast<KLineEdit*>(child("Command"))->text(); }
        QString name()    { return static_cast<KLineEdit*>(child("ColumnName"))->text(); }
    };

void
Playlist::addCustomColumn()
{
    CustomColumnDialog dialog( this );

    if ( dialog.exec() == QDialog::Accepted ) {
        const int index = addColumn( dialog.name(), 100 );
        QStringList args = QStringList::split( ' ', dialog.command() );

        QStringList::Iterator pcf = args.find( "%f" );
        if ( pcf == args.end() ) {
            //there is no %f, so add one on the end
            //TODO prolly this is confusing, instead ask the user if we should add one
            args += "%f";
            --pcf;
        }

        debug() << args << endl;

        //TODO need to do it with a %u for url and %f for file
        //FIXME gets stuck it seems if you submit broken commands

        //FIXME issues with the column resize stuff that cause freezing in eventFilters

        for( MyIt it( this ); *it; ++it ) {
            if( (*it)->url().protocol() != "file" )
               continue;

            *pcf = (*it)->url().path();

            debug() << args << endl;

            QProcess p( args );
            for( p.start(); p.isRunning(); /*kapp->processEvents()*/ )
                ::usleep( 5000 );

            (*it)->setText( index, p.readStdout() );
        }
    }
}

#include <taglib/fileref.h>
#include <taglib/tag.h>

TagWriter::TagWriter( PlaylistItem *item, const QString &oldTag, const QString &newTag, const int col, const bool updateView )
        : ThreadWeaver::Job( "TagWriter" )
        , m_item( item )
        , m_failed( true )
        , m_oldTagString( oldTag )
        , m_newTagString( newTag )
        , m_tagType( col )
        , m_updateView( updateView )
{
    Playlist::instance()->lock();

    item->setText( col, i18n( "Writing tag..." ) );
}

TagWriter::~TagWriter()
{
    Playlist::instance()->unlock();
}

bool
TagWriter::doJob()
{
    const QString path = m_item->url().path();
    //TODO I think this causes problems with writing and reading tags for non latin1 people
    //check with wheels abolut what it does and why to do it
    //TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding( TagLib::String::UTF8 );
    TagLib::FileRef f( QFile::encodeName( path ), false );

    if ( !f.isNull() )
    {
        TagLib::Tag *t = f.tag();

        switch ( m_tagType )
        {
            case PlaylistItem::Title:
                t->setTitle( QStringToTString( m_newTagString ));
                break;
            case PlaylistItem::Artist:
                t->setArtist( QStringToTString( m_newTagString ) );
                break;
            case PlaylistItem::Album:
                t->setAlbum( QStringToTString( m_newTagString ) );
                break;
            case PlaylistItem::Year:
                t->setYear( m_newTagString.toInt() );
                break;
            case PlaylistItem::Comment:
                //FIXME how does this work for vorbis files?
                //Are we likely to overwrite some other comments?
                //Vorbis can have multiple comment fields..
                t->setComment( QStringToTString( m_newTagString ) );
                break;
            case PlaylistItem::Genre:
                t->setGenre( QStringToTString( m_newTagString ) );
                break;
            case PlaylistItem::Track:
                t->setTrack( m_newTagString.toInt() );
                break;

            default:
                return true;
        }

        m_failed = !f.save();
    }

    return true;
}

void
TagWriter::completeJob()
{
    switch( m_failed ) {
    case true:
        // we write a space for some reason I cannot recall
        m_item->setText( m_tagType, m_oldTagString.isEmpty() ? " " : m_oldTagString );
        amaroK::StatusBar::instance()->longMessage( i18n(
                "Sorry, the tag for %1 could not be changed." ).arg( m_item->url().fileName() ) );
        break;

    case false:
        m_item->setText( m_tagType, m_newTagString.isEmpty() ? " " : m_newTagString );
        CollectionDB::instance()->updateURL( m_item->url().path(), m_updateView );
    }
}


#include "playlist.moc"
