// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information


#ifndef AMAROK_SEARCHBROWSER_H
#define AMAROK_SEARCHBROWSER_H

#include <qvbox.h>
#include <klistview.h>

class ThreadWeaver;
class QCustomEvent;
class KURLComboBox;
class KLineEdit;

class SearchBrowser : public QVBox
{
    Q_OBJECT

    class SearchListView : public KListView
    {
        public:
            SearchListView( QWidget *parent=0, const char *name=0 );

        protected:
            virtual class QDragObject *dragObject();
    };

    public:
        SearchBrowser( QWidget *parent=0, const char *name=0 );
        ~SearchBrowser();

    public slots:
        void slotStartSearch();

    private:
        void customEvent( QCustomEvent* );

        ThreadWeaver* m_weaver;
        SearchListView *resultView;
        KListView *historyView;
        KLineEdit *searchEdit;
        KURLComboBox *urlEdit;
        class QSplitter *splitter;
};

#endif /* AMAROK_SEARCHBROWSER_H */


