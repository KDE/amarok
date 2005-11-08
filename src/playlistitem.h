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

class PlaylistItem : public QObject, public KListViewItem
{
Q_OBJECT
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
            Type,
            Playcount,
            Moodbar,
            NUM_COLUMNS
        };

        QMutex theArrayLock;
        QValueVector<QColor> theArray;
        QPixmap theMoodbar;

        void setArray(const QValueVector<QColor> array);


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
        inline const KURL &url()   const { return m_url; }
        inline QString filename()  const { return m_url.isEmpty() ? QString() : m_url.fileName(); }
        inline QString title()     const { return KListViewItem::text( Title ); }
        inline QString artist()    const { return KListViewItem::text( Artist ); }
        inline QString album()     const { return KListViewItem::text( Album ); }
        inline int     year()      const { return m_year; }
        inline QString comment()   const { return KListViewItem::text( Comment ); }
        inline QString genre()     const { return KListViewItem::text( Genre ); }
        inline int     track()     const { return m_track; }
        inline QString directory() const { return m_url.isEmpty() ? QString() : m_url.directory(); }
        inline int     length()    const { return m_length; }
        inline int     bitrate()   const { return m_bitrate; }
        inline int     score()     const { return m_score; }
        inline QString type()      const { return filename().mid( filename().findRev( '.' ) + 1 ); }
        inline int     playcount() const { return m_playcount; }

        /// some setters
        inline void setTitle(     const QString &title )     { KListViewItem::setText(
                                                               Title,   title );                  update(); }
        inline void setArtist(    const QString &artist )    { KListViewItem::setText(
                                                               Artist,  attemptStore( artist ) ); update(); }
        inline void setAlbum(     const QString &album )     { KListViewItem::setText(
                                                               Album,   attemptStore( album ) );  update(); }
        inline void setComment(   const QString &comment )   { KListViewItem::setText(
                                                               Comment, comment );                update(); }
        inline void setGenre(     const QString &genre )     { KListViewItem::setText(
                                                               Genre,   attemptStore( genre ) );  update(); }
        inline void setYear(      int            year )      { m_year      = year;      update(); }
        inline void setTrack(     int            track )     { m_track     = track;     update(); }
        inline void setLength(    int            length )    { m_length    = length;    update(); }
        inline void setBitrate(   int            bitrate )   { m_bitrate   = bitrate;   update(); }
        inline void setScore(     int            score )     { m_score     = score;     update(); }
        inline void setPlaycount( int            playcount ) { m_playcount = playcount; update(); }

        /// @return does the file exist?
        bool exists() const { return !m_missing; }

        void checkMood();
        bool readMood();

        /// like QWidget::update()
        void update() const;

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

        virtual void paintCell( QPainter*, const QColorGroup&, int, int, int );

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
        int m_score;
        int m_playcount;
        bool m_missing;
        bool m_enabled;

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

#endif
