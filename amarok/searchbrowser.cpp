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
#include <kurlcompletion.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <kurldrag.h>
#include "threadweaver.h"


SearchBrowser::SearchListView::SearchListView( QWidget *parent, const char *name )
        : KListView ( parent, name )
{}


SearchBrowser::SearchBrowser( QWidget *parent, const char *name )
        : QVBox( parent, name )
        , m_weaver( new ThreadWeaver( this ) )
{
    QHBox *hb = new QHBox( this );
    searchEdit = new KLineEdit( hb );
    QWidget *searchButton = new QPushButton( "&Search", hb );

    urlEdit = new KURLComboBox( KURLComboBox::Directories, TRUE, this );
    KURLCompletion *cmpl = new KURLCompletion();
    urlEdit->setCompletionObject( cmpl );
    urlEdit->setURL( "/" );

    resultView  = new SearchListView( this );
    historyView = new KListView( this );

    resultView->setDragEnabled( TRUE );
    resultView->addColumn( i18n( "Filename" ) );
    resultView->addColumn( i18n( "Directory" ) );

    historyView->addColumn( i18n( "Search Token" ) );
    historyView->addColumn( i18n( "Counter" ) );
    historyView->addColumn( i18n( "Progress" ) );
    historyView->addColumn( i18n( "Base Folder" ) );

    historyView->setColumnWidthMode( 0, QListView::Manual );
    historyView->setColumnWidthMode( 1, QListView::Manual );
    historyView->setColumnWidthMode( 2, QListView::Manual );
    historyView->setColumnWidthMode( 3, QListView::Manual );

    connect( searchEdit,   SIGNAL( returnPressed() ), this, SLOT( slotStartSearch() ) );
    connect( searchButton, SIGNAL( clicked() ),       this, SLOT( slotStartSearch() ) );
}


SearchBrowser::~SearchBrowser()
{}


void SearchBrowser::slotStartSearch()
{
    KListViewItem *item;
    item = new KListViewItem( historyView, searchEdit->text() );
    item->setText( 1, "0" );
    item->setText( 3, urlEdit->currentText() );
    
    m_weaver->append( new SearchModule( this, urlEdit->currentText(), searchEdit->text(), resultView, item ) );
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
    if ( e->type() == (QEvent::Type) ThreadWeaver::Job::SearchModule )
    {
        SearchModule *sm = static_cast<SearchModule *>( e );

        kdDebug() << "********************************\n";
        kdDebug() << "SearchModuleEvent arrived.\n";
        kdDebug() << "********************************\n";
        
    }
}

        
#include "searchbrowser.moc"

