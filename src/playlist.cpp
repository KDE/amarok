/* Copyright 2002-2004 Mark Kretschmann, Max Howell
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
#include "collectiondb.h"
#include "debug.h"
#include "enginecontroller.h"
#include "k3bexporter.h"
#include "metabundle.h"
#include "osd.h"
#include "playlist.h"
#include "playlistitem.h"
#include "playlistloader.h"
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
#include <qvaluevector.h>    //playNextTrack()
#include <qlayout.h>

#include <kaction.h>
#include <kapplication.h>
#include <kcursor.h>         //setOverrideCursor()
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
    #include <unistd.h>      //usleep()

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
        : QListViewItemIterator( item, flags == All ? 0 : flags | Visible )
    {}

    MyIterator( QListView *view, int flags = 0 )
        : QListViewItemIterator( view, flags == All ? 0 : flags | Visible )
    {}

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


/**
 * Threaded tag-updating
 */

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


Playlist *Playlist::s_instance = 0;


Playlist::Playlist( QWidget *parent )
        : KListView( parent, "ThePlaylist" )
        , EngineObserver( EngineController::instance() )
        , m_currentTrack( 0 )
        , m_marker( 0 )
        , m_firstColumn( 0 )
        , m_totalLength( 0 )
        , m_selectCounter( 0 )
        , m_selectLength( 0 )
        , m_undoDir( amaroK::saveLocation( "undo/" ) )
        , m_undoCounter( 0 )
        , m_stopAfterCurrent( false )
        , m_showHelp( true )
        , m_lockStack( 0 )
        , m_columnFraction( 13, 0 )
{
    s_instance = this;

    EngineController* const ec = EngineController::instance();
    connect( ec, SIGNAL(orderPrevious()), SLOT(playPrevTrack()) );
    connect( ec, SIGNAL(orderNext()),     SLOT(playNextTrack()) );
    connect( ec, SIGNAL(orderCurrent()),  SLOT(playCurrentTrack()) );


    setShowSortIndicator( true );
    setDropVisualizer( false );   //we handle the drawing for ourselves
    setDropVisualizerWidth( 3 );
    setItemsRenameable( true );
    KListView::setSorting( NO_SORT ); //use base so we don't saveUndoState() too
    setAcceptDrops( true );
    setSelectionMode( QListView::Extended );
    setAllColumnsShowFocus( true );

    #if KDE_IS_VERSION( 3, 3, 91 )
    setShadeSortColumn( true );
    #endif

    //NOTE order is critical because we can't set indexes or ids
    addColumn( i18n( "Track Name" ),   0 );
    addColumn( i18n( "Title"      ), 200 ); //displays trackname if no title tag
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

    setRenameable( 0, false ); //TODO allow renaming of the filename
    setRenameable( 1 );
    setRenameable( 2 );
    setRenameable( 3 );
    setRenameable( 4 );
    setRenameable( 5 );
    setRenameable( 6 );
    setRenameable( 7 );
    setRenameable( 11 );
    setColumnAlignment(  7, Qt::AlignCenter ); //track
    setColumnAlignment(  9, Qt::AlignRight );  //length
    setColumnAlignment( 10, Qt::AlignCenter ); //bitrate
    setColumnAlignment( 11, Qt::AlignCenter ); //score


    connect( this,     SIGNAL( doubleClicked( QListViewItem* ) ),
             this,       SLOT( activate( QListViewItem* ) ) );
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


    connect( &Glow::timer, SIGNAL(timeout()), SLOT(slotGlowTimer()) );


    KActionCollection* const ac = amaroK::actionCollection();
    KStdAction::copy( this, SLOT( copyToClipboard() ), ac, "playlist_copy" );
    KStdAction::selectAll( this, SLOT( selectAll() ), ac, "playlist_select_all" );
    m_clearButton = KStdAction::clear( this, SLOT( clear() ), ac, "playlist_clear" );
    m_undoButton  = KStdAction::undo( this, SLOT( undo() ), ac, "playlist_undo" );
    m_redoButton  = KStdAction::redo( this, SLOT( redo() ), ac, "playlist_redo" );
    new KAction( i18n( "S&huffle" ), "rebuild", CTRL+Key_H, this, SLOT( shuffle() ), ac, "playlist_shuffle" );
    new KAction( i18n( "&Goto Current Track" ), "today", CTRL+Key_Enter, this, SLOT( showCurrentTrack() ), ac, "playlist_show" );
    new KAction( i18n( "Remove Duplicates / Missing" ), 0, this, SLOT( removeDuplicates() ), ac, "playlist_remove_duplicates" );

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
}

Playlist::~Playlist()
{
    saveLayout( KGlobal::config(), "PlaylistColumnsLayout" );

    if( AmarokConfig::savePlaylist() ) saveXML( defaultPlaylistPath() );

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
            after = currentTrack();   //insert after the current track. enables the click-n-reaction feeling

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
        ThreadWeaver::instance()->queueJob( new UrlLoader( list, after, directPlay ) );
    }
    else
        amaroK::StatusBar::instance()->shortMessage( i18n("Cannot insert nothing into the playlist.") );
}

void
Playlist::insertMediaSql( const QString& sql, int options )
{
    // TODO Implement more options

    if ( options & Replace )
        clear();

    setSorting( NO_SORT );
    ThreadWeaver::instance()->queueJob( new SqlLoader( sql, 0 ) );
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
        ThreadWeaver::instance()->queueJob( new UrlLoader( url, 0 ) );
}



////////////////////////////////////////////////////////////////////////////////
/// Current Track Handling
////////////////////////////////////////////////////////////////////////////////

void
Playlist::playNextTrack()
{
    PlaylistItem *item = currentTrack();

    if ( m_stopAfterCurrent )
    {
        m_stopAfterCurrent = false;
        activate( 0 );
        return;
    }

    if( !AmarokConfig::repeatTrack() )
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
        }
        else item = *MyIt( this ); //ie. first visible item

        if ( !item && AmarokConfig::repeatPlaylist() )
            item = *MyIt( this ); //ie. first visible item
    }

    if ( EngineController::engine()->loaded() )
        activate( item );
    else
        setCurrentTrack( item );
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
        playNextTrack();

    //we must do this even if the above is correct
    //since the engine is not loaded the first time the user presses play
    //then calling the next() function wont play it
    activate( currentTrack() );
}

void
Playlist::queue( QListViewItem *item )
{
    #define item static_cast<PlaylistItem*>(item)

    const int  queueIndex  = m_nextTracks.findRef( item );
    const bool isQueued    = queueIndex != -1;

    item->setSelected( false ); //for prettiness

    if( isQueued )
        //remove the item, this is better way than remove( item )
        m_nextTracks.remove( queueIndex ); //sets current() to next item

    else
        m_nextTracks.append( item );

    refreshNextTracks(); // from current()

    //NOTE "item" is repainted due to the setSelected() call
    // ^__ not if called in bulk by context/collection browsers

    updateNextPrev();
    #undef item
}

void
Playlist::activate( QListViewItem *item )
{
    ///item will be played if possible, the playback may be delayed
    ///so we start the glow anyway and hope

    //All internal requests for playback should come via
    //this function please!

    if( item )
    {
        #define item static_cast<PlaylistItem*>(item)

        m_prevTracks.append( item );

        //if we are playing something from the next tracks
        //list, remove it from the list
        if ( m_nextTracks.removeRef( item ) )
            refreshNextTracks();

        //looks bad painting selected and glowing
        //only do when user explicitely activates an item though
        item->setSelected( false );

        setCurrentTrack( item );

        //use PlaylistItem::MetaBundle as it also updates the audioProps
        EngineController::instance()->play( item );
        #undef item
    }
    else
    {
        //we have reached the end of the playlist

        setCurrentTrack( 0 );
        EngineController::instance()->stop();
        amaroK::OSD::instance()->OSDWidget::show( i18n("Playlist finished") );
    }
}

void
Playlist::setCurrentTrack( PlaylistItem *item )
{
    ///mark item as the current track, it makes it glow and everything

    const bool canScroll = !renameLineEdit()->isVisible() && selectedItems().count() < 2;

    //if nothing is current and then playback starts, we must show the currentTrack
    if( !m_currentTrack && canScroll ) ensureItemVisible( item ); //handles 0 gracefully

    if( AmarokConfig::playlistFollowActive() && m_currentTrack && item && canScroll )
    {
        // if old item in view and the new one isn't do scrolling
        int currentY = itemPos( m_currentTrack );

        if( currentY + m_currentTrack->height() <= contentsY() + visibleHeight()
            && currentY >= contentsY() )
        {
            const uint scrollMax = 2 * item->height();
            // Scroll towards the middle but no more than two lines extra
            uint scrollAdd = viewport()->height() / 2 - item->height();
            if( scrollAdd > scrollMax ) scrollAdd = scrollMax;

            int itemY = itemPos( item );
            int itemH = item->height();

            if( itemY + itemH > contentsY() + visibleHeight() ) // scroll down
            {
                setContentsPos( contentsX(), itemY - visibleHeight() + itemH + scrollAdd );
            }
            else if( itemY < contentsY() ) // scroll up
            {
                setContentsPos( contentsX(), itemY - scrollAdd );
            }
        }
    }

    PlaylistItem *prev = m_currentTrack;
    m_currentTrack = item;

    if ( prev ) {
        //reset to normal height
        prev->invalidateHeight();
        prev->setup();
        //remove pixmap in first column
        prev->setPixmap( m_firstColumn, QPixmap() );
    }

    updateNextPrev();

    Glow::reset();
    slotGlowTimer();
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

        if( item )
        {
           //display "Play" icon
           item->setPixmap( m_firstColumn, locate( "data", QString( "amarok/images/currenttrack_play.png" ) ) );
           PlaylistItem::setPixmapChanged();
        }
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
    amaroK::actionCollection()->action( "prev" )->setEnabled( isTrackBefore() );
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
}

void
Playlist::engineStateChanged( Engine::State state )
{
    switch( state )
    {
    case Engine::Playing:
        amaroK::actionCollection()->action( "pause" )->setEnabled( true );
        amaroK::actionCollection()->action( "stop" )->setEnabled( true );
        amaroK::actionCollection()->action( "playlist_show" )->setEnabled( true );

        Glow::startTimer();

        if( m_currentTrack ) {
            m_currentTrack->setPixmap( m_firstColumn, amaroK::getPNG( AmarokConfig::repeatTrack()
                ? "currenttrack_repeat"
                : "currenttrack_play" ) );
            PlaylistItem::setPixmapChanged();
        }
        break;

    case Engine::Paused:
        amaroK::actionCollection()->action( "pause" )->setEnabled( true );
        amaroK::actionCollection()->action( "stop" )->setEnabled( true );
        amaroK::actionCollection()->action( "playlist_show" )->setEnabled( true );

        Glow::reset();

        if( m_currentTrack ) {
            m_currentTrack->setPixmap( m_firstColumn, amaroK::getPNG( "currenttrack_pause" ) );
            PlaylistItem::setPixmapChanged();

            slotGlowTimer(); //update glow state
        }
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

            //reset glow state
            slotGlowTimer();
        }

    case Engine::Idle:
        ;
    }
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
    m_totalLength = 0;

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

    emit itemCountChanged( childCount(), m_totalLength, 0, 0 );
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
    KListView::rename( item, column );

    switch( column )
    {
        case PlaylistItem::Artist:
            renameLineEdit()->completionObject()->setItems( CollectionDB::instance()->artistList() );
            break;

        case PlaylistItem::Album:
            renameLineEdit()->completionObject()->setItems( CollectionDB::instance()->albumList() );
            break;

        case PlaylistItem::Genre:
            renameLineEdit()->completionObject()->setItems( MetaBundle::genreList() );
            break;

        default:
            renameLineEdit()->completionObject()->clear();
            break;
    }

    renameLineEdit()->completionObject()->setCompletionMode( KGlobalSettings::CompletionPopupAuto );

    m_editOldTag = static_cast<PlaylistItem *>(item)->exactText( column );
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
        m_currentTrack->setPixmap( m_firstColumn, locate( "data", QString( "amarok/images/currenttrack_play.png" ) ) );
        PlaylistItem::setPixmapChanged();
    }
}

void
Playlist::paletteChange( const QPalette &p )
{
    DEBUG_FUNC_INFO

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

    // repaint currentTrack marker
    Glow::reset();
    slotGlowTimer();
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
        QSimpleRichText t( i18n(
                "<div align=center>"
                  "<h3>The Playlist</h3>"
                    "This is the playlist. "
                    "To create a listing, "
                      "<b>drag</b> tracks from the browser-panels on the left, "
                      "<b>drop</b> them here and then <b>double-click</b> them to start playback!"
                  "<h3>The Browsers</h3>"
                    "The browsers are the source of all your music. "
                    "The collection-browser holds your collection. "
                    "The playlist-browser holds your pre-set playlistings. "
                    "The file-browser shows a file-selector which you can use to access any music on your computer. "
                "</div>" ), QApplication::font() );

        const uint wd3 = viewport()->width() / 3;

        t.setWidth( wd3 );

        const uint w = t.width() + 30;
        const uint h = t.height() + 30;
        const uint y = (viewport()->height() - h) / 2;

        p.setBrush( colorGroup().background() );
        p.drawRoundRect( wd3-15, y-15, w, h, (8*200)/w, (8*200)/h );
        t.draw( &p, wd3, y, QRect(), colorGroup() );
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
        {
            if( m_nextTracks.removeRef( item ) )
            {
                //m_nextTracks.current() is now item
                refreshNextTracks(); //will repaint from current()
            }
            else m_nextTracks.append( item );

            item->repaint(); //we need to repaint item in both cases

            updateNextPrev();
        }

        return TRUE; //yum!
    }

    //allow the header to process this
    return KListView::eventFilter( o, e );

    #undef me
}

void
Playlist::customEvent( QCustomEvent *e )
{
    switch( e->type() )
    {
    case ThreadWeaver::Job::JobStartedEvent:
        lock(); // prevent user removing items as this could be bad
        break;

    case ThreadWeaver::Job::JobFinishedEvent: {
        unlock();

        refreshNextTracks( 0 );

        // Disable help if playlist is populated
        if ( !isEmpty() )
            m_showHelp = false;

        //necessary usually
        m_totalLength = 0;
        int itemCount = 0;
        QListViewItemIterator it( this );
        for( ; it.current(); ++it, itemCount++ ) {
            int length = static_cast<PlaylistItem *>(*it)->seconds().toInt();
            if( length > 0 )
                m_totalLength += length;
        }
        emit itemCountChanged( itemCount, m_totalLength, m_selectCounter, m_selectLength );

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

        //force redraw of currentTrack marker, play icon, etc.
        //setCurrentTrack( currentTrack() );

        break; }

    default:
         ;
    }

    updateNextPrev();
}



////////////////////////////////////////////////////////////////////////////////
/// Misc Public Methods
////////////////////////////////////////////////////////////////////////////////

void
Playlist::saveM3U( const QString &path ) const
{
    QFile file( path );

    if( file.open( IO_WriteOnly ) )
    {
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
            stream << (url.protocol() == "file" ? url.path() : url.url());
            stream << "\n";
        }
    }
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
Playlist::shuffle() //SLOT
{
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
    for( MyIterator it( this, MyIt::Selected ); *it; list.prepend( *it ), ++it );

    if( list.isEmpty() ) return;
    saveUndoState();

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
    int count  = urls.count();
    int button = KMessageBox::warningContinueCancel( this,
                    i18n( "<p>You have selected %1 to be <b>irreversibly</b> deleted." )
                        .arg( count > 1 ?
                            i18n( "1 file", "<u>%n files</u>", count ) : //we must use this form of i18n()
                            static_cast<PlaylistItem*>(currentItem())->url().prettyURL().prepend("<i>'").append("'</i>") ),
                    QString::null,
                    i18n("&Delete") );

    if ( button == KMessageBox::Continue )
    {
        // TODO We need to check which files have been deleted successfully
        KIO::DeleteJob* job = KIO::del( urls );
        connect( job, SIGNAL(result( KIO::Job* )), SLOT(removeSelectedItems()) );

        job->setAutoErrorHandlingEnabled( false );

        amaroK::StatusBar::instance()->newProgressOperation( job )
                .setDescription( i18n("Deleting files") );
    }
}

void
Playlist::removeDuplicates() //SLOT
{
    // Remove dead entries:

    for( QListViewItemIterator it( this ); it.current(); ) {
        PlaylistItem* item = static_cast<PlaylistItem*>( *it );
        if ( !item->exists() ) {
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
            text.prepend( playlistItem->title() + " :: " );

        // Copy both to clipboard and X11-selection
        QApplication::clipboard()->setText( text, QClipboard::Clipboard );
        QApplication::clipboard()->setText( text, QClipboard::Selection );

        amaroK::OSD::instance()->OSDWidget::show( i18n( "Copied: %1" ).arg( text ) );
    }
}

void Playlist::undo() { if( !isLocked() ) switchState( m_undoList, m_redoList ); } //SLOT
void Playlist::redo() { if( !isLocked() ) switchState( m_redoList, m_undoList ); } //SLOT

void
Playlist::updateMetaData( const MetaBundle &mb ) //SLOT
{
    for( MyIt it( this, MyIt::All ); *it; ++it )
        if( mb.url() == (*it)->url() )
            (*it)->setText( mb );
}

void
Playlist::setFilter( const QString &query ) //SLOT
{
    //TODO if we provided the lineEdit m_lastSearch would be unecessary

    const QString loweredQuery = query.lower();
    const QStringList terms = QStringList::split( ' ', loweredQuery );
    MyIt it( this, loweredQuery.startsWith( m_lastSearch ) ? MyIt::Visible : MyIt::All );
    int y;

    for( ;*it; ++it ) {
        bool visible = true;

        for( uint x = 0; visible && x < terms.count(); ++x ) {
            for( y = 0; y < columns(); ++y )
                if ( columnWidth( y ) && (*it)->exactText( y ).lower().contains( terms[x] ) )
                    break;
            visible = ( y < columns() );
        }

        (*it)->setVisible( visible );
    }

    m_lastSearch = loweredQuery;

    // to me it seems sensible to do this, BUT if it seems annoying to you, remove it
    showCurrentTrack();
    triggerUpdate();
}

void
Playlist::scoreChanged( const QString &path, int score )
{
    for( MyIt it( this, MyIt::All ); *it; ++it )
    {
        PlaylistItem *item = (PlaylistItem*)*it;
        if ( item->url().path() == path )
            item->setText( PlaylistItem::Score, QString::number( score ) );
    }
}

void
Playlist::showContextMenu( QListViewItem *item, const QPoint &p, int col ) //SLOT
{
    #define item static_cast<PlaylistItem*>(item)

    enum { PLAY, PLAY_NEXT, STOP_DONE, VIEW, EDIT, FILL_DOWN, COPY, REMOVE, DELETE,
        BURN_MENU, BURN_SELECTION_DATA, BURN_SELECTION_AUDIO, BURN_ALBUM_DATA, BURN_ALBUM_AUDIO,
        BURN_ARTIST_DATA, BURN_ARTIST_AUDIO };

    if( item == 0 ) return; //technically we should show "Remove" but this is far neater

    const bool canRename   = isRenameable( col );
    const bool isCurrent   = (item == m_currentTrack);
    const bool isPlaying   = EngineController::engine()->state() == Engine::Playing;
    const int  queueIndex  = m_nextTracks.findRef( item );
    const bool isQueued    = queueIndex != -1;
    const uint itemCount   = selectedItems().count();
    const bool trackColumn = col == PlaylistItem::Track;
    const QString tagName  = columnText( col );
    const QString tag      = item->exactText( col );
    //Markey, sorry for the lengths of these lines! -mxcl

    KPopupMenu popup( this );

    popup.insertTitle( KStringHandler::rsqueeze( MetaBundle( item ).prettyTitle(), 50 ) );

    popup.insertItem( SmallIconSet( "player_play" ), isCurrent && isPlaying
        ? i18n( "&Restart" )
        : i18n( "&Play" ), 0, 0, Key_Enter, PLAY );

    if ( !isQueued ) {
        //not in nextTracks queue
        QString nextText = i18n("&Queue Track" );

        const uint nextIndex = m_nextTracks.count() + 1;
        if ( nextIndex > 1 )
            nextText += QString( " (%1)" ).arg( nextIndex );

        popup.insertItem( SmallIconSet( "2rightarrow" ), nextText, PLAY_NEXT );
    }
    else
        popup.insertItem( SmallIconSet( "2leftarrow" ), i18n( "&Dequeue (%1)" ).arg( queueIndex+1 ), PLAY_NEXT );

    if( isCurrent )
    {
       amaroK::actionCollection()->action( "pause" )->plug( &popup );
       popup.insertItem( i18n( "&Stop Playing After Track" ), STOP_DONE );
       popup.setItemChecked( STOP_DONE, m_stopAfterCurrent );
    }

    popup.insertSeparator();

    popup.insertItem( SmallIconSet( "edit" ), (itemCount == 1
        ? i18n( "&Edit Tag '%1'" )
        : i18n( "&Edit '%1' Tag For Selected Tracks" )).arg( tagName ), 0, 0, Key_F2, EDIT );
    popup.insertItem( trackColumn
        ? i18n("&Iteratively Assign Track Numbers")
        : i18n("Write '%1' For Selected Tracks").arg( KStringHandler::rsqueeze( tag, 30 ) ), FILL_DOWN );
    popup.insertItem( SmallIconSet( "editcopy" ), i18n( "&Copy Meta-String" ), 0, 0, CTRL+Key_C, COPY );

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

    popup.insertItem( SmallIconSet( "info" ), i18n( "&View/Edit Meta Information..." ), VIEW ); //TODO rename properties


    popup.setItemEnabled( EDIT, canRename ); //only enable for columns that have editable tags
    popup.setItemEnabled( FILL_DOWN, canRename && itemCount > 1 );
    popup.setItemEnabled( BURN_MENU, item->url().isLocalFile() && K3bExporter::isAvailable() );
    popup.setItemEnabled( REMOVE, !isLocked() ); // can't remove things when playlist is locked,
    popup.setItemEnabled( DELETE, !isLocked() ); // that's the whole point


    switch( popup.exec( p ) )
    {
    case PLAY:
        activate( item );
        break;

    case PLAY_NEXT:
        queue( item );
        break;

    case STOP_DONE:
        m_stopAfterCurrent = !popup.isItemChecked( STOP_DONE );
        break;

    case VIEW:
    {
        showTagDialog( selectedItems() );
        break;
    }

    case EDIT:
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
Playlist::mapToLogicalColumn( int physical )
{
    int logical;

    //skip hidden columns
    do logical = header()->mapToSection( physical++ );
    while ( !header()->sectionSize( logical ) );

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

    //keep m_nextTracks queue synchronised
    if( m_nextTracks.removeRef( item ) )
       refreshNextTracks();

    //keep recent buffer synchronised
    m_prevTracks.removeRef( item ); //removes all pointers to item

    int length = item->seconds().toInt();
    if( length > 0 ) m_totalLength -= length;
    if( m_selectCounter > 0 ) m_selectCounter--;
    if( m_selectLength > 0 ) m_selectLength -= length;

    emit itemCountChanged( childCount()-1, m_totalLength, m_selectCounter, m_selectLength );
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

      saveXML( fileName );
      list.append( fileName );

      return true;
   }

   return false;
}

void
Playlist::switchState( QStringList &loadFromMe, QStringList &saveToMe )
{
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
    m_selectCounter = 0;
    m_selectLength = 0;

    for( MyIt it( this, MyIt::Selected ); *it; ++it )
    {
        ++m_selectCounter;
        int length = static_cast<PlaylistItem *>(*it)->seconds().toInt();
        if( length > 0 )
            m_selectLength += length;
    }

    emit itemCountChanged( childCount(), m_totalLength, m_selectCounter, m_selectLength );
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
Playlist::slotRepeatTrackToggled( bool enabled )
{
    if ( !m_currentTrack || EngineController::engine()->state() == Engine::Idle )
        return;

    if ( enabled ) {
        m_currentTrack->setPixmap( m_firstColumn, amaroK::getPNG( "currenttrack_repeat" ) );
        PlaylistItem::setPixmapChanged();
    }
    else
        engineStateChanged( EngineController::engine()->state() );
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
Playlist::writeTag( QListViewItem *lvi, const QString &newTag, int column ) //SLOT
{
    QPtrList<QListViewItem> list;    //the list of the items to be edited
    if( !lvi->isSelected() )
        list.append( lvi );    //when the user is using the tab ordered renaming edit only the renaming item
    else
        list = selectedItems();

    for( QListViewItem *item = list.first(); item; item = list.next() ) {
        #define item static_cast<PlaylistItem*>(item)

        if( !item->isVisible() )
           continue;

        const QString &oldTag = item == lvi ? m_editOldTag : item->exactText(column);
        if( oldTag != newTag && !(oldTag.isEmpty() && newTag.isEmpty()) )  //write the new tag only if it's changed
        {
            if ( column == PlaylistItem::Score )
                // update score in database, only
                CollectionDB::instance()->setSongPercentage( item->url().path(), newTag.toInt() );
            else
                ThreadWeaver::instance()->queueJob( new TagWriter( item, oldTag, newTag, column ) );
        }

        #undef item
    }

    /*QListViewItem *below = lvi->itemBelow();
    //FIXME will result in nesting of this function?
    if ( below && below->isSelected() )
        rename( below, column );*/
}

void Playlist::showTagDialog( QPtrList<QListViewItem> items )
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
void
Playlist::addCustomColumn()
{
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
        CollectionDB::instance()->updateURL( m_item->url().path(), m_updateView );
        break;

    case false:
        m_item->setText( m_tagType, m_newTagString.isEmpty() ? " " : m_newTagString );
    }
}

#include "playlist.moc"
