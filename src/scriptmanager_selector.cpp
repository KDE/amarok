// (c) 2003 Scott Wheeler <wheeler@kde.org>,
// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include <qcheckbox.h>

#include <kapplication.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <klistview.h>
#include <kpushbutton.h>

#include "scriptmanagerbase.h"
#include "scriptmanager_selector.h"


////////////////////////////////////////////////////////////////////////////////
// public interface
////////////////////////////////////////////////////////////////////////////////

ScriptManager::Selector* ScriptManager::Selector::instance = 0;


ScriptManager::Selector::Selector( const QStringList &directories, QWidget *parent, const char *name )
        : KDialogBase( parent, name, true, 0, Ok | Cancel, Ok, true )
        , m_dirList( directories )
{
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Script Manager" ) ) );

    setWFlags( Qt::WDestructiveClose );
    setModal( false );

    m_base = new ScriptManagerBase( this );
    setMainWidget( m_base );
    m_base->directoryListView->setFullWidth( true );

    connect( m_base->addDirectoryButton, SIGNAL( clicked() ),
             SLOT( slotAddDirectory() ) );
    connect( m_base->removeDirectoryButton, SIGNAL( clicked() ),
             SLOT( slotRemoveDirectory() ) );
    connect( m_base->editButton, SIGNAL( clicked() ),
             SLOT( slotEditScript() ) );
    connect( m_base->runButton, SIGNAL( clicked() ),
             SLOT( slotRunScript() ) );
    connect( m_base->stopButton, SIGNAL( clicked() ),
             SLOT( slotStopScript() ) );
    connect( m_base->configureScriptButton, SIGNAL( clicked() ),
             SLOT( slotConfigureScript() ) );

    QStringList::ConstIterator it = directories.begin();
    for ( ; it != directories.end(); ++it )
        new KListViewItem( m_base->directoryListView, *it );

    //     m_base->scanRecursivelyCheckBox->setChecked( scanRecursively );
    //     m_base->monitorChangesCheckBox->setChecked( monitorChanges );

    QSize sz = sizeHint();
    setMinimumSize( kMax( 350, sz.width() ), kMax( 250, sz.height() ) );
    resize( sizeHint() );
}


ScriptManager::Selector::~Selector()
{
    instance = 0;
}


ScriptManager::Selector::Result
ScriptManager::Selector::exec()
{
    m_result.status = static_cast<DialogCode>( KDialogBase::exec() );
    m_result.dirs = m_dirList;
    return m_result;
}


////////////////////////////////////////////////////////////////////////////////
// private interface
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::Selector::slotAddDirectory()
{
    KFileDialog dia( QString::null, "*.*|" + i18n("amaroK Scripts" ), 0, 0, true );
    dia.setMode( KFile::File | KFile::ExistingOnly );
    dia.exec();
    QString dir = dia.selectedURL().path();

    if ( !dir.isEmpty() && m_dirList.find( dir ) == m_dirList.end() ) {
        m_dirList.append( dir );
        new KListViewItem( m_base->directoryListView, dir );
        m_result.addedDirs.append( dir );
    }
}


void
ScriptManager::Selector::slotRemoveDirectory()
{
    if ( !m_base->directoryListView->selectedItem() )
        return ;

    QString dir = m_base->directoryListView->selectedItem() ->text( 0 );
    m_dirList.remove( dir );
    m_result.removedDirs.append( dir );
    delete m_base->directoryListView->selectedItem();
}


void
ScriptManager::Selector::slotEditScript()
{
    if ( !m_base->directoryListView->selectedItem() )
        return ;

   emit signalEditScript( m_base->directoryListView->selectedItem()->text( 0 ) );
}


void
ScriptManager::Selector::slotRunScript()
{
    if ( !m_base->directoryListView->selectedItem() )
        return ;

   emit signalRunScript( m_base->directoryListView->selectedItem()->text( 0 ) );
}


void
ScriptManager::Selector::slotStopScript()
{
    if ( !m_base->directoryListView->selectedItem() )
        return ;

   emit signalStopScript( m_base->directoryListView->selectedItem()->text( 0 ) );
}


void
ScriptManager::Selector::slotConfigureScript()
{
    if ( !m_base->directoryListView->selectedItem() )
        return ;

   emit signalConfigureScript( m_base->directoryListView->selectedItem()->text( 0 ) );
}


#include "scriptmanager_selector.moc"
