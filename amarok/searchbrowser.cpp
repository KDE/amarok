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


SearchBrowser::SearchListView::SearchListView( QWidget *parent, const char *name )
        : KListView ( parent, name )
{}


SearchBrowser::SearchBrowser( QWidget *parent, const char *name )
        : QVBox( parent, name )
{
    QHBox     *hb    = new QHBox( this );
    searchEdit       = new KLineEdit( hb );
    QWidget   *b     = new QPushButton( "&Search", hb );

    urlEdit     = new KURLComboBox( KURLComboBox::Directories, TRUE, this );
    KURLCompletion *cmpl = new KURLCompletion();
    urlEdit->setCompletionObject( cmpl );
    urlEdit->setURL( "/" );

    resultView  = new SearchListView( this );
    historyView = new KListView( this );

    resultView->setDragEnabled( TRUE );
    resultView->addColumn( i18n( "Filename" ) );
    resultView->addColumn( i18n( "Directory" ) );
//    resultView->addColumn( i18n( "Title" ) );
//    resultView->addColumn( i18n( "Artist" ) );

    historyView->addColumn( i18n( "Search Token" ) );
    historyView->addColumn( i18n( "Counter" ) );
    historyView->addColumn( i18n( "Base Folder" ) );

    connect( searchEdit, SIGNAL( returnPressed() ), this, SLOT( slotStartSearch() ) );
}


SearchBrowser::~SearchBrowser()
{}


void SearchBrowser::slotStartSearch()
{
    SearchThread *worker = new SearchThread( this );
    worker->start( QThread::LowestPriority );
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


SearchBrowser::SearchThread::SearchThread( SearchBrowser *parent ) : QThread()
{
    this->parent = parent;
    resultCount = 0;
}


SearchBrowser::SearchThread::~SearchThread()
{
}


void SearchBrowser::SearchThread::run()
{
    QString token = parent->searchEdit->text();
    KURL *url = new KURL( parent->urlEdit->currentText() );
    
    item = new KListViewItem( parent->historyView, token );
    item->setText( 1, "0" );
    item->setText( 2, url->directory( FALSE, FALSE ) );
    
    parent->resultView->clear();    
    searchDir( url->directory( FALSE, FALSE ).local8Bit() );
}


void SearchBrowser::SearchThread::searchDir( QString path )
{
    QString token = parent->searchEdit->text();

    kdDebug() << "Reading DIRECTORY: " << path << endl;

    DIR *d = opendir( path.local8Bit() );
    if ( d )
    {
        dirent *ent;
        while ( ( ent = readdir( d ) ) )
        {
            QString file( ent->d_name );
            
            if ( file != "." && file != ".." )
            {
                DIR *t = opendir( path.local8Bit() + file.local8Bit() + "/" );
                if ( t )
                {
                    closedir( t );
                    searchDir( path + file + "/" );
                }
                else
                    if ( file.contains( token, FALSE ) )
                    {
                        item->setText( 1, QString::number( ++resultCount ) );
                        
                        KListViewItem *resItem = new KListViewItem( parent->resultView, file );
                        resItem->setText( 1, path );
                        resItem->setText( 2, path + file );
                    }
            }
        }
        closedir( d );
        free( ent );
    }
}

#include "searchbrowser.moc"

