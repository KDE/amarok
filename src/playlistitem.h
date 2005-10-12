//Maintainer: Max Howell <max.howell@methylblue.com>
//Copyright:  GPL v2

//NOTE please show restraint when adding data members to this class!
//     some users have playlists with 20,000 items or more in, one 32 bit int adds up rapidly!


#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <klistview.h> //baseclass
#include <kurl.h>      //stack allocated

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
            Filename  = 0,
            Title     = 1,
            Artist    = 2,
            Album     = 3,
            Year      = 4,
            Comment   = 5,
            Genre     = 6,
            Track     = 7,
            Directory = 8,
            Length    = 9,
            Bitrate   = 10,
            Score     = 11,
            Type      = 12,
            Playcount = 13
        };

        static const int NUM_COLUMNS = 14;

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

        QString exactText( int col ) const { return KListViewItem::text( col ); }

        /**
         * Sets the information like the title, artist and album of the PlaylistItem
         * according to the MetaBundle @p bundle. If the PlaylistItem has a score
         * it will also be set.
         */
        void setText( const MetaBundle& bundle );
        void setText( int, const QString& ); //virtual

        bool isEnabled() const { return m_enabled; }
        void setEnabled( bool enable );

        void setSelected( bool selected );
        void setVisible( bool visible );

        /// convenience functions
        Playlist *listView() const { return (Playlist*)KListViewItem::listView(); }
        PlaylistItem *nextSibling() const { return (PlaylistItem*)KListViewItem::nextSibling(); }

        /// some accessors
        const KURL &url() const { return m_url; }
        QString filename() const { return KListViewItem::text( Filename ); }
        QString title() const { return KListViewItem::text( Title ); }
        QString artist() const { return KListViewItem::text( Artist ); }
        QString album() const { return KListViewItem::text( Album ); }

        /// @return the length of the PlaylistItem in seconds
        QString seconds() const;

        /// @return does the file exist?
        bool exists() const { return !m_missing; }

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

        /**
        * @return The text of the column @p column. If there is no text set for
        * the title this method returns a pretty version of the filename
        */
        virtual QString text( int column ) const;

        virtual void paintCell( QPainter*, const QColorGroup&, int, int, int );

        // Used for sorting
        virtual int  compare( QListViewItem*, int, bool ) const;

        static QString filename( const KURL &u ) { return u.protocol() == "http" ? u.prettyURL() : u.fileName(); }

        const KURL m_url;
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
