// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#ifndef SMARTPLAYLIST_H
#define SMARTPLAYLIST_H

#include <klistview.h>
#include <kurl.h>
#include <qvbox.h>

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

    protected:
        virtual class QDragObject *dragObject();
        virtual void paintEmptyArea( QPainter *p, const QRect &r );

    private slots:
        void loadPlaylistSlot( QListViewItem * );
        void collectionScanDone();

    private:
        void loadDefaultPlaylists();
        void loadCustomPlaylists();
        KURL::List loadSmartPlaylist( QListViewItem *item );    //query the database and returns a list of url
        QString customPlaylistsFile();

        bool m_loaded;
};


class SmartPlaylist : public KListViewItem
{
    public:
        SmartPlaylist( KListView *parent, KListViewItem *after, QString name,
                       QString query = QString::null, QString icon = QString::null,
                       bool custom = false );
        SmartPlaylist( SmartPlaylist *parent, KListViewItem *after, QString name,
                       QString query = QString::null, QString icon = QString::null,
                       bool custom = false );

        void setQuery( const QString &query ) { m_query = query; };
        const QString &query() { return m_query; }
        bool isCustom() { return m_custom; }
        void setKey( int pos ) { m_key = pos; }
        QString key( int c, bool ) const;

    private:
        QString m_query;
        bool m_custom;
        int m_key;
};

#endif
