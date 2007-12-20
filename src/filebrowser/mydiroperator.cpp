#include "mydiroperator.h"
#include "MainWindow.h"

#include <KActionCollection>
#include <kurl.h>

#include <QDir>

MyDirOperator::MyDirOperator ( const KUrl &url, QWidget *parent )
    : KDirOperator( url, parent )
{
    //MyDirLister* dirlister = new MyDirLister( true );
  //  dirlister->setMainWindow( MainWindow::self() );
    //setDirLister( dirlister );
}

#include "mydiroperator.moc"
