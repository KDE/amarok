/* Copyright 2002-2004 Mark Kretschmann, Max Howell, Christian Muehlhaeuser
 * Copyright 2005-2006 Seb Ruiz, Mike Diehl, Ian Monroe, Gábor Lehel, Alexandre Pereira de Oliveira
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

#include <config.h>
#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "debug.h"
#include "collectiondb.h"
#include "collectionbrowser.h"
#include "columnlist.h"
#include "deletedialog.h"
#include "enginecontroller.h"
#include "expression.h"
#include "k3bexporter.h"
#include "metabundle.h"
#include "mountpointmanager.h"
#include "osd.h"
#include "playerwindow.h"
#include "playlistitem.h"
#include "playlistbrowser.h"
#include "playlistbrowseritem.h" //for stream editor dialog
#include "playlistloader.h"
#include "playlistselection.h"
#include "queuemanager.h"
#include "prettypopupmenu.h"
#include "scriptmanager.h"
#include "sliderwidget.h"
#include "starmanager.h"
#include "statusbar.h"       //for status messages
#include "tagdialog.h"
#include "threadmanager.h"
#include "xspfplaylist.h"

#include <cmath> //for pow() in playNextTrack()

#include <qbuffer.h>
#include <qclipboard.h>      //copyToClipboard(), slotMouseButtonPressed()
#include <qcolor.h>
#include <qevent.h>
#include <qfile.h>           //undo system
#include <qheader.h>         //eventFilter()
#include <qlabel.h>          //showUsageMessage()
#include <qpainter.h>
#include <qpen.h>            //slotGlowTimer()
#include <qsimplerichtext.h> //toolTipText()
#include <qsortedlist.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qvaluelist.h>      //addHybridTracks()
#include <qvaluevector.h>    //playNextTrack()
#include <qlayout.h>

#include <kaction.h>
#include <kapplication.h>
#include <kcursor.h>         //setOverrideCursor()
#include <kdialogbase.h>
#include <kglobalsettings.h> //rename()
#include <kiconeffect.h>
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

#include <cstdlib> // abs

extern "C"
{
    #if KDE_VERSION < KDE_MAKE_VERSION(3,3,91)
    #include <X11/Xlib.h>    //ControlMask in contentsDragMoveEvent()
    #endif
}

#include "playlist.h"

namespace Amarok
{
    const DynamicMode *dynamicMode() { return Playlist::instance() ? Playlist::instance()->dynamicMode() : 0; }
}

typedef PlaylistIterator MyIt;


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS TagWriter : Threaded tag-updating
//////////////////////////////////////////////////////////////////////////////////////////

class TagWriter : public ThreadManager::Job
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

QMutex* Playlist::s_dynamicADTMutex = new QMutex();
Playlist *Playlist::s_instance = 0;

Playlist::Playlist( QWidget *parent )
        : KListView( parent, "ThePlaylist" )
        , EngineObserver( EngineController::instance() )
        , m_startupTime_t( QDateTime::currentDateTime().toTime_t() )
        , m_oldestTime_t( CollectionDB::instance()->query( "SELECT MIN( createdate ) FROM statistics;" ).first().toInt() )
        , m_currentTrack( 0 )
        , m_marker( 0 )
        , m_hoveredRating( 0 )
        , m_firstColumn( 0 )
        , m_totalCount( 0 )
        , m_totalLength( 0 )
        , m_selCount( 0 )
        , m_selLength( 0 )
        , m_visCount( 0 )
        , m_visLength( 0 )
        , m_total( 0 )
        , m_itemCountDirty( false )
        , m_undoButton( 0 )
        , m_redoButton( 0 )
        , m_clearButton( 0 )
        , m_undoDir( Amarok::saveLocation( "undo/" ) )
        , m_undoCounter( 0 )
        , m_dynamicMode( 0 )
        , m_stopAfterTrack( 0 )
        , m_stopAfterMode( DoNotStop )
        , m_showHelp( true )
        , m_dynamicDirt( false )
        , m_queueDirt( false )
        , m_undoDirt( false )
        , m_insertFromADT( 0 )
        , m_itemToReallyCenter( 0 )
        , m_renameItem( 0 )
        , m_lockStack( 0 )
        , m_columnFraction( PlaylistItem::NUM_COLUMNS, 0 )
        , m_oldRandom( 0 )
        , m_oldRepeat( 0 )
        , m_playlistName( i18n( "Untitled" ) )
        , m_proposeOverwriting( false )
        , m_urlIndex( &PlaylistItem::url )

{
    s_instance = this;

    connect( CollectionDB::instance(), SIGNAL(fileMoved(const QString&,
            const QString&, const QString&)), SLOT(updateEntriesUrl(const QString&,
            const QString&, const QString&)) );
    connect( CollectionDB::instance(), SIGNAL(uniqueIdChanged(const QString&,
            const QString&, const QString&)), SLOT(updateEntriesUniqueId(const QString&,
            const QString&, const QString&)) );
    connect( CollectionDB::instance(), SIGNAL(fileDeleted(const QString&,
            const QString&)), SLOT(updateEntriesStatusDeleted(const QString&, const QString&)) );
    connect( CollectionDB::instance(), SIGNAL(fileAdded(const QString&,
            const QString&)), SLOT(updateEntriesStatusAdded(const QString&, const QString&)) );
    connect( CollectionDB::instance(), SIGNAL(filesAdded(const QMap<QString,QString>&)),
            SLOT(updateEntriesStatusAdded(const QMap<QString,QString>&)) );


    initStarPixmaps();

    EngineController* const ec = EngineController::instance();
    connect( ec, SIGNAL(orderPrevious()), SLOT(playPrevTrack()) );
    connect( ec, SIGNAL(orderNext( const bool )),     SLOT(playNextTrack( const bool )) );
    connect( ec, SIGNAL(orderCurrent()),  SLOT(playCurrentTrack()) );

    connect( this, SIGNAL( itemCountChanged( int, int, int, int, int, int ) ), ec, SLOT( playlistChanged() ) );


    setShowSortIndicator( true );
    setDropVisualizer( false );   //we handle the drawing for ourselves
    setDropVisualizerWidth( 3 );

    // FIXME: This doesn't work, and steals focus when an item is clicked twice.
    //setItemsRenameable( true );

    setAcceptDrops( true );
    setSelectionMode( QListView::Extended );
    setAllColumnsShowFocus( true );
    //setItemMargin( 1 ); //aesthetics

    setMouseTracking( true );

    #if KDE_IS_VERSION( 3, 3, 91 )
    setShadeSortColumn( true );
    #endif

    for( int i = 0; i < MetaBundle::NUM_COLUMNS; ++i )
    {
        addColumn( PlaylistItem::prettyColumnName( i ), 0 );
        switch( i )
        {
            case PlaylistItem::Title:
            case PlaylistItem::Artist:
            case PlaylistItem::Composer:
            case PlaylistItem::Year:
            case PlaylistItem::Album:
            case PlaylistItem::DiscNumber:
            case PlaylistItem::Track:
            case PlaylistItem::Bpm:
            case PlaylistItem::Genre:
            case PlaylistItem::Comment:
            case PlaylistItem::Score:
            case PlaylistItem::Rating:
                setRenameable( i, true );
                continue;
            default:
                setRenameable( i, false );
        }
    }

    setColumnWidth( PlaylistItem::Title,  200 );
    setColumnWidth( PlaylistItem::Artist, 100 );
    setColumnWidth( PlaylistItem::Album,  100 );
    setColumnWidth( PlaylistItem::Length, 80 );
    if( AmarokConfig::showMoodbar() )
        setColumnWidth( PlaylistItem::Mood, 120 );
    if( AmarokConfig::useRatings() )
        setColumnWidth( PlaylistItem::Rating, PlaylistItem::ratingColumnWidth() );

    setColumnAlignment( PlaylistItem::Length,     Qt::AlignRight  );
    setColumnAlignment( PlaylistItem::Track,      Qt::AlignCenter );
    setColumnAlignment( PlaylistItem::DiscNumber, Qt::AlignCenter );
    setColumnAlignment( PlaylistItem::Bpm,        Qt::AlignRight  );
    setColumnAlignment( PlaylistItem::Year,       Qt::AlignCenter );
    setColumnAlignment( PlaylistItem::Bitrate,    Qt::AlignCenter );
    setColumnAlignment( PlaylistItem::SampleRate, Qt::AlignCenter );
    setColumnAlignment( PlaylistItem::Filesize,   Qt::AlignCenter );
    setColumnAlignment( PlaylistItem::Score,      Qt::AlignCenter );
    setColumnAlignment( PlaylistItem::Type,       Qt::AlignCenter );
    setColumnAlignment( PlaylistItem::PlayCount,  Qt::AlignCenter );


    connect( this,     SIGNAL( doubleClicked( QListViewItem* ) ),
             this,       SLOT( doubleClicked( QListViewItem* ) ) );
    connect( this,     SIGNAL( returnPressed( QListViewItem* ) ),
             this,       SLOT( activate( QListViewItem* ) ) );
    connect( this,     SIGNAL( mouseButtonPressed( int, QListViewItem*, const QPoint&, int ) ),
             this,       SLOT( slotMouseButtonPressed( int, QListViewItem*, const QPoint&, int ) ) );
    connect( this,     SIGNAL( queueChanged(     const PLItemList &, const PLItemList & ) ),
             this,       SLOT( slotQueueChanged( const PLItemList &, const PLItemList & ) ) );
    connect( this,     SIGNAL( itemRenamed( QListViewItem*, const QString&, int ) ),
             this,       SLOT( writeTag( QListViewItem*, const QString&, int ) ) );
    connect( this,     SIGNAL( aboutToClear() ),
             this,       SLOT( saveUndoState() ) );
    connect( CollectionDB::instance(), SIGNAL( scoreChanged( const QString&, float ) ),
             this,       SLOT( scoreChanged( const QString&, float ) ) );
    connect( CollectionDB::instance(), SIGNAL( ratingChanged( const QString&, int ) ),
             this,       SLOT( ratingChanged( const QString&, int ) ) );
    connect( CollectionDB::instance(), SIGNAL( fileMoved( const QString&, const QString& ) ),
             this,       SLOT( fileMoved( const QString&, const QString& ) ) );
    connect( header(), SIGNAL( indexChange( int, int, int ) ),
             this,       SLOT( columnOrderChanged() ) ),


    connect( &Glow::timer, SIGNAL(timeout()), SLOT(slotGlowTimer()) );


    KActionCollection* const ac = Amarok::actionCollection();
    KAction *copy = KStdAction::copy( this, SLOT( copyToClipboard() ), ac, "playlist_copy" );
    KStdAction::selectAll( this, SLOT( selectAll() ), ac, "playlist_select_all" );

    m_clearButton = new KAction( i18n( "clear playlist", "&Clear" ), Amarok::icon( "playlist_clear" ), 0, this, SLOT( clear() ), ac, "playlist_clear" );
    m_undoButton  = KStdAction::undo( this, SLOT( undo() ), ac, "playlist_undo" );
    m_redoButton  = KStdAction::redo( this, SLOT( redo() ), ac, "playlist_redo" );
    m_undoButton ->setIcon( Amarok::icon( "undo" ) );
    m_redoButton ->setIcon( Amarok::icon( "redo" ) );

    new KAction( i18n( "&Repopulate" ), Amarok::icon( "playlist_refresh" ), 0, this, SLOT( repopulate() ), ac, "repopulate" );
    new KAction( i18n( "S&huffle" ), "rebuild", CTRL+Key_H, this, SLOT( shuffle() ), ac, "playlist_shuffle" );
    KAction *gotoCurrent = new KAction( i18n( "&Go To Current Track" ), Amarok::icon( "music" ), CTRL+Key_J, this, SLOT( showCurrentTrack() ), ac, "playlist_show" );
    new KAction( i18n( "&Remove Duplicate && Dead Entries" ), 0, this, SLOT( removeDuplicates() ), ac, "playlist_remove_duplicates" );
    new KAction( i18n( "&Queue Selected Tracks" ), Amarok::icon( "queue_track" ), CTRL+Key_D, this, SLOT( queueSelected() ), ac, "queue_selected" );
    KToggleAction *stopafter = new KToggleAction( i18n( "&Stop Playing After Track" ), Amarok::icon( "stop" ), CTRL+ALT+Key_V,
                            this, SLOT( toggleStopAfterCurrentItem() ), ac, "stop_after" );

    { // KAction idiocy -- shortcuts don't work until they've been plugged into a menu
        KPopupMenu asdf;

        copy->plug( &asdf );
        stopafter->plug( &asdf );
        gotoCurrent->plug( &asdf );

        copy->unplug( &asdf );
        stopafter->unplug( &asdf );
        gotoCurrent->unplug( &asdf );
    }

    //ensure we update action enabled states when repeat Playlist is toggled
    connect( ac->action( "repeat" ), SIGNAL(activated( int )), SLOT(updateNextPrev()) );
    connect( ac->action( "repeat" ), SIGNAL( activated( int ) ), SLOT( generateInfo() ) );
    connect( ac->action( "favor_tracks" ), SIGNAL( activated( int ) ), SLOT( generateInfo() ) );
    connect( ac->action( "random_mode" ), SIGNAL( activated( int ) ), SLOT( generateInfo() ) );


    // undostates are written in chronological order, so this is a clever way to get them back in the correct order :)
    QStringList undos = m_undoDir.entryList( QString("*.xml"), QDir::Files, QDir::Time );

    foreach( undos )
        m_undoList.append( m_undoDir.absPath() + '/' + (*it) );

    m_undoCounter = m_undoList.count();

    m_undoButton->setEnabled( !m_undoList.isEmpty() );
    m_redoButton->setEnabled( false );

    engineStateChanged( EngineController::engine()->state() ); //initialise state of UI
    paletteChange( palette() ); //sets up glowColors
    restoreLayout( KGlobal::config(), "PlaylistColumnsLayout" );

    // Sorting must be disabled when current.xml is being loaded. See BUG 113042
    KListView::setSorting( NO_SORT ); //use base so we don't saveUndoState() too

    setDynamicMode( 0 );

    m_smartResizing = Amarok::config( "PlaylistWindow" )->readBoolEntry( "Smart Resizing", true );

    columnOrderChanged();
    //cause the column fractions to be updated, but in a safe way, ie no specific column
    columnResizeEvent( header()->count(), 0, 0 );

    //do after you resize all the columns
    connect( header(), SIGNAL(sizeChange( int, int, int )), SLOT(columnResizeEvent( int, int, int )) );

    connect( this, SIGNAL( contentsMoving( int, int ) ), SLOT( slotContentsMoving() ) );

    connect( App::instance(), SIGNAL( useScores( bool ) ), this, SLOT( slotUseScores( bool ) ) );
    connect( App::instance(), SIGNAL( useRatings( bool ) ), this, SLOT( slotUseRatings( bool ) ) );
    connect( App::instance(), SIGNAL( moodbarPrefs(     bool, bool, int, bool ) ),
             this,            SLOT(   slotMoodbarPrefs( bool, bool, int, bool ) ) );

    Amarok::ToolTip::add( this, viewport() );

    header()->installEventFilter( this );
    renameLineEdit()->installEventFilter( this );
    setTabOrderedRenaming( false );

    m_filtertimer = new QTimer( this );
    connect( m_filtertimer, SIGNAL(timeout()), this, SLOT(setDelayedFilter()) );

    connect( MountPointManager::instance(), SIGNAL(mediumConnected( int )),
            SLOT(mediumChange( int )) );
    connect( MountPointManager::instance(), SIGNAL(mediumRemoved( int )),
            SLOT(mediumChange( int )) );

    m_clicktimer = new QTimer( this );
    connect( m_clicktimer, SIGNAL(timeout()), this, SLOT(slotSingleClick()) );
}

Playlist::~Playlist()
{
    saveLayout( KGlobal::config(), "PlaylistColumnsLayout" );

    if( AmarokConfig::savePlaylist() && m_lockStack == 0 ) saveXML( defaultPlaylistPath() );

    //speed up quit a little
    safeClear();   //our implementation is slow
    Amarok::ToolTip::remove( viewport() );
    blockSignals( true ); //might help
    s_instance = 0;
}


////////////////////////////////////////////////////////////////////////////////
/// Media Handling
////////////////////////////////////////////////////////////////////////////////

void
Playlist::mediumChange( int deviceid ) // SLOT
{
    Q_UNUSED( deviceid );

    for( QListViewItem *it = firstChild();
            it;
            it = it->nextSibling() )
    {
        PlaylistItem *p = dynamic_cast<PlaylistItem *>( it );
        if( p )
        {
            bool exist = p->exists();
            if( exist != p->checkExists() )
            {
                p->setFilestatusEnabled( p->checkExists() );
                p->update();
            }
        }
    }
}

void
Playlist::insertMedia( const KURL::List &list, int options )
{
    if( list.isEmpty() ) {
        Amarok::StatusBar::instance()->shortMessage( i18n("Attempted to insert nothing into playlist.") );
        return; // don't add empty items
    }

    const bool isPlaying   = EngineController::engine()->state() == Engine::Playing;
    if( isPlaying )
        options &= ~Playlist::StartPlay;
    bool directPlay = options & (Playlist::DirectPlay | Playlist::StartPlay);

    if( options & Replace )
       clear();
    else
        options |= Playlist::Colorize;

    PlaylistItem *after = lastItem();

    KURL::List addMe;
    QPtrList<PlaylistItem> alreadyHave;

    // Filter out duplicates
    foreachType( KURL::List, list ) {
        PlaylistItem *item = m_urlIndex.getFirst( *it );
        if ( item )
            alreadyHave.append( item );
        else 
            addMe.append( *it );
    }

    if( options & Queue )
    {
        if ( addMe.isEmpty() ) // all songs to be queued are already in the playlist
        {
            // queue all the songs
            foreachType( QPtrList<PlaylistItem>, alreadyHave )
                queue( *it, false, false );
            return;
        } else {
            // We add the track after the last track on queue, or after current if the queue is empty
            after = m_nextTracks.isEmpty() ? currentTrack() : m_nextTracks.getLast();
            // If there's no tracks on the queue, and there's no current track, fall back to the last item
            if ( !after )
                after = lastItem();
        }
    }
    else if( options & Unique ) {
        int alreadyOnPlaylist = alreadyHave.count();
        if ( alreadyOnPlaylist )
        {
            if (directPlay) activate( alreadyHave.getFirst() );
            Amarok::StatusBar::instance()->shortMessage(
                i18n("One track was already in the playlist, so it was not added.",
                     "%n tracks were already in the playlist, so they were not added.",
                     alreadyOnPlaylist ) );
        }
    }

    if( options & Unique || options & Queue )
        insertMediaInternal( addMe, after, options );
    else
        insertMediaInternal(  list, after, options );
}

void
Playlist::insertMediaInternal( const KURL::List &list, PlaylistItem *after, int options )
{
    if ( !list.isEmpty() ) {
        setSorting( NO_SORT );

        // prevent association with something that is about to be deleted
        // TODO improve the playlist with a list of items that are volatile or something
        while( after && after->url().isEmpty() )
            after = static_cast<PlaylistItem*>( after->itemAbove() );

        ThreadManager::instance()->queueJob( new UrlLoader( list, after, options ) );
        ScriptManager::instance()->notifyPlaylistChange("changed");
    }
}

void
Playlist::insertMediaSql( const QString& sql, int options )
{
    const bool isPlaying   = EngineController::engine()->state() == Engine::Playing;
    if( isPlaying )
        options &= ~Playlist::StartPlay;

    // TODO Implement more options
    PlaylistItem *after = 0;

    if ( options & Replace )
        clear();
    if ( options & Append )
        after = lastItem();

    setSorting( NO_SORT );
    ThreadManager::instance()->queueJob( new SqlLoader( sql, after, options ) );
    ScriptManager::instance()->notifyPlaylistChange("changed");
}

void
Playlist::addDynamicModeTracks( uint songCount )
{
    if( songCount < 1 ) return;

    int  currentPos = 0;
    for( MyIt it( this, MyIt::Visible ); *it; ++it )
    {
        if( m_currentTrack && *it == m_currentTrack )
            break;
        else if( !m_currentTrack && (*it)->isDynamicEnabled() )
            break;

        ++currentPos;
    }
    currentPos++;

    int required  = currentPos + dynamicMode()->upcomingCount(); // currentPos handles currentTrack
    int remainder = totalTrackCount();

    if( required > remainder )
        songCount = required - remainder;

    DynamicMode *m = modifyDynamicMode();
    KURL::List tracksToInsert = m->retrieveTracks( songCount );
    Playlist::instance()->finishedModifying( m );

    insertMedia( tracksToInsert, Playlist::Unique );
}


/**
 *  @param songCount : Number of tracks to be shown after the current track
 */

void
Playlist::adjustDynamicUpcoming( bool saveUndo )
{
    /**
     *  If m_currentTrack exists, we iterate until we find it
     *  Else, we iterate until we find an item which is enabled
     **/
    MyIt it( this, MyIt::Visible ); //Notice we'll use this up to the end of the function!
    //Skip previously played
    for( ; *it; ++it )
    {
        if( m_currentTrack && *it == m_currentTrack )
            break;
        else if( !m_currentTrack && (*it)->isDynamicEnabled() )
            break;
    }
    //Skip current
    if( m_currentTrack )
        ++it;

    int  x = 0;
    for ( ; *it && x < dynamicMode()->upcomingCount() ; ++it, ++x );

    if ( x < dynamicMode()->upcomingCount() )
    {
        addDynamicModeTracks( dynamicMode()->upcomingCount() - x );
        ScriptManager::instance()->notifyPlaylistChange("changed");
    }

    if( saveUndo )
        saveUndoState();
}

/**
 *  @param songCount : Number of tracks to be shown before the current track
  */

void
Playlist::adjustDynamicPrevious( uint songCount, bool saveUndo )
{
    int current = currentTrackIndex();
    int x = current - songCount;

    QPtrList<QListViewItem> list;
    int y=0;
    for( QListViewItemIterator it( firstChild() ); y < x ; list.prepend( *it ), ++it, y++ );

    if( list.isEmpty() ) return;
    if ( saveUndo )
        saveUndoState();

    //remove the items
    for( QListViewItem *item = list.first(); item; item = list.next() )
    {
        removeItem( static_cast<PlaylistItem*>( item ) );
        delete item;
    }
    ScriptManager::instance()->notifyPlaylistChange("changed");
}

void
Playlist::setDynamicHistory( bool enable /*false*/ )
{
    if( !m_currentTrack )
        return;

    for( PlaylistIterator it( this, PlaylistIterator::All ) ; *it ; ++it )
    {
        if( *it == m_currentTrack )          break;

        //avoid repainting if we can.
        if( (*it)->isDynamicEnabled() == enable )
        {
            (*it)->setDynamicEnabled( !enable );
            (*it)->update();
        }
    }
}

QString
Playlist::defaultPlaylistPath() //static
{
    return Amarok::saveLocation() + "current.xml";
}

void
Playlist::restoreSession()
{
    KURL url;

    if ( Amarok::config()->readBoolEntry( "First 1.4 Run", true ) ) {
        // On first startup of 1.4, we load a special playlist with an intro track
        url.setPath( locate( "data", "amarok/data/firstrun.m3u" ) );
        Amarok::config()->writeEntry( "First 1.4 Run", false );
    }
    else
        url.setPath( Amarok::saveLocation() + "current.xml" );

    // check it exists, because on the first ever run it doesn't and
    // it looks bad to show "some URLs were not suitable.." on the
    // first ever-run
    if( QFile::exists( url.path() ) )
    {
        ThreadManager::instance()->queueJob( new UrlLoader( url, 0, 0 ) );
    }
}

/*
    The following two functions (saveLayout(), restoreLayout()), taken from klistview.cpp, are largely
    Copyright (C) 2000 Reginald Stadlbauer <reggie@kde.org>
    Copyright (C) 2000,2003 Charles Samuels <charles@kde.org>
    Copyright (C) 2000 Peter Putzer
*/
void Playlist::saveLayout(KConfig *config, const QString &group) const
{
  KConfigGroupSaver saver(config, group);
  QStringList names, widths, order;

  const int colCount = columns();
  QHeader* const thisHeader = header();
  for (int i = 0; i < colCount; ++i)
  {
    names << PlaylistItem::exactColumnName(i);
    widths << QString::number(columnWidth(i));
    order << QString::number(thisHeader->mapToIndex(i));
  }
  config->writeEntry("ColumnsVersion", 1);
  config->writeEntry("ColumnNames", names);
  config->writeEntry("ColumnWidths", widths);
  config->writeEntry("ColumnOrder", order);
  config->writeEntry("SortColumn", columnSorted());
  config->writeEntry("SortAscending", ascendingSort());
}

void Playlist::restoreLayout(KConfig *config, const QString &group)
{
  KConfigGroupSaver saver(config, group);
  int version = config->readNumEntry("ColumnsVersion", 0);

  QValueList<int> iorder; //internal ordering
  if( version )
  {
    QStringList names = config->readListEntry("ColumnNames");
    for( int i = 0, n = names.count(); i < n; ++i )
    {
        bool found = false;
        for( int ii = i; ii < PlaylistItem::NUM_COLUMNS; ++ii ) //most likely, it's where we left it
        {
            if( names[i] == PlaylistItem::exactColumnName(ii) )
            {
                iorder.append(ii);
                found = true;
                break;
            }
        }
        if( !found )
        {
            for( int ii = 0; ii < i; ++ii ) //but maybe it's not
                if( names[i] == PlaylistItem::exactColumnName(ii) )
                {
                    iorder.append(ii);
                    found = true;
                    break;
                }
        }
        if( !found )
            return; //oops? -- revert to the default.
    }
  }
  else
  {
    int oldorder[] = { 0, 1, 2, 5, 4, 9, 8, 7, 10, 12, 13, 15, 16, 11, 17, 18, 19, 3, 6, 20 };
    for( int i = 0; i != 20; ++i )
        iorder.append(oldorder[i]);
  }


  QStringList cols = config->readListEntry("ColumnWidths");
  int i = 0;
  { // scope the iterators
    QStringList::ConstIterator it = cols.constBegin();
    const QStringList::ConstIterator itEnd = cols.constEnd();
    for (; it != itEnd; ++it)
      setColumnWidth(iorder[i++], (*it).toInt());
  }


  // move sections in the correct sequence: from lowest to highest index position
  // otherwise we move a section from an index, which modifies
  // all index numbers to the right of the moved one
  cols = config->readListEntry("ColumnOrder");
  const int colCount = columns();
  for (i = 0; i < colCount; ++i)   // final index positions from lowest to highest
  {
    QStringList::ConstIterator it = cols.constBegin();
    const QStringList::ConstIterator itEnd = cols.constEnd();

    int section = 0;
    for (; (it != itEnd) && (iorder[(*it).toInt()] != i); ++it, ++section) ;

    if ( it != itEnd ) {
      // found the section to move to position i
      header()->moveSection(iorder[section], i);
    }
  }

  if ( config->hasKey("SortColumn") )
  {
    const int sort = config->readNumEntry("SortColumn");
    if( sort >= 0 && uint(sort) < iorder.count() )
        setSorting(iorder[config->readNumEntry("SortColumn")], config->readBoolEntry("SortAscending", true));
  }

  if( !AmarokConfig::useScores() )
    hideColumn( PlaylistItem::Score );
  if( !AmarokConfig::useRatings() )
    hideColumn( PlaylistItem::Rating );
  if( !AmarokConfig::showMoodbar() )
    hideColumn( PlaylistItem::Mood );
}

void
Playlist::addToUniqueMap( const QString uniqueid, PlaylistItem* item )
{
    QPtrList<PlaylistItem> *list;
    if( m_uniqueMap.contains( uniqueid ) )
        list = m_uniqueMap[uniqueid];
    else
        list = new QPtrList<PlaylistItem>();
    list->append( item );
    if( !m_uniqueMap.contains( uniqueid ) )
        m_uniqueMap[uniqueid] = list;
}

void
Playlist::removeFromUniqueMap( const QString uniqueid, PlaylistItem* item )
{
    if( !m_uniqueMap.contains( uniqueid ) )
        return;

    QPtrList<PlaylistItem> *list;
    list = m_uniqueMap[uniqueid];

    list->remove( item ); //don't care about return value

    if( list->isEmpty() )
    {
        delete list;
        m_uniqueMap.remove( uniqueid );
    }
}

void
Playlist::updateEntriesUrl( const QString &oldUrl, const QString &newUrl, const QString &uniqueid )
{
    // Make sure the MoodServer gets this signal first!
    MoodServer::instance()->slotFileMoved( oldUrl, newUrl );

    QPtrList<PlaylistItem> *list;
    if( m_uniqueMap.contains( uniqueid ) )
    {
        list = m_uniqueMap[uniqueid];
        PlaylistItem *item;
        for( item = list->first(); item; item = list->next() )
        {
            item->setUrl( KURL( newUrl ) );
            item->setFilestatusEnabled( item->checkExists() );
        }
    }
}

void
Playlist::updateEntriesUniqueId( const QString &/*url*/, const QString &oldid, const QString &newid )
{
    QPtrList<PlaylistItem> *list, *oldlist;
    if( m_uniqueMap.contains( oldid ) )
    {
        list = m_uniqueMap[oldid];
        m_uniqueMap.remove( oldid );
        PlaylistItem *item;
        for( item = list->first(); item; item = list->next() )
        {
            item->setUniqueId( newid );
            item->readTags();
        }
        if( !m_uniqueMap.contains( newid ) )
            m_uniqueMap[newid] = list;
        else
        {
            oldlist = m_uniqueMap[newid];
            for( item = list->first(); item; item = list->next() )
                oldlist->append( item );
            delete list;
        }
    }
}

void
Playlist::updateEntriesStatusDeleted( const QString &/*absPath*/, const QString &uniqueid )
{
    QPtrList<PlaylistItem> *list;
    if( m_uniqueMap.contains( uniqueid ) )
    {
        list = m_uniqueMap[uniqueid];
        PlaylistItem *item;
        for( item = list->first(); item; item = list->next() )
            item->setFilestatusEnabled( false );
    }
}

void
Playlist::updateEntriesStatusAdded( const QString &absPath, const QString &uniqueid )
{
    QPtrList<PlaylistItem> *list;
    if( m_uniqueMap.contains( uniqueid ) )
    {
        list = m_uniqueMap[uniqueid];
        if( !list )
            return;
        PlaylistItem *item;
        for( item = list->first(); item; item = list->next() )
        {
            if( absPath != item->url().path() )
                item->setPath( absPath ); //in case the UID was the same, but the path has changed
            item->setFilestatusEnabled( true );
        }
    }
}

void
Playlist::updateEntriesStatusAdded( const QMap<QString,QString> &map )
{
    QMap<QString,QPtrList<PlaylistItem>*> uniquecopy( m_uniqueMap );

    QMap<QString,QPtrList<PlaylistItem>*>::Iterator it;
    for( it = uniquecopy.begin(); it != uniquecopy.end(); ++it )
    {
        if( map.contains( it.key()  ))
        {
            updateEntriesStatusAdded( map[it.key()], it.key() );
            uniquecopy.remove( it );
        }
    }

    for( it = uniquecopy.begin(); it != uniquecopy.end(); ++it )
        updateEntriesStatusDeleted( QString::null, it.key() );
}

////////////////////////////////////////////////////////////////////////////////
/// Current Track Handling
////////////////////////////////////////////////////////////////////////////////

void
Playlist::playNextTrack( bool forceNext )
{
    PlaylistItem *item = currentTrack();

    if( !m_visCount || stopAfterMode() == StopAfterCurrent )
    {
        if( dynamicMode() && m_visCount )
        {
            item->setDynamicEnabled( false );
            advanceDynamicTrack();
            m_dynamicDirt = false;
        }

        EngineController::instance()->stop();
        setStopAfterMode( DoNotStop );

        if( !AmarokConfig::randomMode() ) {
            item = MyIt::nextVisible( item );
            while( item && ( !checkFileStatus( item ) || !item->exists() ) )
                item = MyIt::nextVisible( item );
            setCurrentTrack( item );
        }

        return;
    }

    if( !Amarok::repeatTrack() || forceNext )
    {
        if( !m_nextTracks.isEmpty() )
        {
            item = m_nextTracks.first();
            m_nextTracks.remove();
            if ( dynamicMode() )
                // move queued track to the top of the playlist, to prevent it from being played twice
                // this is done automatically by most queue changing functions, but not if the user manually moves the track
                moveItem( item, 0, m_currentTrack );
            emit queueChanged( PLItemList(), PLItemList( item ) );
        }

        else if( Amarok::entireAlbums() && m_currentTrack && m_currentTrack->nextInAlbum() )
            item = m_currentTrack->nextInAlbum();

        else if( Amarok::repeatAlbum() &&
                 repeatAlbumTrackCount() && ( repeatAlbumTrackCount() > 1 || !forceNext ) )
            item = m_currentTrack->m_album->tracks.getFirst();

        else if( AmarokConfig::randomMode() )
        {
            QValueVector<PlaylistItem*> tracks;

            //make a list of everything we can play
            if( Amarok::randomAlbums() ) // add the first visible track from every unplayed album
            {
                for( ArtistAlbumMap::const_iterator it = m_albums.constBegin(), end = m_albums.constEnd(); it != end;   ++it )
                    for( AlbumMap::const_iterator it2 = (*it).constBegin(), end2 = (*it).constEnd(); it2 != end2; ++it2 )
                        if( m_prevAlbums.findRef( *it2 ) == -1 ) {
                            if ( (*it2)->tracks.getFirst() )
                                tracks.append( (*it2)->tracks.getFirst() );
                        }
            }
            else
                for( MyIt it( this ); *it; ++it )
                    if ( !m_prevTracks.containsRef( *it ) && checkFileStatus( *it ) && (*it)->exists() )
                        tracks.push_back( *it );
            if( tracks.isEmpty() )
            {
                //we have played everything

                item = 0;

                if( Amarok::randomAlbums() )
                {
                    if ( m_prevAlbums.count() <= 8 ) {
                        m_prevAlbums.first();
                        while( m_prevAlbums.count() )
                            removeFromPreviousAlbums();

                        if( m_currentTrack )
                        {
                            // don't add it to previous albums if we only have one album in the playlist
                            // would loop infinitely otherwise
                            QPtrList<PlaylistAlbum> albums;
                            for( PlaylistIterator it( this, PlaylistIterator::Visible ); *it && albums.count() <= 1; ++it )
                                if( albums.findRef( (*it)->m_album ) == -1 )
                                    albums.append( (*it)->m_album );

                            if ( albums.count() > 1 )
                                appendToPreviousAlbums( m_currentTrack->m_album );
                        }
                    }
                    else {
                        m_prevAlbums.first(); //set's current item to first item

                        //keep 80 tracks in the previous list so item time user pushes play
                        //we don't risk playing anything too recent
                        while( m_prevAlbums.count() > 8 )
                            removeFromPreviousAlbums(); //removes current item
                    }
                }

                else
                {
                    if ( m_prevTracks.count() <= 80 ) {
                        m_prevTracks.first();
                        while( m_prevTracks.count() )
                            removeFromPreviousTracks();

                        if( m_currentTrack )
                        {
                            // don't add it to previous tracks if we only have one file in the playlist
                            // would loop infinitely otherwise
                            int count = 0;
                            for( PlaylistIterator it( this, PlaylistIterator::Visible ); *it && count <= 1; ++it )
                                ++count;

                            if ( count > 1 )
                                appendToPreviousTracks( m_currentTrack );
                        }
                    }
                    else {
                        m_prevTracks.first(); //set's current item to first item

                        //keep 80 tracks in the previous list so item time user pushes play
                        //we don't risk playing anything too recent
                        while( m_prevTracks.count() > 80 )
                            removeFromPreviousTracks(); //removes current item
                    }
                }

                if( Amarok::repeatPlaylist() )
                {
                    playNextTrack();
                    return;
                }
                //else we stop via activate( 0 ) below
            }
            else
            {
                if( Amarok::favorNone() )
                    item = tracks.at( KApplication::random() % tracks.count() ); //is O(1)
                else
                {
                    const uint currenttime_t = QDateTime::currentDateTime().toTime_t();
                    QValueVector<int> weights( tracks.size() );
                    Q_INT64 total = m_total;
                    if( Amarok::randomAlbums() )
                    {
                        for( int i = 0, n = tracks.count(); i < n; ++i )
                        {
                            weights[i] = tracks.at( i )->m_album->total;
                            if( Amarok::favorLastPlay() )
                            {
                                const int inc = int( float( ( currenttime_t - m_startupTime_t )
                                                            * tracks.at( i )->m_album->tracks.count() + 0.5 )
                                                     / tracks.at( i )->m_album->tracks.count() );
                                weights[i] += inc;
                                total += inc;
                            }
                        }
                    }
                    else
                    {
                        for( int i = 0, n = tracks.count(); i < n; ++i )
                        {
                            weights[i] = tracks.at( i )->totalIncrementAmount();
                            if( Amarok::favorLastPlay() )
                                weights[i] += currenttime_t - m_startupTime_t;
                        }
                        if( Amarok::favorLastPlay() )
                            total += ( currenttime_t - m_startupTime_t ) * weights.count();
                    }

                    Q_INT64 random;
                    if( Amarok::favorLastPlay() ) //really big huge numbers
                    {
                        Q_INT64 r = Q_INT64( ( KApplication::random() / pow( 2, sizeof( int ) * 8 ) )
                                                                      * pow( 2, 64 ) );
                        random = r % total;
                    }
                    else
                        random = KApplication::random() % total;
                    int i = 0;
                    for( int n = tracks.count(); i < n && random >= 0; ++i )
                        random -= weights.at( i );
                    item = tracks.at( i-1 );
                }
            }
        }
        else if( item )
        {
            item = MyIt::nextVisible( item );
            while( item && ( !checkFileStatus( item ) || !item->exists() ) )
                item = MyIt::nextVisible( item );
        }
        else
        {
            item = *MyIt( this ); //ie. first visible item
            while( item && ( !checkFileStatus( item ) || !item->exists() ) )
                item = item->nextSibling();
        }


        if ( dynamicMode() && item != firstChild() )
        {
            if( currentTrack() )
                currentTrack()->setDynamicEnabled( false );
            advanceDynamicTrack();
        }

        if ( !item && Amarok::repeatPlaylist() )
            item = *MyIt( this ); //ie. first visible item
    }


    if ( EngineController::engine()->loaded() )
        activate( item );
    else
        setCurrentTrack( item );
}

//This is called before setCurrentItem( item );
void
Playlist::advanceDynamicTrack()
{
    int x = currentTrackIndex();
    bool didDelete = false;
    if( dynamicMode()->cycleTracks() )
    {
        if( x >= dynamicMode()->previousCount() )
        {
            PlaylistItem *first = firstChild();
            removeItem( first );
            delete first;
            didDelete = true;
        }
    }

    const int upcomingTracks = childCount() - x - 1;

    // Just starting to play from stopped, don't append something needlessely
    // or, we have more than enough items in the queue.
    bool dontAppend = ( !didDelete &&
                        ( EngineController::instance()->engine()->state() == Engine::Empty ) ) ||
                        upcomingTracks > dynamicMode()->upcomingCount();

    //keep upcomingTracks requirement, this seems to break StopAfterCurrent
    if( !dontAppend && stopAfterMode() != StopAfterCurrent )
    {
        s_dynamicADTMutex->lock();
        m_insertFromADT++;
        s_dynamicADTMutex->unlock();
        addDynamicModeTracks( 1 );
    }
    m_dynamicDirt = true;
}

void
Playlist::playPrevTrack()
{
    PlaylistItem *item = currentTrack();

    if( Amarok::entireAlbums() )
    {
        item = 0;
        if( m_currentTrack )
        {
            item = m_currentTrack->prevInAlbum();
            if( !item && Amarok::repeatAlbum() && m_currentTrack->m_album->tracks.count() )
                item = m_currentTrack->m_album->tracks.getLast();
        }
        if( !item )
        {
            PlaylistAlbum* a = m_prevAlbums.last();
            while( a && !a->tracks.count() )
            {
                removeFromPreviousAlbums();
                a = m_prevAlbums.last();
            }
            if( a )
            {
                item = a->tracks.getLast();
                removeFromPreviousAlbums();
            }
        }
        if( !item )
        {
            item = *static_cast<MyIt&>(--MyIt( item ));
            while( item && !checkFileStatus( item ) )
                item = *static_cast<MyIt&>(--MyIt( item ));
        }
    }
    else
    {
        if( dynamicMode() )
        {
        }
        else if( !AmarokConfig::randomMode() || m_prevTracks.count() <= 1 )
        {
            if( item )
            {
                item = MyIt::prevVisible( item );
                while( item && ( !checkFileStatus( item ) || !item->isEnabled() ) )
                    item = MyIt::prevVisible( item );
            }
            else
            {
                item = *MyIt( this ); //ie. first visible item
                while( item && ( !checkFileStatus( item ) || !item->isEnabled() ) )
                    item = item->nextSibling();
            }
        }
        else {
            // if enough songs in buffer, jump to the previous one
            m_prevTracks.last();
            removeFromPreviousTracks(); //remove the track playing now
            item = m_prevTracks.last();

            // we need to remove this item now, since it will be added in activate() again
            removeFromPreviousTracks();
        }
    }

    if ( !item && Amarok::repeatPlaylist() )
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
        playNextTrack( Amarok::repeatTrack() );

    //we must do this even if the above is correct
    //since the engine is not loaded the first time the user presses play
    //then calling the next() function wont play it
    activate( currentTrack() );
}

void
Playlist::setSelectedRatings( int rating )
{
    if( !m_selCount && currentItem() && currentItem()->isVisible() )
        CollectionDB::instance()->setSongRating( currentItem()->url().path(), rating, true );

    else
        for( MyIt it( this, MyIt::Selected ); *it; ++it )
            CollectionDB::instance()->setSongRating( (*it)->url().path(), rating, true );
}

void
Playlist::queueSelected()
{
    PLItemList in, out;
    QPtrList<QListViewItem> dynamicList;

    for( MyIt it( this, MyIt::Selected ); *it; ++it )
    {
        // Dequeuing selection with dynamic doesn't work due to the moving of the track after the last queued
        if( dynamicMode() )
        {
            ( !m_nextTracks.containsRef( *it ) ? in : out ).append( *it );
            dynamicList.append( *it );
        }
        else
        {
            queue( *it, true );
            ( m_nextTracks.containsRef( *it ) ? in : out ).append( *it );
        }

    }

    if( dynamicMode() )
    {
        QListViewItem *item = dynamicList.first();
        if( m_nextTracks.containsRef( static_cast<PlaylistItem*>(item) ) )
        {
            for( item = dynamicList.last(); item; item = dynamicList.prev() )
                queue( item, true );
        }
        else
        {
            for( ; item; item = dynamicList.next() )
                queue( item, true );
        }
    }

    emit queueChanged( in, out );
}

void
Playlist::queue( QListViewItem *item, bool multi, bool invertQueue )
{
    #define item static_cast<PlaylistItem*>(item)

    const int  queueIndex  = m_nextTracks.findRef( item );
    const bool isQueued    = queueIndex != -1;

    if( isQueued )
    {
        if( invertQueue )
        {
            //remove the item, this is better way than remove( item )
            m_nextTracks.remove( queueIndex ); //sets current() to next item

            if( dynamicMode() ) // we move the item after the last queued item to preserve the ordered 'queue'.
            {
                PlaylistItem *after = m_nextTracks.last();

                if( after )
                    moveItem( item, 0, after );
            }
        }
    }
    else if( !dynamicMode() )
        m_nextTracks.append( item );

    else // Dynamic mode
    {
        PlaylistItem *after;
        m_nextTracks.isEmpty() ?
            after = m_currentTrack :
            after = m_nextTracks.last();

        if( !after )
        {
            after = firstChild();
            while( after && !after->isDynamicEnabled() )
            {
                if( after->nextSibling()->isDynamicEnabled() )
                    break;
                after = after->nextSibling();
            }
        }

        if( item->isDynamicEnabled() && item != m_currentTrack )
        {
            this->moveItem( item, 0, after );
            m_nextTracks.append( item );
        }
        else
        {
            /// we do the actual queuing through customEvent, since insertMedia is threaded
            m_queueDirt = true;
            insertMediaInternal( item->url(), after );
        }
    }

    if( !multi )
    {
        if( isQueued ) //no longer
        {
            if( invertQueue )
                emit queueChanged( PLItemList(), PLItemList( item ) );
        }
        else
            emit queueChanged( PLItemList( item ), PLItemList() );
    }

    #undef item
}

void
Playlist::sortQueuedItems() // used by dynamic mode
{
    PlaylistItem *last = m_currentTrack;
    for( PlaylistItem *item = m_nextTracks.first(); item; item = m_nextTracks.next() )
    {
        if( item->itemAbove() != last )
            item->moveItem( last );
        last = item;
    }
}

void Playlist::setStopAfterCurrent( bool on )
{
    PlaylistItem *prev_stopafter = m_stopAfterTrack;

    if( on ) {
        setStopAfterItem( m_currentTrack );
    }
    else {
        setStopAfterMode( DoNotStop );
    }

    if( m_stopAfterTrack )
        m_stopAfterTrack->update();
    if( prev_stopafter )
        prev_stopafter->update();
}

void Playlist::setStopAfterItem( PlaylistItem *item )
{
    if( !item ) {
        setStopAfterMode( DoNotStop );
        return;
    }
    else if( item == m_currentTrack )
        setStopAfterMode( StopAfterCurrent );
    else if( item == m_nextTracks.getLast() )
        setStopAfterMode( StopAfterQueue );
    else
        setStopAfterMode( StopAfterQueue );
    m_stopAfterTrack = item;
}

void Playlist::toggleStopAfterCurrentItem()
{
    PlaylistItem *item = currentItem();
    if( !item && m_selCount == 1 )
        item = *MyIt( this, MyIt::Visible | MyIt::Selected );
    if( !item )
        return;

    PlaylistItem *prev_stopafter = m_stopAfterTrack;
    if( m_stopAfterTrack == item ) {
            m_stopAfterTrack = 0;
            setStopAfterMode( DoNotStop );
    }
    else
    {
        setStopAfterItem( item );
        item->setSelected( false );
        item->update();
    }

    if( prev_stopafter )
        prev_stopafter->update();
}

void Playlist::toggleStopAfterCurrentTrack()
{
    PlaylistItem *item = currentTrack();
    if( !item )
        return;

    PlaylistItem *prev_stopafter = m_stopAfterTrack;
    if( m_stopAfterTrack == item ) {
        setStopAfterMode( DoNotStop );
        Amarok::OSD::instance()->OSDWidget::show( i18n("Stop Playing After Track: Off") );
    }
    else
    {
        setStopAfterItem( item );
        item->setSelected( false );
        item->update();
        Amarok::OSD::instance()->OSDWidget::show( i18n("Stop Playing After Track: On") );
    }

    if( prev_stopafter )
        prev_stopafter->update();
}

void Playlist::setStopAfterMode( int mode )
{
    PlaylistItem *prevStopAfter = m_stopAfterTrack;
    m_stopAfterMode = mode;
    switch( mode )
    {
        case DoNotStop:
            m_stopAfterTrack = 0;
            break;
        case StopAfterCurrent:
            m_stopAfterTrack = m_currentTrack;
            break;
        case StopAfterQueue:
            m_stopAfterTrack = m_nextTracks.count() ? m_nextTracks.getLast() : m_currentTrack;
            break;
    }

    if( prevStopAfter )
        prevStopAfter->update();
    if( m_stopAfterTrack )
        m_stopAfterTrack->update();
}

int Playlist::stopAfterMode()
{
    if ( m_stopAfterMode != DoNotStop
         && m_stopAfterTrack && m_stopAfterTrack == m_currentTrack ) {
         m_stopAfterMode = StopAfterCurrent;
    }

    return m_stopAfterMode;
}

void Playlist::generateInfo()
{
    m_albums.clear();
    if( Amarok::entireAlbums() )
        for( MyIt it( this, MyIt::All ); *it; ++it )
            (*it)->refAlbum();
    m_total = 0;
    if( Amarok::entireAlbums() || AmarokConfig::favorTracks() )
        for( MyIt it( this, MyIt::Visible ); *it; ++it )
            (*it)->incrementTotals();
}

void Playlist::doubleClicked( QListViewItem *item )
{
    /* We have to check if the item exists before calling activate, otherwise clicking on an empty
    playlist space would stop playing (check BR #105106)*/
    if( item && m_hoveredRating != item )
        activate( item );
}

void
Playlist::slotCountChanged()
{
    if( m_itemCountDirty )
        emit itemCountChanged( totalTrackCount(), m_totalLength,
                               m_visCount,        m_visLength,
                               m_selCount,        m_selLength    );

    m_itemCountDirty = false;
}

bool
Playlist::checkFileStatus( PlaylistItem * item )
{
    //DEBUG_BLOCK
    //debug() << "uniqueid of item = " << item->uniqueId() << ", url = " << item->url().path() << endl;
    if( !item->checkExists() )
    {
        //debug() << "not found, finding new url" << endl;
        QString path = QString::null;
        if( !item->uniqueId().isEmpty() )
        {
            path = CollectionDB::instance()->urlFromUniqueId( item->uniqueId() );
            //debug() << "found path = " << path << endl;
        }
        else
        {
            //debug() << "Setting uniqueid of item and trying again" << endl;
            item->setUniqueId();
            if( !item->uniqueId().isEmpty() )
                path = CollectionDB::instance()->urlFromUniqueId( item->uniqueId() );
        }
        if( !path.isEmpty() )
        {
            item->setUrl( KURL( path ) );
            if( item->checkExists() )
                item->setFilestatusEnabled( true );
            else
                item->setFilestatusEnabled( false );
        }
        else
            item->setFilestatusEnabled( false );
    }
    else if( !item->isFilestatusEnabled() )
        item->setFilestatusEnabled( true );

    bool returnValue = item->isFilestatusEnabled();

    return returnValue;
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
        Amarok::OSD::instance()->OSDWidget::show( i18n("Playlist finished"),
                                            QImage( KIconLoader().iconPath( "amarok", -KIcon::SizeHuge ) ) );
        return;
    }

    #define item static_cast<PlaylistItem*>(item)

    if ( !checkFileStatus( item ) )
    {
        Amarok::StatusBar::instance()->shortMessage( i18n("Local file does not exist.") );
        return;
    }

    if( dynamicMode() && !Amarok::repeatTrack() )
    {
        if( m_currentTrack && item->isDynamicEnabled() )
        {
            if( item != m_currentTrack )
                this->moveItem( item, 0, m_currentTrack );
        }
        else
        {
            MyIt it( this, MyIt::Visible );
            bool hasHistory = false;
            if ( *it && !(*it)->isDynamicEnabled() )
            {
                hasHistory = true;
                for(  ; *it && !(*it)->isDynamicEnabled() ; ++it );
            }

            if( item->isDynamicEnabled() )
            {
                hasHistory ?
                    this->moveItem( item, 0, *it ) :
                    this->moveItem( item, 0,   0 );
            }
            else // !item->isDynamicEnabled()
            {
                hasHistory ?
                    insertMediaInternal( item->url(), *it ):
                    insertMediaInternal( item->url(), 0 );
                m_dynamicDirt = true;
                return;
            }

        }
        if( !m_dynamicDirt && m_currentTrack && m_currentTrack != item )
        {
            m_currentTrack->setDynamicEnabled( false );
            advanceDynamicTrack();
        }
    }

    if( Amarok::entireAlbums() )
    {
        if( !item->nextInAlbum() )
            appendToPreviousAlbums( item->m_album );
    }
    else
        appendToPreviousTracks( item );

    //if we are playing something from the next tracks
    //list, remove it from the list
    if( m_nextTracks.removeRef( item ) )
        emit queueChanged( PLItemList(), PLItemList( item ) );

    //looks bad painting selected and glowing
    //only do when user explicitly activates an item though
    item->setSelected( false );

    setCurrentTrack( item );

    m_dynamicDirt = false;

    //use PlaylistItem::MetaBundle as it also updates the audioProps
    EngineController::instance()->play( *item );
    #undef item
}

QPair<QString, QRect> Playlist::toolTipText( QWidget*, const QPoint &pos ) const
{
    PlaylistItem *item = static_cast<PlaylistItem*>( itemAt( pos ) );
    if( !item )
        return QPair<QString, QRect>( QString::null, QRect() );

    const QPoint contentsPos = viewportToContents( pos );
    const int col = header()->sectionAt( contentsPos.x() );

    if( item == m_renameItem && col == m_renameColumn )
        return QPair<QString, QRect>( QString::null, QRect() );

    QString text;
    if( col == PlaylistItem::Rating )
        text = item->ratingDescription( item->rating() );
    else
        text = item->text( col );

    QRect irect = itemRect( item );
    const int headerPos = header()->sectionPos( col );
    irect.setLeft( headerPos - 1 );
    irect.setRight( headerPos + header()->sectionSize( col ) );

    static QFont f;
    static int minbearing = 1337 + 666; //can be 0 or negative, 2003 is less likely
    if( minbearing == 2003 || f != font() )
    {
        f = font(); //getting your bearings can be expensive, so we cache them
        minbearing = fontMetrics().minLeftBearing() + fontMetrics().minRightBearing();
    }

    int itemWidth = irect.width() - itemMargin() * 2 + minbearing - 2;
    if( item->pixmap( col ) )
        itemWidth -= item->pixmap( col )->width();
    if( item == m_currentTrack )
    {
        if( col == m_firstColumn )
            itemWidth -= 12;
        if( col == mapToLogicalColumn( numVisibleColumns() - 1 ) )
            itemWidth -= 12;
    }

    if( col != PlaylistItem::Rating && fontMetrics().width( text ) <= itemWidth )
        return QPair<QString, QRect>( QString::null, QRect() );

    QRect globalRect( viewport()->mapToGlobal( irect.topLeft() ), irect.size() );
    QSimpleRichText t( text, font() );
    int dright = QApplication::desktop()->screenGeometry( qscrollview() ).topRight().x();
    t.setWidth( dright - globalRect.left() );
    if( col == PlaylistItem::Rating )
        globalRect.setRight( kMin( dright, kMax( globalRect.left() + t.widthUsed(), globalRect.left() + ( StarManager::instance()->getGreyStar()->width() + 1 ) * ( ( item->rating() + 1 ) / 2 ) ) ) );
    else
        globalRect.setRight( kMin( globalRect.left() + t.widthUsed(), dright ) );
    globalRect.setBottom( globalRect.top() + kMax( irect.height(), t.height() ) - 1 );

    if( ( col == PlaylistItem::Rating && PlaylistItem::ratingAtPoint( contentsPos.x() ) <= item->rating() + 1 ) ||
        ( col != PlaylistItem::Rating ) )
    {
        text = text.replace( "&", "&amp;" ).replace( "<", "&lt;" ).replace( ">", "&gt;" );
        if( item->isCurrent() )
        {
            text = QString("<i>%1</i>").arg( text );
            Amarok::ToolTip::s_hack = 1; //HACK for precise positioning
        }
        return QPair<QString, QRect>( text, globalRect );
    }

    return QPair<QString, QRect>( QString::null, QRect() );
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
    if( item && ( !prev || prev == currentItem() ) && !renameLineEdit()->isVisible() && m_selCount < 2 )
    {
        if( !prev )
            //if nothing is current and then playback starts, we must show the currentTrack
            ensureItemCentered( item ); //handles 0 gracefully

        else {
            const int prevY = itemPos( prev );
            const int prevH = prev->height();

            // check if the previous track is visible
            if( prevY <= contentsY() + visibleHeight() && prevY + prevH >= contentsY() )
            {
                // in random mode always jump, if previous track is visible
                if( AmarokConfig::randomMode() )
                    ensureItemCentered( item );
                else if( prev && prev == currentItem() )
                    setCurrentItem( item );

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
    if ( m_currentTrack )
        m_currentTrack->setIsNew(false);

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
Playlist::currentTrackIndex( bool onlyCountVisible )
{
    int index = 0;
    for( MyIt it( this, onlyCountVisible ? MyIt::Visible : MyIt::All ); *it; ++it )
    {
        if ( *it == m_currentTrack )
            return index;
        ++index;
    }

    return -1;
}

int
Playlist::totalTrackCount() const
{
    return m_totalCount;
}

BundleList
Playlist::nextTracks() const
{
    BundleList list;
    for( QPtrListIterator<PlaylistItem> it( m_nextTracks ); *it; ++it )
        list << (**it);
    return list;
}

uint
Playlist::repeatAlbumTrackCount() const
{
    if ( m_currentTrack && m_currentTrack->m_album )
        return m_currentTrack->m_album->tracks.count();
    else
        return 0;
}

const DynamicMode*
Playlist::dynamicMode() const
{
    return m_dynamicMode;
}

DynamicMode*
Playlist::modifyDynamicMode()
{
    DynamicMode *m = m_dynamicMode;
    if( !m )
        return 0;
    m_dynamicMode = new DynamicMode( *m );
    return m;
}

void
Playlist::finishedModifying( DynamicMode *mode )
{
    DynamicMode *m = m_dynamicMode;
    setDynamicMode( mode );
    delete m;
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

    m_currentTrack->setPixmap( m_firstColumn, pixmap.isNull() ? QPixmap() : Amarok::getPNG( pixmap ) );
    PlaylistItem::setPixmapChanged();
}

PlaylistItem*
Playlist::restoreCurrentTrack()
{
    ///It is always possible that the current track has been lost
    ///eg it was removed and then reinserted, here we check

    const KURL url = EngineController::instance()->playingURL();

    if ( !(m_currentTrack && ( m_currentTrack->url() == url  || !m_currentTrack->url().isEmpty() && url.isEmpty() ) ) )
    {
        PlaylistItem* item;

        for( item = firstChild();
             item && item->url() != url;
             item = item->nextSibling() )
        {}

        setCurrentTrack( item ); //set even if NULL
    }

    if( m_currentTrack && EngineController::instance()->engine()->state() == Engine::Playing && !Glow::timer.isActive() )
        Glow::startTimer();

    return m_currentTrack;
}

void
Playlist::countChanged()
{
    if( !m_itemCountDirty )
    {
        m_itemCountDirty = true;
        QTimer::singleShot( 0, this, SLOT( slotCountChanged() ) );
    }
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
          totalTrackCount() > 1 && ( AmarokConfig::randomMode() || Amarok::repeatPlaylist()
          || Amarok::repeatAlbum() && repeatAlbumTrackCount() > 1 );
}

bool
Playlist::isTrackBefore() const
{
    //order is carefully crafted, remember count() is O(n)

    return !isEmpty() &&
           (
               currentTrack() && (currentTrack()->itemAbove() || Amarok::repeatPlaylist() && totalTrackCount() > 1)
               ||
               AmarokConfig::randomMode() && totalTrackCount() > 1
           );
}

void
Playlist::updateNextPrev()
{
    Amarok::actionCollection()->action( "play" )->setEnabled( !isEmpty() );
    Amarok::actionCollection()->action( "prev" )->setEnabled( isTrackBefore() );
    Amarok::actionCollection()->action( "next" )->setEnabled( isTrackAfter() );
    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( !isEmpty() );
    Amarok::actionCollection()->action( "playlist_show" )->setEnabled( m_currentTrack );

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
    if ( !bundle.podcastBundle() )
    {
        if ( m_currentTrack && !trackChanged ) {
            //if the track hasn't changed then this is a meta-data update
            if( stopAfterMode() == StopAfterCurrent || !m_nextTracks.isEmpty() )
                Playlist::instance()->playNextTrack( true );
            //this is a hack, I repeat a hack! FIXME FIXME
            //we do it because often the stream title is from the pls file and is informative
            //we don't want to lose it when we get the meta data
            else if ( m_currentTrack->artist().isEmpty() ) {
                QString comment = m_currentTrack->title();
                m_currentTrack->copyFrom( bundle );
                m_currentTrack->setComment( comment );
            }
            else
                m_currentTrack->copyFrom( bundle );
        }
        else
            //ensure the currentTrack is set correctly and highlight it
            restoreCurrentTrack();
    }
    else
        //ensure the currentTrack is set correctly and highlight it
        restoreCurrentTrack();

    if( m_currentTrack )
        m_currentTrack->filter( m_filter );
}

void
Playlist::engineStateChanged( Engine::State state, Engine::State /*oldState*/ )
{
    switch( state )
    {
    case Engine::Playing:
        Amarok::actionCollection()->action( "pause" )->setEnabled( true );
        Amarok::actionCollection()->action( "stop" )->setEnabled( true );

        Glow::startTimer();

        break;

    case Engine::Paused:
        Amarok::actionCollection()->action( "pause" )->setEnabled( false );
        Amarok::actionCollection()->action( "stop" )->setEnabled( true );

        Glow::reset();

        if( m_currentTrack )
            slotGlowTimer(); //update glow state

        break;

    case Engine::Empty:
        Amarok::actionCollection()->action( "pause" )->setEnabled( false );
        Amarok::actionCollection()->action( "stop" )->setEnabled( false );

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
        slotGlowTimer();

        break;
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

    disableDynamicMode();

    emit aboutToClear(); //will saveUndoState()

    setCurrentTrack( 0 );
    m_prevTracks.clear();
    m_prevAlbums.clear();

    if (m_stopAfterTrack) {
        m_stopAfterTrack = 0;
        if ( stopAfterMode() != StopAfterCurrent ) {
            setStopAfterMode( DoNotStop );
        }
    }
    const PLItemList prev = m_nextTracks;
    m_nextTracks.clear();
    emit queueChanged( PLItemList(), prev );

    // Update player button states
    Amarok::actionCollection()->action( "play" )->setEnabled( false );
    Amarok::actionCollection()->action( "prev" )->setEnabled( false );
    Amarok::actionCollection()->action( "next" )->setEnabled( false );
    Amarok::actionCollection()->action( "playlist_clear" )->setEnabled( false );

    ThreadManager::instance()->abortAllJobsNamed( "TagWriter" );

    // something to bear in mind, if there is any event in the loop
    // that depends on a PlaylistItem, we are about to crash Amarok
    // never unlock() the Playlist until it is safe!
    safeClear();
    m_total = 0;
    m_albums.clear();

    setPlaylistName( i18n( "Untitled" ) );
    ScriptManager::instance()->notifyPlaylistChange("cleared");
}

/**
 * Workaround for Qt 3.3.5 bug in QListView::clear()
 * @see http://lists.kde.org/?l=kde-devel&m=113113845120155&w=2
 * @see BUG 116004
 */
void
Playlist::safeClear()
{
    /* 3.3.5 and 3.3.6 have bad KListView::clear() functions.
       3.3.5 forgets to clear the pointer to the highlighted item.
       3.3.6 forgets to clear the pointer to the last dragged item */
    if ( strcmp( qVersion(), "3.3.5" ) == 0
         || strcmp( qVersion(), "3.3.6" ) == 0 )
    {
        bool block = signalsBlocked();
        blockSignals( true );
        clearSelection();

        QListViewItem *c = firstChild();
        QListViewItem *n;
        while( c ) {
            n = c->nextSibling();
            if ( !static_cast<PlaylistItem *>( c )->isEmpty() ) //avoid deleting markers
                delete c;
            c = n;
        }
        blockSignals( block );
        triggerUpdate();
    }
    else
        KListView::clear();
}

void
Playlist::setSorting( int col, bool b )
{
    saveUndoState();

    //HACK There are reasons to allow sorting in dynamic mode, but
    //it breaks other things that I don't have the time or patience
    //to figure out...at least right now

    if( !dynamicMode() )
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
    if( !item )
        return;

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

        case PlaylistItem::Composer:
            renameLineEdit()->completionObject()->setItems( CollectionDB::instance()->composerList() );
            break;

        default:
            renameLineEdit()->completionObject()->clear();
            break;
    }

    renameLineEdit()->completionObject()->setCompletionMode( KGlobalSettings::CompletionPopupAuto );
    renameLineEdit()->completionObject()->setIgnoreCase( true );

    m_editOldTag = static_cast<PlaylistItem *>(item)->exactText( column );

    if( m_selCount <= 1 )
    {
        if( currentItem() )
            currentItem()->setSelected( false );
        item->setSelected( true );
    }
    setCurrentItem( item );
    KListView::rename( item, column );

    m_renameItem = item;
    m_renameColumn = column;

    static_cast<PlaylistItem*>(item)->setIsBeingRenamed( true );

}

void
Playlist::writeTag( QListViewItem *qitem, const QString &, int column ) //SLOT
{
    const bool dynamicEnabled = static_cast<PlaylistItem*>(qitem)->isDynamicEnabled();

    if( m_itemsToChangeTagsFor.isEmpty() )
        m_itemsToChangeTagsFor.append( static_cast<PlaylistItem*>( qitem ) );

    const QString newTag = static_cast<PlaylistItem*>( qitem )->exactText( column );

    for( PlaylistItem *item = m_itemsToChangeTagsFor.first(); item; item = m_itemsToChangeTagsFor.next() )
    {
        if( !checkFileStatus( item ) )
            continue;

        const QString oldTag = item == qitem ? m_editOldTag : item->exactText(column);

        if( column == PlaylistItem::Score )
            CollectionDB::instance()->setSongPercentage( item->url().path(), newTag.toInt() );
        else if( column == PlaylistItem::Rating )
            CollectionDB::instance()->setSongRating( item->url().path(), newTag.toInt() );
        else
            if (oldTag != newTag)
                ThreadManager::instance()->queueJob( new TagWriter( item, oldTag, newTag, column ) );
            else if( item->deleteAfterEditing() )
            {
                removeItem( item );
                delete item;
            }
    }

    if( dynamicMode() )
        static_cast<PlaylistItem*>(qitem)->setDynamicEnabled( dynamicEnabled );

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
    QResizeEvent e( size(), QSize() );
    viewportResizeEvent( &e );
    emit columnsChanged();
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
    const bool ctrlPressed = KApplication::keyboardModifiers() & ControlMask;
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

    //make sure to disable only if in dynamic mode and you're inserting 
    //at the beginning or in the middle of the disabled tracks
    //Also, that the dynamic playlist has any tracks (suggested may not)
    if( dynamicMode() && Playlist::instance()->firstChild() &&
            ( !m_marker || !( static_cast<PlaylistItem *>(m_marker)->isDynamicEnabled() )  ) && 
            currentTrackIndex() != -1 )
    {
        // If marker is disabled, and there is a current track, or marker is not the last enabled track
        // don't allow inserting
        if( ( m_marker && ( m_currentTrack || ( m_marker->itemBelow() &&
                            !( static_cast<PlaylistItem *>(m_marker->itemBelow())->isDynamicEnabled() ) ) ) )
            || ( !m_marker ) )
        {
            slotEraseMarker();
            return;
        }
    }

    if( !after )
        findDrop( e->pos(), parent, after ); //shouldn't happen, but you never know!

    slotEraseMarker();

    if ( e->source() == viewport() ) {
        setSorting( NO_SORT ); //disableSorting and saveState()
        movableDropEvent( parent, after );
        QPtrList<QListViewItem> items = selectedItems();
        if( dynamicMode() && after )
        {
            QListViewItem *item;
            bool enabled = static_cast<PlaylistItem *>(after)->isDynamicEnabled();
            for( item = items.first(); item; item = items.next() )
                    static_cast<PlaylistItem *>(item)->setDynamicEnabled( enabled );
        }
        ScriptManager::instance()->notifyPlaylistChange("reordered");
    }

    else {
        QString data;
        QCString subtype;
        QTextDrag::decode( e, data, subtype );

        debug() << "QTextDrag::subtype(): " << subtype << endl;

        if( subtype == "amarok-sql" ) {
            setSorting( NO_SORT );
            QString query = data.section( "\n", 1 );
            ThreadManager::instance()->queueJob( new SqlLoader( query, after ) );
            ScriptManager::instance()->notifyPlaylistChange("changed");
        }

        else if( subtype == "dynamic" ) {
            // Deserialize pointer
            DynamicEntry* entry = reinterpret_cast<DynamicEntry*>( data.toULongLong() );

            loadDynamicMode( entry );
        }

        else if( KURLDrag::canDecode( e ) )
        {
            debug() << "KURLDrag::canDecode" << endl;

            KURL::List list;
            KURLDrag::decode( e, list );
            insertMediaInternal( list, static_cast<PlaylistItem*>( after ) );
        }
        else
            e->ignore();
    }

    updateNextPrev();
}

QDragObject*
Playlist::dragObject()
{
    DEBUG_THREAD_FUNC_INFO

    KURL::List list;

    for( MyIt it( this, MyIt::Selected ); *it; ++it )
    {
        const PlaylistItem *item = static_cast<PlaylistItem*>( *it );
        const KURL url = item->url();
        list += url;
    }

    KURLDrag *drag = new KURLDrag( list, viewport() );
    drag->setPixmap( CollectionDB::createDragPixmap( list ),
                     QPoint( CollectionDB::DRAGPIXMAP_OFFSET_X, CollectionDB::DRAGPIXMAP_OFFSET_Y ) );
    return drag;
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
		delete t;
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
    if ( !m_smartResizing ) {
        KListView::viewportResizeEvent( e );
        return;
    }
    //only be clever with the sizing if there is not many items
    //TODO don't allow an item to be made too small (ie less than 50% of ideal width)

    //makes this much quicker
    header()->blockSignals( true );

    const double W = (double)e->size().width() - negativeWidth;

    for( uint c = 0; c < m_columnFraction.size(); ++c ) {
        switch( c ) {
        case PlaylistItem::Track:
        case PlaylistItem::Bitrate:
        case PlaylistItem::SampleRate:
        case PlaylistItem::Filesize:
        case PlaylistItem::Score:
        case PlaylistItem::Rating:
        case PlaylistItem::Type:
        case PlaylistItem::PlayCount:
        case PlaylistItem::Length:
        case PlaylistItem::Year:
        case PlaylistItem::DiscNumber:
        case PlaylistItem::Bpm:
            break; //these columns retain their width - their items tend to have uniform size
        default:
            if( m_columnFraction[c] > 0 )
                setColumnWidth( c, int(W * m_columnFraction[c]) );
        }
    }

    header()->blockSignals( false );

    //ensure that the listview scrollbars are updated etc.
    triggerUpdate();
}

void
Playlist::columnResizeEvent( int col, int oldw, int neww )
{
    if ( !m_smartResizing )
        return;
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
            case PlaylistItem::SampleRate:
            case PlaylistItem::Filesize:
            case PlaylistItem::Score:
            case PlaylistItem::Rating:
            case PlaylistItem::Type:
            case PlaylistItem::PlayCount:
            case PlaylistItem::Length:
            case PlaylistItem::Year:
            case PlaylistItem::DiscNumber:
            case PlaylistItem::Bpm:
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
        case PlaylistItem::SampleRate:
        case PlaylistItem::Filesize:
        case PlaylistItem::Score:
        case PlaylistItem::Rating:
        case PlaylistItem::Type:
        case PlaylistItem::PlayCount:
        case PlaylistItem::Length:
        case PlaylistItem::Year:
        case PlaylistItem::DiscNumber:
        case PlaylistItem::Bpm:
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
        emit columnsChanged();
    }
}

bool
Playlist::eventFilter( QObject *o, QEvent *e )
{
    #define me static_cast<QMouseEvent*>(e)
    #define ke static_cast<QKeyEvent*>(e)

    if( o == header() && e->type() == QEvent::MouseButtonPress && me->button() == Qt::RightButton )
    {
        enum { HIDE = 1000, SELECT, CUSTOM, SMARTRESIZING };

        const int mouseOverColumn = header()->sectionAt( me->pos().x() );

        KPopupMenu popup;
        if( mouseOverColumn >= 0 )
            popup.insertItem( i18n("&Hide %1").arg( columnText( mouseOverColumn ) ), HIDE ); //TODO

        KPopupMenu sub;
        for( int i = 0; i < columns(); ++i ) //columns() references a property
            if( !columnWidth( i ) )
                sub.insertItem( columnText( i ), i, i + 1 );
        sub.setItemVisible( PlaylistItem::Score, AmarokConfig::useScores() );
        sub.setItemVisible( PlaylistItem::Rating, AmarokConfig::useRatings() );
        sub.setItemVisible( PlaylistItem::Mood, AmarokConfig::showMoodbar() );

        popup.insertItem( i18n("&Show Column" ), &sub );

        popup.insertItem( i18n("Select &Columns..."), SELECT );

        popup.insertItem( i18n("&Fit to Width"), SMARTRESIZING );
        popup.setItemChecked( SMARTRESIZING, m_smartResizing );

        int col = popup.exec( static_cast<QMouseEvent *>(e)->globalPos() );

        switch( col ) {
        case HIDE:
            {
                hideColumn( mouseOverColumn );
                QResizeEvent e( size(), QSize() );
                viewportResizeEvent( &e );
            }
            break;

        case SELECT:
            ColumnsDialog::display();
            break;

        case CUSTOM:
            addCustomColumn();
            break;

        case SMARTRESIZING:
            m_smartResizing = !m_smartResizing;
            Amarok::config( "PlaylistWindow" )->writeEntry( "Smart Resizing", m_smartResizing );
            if ( m_smartResizing )
                columnResizeEvent( 0, 0, 0 ); //force refit. FIXME: It doesn't work perfectly
            break;

        default:
            if( col != -1 )
            {
                adjustColumn( col );
                header()->setResizeEnabled( true, col );
            }
        }

        //determine first visible column again, since it has changed
        columnOrderChanged();
        //eat event
        return true;
    }

    // not in slotMouseButtonPressed because we need to disable normal usage.
    if( o == viewport() && e->type() == QEvent::MouseButtonPress && me->state() == Qt::ControlButton && me->button() == RightButton )
    {
        PlaylistItem *item = static_cast<PlaylistItem*>( itemAt( me->pos() ) );

        if( !item )
            return true;

        item->isSelected() ?
            queueSelected():
            queue( item );

        return true; //yum!
    }

    // trigger in-place tag editing
    else if( o == viewport() && e->type() == QEvent::MouseButtonPress && me->button() == LeftButton )
    {
        m_clicktimer->stop();
        m_itemToRename = 0;
        int col = header()->sectionAt( viewportToContents( me->pos() ).x() );
        if( col != PlaylistItem::Rating )
        {
            PlaylistItem *item = static_cast<PlaylistItem*>( itemAt( me->pos() ) );
            bool edit = item
                && item->isSelected()
                && selectedItems().count()==1
                && (me->state() & ~LeftButton) == 0
                && item->url().isLocalFile();
            if( edit )
            {
                m_clickPos = me->pos();
                m_itemToRename = item;
                m_columnToRename = col;
                //return true;
            }
        }
    }

    else if( o == viewport() && e->type() == QEvent::MouseButtonRelease && me->button() == LeftButton )
    {
        int col = header()->sectionAt( viewportToContents( me->pos() ).x() );
        if( col != PlaylistItem::Rating )
        {
            PlaylistItem *item = static_cast<PlaylistItem*>( itemAt( me->pos() ) );
            if( item == m_itemToRename && me->pos() == m_clickPos )
            {
                m_clicktimer->start( int( QApplication::doubleClickInterval() ), true );
                return true;
            }
            else
            {
                m_itemToRename = 0;
            }
        }
    }

    // avoid in-place tag editing upon double-clicks
    else if( e->type() == QEvent::MouseButtonDblClick && me->button() == Qt::LeftButton )
    {
        m_itemToRename = 0;
        m_clicktimer->stop();
    }

    // Toggle play/pause if user middle-clicks on current track
    else if( o == viewport() && e->type() == QEvent::MouseButtonPress && me->button() == MidButton )
    {
        PlaylistItem *item = static_cast<PlaylistItem*>( itemAt( me->pos() ) );

        if( item && item == m_currentTrack )
        {
            EngineController::instance()->playPause();
            return true; //yum!
        }
    }

    else if( o == renameLineEdit() && e->type() == 6 /*QEvent::KeyPress*/ && m_renameItem )
    {
        const int visibleCols = numVisibleColumns();
        int physicalColumn = visibleCols - 1;

        while( mapToLogicalColumn( physicalColumn ) != m_renameColumn && physicalColumn >= 0 )
            physicalColumn--;
        if( physicalColumn < 0 )
        {
            warning() << "the column counting code is wrong! tell illissius." << endl;
            return false;
        }

        int column = m_renameColumn;
        QListViewItem *item = m_renameItem;

        if( ke->state() & Qt::AltButton )
        {
            if( ke->key() == Qt::Key_Up && m_visCount > 1 )
                if( !( item = m_renameItem->itemAbove() ) )
                {
                    item = *MyIt( this, MyIt::Visible );
                    while( item->itemBelow() )
                        item = item->itemBelow();
                }
            if( ke->key() == Qt::Key_Down && m_visCount > 1 )
                if( !( item = m_renameItem->itemBelow() ) )
                    item = *MyIt( this, MyIt::Visible );
            if( ke->key() == Qt::Key_Left )
                do
                {
                    if( physicalColumn == 0 )
                        physicalColumn = visibleCols - 1;
                    else
                        physicalColumn--;
                    column = mapToLogicalColumn( physicalColumn );
                } while( !isRenameable( column ) );
            if( ke->key() == Qt::Key_Right )
                do
                {
                    if( physicalColumn == visibleCols - 1 )
                        physicalColumn = 0;
                    else
                        physicalColumn++;
                    column = mapToLogicalColumn( physicalColumn );
                } while( !isRenameable( column ) );
        }

        if( ke->key() == Qt::Key_Tab )
            do
            {
                if( physicalColumn == visibleCols - 1 )
                {
                    if( !( item = m_renameItem->itemBelow() ) )
                        item = *MyIt( this, MyIt::Visible );
                    physicalColumn = 0;
                }
                else
                    physicalColumn++;
                column = mapToLogicalColumn( physicalColumn );
            } while( !isRenameable( column ) );
        if( ke->key() == Qt::Key_Backtab )
            do
            {
                if( physicalColumn == 0 )
                {
                    if( !( item = m_renameItem->itemAbove() ) )
                    {
                        item = *MyIt( this, MyIt::Visible );
                        while( item->itemBelow() )
                            item = item->itemBelow();
                    }
                    physicalColumn = visibleCols - 1;
                }
                else
                    physicalColumn--;
                column = mapToLogicalColumn( physicalColumn );
            } while( !isRenameable( column ) );

        if( item != m_renameItem || column != m_renameColumn )
        {
            if( !item->isSelected() )
                m_itemsToChangeTagsFor.clear();
                //the item that actually got changed will get added back, in writeTag()
            m_renameItem->setText( m_renameColumn, renameLineEdit()->text() );
            doneEditing( m_renameItem, m_renameColumn );
            rename( item, column );
            return true;
        }
    }

    else if( o == renameLineEdit() && ( e->type() == QEvent::Hide || e->type() == QEvent::Close ) )
    {
        m_renameItem = 0;
    }

    //allow the header to process this
    return KListView::eventFilter( o, e );

    #undef me
    #undef ke
}

void
Playlist::slotSingleClick()
{
    if( m_itemToRename )
    {
        rename( m_itemToRename, m_columnToRename );
    }

    m_itemToRename = 0;
}

void
Playlist::customEvent( QCustomEvent *e )
{
    if( e->type() == (int)UrlLoader::JobFinishedEvent ) {
        refreshNextTracks( 0 );
        PLItemList in, out;

        // Disable help if playlist is populated
        if ( !isEmpty() )
            m_showHelp = false;

        if ( !m_queueList.isEmpty() ) {
            KURL::List::Iterator jt;
            for( MyIt it( this, MyIt::All ); *it; ++it ) {
                jt = m_queueList.find( (*it)->url() );

                if ( jt != m_queueList.end() ) {
                    queue( *it );
                    ( m_nextTracks.containsRef( *it ) ? in : out ).append( *it );
                    m_queueList.remove( jt );
                }
            }
            m_queueList.clear();
        }

        if( m_dynamicDirt )
        {
            PlaylistItem *after = m_currentTrack;
            if( !after )
            {
                after = firstChild();
                while( after && !after->isDynamicEnabled() )
                    after = after->nextSibling();
            }
            else
                after = static_cast<PlaylistItem *>( after->itemBelow() );

            if( after )
            {
                PlaylistItem *prev = static_cast<PlaylistItem *>( after->itemAbove() );
                if( prev && dynamicMode() )
                    prev->setDynamicEnabled( false );

                s_dynamicADTMutex->lock();
                if( m_insertFromADT > 0 )
                {
                    if( EngineController::engine()->state() == Engine::Playing )
                        activate( after );
                    m_insertFromADT--;
                }
                else
                    activate( after );
                s_dynamicADTMutex->unlock();
                if( dynamicMode() && dynamicMode()->cycleTracks() )
                    adjustDynamicPrevious( dynamicMode()->previousCount() );
            }
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
                while( after && !after->isDynamicEnabled() )
                    after = after->nextSibling();
            }
            else
                after = static_cast<PlaylistItem *>( after->itemBelow() );

            if( after )
            {
                m_nextTracks.append( after );

                in.append( after );
            }

            m_queueDirt = false;
        }

        if( !in.isEmpty() || !out.isEmpty() )
            emit queueChanged( in, out );

        //force redraw of currentTrack marker, play icon, etc.
        restoreCurrentTrack();
    }

    updateNextPrev();
}



////////////////////////////////////////////////////////////////////////////////
/// Misc Public Methods
////////////////////////////////////////////////////////////////////////////////

bool
Playlist::saveM3U( const QString &path, bool relative ) const
{
    QValueList<KURL> urls;
    QValueList<QString> titles;
    QValueList<int> lengths;
    for( MyIt it( firstChild(), MyIt::Visible ); *it; ++it )
    {
        urls << (*it)->url();
        titles << (*it)->title();
        lengths << (*it)->length();
    }
    return PlaylistBrowser::savePlaylist( path, urls, titles, lengths, relative );
}

void
Playlist::saveXML( const QString &path )
{
    DEBUG_BLOCK

    QFile file( path );
    if( !file.open( IO_WriteOnly | IO_Truncate | IO_Raw ) ) return;

    // Manual buffering since QFile's is slow for whatever reason
    const uint kWriteSize = 256 * 1024;

    QBuffer buffer;
    buffer.open(IO_WriteOnly);
    
    QTextStream stream( &buffer );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";

    QString dynamic;
    if( dynamicMode() )
    {
        const QString title = ( dynamicMode()->title() ).replace( "&", "&amp;" )
                                                        .replace( "<", "&lt;"  )
                                                        .replace( ">", "&gt;"  );
        dynamic = QString(" dynamicMode=\"%1\"").arg( title );
    }
    stream << QString( "<playlist product=\"%1\" version=\"%2\"%3>\n" )
              .arg( "Amarok" ).arg( Amarok::xmlVersion() ).arg( dynamic );

    for( MyIt it( this, MyIt::All ); *it; ++it )
    {
        const PlaylistItem *item = *it;
        if( item->isEmpty() ) continue;  // Skip marker items and such

        QStringList attributes;
        const int queueIndex = m_nextTracks.findRef( item );
        if ( queueIndex != -1 )
            attributes << "queue_index" << QString::number( queueIndex + 1 );
        else if ( item == currentTrack() )
            attributes << "queue_index" << QString::number( 0 );

        if( !item->isDynamicEnabled() )
            attributes << "dynamicdisabled" << "true";

        if( m_stopAfterTrack == item )
            attributes << "stop_after" << "true";

        item->save( stream, attributes );

        if ( buffer.at() >= kWriteSize )
        {
            file.writeBlock( buffer.buffer().data(), buffer.at() );
            buffer.reset();
        }
    }

    stream << "</playlist>\n";
    file.writeBlock(buffer.buffer().data(), buffer.at());
    file.close();
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
Playlist::addCustomMenuItem( const QString &submenu, const QString &itemTitle )  //for dcop
{
    m_customSubmenuItem[submenu] << itemTitle;
}

bool
Playlist::removeCustomMenuItem( const QString &submenu, const QString &itemTitle )  //for dcop
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
        message += ' ' + url.url();
    }
    ScriptManager::instance()->customMenuClicked( message );
}

void
Playlist::setDynamicMode( DynamicMode *mode ) //SLOT
{
    // if mode == 0, then dynamic mode was just turned off.

    DynamicMode* const prev = m_dynamicMode;
    m_dynamicMode = mode;
    if( mode )
        AmarokConfig::setLastDynamicMode( mode->title() );
    emit dynamicModeChanged( mode );

    if( mode )
    {
        m_oldRandom = AmarokConfig::randomMode();
        m_oldRepeat = AmarokConfig::repeat();
    }
    Amarok::actionCollection()->action( "random_mode" )->setEnabled( !mode );
    Amarok::actionCollection()->action( "repeat" )->setEnabled( !mode  );
    Amarok::actionCollection()->action( "playlist_shuffle" )->setEnabled( !mode );
    Amarok::actionCollection()->action( "repopulate" )->setEnabled( mode );
    if( prev && mode )
    {
        if( prev->previousCount() != mode->previousCount() )
            adjustDynamicPrevious( mode->previousCount(), true );
        if( prev->upcomingCount() != mode->upcomingCount() )
            adjustDynamicUpcoming( true );
    }
    else if( !prev )
    {
        if( mode )
            adjustDynamicPrevious( mode->previousCount(), true );
        setDynamicHistory( true ); // disable items!
    }

    else if( !mode ) // enable items again, dynamic mode is no more
        setDynamicHistory( false );
}

void
Playlist::loadDynamicMode( DynamicMode *mode ) //SLOT
{
    saveUndoState();
    setDynamicMode( mode );
    if( isEmpty() )
        repopulate();
}

void
Playlist::editActiveDynamicMode() //SLOT
{
    if( !m_dynamicMode )
        return;

    DynamicMode *m = modifyDynamicMode();
    ConfigDynamic::editDynamicPlaylist( PlaylistWindow::self(), m );
    m->rebuildCachedItemSet();
    finishedModifying( m );
}

void
Playlist::disableDynamicMode() //SLOT
{
    if( !m_dynamicMode )
        return;
    setDynamicMode( 0 );
    AmarokConfig::setRandomMode( m_oldRandom );
    AmarokConfig::setRepeat( m_oldRepeat );
    static_cast<KSelectAction*>(Amarok::actionCollection()->action( "random_mode" ))->setCurrentItem( m_oldRandom );
    static_cast<KSelectAction*>(Amarok::actionCollection()->action( "repeat" ))->setCurrentItem( m_oldRepeat );
}

void
Playlist::rebuildDynamicModeCache() //SLOT
{
    if( !m_dynamicMode )
        return;

    DynamicMode *m = modifyDynamicMode();
    m->rebuildCachedItemSet();
    finishedModifying( m );
}

void
Playlist::repopulate() //SLOT
{
    if( !m_dynamicMode )
        return;

    // Repopulate the upcoming tracks
    MyIt it( this, MyIt::All );
    QPtrList<QListViewItem> list;

    for( ; *it; ++it )
    {
        PlaylistItem *item = static_cast<PlaylistItem *>(*it);
        int     queueIndex = m_nextTracks.findRef( item );
        bool    isQueued   = queueIndex != -1;
        bool    isMarker   = item->isEmpty();
        // markers are used by playlistloader, and removing them is not good

        if( !item->isDynamicEnabled() || item == m_currentTrack || isQueued || isMarker )
            continue;

        list.prepend( *it );
    }

    saveUndoState();

    //remove the items
    for( QListViewItem *item = list.first(); item; item = list.next() )
    {
        removeItem( static_cast<PlaylistItem*>( item ) );
        delete item;
    }

    //calling advanceDynamicTrack will remove an item too, which is undesirable
    //block signals to avoid saveUndoState being called
    blockSignals( true );
    addDynamicModeTracks( dynamicMode()->upcomingCount() );
    blockSignals( false );
}

void
Playlist::shuffle() //SLOT
{
    if( dynamicMode() )
        return;

    QPtrList<QListViewItem> list;

    setSorting( NO_SORT );

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
    ScriptManager::instance()->notifyPlaylistChange("reordered");
}

void
Playlist::removeSelectedItems() //SLOT
{
    if( isLocked() ) return;

    //assemble a list of what needs removing
    //calling removeItem() iteratively is more efficient if they are in _reverse_ order, hence the prepend()
    PLItemList queued, list;
    int dontReplaceDynamic = 0;

    for( PlaylistIterator it( this, MyIt::Selected ); *it; ++it )
    {
        if( !(*it)->isDynamicEnabled() )
            dontReplaceDynamic++;
        ( m_nextTracks.contains( *it ) ? queued : list ).prepend( *it );
    }

    if( (int)list.count() == childCount() )
    {
        //clear() will saveUndoState for us.
        clear();        // faster
        return;
    }

    if( list.isEmpty() && queued.isEmpty() ) return;
    saveUndoState();

    if( dynamicMode() )
    {
        int currentTracks = childCount();
        int minTracks     = dynamicMode()->upcomingCount();

        if( m_currentTrack )
            currentTracks -= currentTrackIndex() + 1;

        int difference = currentTracks - minTracks;

        if( difference >= 0 )
            difference -= list.count();

        if( difference < 0 )
        {
            addDynamicModeTracks( -difference );
        }
    }

    //remove the items
    if( queued.count() )
    {
        for( QListViewItem *item = queued.first(); item; item = queued.next() )
            removeItem( static_cast<PlaylistItem*>( item ), true );

        emit queueChanged( PLItemList(), queued );

        for( QListViewItem *item = queued.first(); item; item = queued.next() )
            delete item;
    }

    for( QListViewItem *item = list.first(); item; item = list.next() )
    {
        removeItem( static_cast<PlaylistItem*>( item ) );
        delete item;
    }

    updateNextPrev();

    ScriptManager::instance()->notifyPlaylistChange("changed");
    //NOTE no need to emit childCountChanged(), removeItem() does that for us

    //select next item in list
    setSelected( currentItem(), true );
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
    if( DeleteDialog::showTrashDialog(this, urls) )
    {
        CollectionDB::instance()->removeSongs( urls );
        removeSelectedItems();
        foreachType( KURL::List, urls )
          CollectionDB::instance()->emitFileDeleted( (*it).path() );
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
        list.prepend( static_cast<PlaylistItem*>( it.current() ) );

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

        QString text = playlistItem->prettyTitle();
        // For streams add the streamtitle too
        //TODO make prettyTitle do this
        if ( playlistItem->url().protocol() == "http" )
            text.append( " :: " + playlistItem->url().url() );

        // Copy both to clipboard and X11-selection
        QApplication::clipboard()->setText( text, QClipboard::Clipboard );
        QApplication::clipboard()->setText( text, QClipboard::Selection );

        Amarok::OSD::instance()->OSDWidget::show( i18n( "Copied: %1" ).arg( text ),
                                 QImage(CollectionDB::instance()->albumImage(*playlistItem )) );
    }
}

void Playlist::undo() //SLOT
{
    if( !isLocked() )
        switchState( m_undoList, m_redoList );
}

void Playlist::redo() //SLOT
{
    if( !isLocked() )
        switchState( m_redoList, m_undoList );
}

void
Playlist::updateMetaData( const MetaBundle &mb ) //SLOT
{
    SHOULD_BE_GUI
    for( MyIt it( this, MyIt::All ); *it; ++it )
        if( mb.url() == (*it)->url() )
        {
            (*it)->copyFrom( mb );
            (*it)->filter( m_filter );
        }
}

void
Playlist::adjustColumn( int n )
{
    if( n == PlaylistItem::Rating )
        setColumnWidth( n, PlaylistItem::ratingColumnWidth() );
    else if( n == PlaylistItem::Mood )
        setColumnWidth( n, 120 );
    else
        KListView::adjustColumn( n );
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
        changeFromQueueManager(dialog.newQueue());
    }
}

void
Playlist::changeFromQueueManager(QPtrList<PlaylistItem> list)
{
    PLItemList oldQueue = m_nextTracks;
    m_nextTracks = list;

    PLItemList in, out;
    // make sure we repaint items no longer queued
    for( PlaylistItem* item = oldQueue.first(); item; item = oldQueue.next() )
        if( !m_nextTracks.containsRef( item ) )
            out << item;
    for( PlaylistItem* item = m_nextTracks.first(); item; item = m_nextTracks.next() )
        if( !oldQueue.containsRef( item ) )
            in << item;

    emit queueChanged( in, out );

    // repaint newly queued or altered queue items
    if( dynamicMode() )
        sortQueuedItems();
    else
        refreshNextTracks();
}

void
Playlist::setFilterSlot( const QString &query ) //SLOT
{
    m_filtertimer->stop();
    if( m_filter != query )
    {
        m_prevfilter = m_filter;
        m_filter = query;
    }
    m_filtertimer->start( 50, true );
}

void
Playlist::setDelayedFilter() //SLOT
{
    setFilter( m_filter );

    //to me it seems sensible to do this, BUT if it seems annoying to you, remove it
    showCurrentTrack();
}

void
Playlist::setFilter( const QString &query ) //SLOT
{
    const bool advanced = ExpressionParser::isAdvancedExpression( query );
    MyIt it( this, ( !advanced && query.lower().contains( m_prevfilter.lower() ) )
                   ? MyIt::Visible
                   : MyIt::All );


    if( advanced )
    {
        ParsedExpression parsed = ExpressionParser::parse( query );
        QValueList<int> visible = visibleColumns();
        for(; *it; ++it )
            (*it)->setVisible( (*it)->matchesParsedExpression( parsed, visible ) );
    }
    else {
        // optimized path
        const QStringList terms = QStringList::split( ' ', query.lower() );
        const MetaBundle::ColumnMask visible = getVisibleColumnMask();
        for(; *it; ++it ) {
            (*it)->setVisible( (*it)->matchesFast(terms, visible));
        }
    }

    if( m_filter != query )
    {
        m_prevfilter = m_filter;
        m_filter = query;
    }
    updateNextPrev();
}

void
Playlist::scoreChanged( const QString &path, float score )
{
    for( MyIt it( this, MyIt::All ); *it; ++it )
    {
        PlaylistItem *item = static_cast<PlaylistItem*>( *it );
        if ( item->url().path() == path )
        {
            item->setScore( score );
            item->setPlayCount( CollectionDB::instance()->getPlayCount( path ) );
            item->setLastPlay( CollectionDB::instance()->getLastPlay( path ).toTime_t() );
            item->filter( m_filter );
        }
    }
}

void
Playlist::ratingChanged( const QString &path, int rating )
{
    for( MyIt it( this, MyIt::All ); *it; ++it )
    {
        PlaylistItem *item = static_cast<PlaylistItem*>( *it );
        if ( item->url().path() == path )
        {
            item->setRating( rating );
            item->filter( m_filter );
        }
    }
}

void
Playlist::fileMoved( const QString &srcPath, const QString &dstPath )
{
    // Make sure the MoodServer gets this signal first!
    MoodServer::instance()->slotFileMoved( srcPath, dstPath );

    for( MyIt it( this, MyIt::All ); *it; ++it )
    {
        PlaylistItem *item = static_cast<PlaylistItem*>( *it );
        if ( item->url().path() == srcPath )
        {
            item->setUrl( KURL::fromPathOrURL( dstPath ) );
            item->filter( m_filter );
        }
    }
}

void
Playlist::appendToPreviousTracks( PlaylistItem *item )
{
    if( !m_prevTracks.containsRef( item ) )
    {
        m_total -= item->totalIncrementAmount();
        m_prevTracks.append( item );
    }
}

void
Playlist::appendToPreviousAlbums( PlaylistAlbum *album )
{
    if( !m_prevAlbums.containsRef( album ) )
    {
        m_total -= album->total;
        m_prevAlbums.append( album );
    }
}

void
Playlist::removeFromPreviousTracks( PlaylistItem *item )
{
    if( item )
    {
        if( m_prevTracks.removeRef( item ) )
            m_total += item->totalIncrementAmount();
    }
    else if( (item = m_prevTracks.current()) != 0 )
        if( m_prevTracks.remove() )
            m_total += item->totalIncrementAmount();
}

void
Playlist::removeFromPreviousAlbums( PlaylistAlbum *album )
{
    if( album )
    {
        if( m_prevAlbums.removeRef( album ) )
            m_total += album->total;
    }
    else if( (album = m_prevAlbums.current()) != 0 )
        if( m_prevAlbums.remove() )
            m_total += album->total;
}


void
Playlist::showContextMenu( QListViewItem *item, const QPoint &p, int col ) //SLOT
{
    //if clicked on an empty area
    enum { REPOPULATE, ENABLEDYNAMIC };
    if( item == 0 )
    {
        KPopupMenu popup;
        Amarok::actionCollection()->action("playlist_save")->plug( &popup );
        Amarok::actionCollection()->action("playlist_clear")->plug( &popup );
        DynamicMode *m = 0;
        if(dynamicMode())
             popup.insertItem( SmallIconSet( Amarok::icon( "dynamic" ) ), i18n("Repopulate"), REPOPULATE);
        else
        {
            Amarok::actionCollection()->action("playlist_shuffle")->plug( &popup );
                m = PlaylistBrowser::instance()->findDynamicModeByTitle( AmarokConfig::lastDynamicMode() );
                if( m )
                    popup.insertItem( SmallIconSet( Amarok::icon( "dynamic" ) ), i18n("L&oad %1").arg( m->title().replace( '&', "&&" ) ), ENABLEDYNAMIC);
        }
        switch(popup.exec(p))
        {
            case ENABLEDYNAMIC:
                loadDynamicMode( m );
                break;
            case REPOPULATE: repopulate(); break;
        }
        return;
    }

    #define item static_cast<PlaylistItem*>(item)

    enum {
        PLAY, PLAY_NEXT, STOP_DONE, VIEW, EDIT, FILL_DOWN, COPY, CROP_PLAYLIST, SAVE_PLAYLIST, REMOVE, FILE_MENU, ORGANIZE, MOVE_TO_COLLECTION, COPY_TO_COLLECTION, DELETE,
        TRASH, REPEAT, LAST }; //keep LAST last

    const bool canRename   = isRenameable( col ) && item->url().isLocalFile();
    const bool isCurrent   = (item == m_currentTrack);
    const bool isPlaying   = EngineController::engine()->state() == Engine::Playing;
    const bool trackColumn = col == PlaylistItem::Track;
    const bool isLastFm    = item->url().protocol() == "lastfm";
    const QString tagName  = columnText( col );
    const QString tag      = item->text( col );

    uint itemCount = 0;
    for( MyIt it( this, MyIt::Selected ); *it; ++it )
        itemCount++;

    PrettyPopupMenu popup;

//     if(itemCount==1)
//         popup.insertTitle( KStringHandler::rsqueeze( MetaBundle( item ).prettyTitle(), 50 ));
//     else
//         popup.insertTitle(i18n("1 Track", "%n Selected Tracks", itemCount));

    if( isCurrent && isLastFm )
    {
        KActionCollection *ac = Amarok::actionCollection();
        if( ac->action( "skip" ) ) ac->action( "skip" )->plug( &popup );
        if( ac->action( "love" ) ) ac->action( "love" )->plug( &popup );
        if( ac->action( "ban" ) ) ac->action( "ban" )->plug( &popup );
        popup.insertSeparator();
    }

    if( !isCurrent || !isPlaying )
        popup.insertItem( SmallIconSet( Amarok::icon( "play" ) ), isCurrent && isPlaying
                ? i18n( "&Restart" )
                : i18n( "&Play" ), PLAY );
    if( isCurrent && !isLastFm && isPlaying )
        Amarok::actionCollection()->action( "pause" )->plug( &popup );

    // Begin queue entry logic
    popup.insertItem( SmallIconSet( Amarok::icon( "queue_track" ) ), i18n("&Queue Selected Tracks"), PLAY_NEXT );

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
            popup.changeItem( PLAY_NEXT, SmallIconSet( Amarok::icon( "dequeue_track" ) ), i18n("&Dequeue Track") );
    } else {
        if ( queueToggle )
            popup.changeItem( PLAY_NEXT, i18n( "Toggle &Queue Status (1 track)", "Toggle &Queue Status (%n tracks)", itemCount ) );
        else
            // remember, queueToggled only gets set to false if there are items queued and not queued.
            // so, if queueToggled is false, all items have the same queue status as the first item.
            if ( !firstQueued )
                popup.changeItem( PLAY_NEXT, i18n( "&Queue Selected Tracks" ) );
            else
                popup.changeItem( PLAY_NEXT, SmallIconSet( Amarok::icon( "dequeue_track" ) ), i18n("&Dequeue Selected Tracks") );
    }
    // End queue entry logic

    bool afterCurrent = false;
    if(  !m_nextTracks.isEmpty() ? m_nextTracks.getLast() : m_currentTrack  )
        for( MyIt it( !m_nextTracks.isEmpty() ? m_nextTracks.getLast() : m_currentTrack, MyIt::Visible ); *it; ++it )
            if( *it == item )
            {
                afterCurrent = true;
                break;
            }

    if( itemCount == 1 )
    {
        Amarok::actionCollection()->action( "stop_after" )->plug( &popup );
        dynamic_cast<KToggleAction *>( Amarok::actionCollection()->action( "stop_after" ) )->setChecked( m_stopAfterTrack == item );
    }

    if( isCurrent && itemCount == 1 )
    {
        popup.insertItem( SmallIconSet( Amarok::icon( "repeat_track" ) ), i18n( "&Repeat Track" ), REPEAT );
        popup.setItemChecked( REPEAT, Amarok::repeatTrack() );
    }

    popup.insertSeparator();

    if( itemCount > 1 )
    {
        popup.insertItem( SmallIconSet( Amarok::icon( "playlist" ) ), i18n("&Set as Playlist (Crop)"), CROP_PLAYLIST );
        popup.insertItem( SmallIconSet( Amarok::icon( "save" ) ), i18n("S&ave as Playlist..."), SAVE_PLAYLIST );
    }

    popup.insertItem( SmallIconSet( Amarok::icon( "remove_from_playlist" ) ), i18n( "Re&move From Playlist" ), this, SLOT( removeSelectedItems() ), Key_Delete, REMOVE );

    popup.insertSeparator();

    KPopupMenu fileMenu;
    if( CollectionDB::instance()->isDirInCollection( item->url().directory() ) )
    {
        fileMenu.insertItem( SmallIconSet( "filesaveas" ), i18n("&Organize File...", "&Organize %n Files...", itemCount), ORGANIZE );
    }
    else
    {
        fileMenu.insertItem( SmallIconSet( "filesaveas" ), i18n("&Copy Track to Collection...", "&Copy %n Tracks to Collection...", itemCount), COPY_TO_COLLECTION );
        fileMenu.insertItem( SmallIconSet( "filesaveas" ), i18n("&Move Track to Collection...", "&Move %n Tracks to Collection...", itemCount), MOVE_TO_COLLECTION );
    }
    fileMenu.insertItem( SmallIconSet( Amarok::icon( "remove" ) ), i18n("&Delete File...", "&Delete %n Selected Files...", itemCount ), this, SLOT( deleteSelectedFiles() ), SHIFT+Key_Delete, DELETE );
    popup.insertItem( SmallIconSet( Amarok::icon( "files" ) ), i18n("Manage &Files"), &fileMenu, FILE_MENU );

    if( itemCount == 1 )
        popup.insertItem( SmallIconSet( Amarok::icon( "editcopy" ) ), i18n( "&Copy Tags to Clipboard" ), COPY );

    if( itemCount > 1 )
        popup.insertItem( trackColumn
                        ? i18n("Iteratively Assign Track &Numbers")
                        : i18n("&Write '%1' for Selected Tracks")
                        .arg( KStringHandler::rsqueeze( tag, 30 ).replace( "&", "&&" ) ), FILL_DOWN );

    popup.insertItem( SmallIconSet( Amarok::icon( "edit" ) ), (itemCount == 1
            ? i18n( "&Edit Tag '%1'" )
            : i18n( "&Edit '%1' Tag for Selected Tracks" )).arg( tagName ), EDIT );

    popup.insertItem( SmallIconSet( Amarok::icon( "info" ) )
        , item->url().isLocalFile() ?
              i18n( "Edit Track &Information...",  "Edit &Information for %n Tracks...", itemCount):
              i18n( "Track &Information...",  "&Information for %n Tracks...", itemCount)
        , VIEW );

    popup.setItemEnabled( EDIT, canRename ); //only enable for columns that have editable tags
    popup.setItemEnabled( FILL_DOWN, canRename );
    popup.setItemEnabled( REMOVE, !isLocked() ); // can't remove things when playlist is locked,
    popup.setItemEnabled( DELETE, !isLocked() && item->url().isLocalFile() );
    popup.setItemEnabled( ORGANIZE, !isLocked() && item->isKioUrl() );
    popup.setItemEnabled( MOVE_TO_COLLECTION, !isLocked() && item->isKioUrl() );
    popup.setItemEnabled( COPY_TO_COLLECTION, !isLocked() && item->isKioUrl() );
    popup.setItemEnabled( VIEW, item->url().isLocalFile() || itemCount == 1 ); // disable for CDAudio multiselection

    if( m_customSubmenuItem.count() > 0 )
        popup.insertSeparator();
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

    const QPoint pos( p.x() - popup.sidePixmapWidth(), p.y() + 3 );
    int menuItemId = popup.exec( pos );
    PLItemList in, out;

    switch( menuItemId )
    {
    case PLAY:
        if( itemCount == 1 )
        {
            //Restarting track on dynamic mode
            if( isCurrent && isPlaying && dynamicMode() )
                m_dynamicDirt = true;
            activate( item );
        }
        else
        {
            MyIt it( this, MyIt::Selected );
            activate( *it );
            ++it;
            for( int i = 0; *it; ++i, ++it )
            {
                in.append( *it );
                m_nextTracks.insert( i, *it );
            }
            emit queueChanged( in, out );
        }
        break;

    case PLAY_NEXT:
        queueSelected();
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
            uint trackNo = (*it)->track();

            //we should start at the next row if we are doing track number
            //and the first row has a number set
            if ( trackColumn && trackNo > 0 )
                ++it;

            ThreadManager::JobList jobs;
            bool updateView = true;
            for( ; *it; ++it ) {
                if ( trackColumn )
                    //special handling for track column
                    newTag = QString::number( ++trackNo );

                else if ( *it == item )
                    //skip the one we are copying
                    continue;

                else if( col == PlaylistItem::Score )
                {
                    CollectionDB::instance()->setSongPercentage( (*it)->url().path(), newTag.toInt() );
                    continue;
                }
                else if( col == PlaylistItem::Rating )
                {
                    CollectionDB::instance()->setSongRating( (*it)->url().path(), newTag.toInt() );
                    continue;
                }

                if ( !(*it)->isEditing( col ) )
                    jobs.prepend( new TagWriter( *it, (*it)->exactText( col ), newTag, col, updateView ) );

                updateView = false;
            }

            ThreadManager::instance()->queueJobs( jobs );
        }
        break;

    case COPY:
        copyToClipboard( item );
        break;

    case CROP_PLAYLIST:
        if( !isLocked() )
        {
            //use "in" for the other just because it's there and not used otherwise
            for( MyIt it( this, MyIt::Unselected | MyIt::Visible ); *it; ++it )
                ( m_nextTracks.containsRef( *it ) ? in : out ).append( *it );

            if( !in.isEmpty() || !out.isEmpty() )
            {
                saveUndoState();

                for( PlaylistItem *it = out.first(); it; it = out.next() )
                    removeItem( it, true );
                if( !out.isEmpty() )
                    emit queueChanged( PLItemList(), out );
                for( PlaylistItem *it = out.first(); it; it = out.next() )
                    delete it;

                for( PlaylistItem *it = in.first(); it; it = in.next() )
                {
                    removeItem( it );
                    delete it;
                }
                ScriptManager::instance()->notifyPlaylistChange("cleared");
            }
        }
        break;

    case SAVE_PLAYLIST:
        saveSelectedAsPlaylist();
        break;

    case REPEAT:
        // FIXME HACK Accessing AmarokConfig::Enum* yields compile errors with GCC 3.3.
        static_cast<KSelectAction*>( Amarok::actionCollection()->action( "repeat" ) )
            ->setCurrentItem( Amarok::repeatTrack()
                              ? 0 /*AmarokConfig::EnumRepeat::Off*/
                              : 1 /*AmarokConfig::EnumRepeat::Track*/ );
        break;

    case ORGANIZE:
    case MOVE_TO_COLLECTION:
    case COPY_TO_COLLECTION:
        {
            KURL::List list;

            for( QListViewItemIterator it( this, QListViewItemIterator::Selected );
                    it.current();
                    ++it )
            {
                PlaylistItem *i= static_cast<PlaylistItem*>(*it);
                KURL url = i->url();
                list << url;
            }
            bool organize = CollectionDB::instance()->isDirInCollection( item->url().directory() );
            bool move = menuItemId==MOVE_TO_COLLECTION;
            CollectionView::instance()->organizeFiles( list,
                    organize ? i18n( "Organize Files" ) : move ? i18n( "Move Tracks to Collection" ) : i18n( "Copy Tracks to Collection"),
                    !organize && !move );
        }
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
/// Misc Protected Methods
////////////////////////////////////////////////////////////////////////////////

void
Playlist::fontChange( const QFont &old )
{
    KListView::fontChange( old );
    initStarPixmaps();
    triggerUpdate();
}

void
Playlist::contentsMouseMoveEvent( QMouseEvent *e )
{
    if( e )
        KListView::contentsMouseMoveEvent( e );
    PlaylistItem *prev = m_hoveredRating;
    const QPoint pos = e ? e->pos() : viewportToContents( viewport()->mapFromGlobal( QCursor::pos() ) );

    PlaylistItem *item = static_cast<PlaylistItem*>( itemAt( contentsToViewport( pos ) ) );
    if( item && pos.x() > header()->sectionPos( PlaylistItem::Rating ) &&
        pos.x() < header()->sectionPos( PlaylistItem::Rating ) + header()->sectionSize( PlaylistItem::Rating ) )
    {
        m_hoveredRating = item;
        m_hoveredRating->updateColumn( PlaylistItem::Rating );
    }
    else
        m_hoveredRating = 0;

    if( prev )
    {
        if( m_selCount > 1 && prev->isSelected() )
            QScrollView::updateContents( header()->sectionPos( PlaylistItem::Rating ) + 1, contentsY(),
                                         header()->sectionSize( PlaylistItem::Rating ) - 2, visibleHeight() );
        else
            prev->updateColumn( PlaylistItem::Rating );
    }
}

void Playlist::leaveEvent( QEvent *e )
{
    KListView::leaveEvent( e );

    PlaylistItem *prev = m_hoveredRating;
    m_hoveredRating = 0;
    if( prev )
        prev->updateColumn( PlaylistItem::Rating );
}

void Playlist::contentsMousePressEvent( QMouseEvent *e )
{
    PlaylistItem *item = static_cast<PlaylistItem*>( itemAt( contentsToViewport( e->pos() ) ) );

    int beginRatingSection = header()->sectionPos( PlaylistItem::Rating );
    int endRatingSection   = beginRatingSection + header()->sectionSize( PlaylistItem::Rating );

    /// Conditions on setting the rating of an item
    if( item &&
       !( e->state()  & Qt::ControlButton || e->state() & Qt::ShiftButton ) &&     // skip if ctrl or shift held
        ( e->button() & Qt::LeftButton )                                    &&     // only on a left click
        ( e->pos().x() > beginRatingSection && e->pos().x() < endRatingSection ) ) // mouse over rating column
    {
        int rating = item->ratingAtPoint( e->pos().x() );

        if( item->isSelected() )
            setSelectedRatings( rating );
        else // toggle half star
            CollectionDB::instance()->setSongRating( item->url().path(), rating, true );
    }
    else
        KListView::contentsMousePressEvent( e );
}

void Playlist::contentsWheelEvent( QWheelEvent *e )
{
    PlaylistItem* const item = static_cast<PlaylistItem*>( itemAt( contentsToViewport( e->pos() ) ) );
    const int column = header()->sectionAt( e->pos().x() );
    const int distance = header()->sectionPos( column ) + header()->sectionSize( column ) - e->pos().x();
    const int maxdistance = fontMetrics().width( QString::number( m_nextTracks.count() ) ) + 7;
    if( item && column == m_firstColumn && distance <= maxdistance && item->isQueued() )
    {
        const int n = e->delta() / 120,
                  s = n / abs(n),
                pos = item->queuePosition();
        PLItemList changed;
        for( int i = 1; i <= abs(n); ++i )
        {
            const int dest = pos + s*i;
            if( kClamp( dest, 0, int( m_nextTracks.count() ) - 1 ) != dest )
                break;
            PlaylistItem* const p = m_nextTracks.at( dest );
            if( changed.findRef( p ) == -1 )
                changed << p;
            if( changed.findRef( m_nextTracks.at( dest - s ) ) == -1 )
                changed << m_nextTracks.at( dest - s );
            m_nextTracks.replace( dest, m_nextTracks.at( dest - s ) );
            m_nextTracks.replace( dest - s, p );
        }

        for( int i = 0, n = changed.count(); i < n; ++i )
            changed.at(i)->update();
    }
    else
        KListView::contentsWheelEvent( e );
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
Playlist::numVisibleColumns() const
{
    int r = 0, i = 1;
    for( const int n = columns(); i <= n; ++i)
        if( columnWidth( i - 1 ) )
            ++r;
    return r;
}

QValueList<int> Playlist::visibleColumns() const
{
    QValueList<int> r;
    for( int i = 0, n = columns(); i < n; ++i)
        if( columnWidth( i ) )
            r.append( i );
    return r;
}

MetaBundle::ColumnMask Playlist::getVisibleColumnMask() const {
    MetaBundle::ColumnMask mask = 0;
    for( int i = 0, n = columns(); i < n; ++i)
        if( columnWidth( i ) ) mask = mask | (1 << i);
    return mask;
}


int
Playlist::mapToLogicalColumn( int physical ) const
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
Playlist::setColumns( QValueList<int> order, QValueList<int> visible )
{
    for( int i = order.count() - 1; i >= 0; --i )
        header()->moveSection( order[i], i );
    for( int i = 0; i < PlaylistItem::NUM_COLUMNS; ++i )
    {
        if( visible.contains( i ) )
            adjustColumn( i );
        else
            hideColumn( i );
    }
    columnOrderChanged();
}

void
Playlist::removeItem( PlaylistItem *item, bool multi )
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
            if( next )
            {
                m_nextTracks.append( next );
                next->update();
            }
        }
    }

    if( m_stopAfterTrack == item ) {
        m_stopAfterTrack = 0; //to be safe
        if (stopAfterMode() != StopAfterCurrent)
            setStopAfterMode( DoNotStop );
    }

    //keep m_nextTracks queue synchronized
    if( m_nextTracks.removeRef( item ) && !multi )
       emit queueChanged( PLItemList(), PLItemList( item ) );

    //keep recent buffer synchronized
    removeFromPreviousTracks( item ); //removes all pointers to item

    updateNextPrev();
}

void Playlist::ensureItemCentered( QListViewItem *item )
{
    if( !item )
        return;

    //HACK -- apparently the various metrics aren't reliable while the UI is still updating & stuff
    m_itemToReallyCenter = item;
    QTimer::singleShot( 0, this, SLOT( reallyEnsureItemCentered() ) );
}

void
Playlist::reallyEnsureItemCentered()
{
    if( QListViewItem *item = m_itemToReallyCenter )
    {
        m_itemToReallyCenter = 0;
        if( m_selCount == 1 )
        {
            PlaylistItem *previtem = *MyIt( this, MyIt::Selected );
            if( previtem && previtem != item )
                previtem->setSelected( false );
        }
        setCurrentItem( item );
        ensureVisible( contentsX(), item->itemPos() + item->height() / 2, 0, visibleHeight() / 2 );
        triggerUpdate();
    }
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
        item->update();
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
      fileName.prepend( m_undoDir.absPath() + '/' );
      fileName.append( ".xml" );

      if ( list.count() >= (uint)AmarokConfig::undoLevels() )
      {
         m_undoDir.remove( list.first() );
         list.pop_front();
      }

      saveXML( fileName );
      list.append( fileName );

      // Reset isNew state of all items in the playlist (determines font coloring)
      PlaylistItem* item = static_cast<PlaylistItem*>( firstChild() );
      while( item ) {
         item->setIsNew( false );
         item = item->nextSibling();
      }
      triggerUpdate();

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

    //this is clear() minus some parts, for instance we don't want to cause a saveUndoState() here
    m_currentTrack = 0;
    disableDynamicMode();
    Glow::reset();
    m_prevTracks.clear();
    m_prevAlbums.clear();
    const PLItemList prev = m_nextTracks;
    m_nextTracks.clear();
    emit queueChanged( PLItemList(), prev );
    ThreadManager::instance()->abortAllJobsNamed( "TagWriter" );
    safeClear();
    m_total = 0;
    m_albums.clear();

    insertMediaInternal( url, 0, 0 ); //because the listview is empty, undoState won't be forced

    m_undoButton->setEnabled( !m_undoList.isEmpty() );
    m_redoButton->setEnabled( !m_redoList.isEmpty() );

    if( dynamicMode() ) setDynamicHistory( true );
    m_undoDirt = false;
}

void
Playlist::saveSelectedAsPlaylist()
{
    MyIt it( this, MyIt::Visible | MyIt::Selected );
    if( !(*it) )
        return; //safety
    const QString album = (*it)->album(),
                  artist = (*it)->artist();
    int suggestion = !album.stripWhiteSpace().isEmpty() ? 1 : !artist.stripWhiteSpace().isEmpty() ? 2 : 3;
    while( *it )
    {
        if( suggestion == 1 && (*it)->album()->lower().stripWhiteSpace() != album.lower().stripWhiteSpace() )
            suggestion = 2;
        if( suggestion == 2 && (*it)->artist()->lower().stripWhiteSpace() != artist.lower().stripWhiteSpace() )
            suggestion = 3;
        if( suggestion == 3 )
            break;
        ++it;
    }
    QString path = PlaylistDialog::getSaveFileName( suggestion == 1 ? album
                                                  : suggestion == 2 ? artist
                                                  : i18n( "Untitled" ) );

    if( path.isEmpty() )
        return;

    QValueList<KURL> urls;
    QValueList<QString> titles;
    QValueList<int> lengths;
    for( it = MyIt( this, MyIt::Visible | MyIt::Selected ); *it; ++it )
    {
        urls << (*it)->url();
        titles << (*it)->title();
        lengths << (*it)->length();
    }

    if( PlaylistBrowser::savePlaylist( path, urls, titles, lengths ) )
        PlaylistWindow::self()->showBrowser( "PlaylistBrowser" );
}

void Playlist::initStarPixmaps()
{
    StarManager::instance()->reinitStars( fontMetrics().height(), itemMargin() );
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

        if( url.isValid() )
            insertMediaInternal( url, static_cast<PlaylistItem*>(after ? after : lastItem()) );

        break;
    }

    case Qt::RightButton:
        showContextMenu( after, p, col );
        break;

    default:
        ;
    }
}

void Playlist::slotContentsMoving()
{
    Amarok::ToolTip::hideTips();
    QTimer::singleShot( 0, this, SLOT( contentsMouseMoveEvent() ) );
}

void
Playlist::slotQueueChanged( const PLItemList &/*in*/, const PLItemList &out)
{
    for( QPtrListIterator<PlaylistItem> it( out ); *it; ++it )
        (*it)->update();
    refreshNextTracks( 0 );
    updateNextPrev();
}

void
Playlist::slotUseScores( bool use )
{
    if( !use && columnWidth( MetaBundle::Score ) )
        hideColumn( MetaBundle::Score );
}

void
Playlist::slotUseRatings( bool use )
{
    if( use && !columnWidth( MetaBundle::Rating ) )
        adjustColumn( MetaBundle::Rating );
    else if( !use && columnWidth( MetaBundle::Rating ) )
        hideColumn( MetaBundle::Rating );
}


// This gets called when the user presses "Ok" or "Apply" in the
// config dialog.
void
Playlist::slotMoodbarPrefs( bool show, bool moodier, int alter, bool withMusic )
{
    (void) moodier;  (void) alter;  (void) withMusic;

    if( !show && columnWidth( MetaBundle::Mood ) )
        hideColumn( MetaBundle::Mood );

    // Reset all of our moodbars, since they may have been permanently
    // disabled before because the Moodbar was disabled.  We need to
    // do this even if the column is hidden.
    if( show )
      {
        // No need to call moodbar().load(), since that will happen
        // automatically next time it's displayed.  We do have to
        // repaint so that they get displayed though.

        for( PlaylistIterator it( this, PlaylistIterator::All ) ; *it ; ++it )
          {
            (*it)->moodbar().reset();
            repaintItem(*it);
          }
      }
}

void
Playlist::slotGlowTimer() //SLOT
{
    if( !currentTrack() ) return;

    using namespace Glow;

    if( counter <= STEPS*2 )
    {
        // 0 -> STEPS -> 0
        const double d = (counter > STEPS) ? 2*STEPS-counter : counter;

        {
            using namespace Base;
            PlaylistItem::glowIntensity = d;
            PlaylistItem::glowBase = QColor( r, g, b );
        }

        {
            using namespace Text;
            PlaylistItem::glowText = QColor( r + int(d*dr), g + int(d*dg), b + int(d*db) );
        }

        if( currentTrack() )
            currentTrack()->update();
    }

    ++counter &= 63; //built in bounds checking with &=
}

void
Playlist::slotRepeatTrackToggled( int /* mode */ )
{
    if( m_currentTrack )
        m_currentTrack->update();
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
    /// the tag dialog was once modal, because we thought that damage would occur
    /// when passing playlist items into the editor and it was removed from the playlist.
    /// This is simply not the case, information is written to the URL, not the item.

    // Playlist::lock();

    if( items.isEmpty() ) return;

    if ( items.count() == 1 )
    {
        PlaylistItem *item = static_cast<PlaylistItem*>( items.first() );
        bool isDaap        = item->url().protocol() == "daap";
        if ( !item->url().isLocalFile() && !isDaap )
        {
            StreamEditor dialog( this, item->title(), item->url().prettyURL(), true );
            if( item->url().protocol() == "cdda" )
                dialog.setCaption( i18n( "CD Audio" ) );
            else
                dialog.setCaption( i18n( "Remote Media" ) );
            dialog.exec();
        }
        else if ( isDaap ) // don't check if exists
        {
            // The tag dialog automatically disables the widgets if the file is not local, which it is not.
            TagDialog *dialog = new TagDialog( *item, item, instance() );
            dialog->show();
        }
        else if ( checkFileStatus( item ) )
        {
            TagDialog *dialog = new TagDialog( *item, item, instance() );
            dialog->show();
        }
        else
            KMessageBox::sorry( this, i18n( "This file does not exist:" ) + ' ' + item->url().path() );
    }
    else {
        //edit multiple tracks in tag dialog
        KURL::List urls;
        for( QListViewItem *item = items.first(); item; item = items.next() )
            if ( item->isVisible() )
                urls << static_cast<PlaylistItem*>( item )->url();

        TagDialog *dialog = new TagDialog( urls, instance() );
        dialog->show();
    }

    // Playlist::unlock();
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

            (*it)->setExactText( index, p.readStdout() );
        }
    }
}

#include <taglib/fileref.h>
#include <taglib/tag.h>

TagWriter::TagWriter( PlaylistItem *item, const QString &oldTag, const QString &newTag, const int col, const bool updateView )
        : ThreadManager::Job( "TagWriter" )
        , m_item( item )
        , m_failed( true )
        , m_oldTagString( oldTag )
        , m_newTagString( newTag )
        , m_tagType( col )
        , m_updateView( updateView )
{
    Playlist::instance()->lock();

    item->setEditing( col );
}

TagWriter::~TagWriter()
{
    Playlist::instance()->unlock();
}

bool
TagWriter::doJob()
{
    MetaBundle mb( m_item->url(), true );

    switch ( m_tagType )
    {
        case PlaylistItem::Title:
            mb.setTitle( m_newTagString );
            break;
        case PlaylistItem::Artist:
            mb.setArtist( m_newTagString );
            break;
        case PlaylistItem::Composer:
            if ( !mb.hasExtendedMetaInformation() )
                return true;
            mb.setComposer( m_newTagString );
            break;
        case PlaylistItem::DiscNumber:
            if ( !mb.hasExtendedMetaInformation() )
                return true;
            mb.setDiscNumber( m_newTagString.toInt() );
            break;
        case PlaylistItem::Bpm:
            if ( !mb.hasExtendedMetaInformation() )
                return true;
            mb.setBpm( m_newTagString.toFloat() );
            break;
        case PlaylistItem::Album:
            mb.setAlbum( m_newTagString );
            break;
        case PlaylistItem::Year:
            mb.setYear( m_newTagString.toInt() );
            break;
        case PlaylistItem::Comment:
            //FIXME how does this work for vorbis files?
            //Are we likely to overwrite some other comments?
            //Vorbis can have multiple comment fields..
            mb.setComment( m_newTagString );
            break;
        case PlaylistItem::Genre:
            mb.setGenre( m_newTagString );
            break;
        case PlaylistItem::Track:
            mb.setTrack( m_newTagString.toInt() );
            break;

        default:
            return true;
    }

    m_failed = !mb.save();
    return true;
}

void
TagWriter::completeJob()
{
    switch( m_failed ) {
    case true:
        // we write a space for some reason I cannot recall
        m_item->setExactText( m_tagType, m_oldTagString.isEmpty() ? " " : m_oldTagString );
        Amarok::StatusBar::instance()->longMessage( i18n(
                "Sorry, the tag for %1 could not be changed." ).arg( m_item->url().fileName() ), KDE::StatusBar::Sorry );
        break;

    case false:
        m_item->setExactText( m_tagType, m_newTagString.isEmpty() ? " " : m_newTagString );
        CollectionDB::instance()->updateURL( m_item->url().path(), m_updateView );

    }
    m_item->setIsBeingRenamed( false );
    m_item->filter( Playlist::instance()->m_filter );
    if( m_item->deleteAfterEditing() )
    {
        Playlist::instance()->removeItem( m_item );
        delete m_item;
    }
}


#include "playlist.moc"
