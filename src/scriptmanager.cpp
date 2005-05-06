/***************************************************************************
 *   Copyright (C) 2004-2005 by Mark Kretschmann <markey@web.de>           *
 *                      2005 by Seb Ruiz <seb100@optusnet.com.au>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#define DEBUG_PREFIX "ScriptManager"

#include "amarok.h"
#include "debug.h"
#include "enginecontroller.h"
#include "metabundle.h"
#include "scriptmanager.h"
#include "scriptmanagerbase.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <qcheckbox.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qfont.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kprocio.h>
#include <kpushbutton.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <ktar.h>
#include <ktextedit.h>

#include <knewstuff/downloaddialog.h> // knewstuff script fetching
#include <knewstuff/engine.h>         // "
#include <knewstuff/knewstuff.h>      // "
#include <knewstuff/provider.h>       // "


////////////////////////////////////////////////////////////////////////////////
// class AmarokScriptNewStuff
////////////////////////////////////////////////////////////////////////////////

/**
 * GHNS Customised Download implementation.
 */
class AmarokScriptNewStuff : public KNewStuff
{
    public:
    AmarokScriptNewStuff(const QString &type, QWidget *parentWidget=0)
             : KNewStuff( type, parentWidget )
    {}

    bool install( const QString& fileName )
    {
        return ScriptManager::instance()->slotInstallScript( fileName );
    }

    virtual bool createUploadFile( const QString& ) { return false; } //make compile on kde 3.5
};


////////////////////////////////////////////////////////////////////////////////
// class ScriptManager
////////////////////////////////////////////////////////////////////////////////

ScriptManager* ScriptManager::s_instance = 0;


ScriptManager::ScriptManager( QWidget *parent, const char *name )
        : KDialogBase( parent, name, false, 0, 0, Ok, false )
        , EngineObserver( EngineController::instance() )
        , m_gui( new ScriptManagerBase( this ) )
{
    DEBUG_BLOCK

    s_instance = this;

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Script Manager" ) ) );

    setMainWidget( m_gui );
    m_gui->listView->setFullWidth( true );

    connect( m_gui->listView, SIGNAL( currentChanged( QListViewItem* ) ), SLOT( slotCurrentChanged( QListViewItem* ) ) );
    connect( m_gui->listView, SIGNAL( doubleClicked ( QListViewItem*, const QPoint&, int ) ), SLOT( slotRunScript() ) );
    connect( m_gui->listView, SIGNAL( rightButtonPressed ( QListViewItem*, const QPoint&, int ) ), SLOT( slotShowContextMenu( QListViewItem*, const QPoint& ) ) );

    connect( m_gui->installButton,   SIGNAL( clicked() ), SLOT( slotInstallScript() ) );
    connect( m_gui->retrieveButton,  SIGNAL( clicked() ), SLOT( slotRetrieveScript() ) );
    connect( m_gui->uninstallButton, SIGNAL( clicked() ), SLOT( slotUninstallScript() ) );
    connect( m_gui->runButton,       SIGNAL( clicked() ), SLOT( slotRunScript() ) );
    connect( m_gui->stopButton,      SIGNAL( clicked() ), SLOT( slotStopScript() ) );
    connect( m_gui->configureButton, SIGNAL( clicked() ), SLOT( slotConfigureScript() ) );
    connect( m_gui->aboutButton,     SIGNAL( clicked() ), SLOT( slotAboutScript() ) );

    m_gui->installButton  ->setIconSet( SmallIconSet( "fileopen" ) );
    m_gui->retrieveButton ->setIconSet( SmallIconSet( "khtml_kget" ) );
    m_gui->uninstallButton->setIconSet( SmallIconSet( "remove" ) );
    m_gui->runButton      ->setIconSet( SmallIconSet( "player_play" ) );
    m_gui->stopButton     ->setIconSet( SmallIconSet( "player_stop" ) );
    m_gui->configureButton->setIconSet( SmallIconSet( "configure" ) );
    m_gui->aboutButton    ->setIconSet( SmallIconSet( "help" ) );

    QSize sz = sizeHint();
    setMinimumSize( kMax( 350, sz.width() ), kMax( 250, sz.height() ) );
    resize( sizeHint() );

    // Delay this call via eventloop, because it's a bit slow and would block
    QTimer::singleShot( 0, this, SLOT( findScripts() ) );
}


ScriptManager::~ScriptManager()
{
    DEBUG_BLOCK

    debug() << "Killing running scripts.\n";

    QStringList runningScripts;
    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it ) {
        if ( it.data().process ) {
            delete it.data().process;
            runningScripts << it.key();
        }
    }

    // Save config
    KConfig* const config = amaroK::config( "ScriptManager" );
    config->writeEntry( "Running Scripts", runningScripts );
    config->writeEntry( "Auto Run", m_gui->checkBox_autoRun->isChecked() );

    s_instance = 0;
}


////////////////////////////////////////////////////////////////////////////////
// public
////////////////////////////////////////////////////////////////////////////////

bool
ScriptManager::runScript( const QString& name )
{
    if ( !m_scripts.contains( name ) )
        return false;

    m_gui->listView->setCurrentItem( m_scripts[name].li );
    return slotRunScript();
}


bool
ScriptManager::stopScript( const QString& name )
{
    if ( !m_scripts.contains( name ) )
        return false;

    m_gui->listView->setCurrentItem( m_scripts[name].li );
    slotStopScript();

    return true;
}


QStringList
ScriptManager::listRunningScripts()
{
    QStringList runningScripts;
    ScriptMap::ConstIterator it;

    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it )
        if ( it.data().process )
            runningScripts << it.key();

    return runningScripts;
}


////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::findScripts() //SLOT
{
    DEBUG_BLOCK

    const QStringList allFiles = kapp->dirs()->findAllResources( "data", "amarok/scripts/*", true );

    // Add found scripts to listview:

    QStringList::ConstIterator it;
    for ( it = allFiles.begin(); it != allFiles.end(); ++it )
        if ( QFileInfo( *it ).isExecutable() )
            loadScript( *it );

    // Handle auto-run:

    KConfig* const config = amaroK::config( "ScriptManager" );
    const bool autoRun = config->readBoolEntry( "Auto Run", false );
    m_gui->checkBox_autoRun->setChecked( autoRun );

    if ( autoRun ) {
        const QStringList runningScripts = config->readListEntry( "Running Scripts" );

        QStringList::ConstIterator it;
        for ( it = runningScripts.begin(); it != runningScripts.end(); ++it ) {
            if ( m_scripts.contains( *it ) ) {
                debug() << "Auto-running script: " << *it << endl;
                m_gui->listView->setCurrentItem( m_scripts[*it].li );
                slotRunScript();
            }
        }
    }

    m_gui->listView->setCurrentItem( m_gui->listView->firstChild() );
    slotCurrentChanged( m_gui->listView->currentItem() );
}


void
ScriptManager::slotCurrentChanged( QListViewItem* item )
{
    if ( item ) {
        const QString name = item->text( 0 );
        m_gui->uninstallButton->setEnabled( true );
        m_gui->runButton->setEnabled( !m_scripts[name].process );
        m_gui->stopButton->setEnabled( m_scripts[name].process );
        m_gui->configureButton->setEnabled( m_scripts[name].process );
        m_gui->aboutButton->setEnabled( true );
    }
    else {
        m_gui->uninstallButton->setEnabled( false );
        m_gui->runButton->setEnabled( false );
        m_gui->stopButton->setEnabled( false );
        m_gui->configureButton->setEnabled( false );
        m_gui->aboutButton->setEnabled( false );
    }
}


bool
ScriptManager::slotInstallScript( const QString& path )
{
    DEBUG_BLOCK

    QString _path = path;

    if ( path.isNull() ) {
        KFileDialog dia( QString::null, "*.amarokscript.tar *.amarokscript.tar.bz2 *.amarokscript.tar.gz|" + i18n( "Script Packages (*.amarokscript.tar, *.amarokscript.tar.bz2, *.amarokscript.tar.gz)" ), 0, 0, true );
        kapp->setTopWidget( &dia );
        dia.setCaption( kapp->makeStdCaption( i18n( "Select Script Package" ) ) );
        dia.setMode( KFile::File | KFile::ExistingOnly );
        if ( !dia.exec() ) return false;
        _path = dia.selectedURL().path();
    }

    KTar archive( _path );
    if ( !archive.open( IO_ReadOnly ) ) {
        KMessageBox::sorry( 0, i18n( "Could not read this package." ) );
        return false;
    }

    QString destination = amaroK::saveLocation( "scripts/" );
    const KArchiveDirectory* const archiveDir = archive.directory();

    // Prevent installing a script that's already installed
    const QString scriptFolder = destination + archiveDir->entries().first();
    if ( QFile::exists( scriptFolder ) ) {
        KMessageBox::error( 0, i18n( "A script with the name '%1' is already installed. "
                                     "Please uninstall it first." ).arg( archiveDir->entries().first() ) );
        return false;
    }

    archiveDir->copyTo( destination );
    m_installSuccess = false;
    recurseInstall( archiveDir, destination );

    if ( m_installSuccess ) {
        KMessageBox::information( 0, i18n( "Script successfully installed." ) );
        return true;
    }
    else {
        KMessageBox::sorry( 0, i18n( "<p>Script installation failed.</p>"
                                     "<p>The package did not contain an executable file. "
                                     "Please inform the package maintainer about this error.</p>" ) );

        // Delete directory recursively
        KIO::NetAccess::del( KURL::fromPathOrURL( scriptFolder ), 0 );
    }

    return false;
}


void
ScriptManager::recurseInstall( const KArchiveDirectory* archiveDir, const QString& destination )
{
    const QStringList entries = archiveDir->entries();

    QStringList::ConstIterator it;
    for ( it = entries.begin(); it != entries.end(); ++it ) {
        const QString entry = *it;
        const KArchiveEntry* const archEntry = archiveDir->entry( entry );

        if ( archEntry->isDirectory() ) {
            KArchiveDirectory* const dir = (KArchiveDirectory*) archEntry;
            recurseInstall( dir, destination + entry + "/" );
        }
        else {
            ::chmod( QFile::encodeName( destination + entry ), archEntry->permissions() );

            if ( QFileInfo( destination + entry ).isExecutable() ) {
                loadScript( destination + entry );
                m_installSuccess = true;
            }
        }
    }
}


void
ScriptManager::slotRetrieveScript()
{
    // Delete KNewStuff's configuration entries. These entries reflect which scripts
    // are already installed. As we cannot yet keep them in sync after uninstalling
    // scripts, we deactivate the check marks entirely.
    amaroK::config()->deleteGroup( "KNewStuffStatus" );

    // we need this because KNewStuffGeneric's install function isn't clever enough
    AmarokScriptNewStuff *kns = new AmarokScriptNewStuff( "amarok/script", this );
    KNS::Engine *engine = new KNS::Engine( kns, "amarok/script", this );
    KNS::DownloadDialog *d = new KNS::DownloadDialog( engine, this );
    d->setType( "amarok/script" );
    // you have to do this by hand when providing your own Engine
    KNS::ProviderLoader *p = new KNS::ProviderLoader( this );
    QObject::connect( p, SIGNAL( providersLoaded(Provider::List*) ), d, SLOT( slotProviders (Provider::List *) ) );
    p->load( "amarok/script", "http://amarok.kde.org/knewstuff/amarokscripts-providers.xml" );

    d->exec();
}


void
ScriptManager::slotUninstallScript()
{
    DEBUG_BLOCK

    const QString name = m_gui->listView->currentItem()->text( 0 );

    if ( KMessageBox::warningYesNo( 0, i18n( "Are you sure you want to uninstall the script '%1'?" ).arg( name ) ) == KMessageBox::No )
        return;

    const QString directory = m_scripts[name].url.directory();

    // Delete directory recursively
    const KURL url = KURL::fromPathOrURL( directory );
    if ( !KIO::NetAccess::del( url, 0 ) ) {
        KMessageBox::sorry( 0, i18n( "<p>Could not uninstall this script.</p><p>The ScriptManager can only uninstall scripts which have been installed as packages.</p>" ) );
        return;
    }

    // Find all scripts that were in the uninstalled folder
    QStringList keys;
    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it )
        if ( it.data().url.directory() == directory )
            keys << it.key();

    // Kill script processes, remove entries from script list
    QStringList::Iterator itKeys;
    for ( itKeys = keys.begin(); itKeys != keys.end(); ++itKeys ) {
        delete m_scripts[*itKeys].li;
        delete m_scripts[*itKeys].process;
        m_scripts.erase( *itKeys );
    }
}


bool
ScriptManager::slotRunScript()
{
    DEBUG_BLOCK

    QListViewItem* const li = m_gui->listView->currentItem();
    const QString name = li->text( 0 );

    // Don't start a script twice
    if ( m_scripts[name].process ) return false;

    const KURL url = m_scripts[name].url;
    KProcIO* script = new KProcIO();
//     script->setComm( (KProcess::Communication) ( KProcess::Stdin | KProcess::Stdout | KProcess::Stderr ) );

    *script << url.path();
    script->setWorkingDirectory( amaroK::saveLocation( "scripts-data/" ) );

    if ( !script->start( KProcess::NotifyOnExit, true ) ) {
        KMessageBox::sorry( 0, i18n( "<p>Could not start the script <i>%1</i>.</p>"
                                     "<p>Please make sure that the file has execute (+x) permissions.</p>" ).arg( name ) );
        delete script;
        return false;
    }

    li->setPixmap( 0, SmallIcon( "player_play" ) );
    debug() << "Running script: " << url.path() << endl;

    m_scripts[name].process = script;
    slotCurrentChanged( m_gui->listView->currentItem() );
//     connect( script, SIGNAL( receivedStdout( KProcess*, char*, int ) ), SLOT( slotReceivedStdout( KProcess*, char*, int ) ) );
    connect( script, SIGNAL( processExited( KProcess* ) ), SLOT( scriptFinished( KProcess* ) ) );
    return true;
}


void
ScriptManager::slotStopScript()
{
    DEBUG_BLOCK

    QListViewItem* const li = m_gui->listView->currentItem();
    const QString name = li->text( 0 );

    // Kill script process (with SIGTERM)
    m_scripts[name].process->kill();
    m_scripts[name].process->detach();

    delete m_scripts[name].process;
    m_scripts[name].process = 0;
    slotCurrentChanged( m_gui->listView->currentItem() );

    li->setPixmap( 0, SmallIcon( "stop" ) );
}


void
ScriptManager::slotConfigureScript()
{
    DEBUG_BLOCK

    const QString name = m_gui->listView->currentItem()->text( 0 );
    if ( !m_scripts[name].process ) return;

    const KURL url = m_scripts[name].url;
    QDir::setCurrent( url.directory() );

    m_scripts[name].process->writeStdin( "configure" );

    debug() << "Starting script configuration." << endl;
}


void
ScriptManager::slotAboutScript()
{
    DEBUG_BLOCK

    const QString name = m_gui->listView->currentItem()->text( 0 );
    QFile file( m_scripts[name].url.directory( false ) + "README" );
    debug() << "Path: " << file.name() << endl;

    if ( !file.open( IO_ReadOnly ) ) {
        KMessageBox::sorry( 0, i18n( "There is no information available for this script." ) );
        return;
    }

    KTextEdit* editor = new KTextEdit();
    kapp->setTopWidget( editor );
    editor->setCaption( kapp->makeStdCaption( i18n( "About %1" ).arg( name ) ) );
    editor->setReadOnly( true );

    QFont font( "fixed" );
    font.setFixedPitch( true );
    font.setStyleHint( QFont::TypeWriter );
    editor->setFont( font );

    QTextStream stream( &file );
    editor->setText( stream.read() );
    editor->setTextFormat( QTextEdit::PlainText );
    editor->resize( 640, 480 );
    editor->show();
}


void
ScriptManager::slotShowContextMenu( QListViewItem* item, const QPoint& pos )
{
    DEBUG_BLOCK
    if ( !item ) return;

    // Look up script entry in our map
    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it )
        if ( it.data().li == item ) break;

    enum { SHOW_LOG, EDIT };
    KPopupMenu menu;
    menu.insertTitle( i18n( "Debugging" ) );
    menu.insertItem( SmallIconSet( "history" ), i18n( "Show Output Log" ), SHOW_LOG );
    menu.insertItem( SmallIconSet( "edit" ), i18n( "Edit" ), EDIT );
    menu.setItemEnabled( SHOW_LOG, it.data().process );
    const int id = menu.exec( pos );

    switch ( id )
    {
        case EDIT:
            KRun::runCommand( "kwrite " + it.data().url.path() );
            break;

        case SHOW_LOG:
            QString line;
            while ( it.data().process->readln( line ) != -1 )
                it.data().log += line;

            KTextEdit* editor = new KTextEdit();
            kapp->setTopWidget( editor );
            editor->setCaption( kapp->makeStdCaption( i18n( "Output Log for %1" ).arg( it.key() ) ) );
            editor->setReadOnly( true );

            QFont font( "fixed" );
            font.setFixedPitch( true );
            font.setStyleHint( QFont::TypeWriter );
            editor->setFont( font );

            editor->setText( it.data().log );
            editor->setTextFormat( QTextEdit::PlainText );
            editor->resize( 640, 480 );
            editor->show();
            break;
    }
}


void
ScriptManager::slotReceivedStdout( KProcess* process, char* buf, int len )
{
    DEBUG_BLOCK

    // Look up script entry in our map
    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it )
        if ( it.data().process == process ) break;

    it.data().log += QString::fromLatin1( buf, len );
}


void
ScriptManager::scriptFinished( KProcess* process ) //SLOT
{
    DEBUG_BLOCK

    // Look up script entry in our map
    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it )
        if ( it.data().process == process ) break;

    // Check if there was an error on exit
    if ( process->normalExit() && process->exitStatus() != 0 ) {
        // Read Stderr log
        KProcIO* proc = static_cast<KProcIO*>( process );
        QString line, details;
        while ( proc->readln( line ) != -1 )
            details += line;

        KMessageBox::detailedError( 0, i18n( "The script '%1' exited with error code: %2" )
                                           .arg( it.key() ).arg( process->exitStatus() ), details );
    }

    // Destroy script process
    delete it.data().process;
    it.data().process = 0;
    it.data().li->setPixmap( 0, SmallIcon( "stop" ) );
    slotCurrentChanged( m_gui->listView->currentItem() );
}


////////////////////////////////////////////////////////////////////////////////
// private
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::notifyScripts( const QString& message )
{
    DEBUG_BLOCK

    debug() << "Sending notification: " << message << endl;

    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it ) {
        KProcIO* const proc = it.data().process;
        if ( proc ) proc->writeStdin( message );
    }
}


void
ScriptManager::loadScript( const QString& path )
{
    DEBUG_BLOCK

    if ( !path.isEmpty() ) {
        const KURL url = KURL::fromPathOrURL( path );

        KListViewItem* li = new KListViewItem( m_gui->listView, url.fileName() );
        li->setPixmap( 0, SmallIcon( "stop" ) );

        ScriptItem item;
        item.url = url;
        item.process = 0;
        item.li = li;

        m_scripts[url.fileName()] = item;
        debug() << "Loaded: " << url.fileName() << endl;

        slotCurrentChanged( m_gui->listView->currentItem() );
    }
}


void
ScriptManager::engineStateChanged( Engine::State state )
{
    DEBUG_BLOCK

    switch ( state )
    {
        case Engine::Empty:
            notifyScripts( "engineStateChange: empty" );
            break;

        case Engine::Idle:
            notifyScripts( "engineStateChange: idle" );
            break;

        case Engine::Paused:
            notifyScripts( "engineStateChange: paused" );
            break;

        case Engine::Playing:
            notifyScripts( "engineStateChange: playing" );
            break;
    }
}


void
ScriptManager::engineNewMetaData( const MetaBundle& /*bundle*/, bool /*trackChanged*/ )
{
    notifyScripts( "trackChange" );
}


void
ScriptManager::engineVolumeChanged( int newVolume )
{
    notifyScripts( "volumeChange: " + QString::number( newVolume ) );
}


#include "scriptmanager.moc"
