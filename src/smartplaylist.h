// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#ifndef SMARTPLAYLIST_H
#define SMARTPLAYLIST_H

#include <klistview.h>
#include <kurl.h>
#include <qvbox.h>

class SmartPlaylist;
class QPainter;
class QRect;

class SmartPlaylistBox : public QVBox
{
    public:
        SmartPlaylistBox( QWidget *parent, const char *name = 0 );
};


class SmartPlaylistView : public KListView
{
Q_OBJECT
    public:
        SmartPlaylistView( QWidget *parent, const char *name = 0 );
        ~SmartPlaylistView();

    public slots:
        void createCustomPlaylist();
        void removeSelectedPlaylists();

    protected:
        virtual class QDragObject *dragObject();
        virtual void paintEmptyArea( QPainter *p, const QRect &r );

    private slots:
        void loadPlaylistSlot( QListViewItem * );
        void showContextMenu( QListViewItem *item, const QPoint &p, int );
        void collectionScanDone();

    private:
        void loadDefaultPlaylists();
        void loadCustomPlaylists();
        KURL::List loadSmartPlaylist( QListViewItem *item );    //query the database and returns a list of url
        QString customPlaylistsFile();

        /// convenience function
        SmartPlaylist *currentItem() { return (SmartPlaylist*)currentItem(); }

        bool m_loaded;
};


class SmartPlaylist : public KListViewItem
{
    public:
        SmartPlaylist(
                KListView *parent,
                KListViewItem *after,
                const QString &name,
                const QString &query = QString::null,
                const QString &icon = QString::null,
                bool custom = false );

        SmartPlaylist(
                SmartPlaylist *parent,
                KListViewItem *after,
                const QString &name,
                const QString &query = QString::null,
                const QString &icon = QString::null,
                bool custom = false );

        void setQuery( const QString &query ) { m_query = query; };
        const QString &query() { return m_query; }
        bool isCustom() { return m_custom; }
        void setKey( int pos ) { m_key = pos; }
        QString key( int c, bool ) const;

        /// returns the URLs for this playlist
        KURL::List urls() const; ///DEPRECATE this!

        /// returns the SQL to get this playlist's URLs
        QString sqlForUrls() const { return QString(); }

        /// returns the SQL to get this playlist's URLs + tags
        QString sqlForTags() const { return QString(); }

    private:
        QString m_query;
        bool m_custom;
        int m_key;
};

#endif
