// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information


#ifndef AMAROK_SEARCHBROWSER_H
#define AMAROK_SEARCHBROWSER_H

#include <qvbox.h>
#include <qthread.h>
#include <klistview.h>
#include <klineedit.h>
#include <kurlcombobox.h>


class SearchBrowser : public QVBox
{
    Q_OBJECT

    class SearchListView : public KListView
    {
        public:
            SearchListView( QWidget *parent=0, const char *name=0 );
            
        protected:
            virtual void startDrag();
    };
    
    public:
        SearchBrowser( QWidget *parent=0, const char *name=0 );
        ~SearchBrowser();

    public slots:
        void slotStartSearch();

    private:
        SearchListView *resultView;
        KListView *historyView;
        KLineEdit *searchEdit;
        KURLComboBox *urlEdit;

    class SearchThread : public QThread
    {
        public:
            friend class SearchBrowser;

            SearchThread( SearchBrowser *parent=0 );
            ~SearchThread();
            
        private:
            void searchDir( QString path );

            uint resultCount;
            SearchBrowser *parent;
            KListViewItem *item;

        protected:
            void run();
    };

    friend class SearchThread;
};

#endif /* AMAROK_SEARCHBROWSER_H */


