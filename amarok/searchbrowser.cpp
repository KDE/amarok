// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information


#include "searchbrowser.h"

#include <kapplication.h> //kapp->config(), QApplication::setOverrideCursor()
#include <kconfig.h>      //config object
#include <klocale.h>
#include <kcursor.h>      //waitCursor()
#include <kdebug.h>
#include <klineedit.h>
#include <kurl.h>
#include <kurlcombobox.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qsplitter.h>
#include <qtextstream.h>
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
    KConfig *config = kapp->config();
    config->setGroup( "SearchBrowser" );

    QHBox *hb1 = new QHBox( this );
    hb1->setSpacing( 4 );
    hb1->setMargin( 2 );
    QLabel *label1 = new QLabel( "Search &for:", hb1 );
    searchEdit = new KLineEdit( hb1 );
    label1->setBuddy( searchEdit );

    QHBox *hb2 = new QHBox( this );
    hb2->setSpacing( 4 );
    hb2->setMargin( 2 );
    QLabel *label2 = new QLabel( "&In:", hb2 );
    urlEdit = new KURLComboBox( KURLComboBox::Directories, TRUE, hb2 );
    label2->setBuddy( urlEdit );
    QWidget *searchButton = new QPushButton( "&Search", hb2 );
    urlEdit->setDuplicatesEnabled( false );
    urlEdit->setCompletionObject( new KURLCompletion() );
    urlEdit->setURLs( config->readListEntry( "History" ) );
    urlEdit->lineEdit()->setText( config->readEntry( "Location", "/" ) );

    splitter    = new QSplitter( Vertical, this );
    resultView  = new SearchListView( splitter );
    historyView = new KListView( splitter );
    QString str = config->readEntry( "Splitter Stream" );
    QTextStream stream( &str, IO_ReadOnly );
    stream >> *splitter; //this sets the splitters position

    resultView->setDragEnabled( TRUE );
    resultView->addColumn( i18n( "Filename" ) );
    resultView->addColumn( i18n( "Directory" ) );
    resultView->setResizeMode( QListView::AllColumns );
    resultView->setSelectionMode( QListView::Extended );
    resultView->setAllColumnsShowFocus( true );
    //resultView->setColumnWidthMode( 1, QListView::Manual ); //NOTE is default

    historyView->addColumn( i18n( "Search Token" ) );
    historyView->addColumn( i18n( "Results" ) );
    historyView->addColumn( i18n( "Progress" ) );
    historyView->addColumn( i18n( "Base Folder" ) );
    historyView->setAllColumnsShowFocus( true );
    historyView->setResizeMode( QListView::AllColumns );

    connect( searchEdit,   SIGNAL( returnPressed() ), SLOT( slotStartSearch() ) );
    connect( urlEdit,      SIGNAL( returnPressed() ), SLOT( slotStartSearch() ) );
    connect( searchButton, SIGNAL( clicked() ),       SLOT( slotStartSearch() ) );

    setFocusProxy( searchEdit ); //so focus is given to a sensible widget when the tab is opened
}


SearchBrowser::~SearchBrowser()
{
    KConfig *config = kapp->config();
    config->setGroup( "SearchBrowser" );
    config->writeEntry( "Location", urlEdit->lineEdit()->text() );
    config->writeEntry( "History", urlEdit->urls() );

    QString str; QTextStream stream( &str, IO_WriteOnly );
    stream << *splitter;

    config->writeEntry( "Splitter Stream", str );
}


void SearchBrowser::slotStartSearch()
{
    QString path = urlEdit->currentText();

    if ( !path.isEmpty() ) //isEmpty() is guarenteed O(1), length() can be O(n)
    {
        urlEdit->insertItem( path );

        // Verify the slash and let KURL parse it
        if ( !path.endsWith( "/" ) )
            path += "/";

        kdDebug() << path << endl;
        KURL *url;
        url = new KURL( path );
        path = url->directory( FALSE, FALSE );
        delete url;

        kdDebug() << path << endl;
        if ( !searchEdit->text().isEmpty() )
        {
            // Create a new item for the HistoryView and pass it to the searching thread
            KListViewItem *item;
            item = new KListViewItem( historyView, searchEdit->text() );
            item->setText( 1, "0" );
            item->setText( 2, "Waiting for other thread" );
            item->setText( 3, path );
            historyView->setSelected( item, true );

            m_weaver->append( new SearchModule( this, path, searchEdit->text(), resultView, item ) );
        }
    }
}


QDragObject *SearchBrowser::SearchListView::dragObject()
{
    KURL::List list;
    KURL url;

    for( QListViewItemIterator it( this, QListViewItemIterator::Selected); *it; ++it )
    {
        url.setPath( (*it)->text( 2 ) );
        list += url;
    }

    return new KURLDrag( list, this );
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
                QApplication::setOverrideCursor( KCursor::workingCursor() );
                resultView->clear();
                break;

            case SearchModule::ProgressEvent::Stop:
                p->item()->setText( 2, "Done" );
                QApplication::restoreOverrideCursor();
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

