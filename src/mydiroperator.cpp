#include "medium.h"
#include "mydiroperator.h"

#include <kurl.h>

#include <qdir.h>

MyDirOperator::MyDirOperator ( const KURL &url, QWidget *parent, Medium *medium ) : KDirOperator( url, parent )
{
    m_medium = medium;
    setDirLister( new MyDirLister( true ) );
    //the delete key event was being eaten with the file browser open
    //so folks couldn't use the del key to remove items from the playlist
    //refer to KDirOperator::setupActions() to understand this code
    KActionCollection* dirActionCollection = static_cast<KActionCollection*>(KDirOperator::child("KDirOperator::myActionCollection"));
    if(dirActionCollection)
    {
        KAction* trash = dirActionCollection->action("trash");
        if(trash)
            trash->setEnabled(false);
    }
}

void
MyDirOperator::myHome()
{
    KURL u;
    u.setPath( m_medium ? m_medium->mountPoint() : QDir::homeDirPath() );
    setURL(u, true);
}

void
MyDirOperator::myCdUp()
{
    KURL tmp( url() );
    tmp.cd( QString::fromLatin1(".."));
    if( m_medium && !tmp.path().startsWith( m_medium->mountPoint() ) )
        tmp.setPath( m_medium->mountPoint() );
    setURL(tmp, true);
}

#include "mydiroperator.moc"
