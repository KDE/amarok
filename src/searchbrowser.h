// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information


#ifndef AMAROK_SEARCHBROWSER_H
#define AMAROK_SEARCHBROWSER_H

#include <qvbox.h>
#include <klistview.h>
#include <kurl.h>

class KLineEdit;
class KURLComboBox;
class QCustomEvent;
class QPushButton;
class ThreadWeaver;

class SearchBrowser : public QVBox
{
    Q_OBJECT

    class SearchListView : public KListView
    {
        public:
            SearchListView( QWidget *parent=0, const char *name=0 );
            KURL::List selectedUrls();

        protected:
            virtual class QDragObject *dragObject();
    };

    class HistoryListView : public KListView
    {
        public:
            class Item : public KListViewItem {
                public:
                    Item( QListView* parent, const QString &s )
                        : KListViewItem( parent, s ) {};
                    void addUrl( const KURL &url ) { m_urlList += url; }
                    const KURL::List& urlList() const { return m_urlList; }

                private:
                    KURL::List m_urlList;
            };

        HistoryListView( QWidget *parent=0, const char *name=0 );
        KURL::List selectedUrls();

        protected:
            virtual class QDragObject *dragObject();
    };

    public:
        SearchBrowser( const char *name );
        ~SearchBrowser();

    public slots:
        void slotStartSearch();

    private slots:
        void stopSearch();
        void slotDoubleClicked( QListViewItem *, const QPoint &, int );
        void showContextMenu( QListViewItem *, const QPoint &, int );
        void historySelectionChanged();

    private:
        void showResults( KURL::List );
        void customEvent( QCustomEvent* );

        ThreadWeaver* m_weaver;
        SearchListView *resultView;
        HistoryListView *historyView;
        KLineEdit *searchEdit;
        KURLComboBox *urlEdit;
        class QSplitter *splitter;
        QPushButton* m_searchButton;
};


#endif /* AMAROK_SEARCHBROWSER_H */


