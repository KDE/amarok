/***************************************************************************
 *   Copyright (C) 2004-2005 by Mark Kretschmann <markey@web.de>           *
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

#include <qdir.h>
#include <qfileinfo.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kpushbutton.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <ktar.h>
#include <ktextedit.h>


ScriptManager* ScriptManager::s_instance = 0;


ScriptManager::ScriptManager( QWidget *parent, const char *name )
        : KDialogBase( parent, name, false, 0, 0, Ok, false )
        , EngineObserver( EngineController::instance() )
        , m_base( new ScriptManagerBase( this ) )
{
    DEBUG_BLOCK

    s_instance = this;

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Script Manager" ) ) );

    setMainWidget( m_base );
    m_base->directoryListView->setFullWidth( true );

    connect( m_base->directoryListView, SIGNAL( currentChanged( QListViewItem* ) ), SLOT( slotCurrentChanged( QListViewItem* ) ) );

    connect( m_base->installButton, SIGNAL( clicked() ), SLOT( slotInstallScript() ) );
    connect( m_base->uninstallButton, SIGNAL( clicked() ), SLOT( slotUninstallScript() ) );
    connect( m_base->editButton, SIGNAL( clicked() ), SLOT( slotEditScript() ) );
    connect( m_base->runButton, SIGNAL( clicked() ), SLOT( slotRunScript() ) );
    connect( m_base->stopButton, SIGNAL( clicked() ), SLOT( slotStopScript() ) );
    connect( m_base->configureButton, SIGNAL( clicked() ), SLOT( slotConfigureScript() ) );
    connect( m_base->aboutButton, SIGNAL( clicked() ), SLOT( slotAboutScript() ) );

    QSize sz = sizeHint();
    setMinimumSize( kMax( 350, sz.width() ), kMax( 250, sz.height() ) );
    resize( sizeHint() );


    QTimer::singleShot( 0, this, SLOT( findScripts() ) );
}


ScriptManager::~ScriptManager()
{
    DEBUG_BLOCK

    debug() << "Killing running scripts.\n";
    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it )
        delete it.data().process;

    s_instance = 0;
}


////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////


void
ScriptManager::findScripts() //SLOT
{
    DEBUG_BLOCK

    QStringList allFiles = kapp->dirs()->findAllResources( "data", "amarok/scripts/*", true );

    //TODO Make this faster

    QStringList::Iterator it;
    for ( it = allFiles.begin(); it != allFiles.end(); ++it )
        if ( QFileInfo( *it ).isExecutable() )
            loadScript( *it );

    slotCurrentChanged( m_base->directoryListView->currentItem() );
}


void
ScriptManager::slotCurrentChanged( QListViewItem* item )
{
    const bool enable = item != 0;

    m_base->uninstallButton->setEnabled( enable );
    m_base->editButton->setEnabled( enable );
    m_base->runButton->setEnabled( enable );
    m_base->stopButton->setEnabled( enable );
    m_base->configureButton->setEnabled( enable );
    m_base->aboutButton->setEnabled( enable );
}


void
ScriptManager::slotInstallScript()
{
    DEBUG_BLOCK

    KFileDialog dia( QString::null, "*.tar *.tar.bz2 *.tar.gz|" + i18n( "Script packages (*.tar, *.tar.bz2, *.tar.gz)" ), 0, 0, true );
    kapp->setTopWidget( &dia );
    dia.setCaption( kapp->makeStdCaption( i18n( "Select Script Package" ) ) );
    dia.setMode( KFile::File | KFile::ExistingOnly );
    if ( !dia.exec() ) return;

    KTar archive( dia.selectedURL().path() );

    if ( !archive.open( IO_ReadOnly ) ) {
        KMessageBox::sorry( 0, i18n( "Could not read this package." ) );
        return;
    }

    QString destination = amaroK::saveLocation( "scripts/" );
    const KArchiveDirectory* archiveDir = archive.directory();
    archiveDir->copyTo( destination );
    destination += archiveDir->name() + "/";

    recurseInstall( archiveDir, destination );

    KMessageBox::information( 0, i18n( "Script successfully installed." ) );
}


/** Copies the file permissions from the tarball */
void
ScriptManager::recurseInstall( const KArchiveDirectory* archiveDir, const QString& destination )
{
    QStringList entries = archiveDir->entries();

    QStringList::Iterator it;
    for ( it = entries.begin(); it != entries.end(); ++it ) {
        const QString entry = *it;
        const KArchiveEntry* archEntry = archiveDir->entry( entry );

        ::chmod( QFile::encodeName( destination + entry ), archEntry->permissions() );

        if ( archEntry->isDirectory() ) {
            KArchiveDirectory* dir = (KArchiveDirectory*) archEntry;
            recurseInstall( dir, destination + entry + "/" );
        }
        else
            if ( QFileInfo( destination + entry ).isExecutable() )
                loadScript( destination + entry );
    }
}


void
ScriptManager::slotUninstallScript()
{
    DEBUG_BLOCK

    QString name = m_base->directoryListView->currentItem()->text( 0 );

    if ( KMessageBox::warningYesNo( 0, i18n( "Are you sure you want to uninstall the script '%1'?" ).arg( name ) ) == KMessageBox::No )
        return;

    const QString directory = m_scripts[name].url.directory();
    QDir dir( directory );
    QStringList files = dir.entryList();

    // Remove all files
    bool rmSuccess = false;
    QStringList::Iterator it;
    for ( it = files.begin(); it != files.end(); ++it )
        rmSuccess |= dir.remove( *it );

    if ( !rmSuccess ) {
        KMessageBox::sorry( 0, i18n( "Could not uninstall this script. The ScriptManager can only uninstall scripts that were installed as packages." ) );
        return;
    }

    // Remove directory as well
    dir.rmdir( directory );

    // Remove all scripts from internal list that were in the uninstalled directory
    ScriptMap::Iterator itScripts;
    for ( itScripts = m_scripts.begin(); itScripts != m_scripts.end(); ++itScripts ) {
        if ( itScripts.data().url.directory() == directory ) {
            delete itScripts.data().li;
            delete itScripts.data().process;
            m_scripts.erase( itScripts );
        }
    }
}


void
ScriptManager::slotEditScript()
{
    DEBUG_BLOCK

    KTextEdit* editor = new KTextEdit();
    kapp->setTopWidget( editor );
    editor->setCaption( kapp->makeStdCaption( i18n( "Edit Script" ) ) );

    QString name = m_base->directoryListView->currentItem()->text( 0 );
    QFile file( m_scripts[name].url.path() );

    if ( file.isWritable() )
        file.open( IO_ReadWrite );
    else {
        KMessageBox::information( 0, i18n( "File is not writable, opening in read-only mode." ) );
        file.open( IO_ReadOnly );
        editor->setReadOnly( true );
    }

    QTextStream stream( &file );
    editor->setText( stream.read() );
    editor->setTextFormat( QTextEdit::PlainText );
    editor->resize( 640, 480 );
    editor->show();
}


void
ScriptManager::slotRunScript()
{
    DEBUG_BLOCK

    QListViewItem* li = m_base->directoryListView->currentItem();
    QString name = li->text( 0 );

    // Return when this script is already running
    if ( m_scripts[name].process ) return;

    KURL url = m_scripts[name].url;
    KProcess* script = new KProcess( this );
    *script << url.path();
    script->setWorkingDirectory( amaroK::saveLocation( "scripts-data/" ) );

    if ( !script->start( KProcess::NotifyOnExit, KProcess::Stdin ) ) {
        KMessageBox::sorry( 0, i18n( "<p>Could not start the script <i>%1</i>.</p>"
                                     "<p>Please make sure that the file has execute (+x) permissions.</p>" ).arg( name ) );
        delete script;
        return;
    }

    li->setPixmap( 0, SmallIcon( "player_play" ) );
    debug() << "Running script: " << url.path() << endl;

    m_scripts[name].process = script;
    connect( script, SIGNAL( processExited( KProcess* ) ), SLOT( scriptFinished( KProcess* ) ) );
}


void
ScriptManager::slotStopScript()
{
    DEBUG_BLOCK

    QListViewItem* li = m_base->directoryListView->currentItem();
    QString name = li->text( 0 );

    // Kill script process
    delete m_scripts[name].process;
    m_scripts[name].process = 0;

    li->setPixmap( 0, SmallIcon( "stop" ) );
}


void
ScriptManager::slotConfigureScript()
{
    DEBUG_BLOCK

    QString name = m_base->directoryListView->currentItem()->text( 0 );
    if ( !m_scripts[name].process ) return;

    KURL url = m_scripts[name].url;
    QDir::setCurrent( url.directory() );

    QString command( "configure\n" );
    m_scripts[name].process->writeStdin( command.latin1(), command.length() );

    debug() << "Starting script configuration." << endl;
}


void
ScriptManager::slotAboutScript()
{
    DEBUG_BLOCK

    QString name = m_base->directoryListView->currentItem()->text( 0 );
    QFile file( m_scripts[name].url.directory( false ) + "README" );
    debug() << "Path: " << file.name() << endl;

    if ( !file.open( IO_ReadOnly ) ) {
        KMessageBox::sorry( 0, i18n( "There is no help text for this script." ) );
        return;
    }

    KTextEdit* editor = new KTextEdit();
    editor->setReadOnly( true );
    kapp->setTopWidget( editor );
    editor->setCaption( kapp->makeStdCaption( i18n( "About %1" ).arg( name ) ) );

    QTextStream stream( &file );
    editor->setText( stream.read() );
    editor->setTextFormat( QTextEdit::PlainText );
    editor->resize( 640, 480 );
    editor->show();
}


void
ScriptManager::scriptFinished( KProcess* process ) //SLOT
{
    DEBUG_BLOCK

    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it ) {
        if ( it.data().process == process ) {
            delete it.data().process;
            it.data().process = 0;
            it.data().li->setPixmap( 0, SmallIcon( "stop" ) );
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// private
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::notifyScripts( const QString& message )
{
    DEBUG_BLOCK

    // Append EOL
    QString msg = message;
    msg.append( "\n" );

    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it )
        if ( it.data().process )
            it.data().process->writeStdin( msg.latin1(), msg.length() );
}


void
ScriptManager::loadScript( const QString& path )
{
    DEBUG_BLOCK

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

        slotCurrentChanged( m_base->directoryListView->currentItem() );
    }
}


void
ScriptManager::engineStateChanged( Engine::State state )
{
    DEBUG_FUNC_INFO

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


#include "scriptmanager.moc"
