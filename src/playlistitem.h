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
#include <qmutex.h>
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
        class ReadMood;
        friend class ReadMood;

        /// Indicates that the current-track pixmap has changed. Animation must be redrawn.
        static void setPixmapChanged() { s_pixmapChanged = true; }

        /// For the glow colouration stuff
        static double glowIntensity;
        static QColor glowText;
        static QColor glowBase;

    public: //reimplemented to update()
    virtual void setTitle( const QString &title );
    virtual void setArtist( const AtomicString &artist );
    virtual void setComposer( const AtomicString &composer );
    virtual void setAlbum( const AtomicString &album );
    virtual void setComment( const AtomicString &comment );
    virtual void setGenre( const AtomicString &genre );
    virtual void setYear( int year );
    virtual void setDiscNumber( int discNumber );
    virtual void setTrack( int track );
    virtual void setLength( int length );
    virtual void setBitrate( int bitrate );
    virtual void setFilesize( int bitrate );
    virtual void setRating( int rating );
    virtual void setScore( int score );
    virtual void setPlayCount( int playcount );
    virtual void setLastPlay( uint lastplay );

    public:
        PlaylistItem( QListView*, QListViewItem* ); //used by PlaylistLoader
        PlaylistItem( const MetaBundle&, QListViewItem* );
        ~PlaylistItem();

        /// pass 'raw' data here, for example "92" for Length, and not "1:32"
        virtual void setText( int column, const QString& );

        /**
        * @return The text of the column @p column, formatted for display purposes.
        * (For example, if the Length is 92, "1:32".)
        */
        virtual QString text( int column ) const;

        void filter( const QString &expression ); //makes visible depending on whether it matches

        bool isQueued() const;
        int queuePosition() const;

        bool isEnabled() const { return m_enabled; }
        void setEnabled( bool enable );

        void setSelected( bool selected );
        void setVisible( bool visible );

        void setEditing( int column );
        bool isEditing( int column ) const;

        /// convenience functions
        Playlist *listView() const { return (Playlist*)KListViewItem::listView(); }
        PlaylistItem *nextSibling() const { return (PlaylistItem*)KListViewItem::nextSibling(); }

        void refreshMood();
        void checkMood();
        bool readMood();

        enum { DrawNormal, DrawGrayed };
        static QPixmap *star( int type = DrawNormal );
        static int ratingAtPoint( int x );

        /// like QWidget::update()
        void update() const;

        //updates only the area of a specific column, avoids flickering of the current item marker
        void updateColumn( int column ) const;

        virtual void setup(); // from QListViewItem

        virtual bool operator== ( const PlaylistItem & item ) const;
        virtual bool operator< ( const PlaylistItem & item ) const;

        PlaylistItem *nextInAlbum() const;
        PlaylistItem *prevInAlbum() const;

    private:
        friend class Playlist;


        struct paintCacheItem {
            int width;
            int height;
            QString text;
            QFont font;
            bool selected;
            QMap<QString, QPixmap> map;
        };

        void setArray(const QValueVector<QColor> array);

        virtual void paintCell( QPainter*, const QColorGroup&, int, int, int );
        void drawRating( QPainter *p );

        // Used for sorting
        virtual int compare( QListViewItem*, int, bool ) const;

        /**
        * Paints a focus indicator on the rectangle (current item). We disable it
        * over the currentTrack, cause it would look like crap and flicker.
        */
        void paintFocus( QPainter*, const QColorGroup&, const QRect& );

        static void imageTransparency( QImage& image, float factor );

        void refAlbum();
        void derefAlbum();

        void decrementTotals();
        void incrementTotals();

        int totalIncrementAmount() const;

        PlaylistAlbum *m_album;
        bool m_enabled;

        class MoodProxyObject: public QObject
        {
            public:
            MoodProxyObject( PlaylistItem *i ): item( i ) { }
            PlaylistItem* item;
        };

        MoodProxyObject *m_proxyForMoods;
        QMutex theArrayLock;
        QValueVector<QColor> theArray;
        QPixmap theMoodbar;
        int theHueOrder;

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


inline void PlaylistItem::setText( int column, const QString &text ) { setExactText( column, text ); }


inline void PlaylistItem::setTitle( const QString &title )
{
    super::setTitle( title );
    update();
}

inline void PlaylistItem::setArtist( const AtomicString &artist )
{
    decrementTotals();
    derefAlbum();
    super::setArtist( artist );
    refAlbum();
    incrementTotals();
    update();
}

inline void PlaylistItem::setComposer( const AtomicString &composer )
{
    super::setComposer( composer );
    update();
}

inline void PlaylistItem::setAlbum( const AtomicString &album )
{
    decrementTotals();
    derefAlbum();
    super::setAlbum( album );
    refAlbum();
    incrementTotals();
    update();
}

inline void PlaylistItem::setComment( const AtomicString &comment )
{
    super::setComment( comment );
    update();
}

inline void PlaylistItem::setGenre( const AtomicString &genre )
{
    super::setGenre( genre );
    update();
}

inline void PlaylistItem::setYear( int year )
{
    super::setYear( year );
    update();
}

inline void PlaylistItem::setDiscNumber( int discNumber )
{
    super::setDiscNumber( discNumber );
    update();
}

inline void PlaylistItem::setTrack( int track )
{
    decrementTotals();
    super::setTrack( track );
    incrementTotals();
    update();
}

inline void PlaylistItem::setLength( int length )
{
    super::setLength( length );
    update();
}

inline void PlaylistItem::setBitrate( int bitrate )
{
    super::setBitrate( bitrate );
    update();
}

inline void PlaylistItem::setFilesize( int bytes )
{
    super::setFilesize( bytes );
    update();
}

inline void PlaylistItem::setRating( int rating )
{
    decrementTotals();
    super::setRating( rating );
    incrementTotals();
    updateColumn( Rating );
}

inline void PlaylistItem::setScore( int score )
{
    decrementTotals();
    super::setScore( score );
    incrementTotals();
    update();
}

inline void PlaylistItem::setPlayCount( int playcount )
{
    super::setPlayCount( playcount );
    update();
}

inline void PlaylistItem::setLastPlay( uint lastplay )
{
    decrementTotals();
    super::setLastPlay( lastplay );
    incrementTotals();
    update();
}

#endif
