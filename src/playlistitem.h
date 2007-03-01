//Maintainer: Max Howell <max.howell@methylblue.com>
//Copyright:  GPL v2

//NOTE please show restraint when adding data members to this class!
//     some users have playlists with 20,000 items or more in, one 32 bit int adds up rapidly!
//     -- on second thought, 80KB isn't all that much. be careful with QStrings, though.


#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include "metabundle.h" //baseclass
#include "amarok_export.h"

#include <klistview.h> //baseclass
#include <kurl.h>      //stack allocated

#include <qcolor.h>    //stack allocated
#include <qfont.h>     //stack allocated
#include <qmap.h>
#include <qpixmap.h>
#include <qvaluevector.h>

class QColorGroup;
class QDomNode;
class QImage;
class QListViewItem;
class QPainter;
class MetaBundle;
class Playlist;
class PlaylistAlbum;

class LIBAMAROK_EXPORT PlaylistItem : public MetaBundle, public KListViewItem
{
    typedef MetaBundle super;
    public:
        /// Indicates that the current-track pixmap has changed. Animation must be redrawn.
        static void setPixmapChanged() { s_pixmapChanged = true; }

        /// For the glow colouration stuff
        static double glowIntensity;
        static QColor glowText;
        static QColor glowBase;

    public:
        PlaylistItem( QListView*, QListViewItem* ); //used by PlaylistLoader
        PlaylistItem( const MetaBundle&, QListViewItem*, bool enabled = true );
        ~PlaylistItem();

        /// pass 'raw' data here, for example "92" for Length, and not "1:32"
        virtual void setText( int column, const QString& );

        /**
        * @return The text of the column @p column, formatted for display purposes.
        * (For example, if the Length is 92, "1:32".)
        */
        virtual QString text( int column ) const;

        void filter( const QString &expression ); //makes visible depending on whether it matches

        bool isCurrent() const;

        bool isQueued() const;
        int queuePosition() const;

        bool isEnabled() const { return m_enabled; }
        bool isDynamicEnabled() const { return m_dynamicEnabled; }
        bool isFilestatusEnabled() const { return m_filestatusEnabled; }
        void setEnabled();
        void setDynamicEnabled( bool enabled );
        void setFilestatusEnabled( bool enabled );
        void setAllCriteriaEnabled( bool enabled );

        void setSelected( bool selected );
        void setVisible( bool visible );

        void setEditing( int column );
        bool isEditing( int column ) const;
        bool anyEditing() const;
        void setIsBeingRenamed( bool renaming ) { m_isBeingRenamed = renaming; }
        bool isBeingRenamed() const { return m_isBeingRenamed; }
        void setDeleteAfterEditing( bool dae ) { m_deleteAfterEdit = dae; }
        bool deleteAfterEditing() const { return m_deleteAfterEdit; }
        void setIsNew( bool is ) { m_isNew = is; }

        /// convenience functions
        Playlist *listView() const { return reinterpret_cast<Playlist*>( KListViewItem::listView() ); }
        PlaylistItem *nextSibling() const { return static_cast<PlaylistItem*>( KListViewItem::nextSibling() ); }

        static int ratingAtPoint( int x );
        static int ratingColumnWidth();

        /// like QWidget::update()
        void update() const;

        //updates only the area of a specific column, avoids flickering of the current item marker
        void updateColumn( int column ) const;

        virtual void setup(); // from QListViewItem

        virtual bool operator== ( const PlaylistItem & item ) const;
        virtual bool operator< ( const PlaylistItem & item ) const;

        PlaylistItem *nextInAlbum() const;
        PlaylistItem *prevInAlbum() const;

    protected:
        virtual void aboutToChange( const QValueList<int> &columns );
        virtual void reactToChanges( const QValueList<int> &columns );

    private:
        friend class Playlist;


        struct paintCacheItem {
            int width;
            int height;
            QString text;
            QFont font;
            QColor color;
            bool selected;
            QMap<QString, QPixmap> map;
        };

        virtual void paintCell( QPainter*, const QColorGroup&, int, int, int );
        void drawRating( QPainter *p );
        void drawRating( QPainter *p, int stars, int greystars, bool half );
        void drawMood( QPainter *p, int width, int height );
        virtual void moodbarJobEvent( int newState );

        // Used for sorting
        virtual int compare( QListViewItem*, int, bool ) const;

        /**
        * Paints a focus indicator on the rectangle (current item). We disable it
        * over the currentTrack, cause it would look like crap and flicker.
        */
        void paintFocus( QPainter*, const QColorGroup&, const QRect& );

        static void imageTransparency( QImage& image, float factor );

        AtomicString artist_album() const; // returns a placeholder 'artist' for compilations

        void refAlbum();
        void derefAlbum();

        void decrementTotals();
        void incrementTotals();

        void incrementCounts();
        void decrementCounts();
        void incrementLengths();
        void decrementLengths();

        int totalIncrementAmount() const;

        PlaylistAlbum *m_album;
        bool m_enabled;
        bool m_dynamicEnabled;
        bool m_filestatusEnabled;
        bool m_deleteAfterEdit;
        bool m_isBeingRenamed;
        bool m_isNew; //New items will be assigned a different color

        static bool s_pixmapChanged;
        static const QString &editingText();
};

class PLItemList: public QPtrList<PlaylistItem>
{
    public:
        PLItemList() : QPtrList<PlaylistItem>() { }
        PLItemList( const QPtrList<PlaylistItem> &list ) : QPtrList<PlaylistItem>( list ) { }
        PLItemList( PlaylistItem *item ) : QPtrList<PlaylistItem>() { append( item ); }

        inline PLItemList &operator<<( PlaylistItem *item ) { append( item ); return *this; }
};


#endif
