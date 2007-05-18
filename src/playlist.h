/***************************************************************************
                        Playlist.h  -  description
                            -------------------
    begin                : Don Dez 5 2002
    copyright            : (C) 2002 by Mark Kretschmann
                           (C) 2005 Ian Monroe
                           (C) 2005 by Gábor Lehel
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_PLAYLIST_H
#define AMAROK_PLAYLIST_H

#include "config-amarok.h"
#include "amarok_export.h"
#include "amarokconfig.h"
#include "engineobserver.h"  //baseclass
#include "dynamicmode.h"
#include "playlistwindow.h"  //friend
#include "playlistitem.h"
#include "metabundle.h"
#include "tooltip.h"         //baseclass
#include "tracktooltip.h"

#include <k3listview.h>       //baseclass
#include <kurl.h>            //KUrl::List
#include <QDir>            //stack allocated
#include <qpoint.h>          //stack allocated
#include <q3ptrlist.h>        //stack allocated
#include <QStringList>     //stack allocated
//Added by qt3to4:
#include <QDragLeaveEvent>
#include <Q3ValueList>
#include <QLabel>
#include <QCustomEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QPaintEvent>
#include <QWheelEvent>
#include <vector>            //stack allocated

class KAction;
class KActionCollection;
class MyAtomicString;
class PlaylistItem;
class PlaylistEntry;
class PlaylistLoader;
class PlaylistAlbum;
class TagWriter;
class Q3BoxLayout;
class QLabel;
class QTimer;

class Medium;

/**
 * @authors Mark Kretschmann && Max Howell
 *
 * Playlist inherits K3ListView privately and thus is no longer a ListView
 * Instead it is a part of PlaylistWindow and they interact in harmony. The change
 * was necessary as it is too dangerous to allow public access to PlaylistItems
 * due to the multi-threading environment.
 *
 * Unfortunately, since QObject is now inaccessible you have to connect slots
 * via one of PlaylistWindow's friend members or in Playlist
 *
 * If you want to add new playlist type functionality you should implement it
 * inside this class or inside PlaylistWindow.
 *
 */


class Playlist : private K3ListView, public EngineObserver, public Amarok::ToolTipClient
{
        Q_OBJECT

    public:
        ~Playlist();

        AMAROK_EXPORT static Playlist *instance() { return s_instance; }
        static QString defaultPlaylistPath();
        static const int NO_SORT = 200;

        static const int Append     = 1;     /// inserts media after the last item in the playlist
        static const int Queue      = 2;     /// inserts media after the currentTrack
        static const int Clear      = 4;     /// clears the playlist first
        static const int Replace    = Clear;
        static const int DirectPlay = 8;     /// start playback of the first item in the list
        static const int Unique     = 16;    /// don't insert anything already in the playlist
        static const int StartPlay  = 32;    /// start playback of the first item in the list if nothing else playing
        static const int Colorize   = 64;    /// colorize newly added items
        static const int DefaultOptions = Append | Unique | StartPlay;

        // it's really just the *ListView parts we want to hide...
        Q3ScrollView *qscrollview() const
        {
            return reinterpret_cast<Q3ScrollView*>( const_cast<Playlist*>( this ) );
        }

        /** Add media to the playlist
         *  @param options you can OR these together, see the enum
         *  @param sql     Sql program to execute */
        AMAROK_EXPORT void insertMedia( KUrl::List, int options = Append );
        void insertMediaSql( const QString& sql, int options = Append );

        // Dynamic mode functions
        void addDynamicModeTracks( uint songCount );
        void adjustDynamicUpcoming( bool saveUndo = false );
        void adjustDynamicPrevious( uint songCount, bool saveUndo = false );
        void advanceDynamicTrack();
        void setDynamicHistory( bool enable = true );

        void burnPlaylist      ( int projectType = -1 );
        void burnSelectedTracks( int projectType = -1 );
        int  currentTrackIndex( bool onlyCountVisible = true );
        bool isEmpty()       const  { return childCount() == 0; }
        AMAROK_EXPORT bool isTrackBefore() const;
        AMAROK_EXPORT bool isTrackAfter()  const;
        void restoreSession();          // called during initialisation
        void setPlaylistName( const QString &name, bool proposeOverwriting = false ) { m_playlistName = name; m_proposeOverwriting = proposeOverwriting; }
        void proposePlaylistName( const QString &name, bool proposeOverwriting = false ) { if( isEmpty() || m_playlistName==i18n("Untitled") ) m_playlistName = name; m_proposeOverwriting = proposeOverwriting; }
        const QString &playlistName() const { return m_playlistName; }
        bool proposeOverwriteOnSave() const { return m_proposeOverwriting; }
        bool saveM3U( const QString&, bool relative = AmarokConfig::relativePlaylist() ) const;
        void saveXML( const QString& );
        int  totalTrackCount() const;
        BundleList nextTracks() const;
        uint repeatAlbumTrackCount() const;    //returns number of tracks from same album
        //as current track that are in playlist (may require Play Albums in Order on).
        //If the information is not available, returns 0.

        //const so you don't change it behind Playlist's back, use modifyDynamicMode() for that
        const DynamicMode *dynamicMode() const;

        //modify the returned DynamicMode, then finishedModifying() it when done
        DynamicMode *modifyDynamicMode();

        //call this every time you modifyDynamicMode(), otherwise you'll get memory leaks and/or crashes
        void finishedModifying( DynamicMode *mode );

        int  stopAfterMode();

        void addCustomMenuItem ( const QString &submenu, const QString &itemTitle );
        void customMenuClicked ( int id );
        bool removeCustomMenuItem( const QString &submenu, const QString &itemTitle );

        void setFont( const QFont &f ) { K3ListView::setFont( f ); } //made public for convenience
        void unsetFont()               { K3ListView::setFont(QFont()); }

        PlaylistItem *firstChild() const { return static_cast<PlaylistItem*>( K3ListView::firstChild() ); }
        PlaylistItem *lastItem()   const { return static_cast<PlaylistItem*>( K3ListView::lastItem() ); }
        PlaylistItem *currentItem() const { return static_cast<PlaylistItem*>( K3ListView::currentItem() ); }

        int  numVisibleColumns() const;
        Q3ValueList<int> visibleColumns() const;
        MetaBundle::ColumnMask getVisibleColumnMask() const;
        int  mapToLogicalColumn( int physical ) const; // Converts physical PlaylistItem column position to logical
        QString columnText( int c ) const { return K3ListView::columnText( c ); };
        void setColumns( Q3ValueList<int> order, Q3ValueList<int> visible );

        /** Call this to prevent items being removed from the playlist, it is mostly for internal use only
         *  Don't forget to unlock() !! */
        void lock();
        void unlock();

        //reimplemented to save columns by name instead of index, to be more resilient to reorderings and such
        void saveLayout(KSharedConfigPtr config, const QString &group) const;
        void restoreLayout(KSharedConfigPtr config, const QString &group);

        //AFT-related functions
        bool checkFileStatus( PlaylistItem * item );
        void addToUniqueMap( const QString uniqueid, PlaylistItem* item );
        void removeFromUniqueMap( const QString uniqueid, PlaylistItem* item );

        enum RequestType { Prev = -1, Current = 0, Next = 1 };
        enum StopAfterMode { DoNotStop, StopAfterCurrent, StopAfterQueue, StopAfterOther };

        class Q3DragObject *dragObject();
        friend class PlaylistItem;
        friend class UrlLoader;
        friend class QueueManager;
        friend class QueueLabel;
        friend class PlaylistWindow;
        friend class ColumnList;
//        friend void Amarok::DcopPlaylistHandler::removeCurrentTrack(); //calls removeItem() and currentTrack()
//        friend void Amarok::DcopPlaylistHandler::removeByIndex( int ); //calls removeItem()
        friend class TagWriter; //calls removeItem()
        friend void PlaylistWindow::init(); //setting up connections etc.
        friend TrackToolTip::TrackToolTip();
        friend bool PlaylistWindow::eventFilter( QObject*, QEvent* ); //for convenience we handle some playlist events here

    public:
        QPair<QString, QRect> toolTipText( QWidget*, const QPoint &pos ) const;

    signals:
        void aboutToClear();
        void itemCountChanged( int newCount, int newLength, int visCount, int visLength, int selCount, int selLength );
        void queueChanged( const QList<PlaylistItem*> &queued, const QList<PlaylistItem*> &dequeued );
        void columnsChanged();
        void dynamicModeChanged( const DynamicMode *newMode );

    public slots:
        void activateByIndex(int);
        void addCustomColumn();
        void appendMedia( const KUrl &url );
        void appendMedia( const QString &path );
        void clear();
        void copyToClipboard( const Q3ListViewItem* = 0 ) const;
        void deleteSelectedFiles();
        void ensureItemCentered( Q3ListViewItem* item );
        void playCurrentTrack();
        void playNextTrack( const bool forceNext = true );
        void playPrevTrack();
        void queueSelected();
        void setSelectedRatings( int rating );
        void redo();
        void removeDuplicates();
        void removeSelectedItems();
        void setDynamicMode( DynamicMode *mode );
        void loadDynamicMode( DynamicMode *mode ); //saveUndoState() + setDynamicMode()
        void disableDynamicMode();
        void editActiveDynamicMode();
        void rebuildDynamicModeCache();
        void repopulate();
        void scoreChanged( const QString &path, float score );
        void ratingChanged( const QString &path, int rating );
        void fileMoved( const QString &srcPath, const QString &dstPath );
        void selectAll() { Q3ListView::selectAll( true ); }
        void setFilter( const QString &filter );
        void setFilterSlot( const QString &filter );                       //uses a delay where applicable
        void setStopAfterCurrent( bool on );
        void setStopAfterItem( PlaylistItem *item );
        void toggleStopAfterCurrentItem();
        void toggleStopAfterCurrentTrack();
        void setStopAfterMode( int mode );
        void showCurrentTrack() { ensureItemCentered( m_currentTrack ); }
        void showQueueManager();
        void changeFromQueueManager( QList<PlaylistItem*> list );
        void shuffle();
        void undo();
        void updateMetaData( const MetaBundle& );
        void adjustColumn( int n );
        void updateEntriesUrl( const QString &oldUrl, const QString &newUrl, const QString &uniqueid );
        void updateEntriesUniqueId( const QString &url, const QString &oldid, const QString &newid );
        void updateEntriesStatusDeleted( const QString &absPath, const QString &uniqueid );
        void updateEntriesStatusAdded( const QString &absPath, const QString &uniqueid );
        void updateEntriesStatusAdded( const QMap<QString,QString> &map );

    protected:
        virtual void fontChange( const QFont &old );

    protected slots:
        void contentsMouseMoveEvent( QMouseEvent *e = 0 );
        void leaveEvent( QEvent *e );
        void contentsMousePressEvent( QMouseEvent *e );
        void contentsWheelEvent( QWheelEvent *e );

    private slots:
        void mediumChange( int );
        void slotCountChanged();
        void activate( Q3ListViewItem* );
        void columnOrderChanged();
        void columnResizeEvent( int, int, int );
        void doubleClicked( Q3ListViewItem* );

        void generateInfo(); //generates info for Random Albums

        /* the only difference multi makes is whether it emits queueChanged(). (if multi, then no)
           if you're queue()ing many items, consider passing true and emitting queueChanged() yourself. */
        /* if invertQueue then queueing an already queued song dequeues it */
        void queue( Q3ListViewItem*, bool multi = false, bool invertQueue = true );

        void saveUndoState();
        void setDelayedFilter();                                           //after the delay is over
        void showContextMenu( Q3ListViewItem*, const QPoint&, int );
        void slotEraseMarker();
        void slotGlowTimer();
        void reallyEnsureItemCentered();
        void slotMouseButtonPressed( int, Q3ListViewItem*, const QPoint&, int );
        void slotSingleClick();
        void slotContentsMoving();
        void slotRepeatTrackToggled( int mode );
        void slotQueueChanged( const QList<PlaylistItem*> &in, const QList<PlaylistItem*> &out);
        void slotUseScores( bool use );
        void slotUseRatings( bool use );
        void slotMoodbarPrefs( bool show, bool moodier, int alter, bool withMusic );
        void updateNextPrev();
        void writeTag( Q3ListViewItem*, const QString&, int );

    private:
        Playlist( QWidget* );
        Playlist( const Playlist& ); //not defined

        AMAROK_EXPORT static Playlist *s_instance;

        void countChanged();

        PlaylistItem *currentTrack() const { return m_currentTrack; }
        PlaylistItem *restoreCurrentTrack();

        void insertMediaInternal( const KUrl::List&, PlaylistItem*, int options = 0 );
        bool isAdvancedQuery( const QString &query );
        void refreshNextTracks( int = -1 );
        void removeItem( PlaylistItem*, bool = false );
        bool saveState( QStringList& );
        void setCurrentTrack( PlaylistItem* );
        void setCurrentTrackPixmap( int state = -1 );
        void showTagDialog( QList<Q3ListViewItem*> items );
        void sortQueuedItems();
        void switchState( QStringList&, QStringList& );
        void saveSelectedAsPlaylist();
        void initStarPixmaps();

        //engine observer functions
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( Engine::State, Engine::State = Engine::Empty );

        /// K3ListView Overloaded functions
        void contentsDropEvent     ( QDropEvent* );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDragMoveEvent ( QDragMoveEvent* );
        void contentsDragLeaveEvent( QDragLeaveEvent* );

        #ifdef PURIST //K3ListView imposes hand cursor so override it
        void contentsMouseMoveEvent( QMouseEvent *e ) { Q3ListView::contentsMouseMoveEvent( e ); }
        #endif

        void customEvent( QEvent* );
        bool eventFilter( QObject*, QEvent* );
        void paletteChange( const QPalette& );
        void rename( Q3ListViewItem*, int );
        void setColumnWidth( int, int );
        void setSorting( int, bool = true );

        void viewportPaintEvent( QPaintEvent* );
        void viewportResizeEvent( QResizeEvent* );

        void appendToPreviousTracks( PlaylistItem *item );
        void appendToPreviousAlbums( PlaylistAlbum *album );
        void removeFromPreviousTracks( PlaylistItem *item = 0 );
        void removeFromPreviousAlbums( PlaylistAlbum *album = 0 );

        typedef QMap<MyAtomicString, PlaylistAlbum*> AlbumMap;
        typedef QMap<MyAtomicString, AlbumMap> ArtistAlbumMap;
        ArtistAlbumMap m_albums;
        uint m_startupTime_t; //QDateTime::currentDateTime().toTime_t as of startup
        uint m_oldestTime_t; //the createdate of the oldest song in the collection


        /// ATTRIBUTES

        PlaylistItem  *m_currentTrack;          //the track that is playing
        Q3ListViewItem *m_marker;                //track that has the drag/drop marker under it
        PlaylistItem  *m_hoveredRating;         //if the mouse is hovering over the rating of an item

        //NOTE these container types were carefully chosen
        Q3PtrList<PlaylistAlbum> m_prevAlbums; //the previously played albums in Entire Albums mode
        PLItemList m_prevTracks;    //the previous history
        QList<PlaylistItem*> m_nextTracks;    //the tracks to be played after the current track

        QString m_filter;
        QString m_prevfilter;
        QTimer *m_filtertimer;

        PLItemList m_itemsToChangeTagsFor;

        bool          m_smartResizing;

        int           m_firstColumn;
        int           m_totalCount;
        int           m_totalLength;
        int           m_selCount;
        int           m_selLength;
        int           m_visCount;
        int           m_visLength;
        qint64       m_total; //for Favor Tracks
        bool          m_itemCountDirty;

        KAction      *m_undoButton;
        KAction      *m_redoButton;
        KAction      *m_clearButton;

        QDir          m_undoDir;
        QStringList   m_undoList;
        QStringList   m_redoList;
        uint          m_undoCounter;

        DynamicMode  *m_dynamicMode;
        KUrl::List    m_queueList;
        PlaylistItem *m_stopAfterTrack;
        int           m_stopAfterMode;
        bool          m_showHelp;
        bool          m_dynamicDirt;        //So we don't call advanceDynamicTrack() on activate()
        bool          m_queueDirt;          //When queuing disabled items, we need to place the marker on the newly inserted item
        bool          m_undoDirt;           //Make sure we don't repopulate the playlist when dynamic mode and undo()
        int           m_insertFromADT;      //Don't automatically start playing if a user hits Next in dynamic mode when not already playing
        static QMutex *s_dynamicADTMutex;

        Q3ListViewItem *m_itemToReallyCenter;
        Q3ListViewItem *m_renameItem;
        int            m_renameColumn;
        QTimer        *m_clicktimer;
        Q3ListViewItem *m_itemToRename;
        QPoint         m_clickPos;
        int            m_columnToRename;

        QMap<QString, QStringList> m_customSubmenuItem;
        QMap<int, QString>         m_customIdItem;

        bool isLocked() const { return m_lockStack > 0; }

        /// stack counter for PLaylist::lock() and unlock()
        int m_lockStack;

        QString m_editOldTag; //text before inline editing ( the new tag is written only if it's changed )

        std::vector<double> m_columnFraction;

        QMap<QString,Q3PtrList<PlaylistItem>*> m_uniqueMap;
        int m_oldRandom;
        int m_oldRepeat;

        QString m_playlistName;
        bool m_proposeOverwriting;
};

class MyAtomicString: public AtomicString
{
public:
    MyAtomicString() { }
    MyAtomicString(const QString &string): AtomicString( string ) { }
    MyAtomicString(const AtomicString &other): AtomicString( other ) { }
    bool operator<(const AtomicString &other) const { return ptr() < other.ptr(); }
};

class PlaylistAlbum
{
public:
    PLItemList tracks;
    int refcount;
    qint64 total; //for Favor Tracks
    PlaylistAlbum(): refcount( 0 ), total( 0 ) { }
};

/**
 * Iterator class that only edits visible items! Preferentially always use
 * this! Invisible items should not be operated on! To iterate over all
 * items use MyIt::All as the flags parameter. MyIt::All cannot be OR'd,
 * sorry.
 */

class PlaylistIterator : public Q3ListViewItemIterator
{
public:
    explicit PlaylistIterator( Q3ListViewItem *item, int flags = 0 )
        //QListViewItemIterator is not great and doesn't allow you to see everything if you
        //mask both Visible and Invisible :( instead just visible items are returned
        : Q3ListViewItemIterator( item, flags == All ? 0 : flags | Visible  )
    {}

    explicit PlaylistIterator( Q3ListView *view, int flags = 0 )
        : Q3ListViewItemIterator( view, flags == All ? 0 : flags | Visible )
    {}

    //FIXME! Dirty hack for enabled/disabled items.
    enum IteratorFlag {
        Visible = Q3ListViewItemIterator::Visible,
        All = Q3ListViewItemIterator::Invisible
    };

    inline PlaylistItem *operator*() { return static_cast<PlaylistItem*>( Q3ListViewItemIterator::operator*() ); }

    /// @return the next visible PlaylistItem after item
    static PlaylistItem *nextVisible( PlaylistItem *item )
    {
        PlaylistIterator it( item );
        return (*it == item) ? *static_cast<PlaylistIterator&>(++it) : *it;
    }

    static PlaylistItem *prevVisible( PlaylistItem *item )
    {
        PlaylistIterator it( item );
        return (*it == item) ? *static_cast<PlaylistIterator&>(--it) : *it;
    }

};

#endif //AMAROK_PLAYLIST_H
