// (c) 2003 Scott Wheeler <wheeler@kde.org>,
// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.


#define DEBUG_PREFIX "ScriptManager"

#include "debug.h"
#include "scriptmanager.h"
#include "scriptmanagerbase.h"

#include <kapplication.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kprocess.h>
#include <kpushbutton.h>
#include <krun.h>
#include <ktextedit.h>


////////////////////////////////////////////////////////////////////////////////
// public interface
////////////////////////////////////////////////////////////////////////////////

ScriptManager* ScriptManager::s_instance = 0;


ScriptManager::ScriptManager( QWidget *parent, const char *name )
        : KDialogBase( parent, name, false, 0, 0, Ok, false )
        , m_base( new ScriptManagerBase( this ) )
{
    DEBUG_FUNC_INFO

    s_instance = this;

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Script Manager" ) ) );

    setMainWidget( m_base );
    m_base->directoryListView->setFullWidth( true );

    connect( m_base->addDirectoryButton, SIGNAL( clicked() ), SLOT( slotAddScript() ) );
    connect( m_base->removeDirectoryButton, SIGNAL( clicked() ), SLOT( slotRemoveScript() ) );
    connect( m_base->editButton, SIGNAL( clicked() ), SLOT( slotEditScript() ) );
    connect( m_base->runButton, SIGNAL( clicked() ), SLOT( slotRunScript() ) );
    connect( m_base->stopButton, SIGNAL( clicked() ), SLOT( slotStopScript() ) );
    connect( m_base->configureScriptButton, SIGNAL( clicked() ), SLOT( slotConfigureScript() ) );

    QSize sz = sizeHint();
    setMinimumSize( kMax( 350, sz.width() ), kMax( 250, sz.height() ) );
    resize( sizeHint() );
}


ScriptManager::~ScriptManager()
{
    DEBUG_BEGIN

    s_instance = 0;

    debug() << "Killing running scripts.\n";

    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it )
        delete it.data().process;

    DEBUG_END
}


////////////////////////////////////////////////////////////////////////////////
// private interface
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::slotAddScript()
{
    KFileDialog dia( QString::null, "*.*|" + i18n("amaroK Scripts" ), 0, 0, true );
    kapp->setTopWidget( &dia );
    dia.setCaption( kapp->makeStdCaption( i18n( "Select Script" ) ) );
    dia.setMode( KFile::File | KFile::ExistingOnly );
    dia.exec();

    QString path = dia.selectedURL().path();

    if ( !path.isEmpty() ) {
        KURL url;
        url.setPath( path );

        KListViewItem* li = new KListViewItem( m_base->directoryListView, url.fileName() );
        li->setPixmap( 0, SmallIcon( "stop" ) );

        ScriptItem item;
        item.url = url;
        item.process = 0;
        item.li = li;

        m_scripts[url.fileName()] = item;
    }
}


void
ScriptManager::slotRemoveScript()
{
    if ( !m_base->directoryListView->selectedItem() ) return ;

    QString name = m_base->directoryListView->selectedItem()->text( 0 );
    m_scripts.erase( name );

    delete m_base->directoryListView->selectedItem();
}


void
ScriptManager::slotEditScript()
{
    DEBUG_FUNC_INFO

    if ( !m_base->directoryListView->selectedItem() ) return ;

    QString name = m_base->directoryListView->selectedItem()->text( 0 );
    QFile file( m_scripts[name].url.path() );

    if ( file.open( IO_ReadWrite ) ){
        KTextEdit* editor = new KTextEdit();
        kapp->setTopWidget( editor );
        editor->setCaption( kapp->makeStdCaption( i18n( "Edit Script" ) ) );

        QTextStream stream( &file );
        editor->setText( stream.read() );
        editor->setTextFormat( QTextEdit::PlainText );
        editor->resize( 640, 480 );
        editor->show();
    }
}


void
ScriptManager::slotRunScript()
{
    DEBUG_BEGIN

    if ( !m_base->directoryListView->selectedItem() ) return ;

    QListViewItem* li = m_base->directoryListView->selectedItem();
    QString name = li->text( 0 );

    // Return when this script is already running
    if ( m_scripts[name].process ) return;

    KURL url = m_scripts[name].url;

    li->setPixmap( 0, SmallIcon( "player_play" ) );
    debug() << "Running script: " << url.path() << endl;

    KProcess* script = new KProcess( this );
    *script << url.path();
    script->setWorkingDirectory( url.directory() );
    script->start( KProcess::NotifyOnExit, KProcess::Stdin );
    m_scripts[name].process = script;
    connect( script, SIGNAL( processExited( KProcess* ) ), SLOT( scriptFinished( KProcess* ) ) );

    DEBUG_END
}


void
ScriptManager::slotStopScript()
{
    DEBUG_FUNC_INFO

    if ( !m_base->directoryListView->selectedItem() ) return ;

    QListViewItem* li = m_base->directoryListView->selectedItem();
    QString name = li->text( 0 );

    // Kill script process
    if ( m_scripts[name].process ) {
        delete m_scripts[name].process;
        m_scripts[name].process = 0;
    }

    li->setPixmap( 0, SmallIcon( "stop" ) );
}


void
ScriptManager::slotConfigureScript()
{
    DEBUG_BEGIN

    if ( !m_base->directoryListView->selectedItem() ) return;

    QString name = m_base->directoryListView->selectedItem()->text( 0 );
    if ( !m_scripts[name].process ) return;

    KURL url = m_scripts[name].url;
    QDir::setCurrent( url.directory() );

    QString command( "configure\n" );
    m_scripts[name].process->writeStdin( command.latin1(), command.length() );

    debug() << "Starting script configuration." << endl;

    DEBUG_END
}


void
ScriptManager::scriptFinished( KProcess* process ) //SLOT
{
    DEBUG_FUNC_INFO

    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it ) {
        if ( it.data().process == process ) {
                delete it.data().process;
                it.data().process = 0;
                it.data().li->setPixmap( 0, SmallIcon( "stop" ) );
        }
    }
}


#include "scriptmanager.moc"
