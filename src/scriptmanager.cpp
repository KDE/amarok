// (c) 2003 Scott Wheeler <wheeler@kde.org>,
// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include "scriptmanager.h"
#include "scriptmanagerbase.h"

#include <qcheckbox.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <klistview.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <krun.h>
#include <ktextedit.h>


////////////////////////////////////////////////////////////////////////////////
// public interface
////////////////////////////////////////////////////////////////////////////////

ScriptManager* ScriptManager::s_instance = 0;


ScriptManager::ScriptManager( QWidget *parent, const char *name )
        : KDialogBase( parent, name, true, 0, Ok | Cancel, Ok, true )
        , m_base( new ScriptManagerBase( this ) )
{
    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Script Manager" ) ) );
    setModal( false );

    setMainWidget( m_base );
    m_base->directoryListView->setFullWidth( true );

    connect( m_base->addDirectoryButton, SIGNAL( clicked() ), SLOT( slotAddScript() ) );
    connect( m_base->removeDirectoryButton, SIGNAL( clicked() ), SLOT( slotRemoveScript() ) );
    connect( m_base->editButton, SIGNAL( clicked() ), SLOT( slotEditScript() ) );
    connect( m_base->runButton, SIGNAL( clicked() ), SLOT( slotRunScript() ) );
    connect( m_base->stopButton, SIGNAL( clicked() ), SLOT( slotStopScript() ) );
    connect( m_base->configureScriptButton, SIGNAL( clicked() ), SLOT( slotConfigureScript() ) );

    ScriptMap::ConstIterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it )
        new KListViewItem( m_base->directoryListView, it.key() );

    QSize sz = sizeHint();
    setMinimumSize( kMax( 350, sz.width() ), kMax( 250, sz.height() ) );
    resize( sizeHint() );
}


ScriptManager::~ScriptManager()
{
    s_instance = 0;
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

        new KListViewItem( m_base->directoryListView, url.fileName() );

        ScriptItem item;
        item.url = url;
        item.process = 0;

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
    kdDebug() << k_funcinfo << endl;

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
    kdDebug() << k_funcinfo << endl;

    if ( !m_base->directoryListView->selectedItem() ) return ;

    QString name = m_base->directoryListView->selectedItem()->text( 0 );
    KURL url = m_scripts[name].url;

    QDir::setCurrent( url.directory() );
    kdDebug() << "Running script: " << url.path() << endl;

    KRun* script = new KRun( url );
    m_scripts[name].process = script;
}


void
ScriptManager::slotStopScript()
{
    if ( !m_base->directoryListView->selectedItem() ) return ;
}


void
ScriptManager::slotConfigureScript()
{
    kdDebug() << k_funcinfo << endl;

    if ( !m_base->directoryListView->selectedItem() ) return ;

    QString name = m_base->directoryListView->selectedItem()->text( 0 );
    KURL url = m_scripts[name].url;

    QString configPath = url.path();
    configPath.insert( configPath.findRev( "." ), "-config" );
    url.setPath( configPath );
    QDir::setCurrent( url.directory() );

    kdDebug() << "Running config script: " << configPath << endl;
    KRun* script = new KRun( url );
}


#include "scriptmanager.moc"
