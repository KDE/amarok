// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information


#include "searchbrowser.h"

#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <kurlcompletion.h>
#include <kurldrag.h>
#include "threadweaver.h"


SearchBrowser::SearchListView::SearchListView( QWidget *parent, const char *name )
        : KListView ( parent, name )
{}


SearchBrowser::SearchBrowser( QWidget *parent, const char *name )
        : QVBox( parent, name )
        , m_weaver( new ThreadWeaver( this ) )
{
    QHBox *hb1 = new QHBox( this );
    hb1->setSpacing( 4 );
    hb1->setMargin( 2 );
    QLabel *label1 = new QLabel( "Search for:", hb1 );
    searchEdit = new KLineEdit( hb1 );

    QHBox *hb2 = new QHBox( this );
    hb2->setSpacing( 4 );
    hb2->setMargin( 2 );
    QLabel *label2 = new QLabel( "In:", hb2 );
    urlEdit = new KURLComboBox( KURLComboBox::Directories, TRUE, hb2 );
    QWidget *searchButton = new QPushButton( "&Search", hb2 );

    KURLCompletion *cmpl = new KURLCompletion();
    urlEdit->setCompletionObject( cmpl );
    urlEdit->setURL( KURL( "/") );

    QVBox *vb1 = new QVBox( this );
    vb1->setSpacing( 2 );
    vb1->setMargin( 2 );
    resultView  = new SearchListView( vb1 );
    historyView = new KListView( vb1 );

    resultView->setDragEnabled( TRUE );
    resultView->addColumn( i18n( "Filename" ) );
    resultView->addColumn( i18n( "Directory" ) );
    resultView->setResizeMode( QListView::AllColumns );
    //resultView->setColumnWidthMode( 1, QListView::Manual ); //NOTE is default

    historyView->addColumn( i18n( "Search Token" ) );
    historyView->addColumn( i18n( "Results" ) );
    historyView->addColumn( i18n( "Progress" ) );
    historyView->addColumn( i18n( "Base Folder" ) );
    historyView->setResizeMode( QListView::AllColumns );

    connect( searchEdit,   SIGNAL( returnPressed() ), this, SLOT( slotStartSearch() ) );
    connect( searchButton, SIGNAL( clicked() ),       this, SLOT( slotStartSearch() ) );

    setFocusProxy( searchEdit ); //so focus is given to a sensible widget when the tab is opened
}


SearchBrowser::~SearchBrowser()
{}


void SearchBrowser::slotStartSearch()
{
    QString path = urlEdit->currentText();
    if ( path.length() )
    {
        // Verify the slash and let KURL parse it
        if ( !path.endsWith( "/" ) )
            path += "/";

        kdDebug() << path << endl;
        KURL *url;
        url = new KURL( path );
        path = url->directory( FALSE, FALSE );
        delete url;

        kdDebug() << path << endl;
        if ( searchEdit->text().length() )
        {
            // Create a new item for the HistoryView and pass it to the searching thread
            KListViewItem *item;
            item = new KListViewItem( historyView, searchEdit->text() );
            item->setText( 1, "0" );
            item->setText( 2, "Waiting for other thread" );
            item->setText( 3, path );

            m_weaver->append( new SearchModule( this, path, searchEdit->text(), resultView, item ) );
        }
    }
}


void SearchBrowser::SearchListView::startDrag()
{
    QListViewItem *item = currentItem();

    if( item )
    {
        KURLDrag *d = new KURLDrag( KURL::List( KURL( item->text( 2 ) ) ), this, "DragObject" );
        d->dragCopy();
    }
}

void SearchBrowser::customEvent( QCustomEvent *e )
{
    if ( e->type() == (QEvent::Type) SearchModule::ProgressEventType )
    {
        SearchModule::ProgressEvent* p =
            static_cast<SearchModule::ProgressEvent*>( e );

        switch ( p->state() ) {
            case SearchModule::ProgressEvent::Start:
                p->item()->setText( 2, "Started" );
                resultView->clear();
                break;

            case SearchModule::ProgressEvent::Stop:
                p->item()->setText( 2, "Done" );
                break;

            case SearchModule::ProgressEvent::Progress:
                // kdDebug() << "********************************\n";
                // kdDebug() << "SearchModuleEvent arrived, found item: " << p->curPath() << p->curFile() << "\n";
                // kdDebug() << "********************************\n";

                QString curToken = p->curPath();
                if ( curToken.startsWith( p->item()->text( 3 ) ) )
                    curToken = curToken.right( curToken.length() - p->item()->text( 3 ).length() );

                p->item()->setText( 1, QString::number( p->count() ) );
                p->item()->setText( 2, curToken );

                KListViewItem *resItem = new KListViewItem( p->resultView(), p->curFile() );
                resItem->setText( 1, p->curPath() );
                resItem->setText( 2, p->curPath() + p->curFile() );
        }
    }
}


#include "searchbrowser.moc"

