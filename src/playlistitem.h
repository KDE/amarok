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
        PlaylistItem( const KURL&, QListViewItem* );
        PlaylistItem( const KURL&, QListView*, QListViewItem* );
        PlaylistItem( const KURL&, QListViewItem*, const MetaBundle& );
        PlaylistItem( const KURL&, QListViewItem*, const QDomNode& );

        QString exactText( int col ) const { return KListViewItem::text( col ); }

        /**
         * Sets the information like the title, artist and album of the PlaylistItem
         * according to the MetaBundle @p bundle. If the PlaylistItem has a score
         * it will also be set.
         */
        void setText( const MetaBundle& bundle);
        void setText( int, const QString& );

        /** Indicates that the current-track pixmap has changed. Animation must be redrawn. */
        static void setPixmapChanged();

        Playlist *listView() const { return (Playlist*)KListViewItem::listView(); }
        PlaylistItem *nextSibling() const { return (PlaylistItem*)KListViewItem::nextSibling(); }
        void setup();

        MetaBundle metaBundle();

        QString trackName() const { return KListViewItem::text( TrackName ); }
        QString title() const { return KListViewItem::text( Title ); }
        const KURL &url() const { return m_url; }

        /**
         * @return the length of the PlaylistItem in seconds
         */
        QString seconds() const;

        static QColor glowText;
        static QColor glowBase;

        static const QString columnName(int n);

        enum Column  { TrackName = 0,
                       Title = 1,
                       Artist = 2,
                       Album = 3,
                       Year = 4,
                       Comment = 5,
                       Genre = 6,
                       Track = 7,
                       Directory = 8,
                       Length = 9,
                       Bitrate = 10,
                       Score = 11 };

        // Used for sorting
        bool operator== ( const PlaylistItem & item ) const;
        bool operator< ( const PlaylistItem & item ) const;

    private:
        struct paintCacheItem {
            int width;
            int height;
            QString text;
            QFont font;
            QMap<QString, QPixmap> map;
        };

        static const int NUM_COLUMNS = 12;

        /**
        * @return The text of the column @p column. If there is no text set for
        * the title this method returns a pretty version of the track name
        */
        QString text( int column ) const;

        int     compare( QListViewItem*, int, bool ) const;
        void    paintCell( QPainter*, const QColorGroup&, int, int, int );


        static QString trackName( const KURL &u ) { return u.protocol() == "http" ? u.prettyURL() : u.fileName(); }

        const KURL m_url;

        static bool s_pixmapChanged;
        static const uint STRING_STORE_SIZE = 80;
        static QString stringStore[STRING_STORE_SIZE];
        static const QString& attemptStore( const QString &candidate);
};

#endif
