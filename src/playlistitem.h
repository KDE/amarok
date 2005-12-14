//Maintainer: Max Howell <max.howell@methylblue.com>
//Copyright:  GPL v2

//NOTE please show restraint when adding data members to this class!
//     some users have playlists with 20,000 items or more in, one 32 bit int adds up rapidly!


#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <klistview.h> //baseclass
#include <kurl.h>      //stack allocated

#include <qpixmap.h>
#include <qvaluevector.h>
#include <qmutex.h>
#include <qcolor.h>    //stack allocated
#include <qfont.h>     //stack allocated
#include <qmap.h>

class QColorGroup;
class QDomNode;
class QListViewItem;
class QPainter;
class MetaBundle;
class Playlist;

class PlaylistItem : public KListViewItem
{
    public:
        enum Column {
            Filename = 0,
            Title,
            Artist,
            Album ,
            Year,
            Comment,
            Genre,
            Track,
            Directory,
            Length,
            Bitrate,
            Score,
            Rating,
            Type,
            Playcount,
            LastPlayed,
            Moodbar,
            NUM_COLUMNS
        };

        class ReadMood;
        friend class ReadMood;

        /// Indicates that the current-track pixmap has changed. Animation must be redrawn.
        static void setPixmapChanged() { s_pixmapChanged = true; }
        static const QString columnName( int n );

        /// For the glow colouration stuff
        static QColor glowText;
        static QColor glowBase;

    public:
        PlaylistItem( QListView*, QListViewItem* ); //used by PlaylistLoader
        PlaylistItem( const MetaBundle&, QListViewItem* );
        PlaylistItem( QDomNode, QListViewItem* );
        ~PlaylistItem();

        /**
         * Sets the information like the title, artist and album of the PlaylistItem
         * according to the MetaBundle @p bundle. If the PlaylistItem has a score
         * it will also be set.
         */
        void setText( const MetaBundle& bundle );

        /// pass 'raw' data here, for example "92" for Length, and not "1:32"
        virtual void setText( int column, const QString& );

        /// @return the exact same thing you set with setText()
        QString exactText( int column ) const;

        /**
        * @return The text of the column @p column, formatted for display purposes.
        * (For example, if the Length is 92, "1:32".)
        */
        virtual QString text( int column ) const;

        bool isEnabled() const { return m_enabled; }
        void setEnabled( bool enable );

        void setSelected( bool selected );
        void setVisible( bool visible );

        void setEditing( int column );
        bool isEditing( int column ) const;

        /// convenience functions
        Playlist *listView() const { return (Playlist*)KListViewItem::listView(); }
        PlaylistItem *nextSibling() const { return (PlaylistItem*)KListViewItem::nextSibling(); }

        /// some accessors
        const KURL &url()   const;
        QString filename()  const;
        QString title()     const;
        QString artist()    const;
        QString album()     const;
        int     year()      const;
        QString comment()   const;
        QString genre()     const;
        int     track()     const;
        QString directory() const;
        int     length()    const;
        int     bitrate()   const;
        int     score()     const;
        int     rating()    const;
        QString type()      const;
        int     playCount() const;
        uint    lastPlay()  const;

        /// some setters
        void setTitle(   const QString &title );
        void setArtist(  const QString &artist );
        void setAlbum(   const QString &album );
        void setComment( const QString &comment );
        void setGenre(   const QString &genre );
        void setYear(      int year );
        void setTrack(     int track );
        void setLength(    int length );
        void setBitrate(   int bitrate );
        void setRating(    int rating );
        void setScore(     int score );
        void setPlaycount( int playcount );
        void setLastPlay( uint lastplay );

        /// @return does the file exist?
        bool exists() const { return !m_missing; }

        void refreshMood();
        void checkMood();
        bool readMood();

        enum { DrawNormal, DrawGrayed };
        QPixmap *star( int type = DrawNormal ) const;
        int ratingAtPoint( int x ) const;

        /// like QWidget::update()
        void update() const;

        //updates only the area of a specific column, avoids flickering of the current item marker
        void updateColumn( int column ) const;

        //used by class Playlist
        virtual void setup();

        //won't compile unless these are public *shrug*
        virtual bool operator== ( const PlaylistItem & item ) const;
        virtual bool operator< ( const PlaylistItem & item ) const;

    private:
        struct paintCacheItem {
            int width;
            int height;
            QString text;
            QFont font;
            QMap<QString, QPixmap> map;
        };

        void setArray(const QValueVector<QColor> array);

        virtual void paintCell( QPainter*, const QColorGroup&, int, int, int );
        void drawRating( QPainter *p );

        // Used for sorting
        virtual int  compare( QListViewItem*, int, bool ) const;

        static QString filename( const KURL &u ) { return u.isEmpty() ? QString() : u.protocol() == "http" ? u.prettyURL() : u.fileName(); }

        /**
        * Paints a focus indicator on the rectangle (current item). We disable it
        * over the currentTrack, cause it would look like crap and flicker.
        */
        void paintFocus( QPainter*, const QColorGroup&, const QRect& );

        const KURL m_url;
        int m_year;
        int m_track;
        int m_length;
        int m_bitrate;

        int  m_score;
        int  m_rating;
        int  m_playCount;
        uint m_lastPlay;

        bool m_missing;
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
        static const uint STRING_STORE_SIZE = 80;
        static QString stringStore[STRING_STORE_SIZE];
        static const QString& attemptStore( const QString &candidate);
};

class PLItemList: public QPtrList<PlaylistItem>
{
    public:
        PLItemList() : QPtrList<PlaylistItem>() { }
        PLItemList( const QPtrList<PlaylistItem> &list ) : QPtrList<PlaylistItem>( list ) { }
        PLItemList( PlaylistItem *item ) : QPtrList<PlaylistItem>() { append( item ); }

        inline PLItemList &operator<<( PlaylistItem *item ) { append( item ); return *this; }
};


inline const KURL &PlaylistItem::url()   const { return m_url; }
inline QString PlaylistItem::filename()  const { return m_url.isEmpty() ? QString() : m_url.fileName(); }
inline QString PlaylistItem::title()     const { return KListViewItem::text( Title ); }
inline QString PlaylistItem::artist()    const { return KListViewItem::text( Artist ); }
inline QString PlaylistItem::album()     const { return KListViewItem::text( Album ); }
inline int     PlaylistItem::year()      const { return m_year; }
inline QString PlaylistItem::comment()   const { return KListViewItem::text( Comment ); }
inline QString PlaylistItem::genre()     const { return KListViewItem::text( Genre ); }
inline int     PlaylistItem::track()     const { return m_track; }
inline QString PlaylistItem::directory() const { return m_url.isEmpty() ? QString() : m_url.directory(); }
inline int     PlaylistItem::length()    const { return m_length; }
inline int     PlaylistItem::bitrate()   const { return m_bitrate; }
inline QString PlaylistItem::type()      const { return filename().mid( filename().findRev( '.' ) + 1 ); }

inline void PlaylistItem::setTitle( const QString &title )
{
    KListViewItem::setText( Title, title );
    update();
}

inline void PlaylistItem::setArtist( const QString &artist )
{
    KListViewItem::setText( Artist, attemptStore( artist ) );
    update();
}

inline void PlaylistItem::setAlbum( const QString &album )
{
    KListViewItem::setText( Album, attemptStore( album ) );
    update();
}

inline void PlaylistItem::setComment( const QString &comment )
{
    KListViewItem::setText( Comment, comment );
    update();
}

inline void PlaylistItem::setGenre( const QString &genre )
{
    KListViewItem::setText( Genre, attemptStore( genre ) );
    update();
}

inline void PlaylistItem::setYear(      int year )      { m_year      = year;      update(); }
inline void PlaylistItem::setTrack(     int track )     { m_track     = track;     update(); }
inline void PlaylistItem::setLength(    int length )    { m_length    = length;    update(); }
inline void PlaylistItem::setBitrate(   int bitrate )   { m_bitrate   = bitrate;   update(); }
inline void PlaylistItem::setRating(    int rating )    { m_rating    = rating;    update(); }
inline void PlaylistItem::setScore(     int score )     { m_score     = score;     update(); }
inline void PlaylistItem::setPlaycount( int playcount ) { m_playCount = playcount; update(); }
inline void PlaylistItem::setLastPlay( uint lastplay )  { m_lastPlay  = lastplay;  update(); }

#endif
