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
#include "scriptmanager.h"
#include "scriptmanagerbase.h"

#include <stdlib.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kpushbutton.h>
#include <krun.h>
#include <ktextedit.h>


ScriptManager* ScriptManager::s_instance = 0;


ScriptManager::ScriptManager( QWidget *parent, const char *name )
        : KDialogBase( parent, name, false, 0, 0, Ok, false )
        , EngineObserver( EngineController::instance() )
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

    restoreScripts();
}


ScriptManager::~ScriptManager()
{
    Debug::Block b( __PRETTY_FUNCTION__ );

    saveScripts();

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
ScriptManager::slotAddScript()
{
    Debug::Block b( __PRETTY_FUNCTION__ );

    //FIXME How to get the resource folder from KStandardDirs?
    QString folder( getenv( "KDEDIR" ) );
    folder.append( "share/apps/amarok/scripts" );
    debug() << "Folder: " << folder << endl;

    KFileDialog dia( folder, "*.*|" + i18n("amaroK Scripts" ), 0, 0, true );
    kapp->setTopWidget( &dia );
    dia.setCaption( kapp->makeStdCaption( i18n( "Select Script" ) ) );
    dia.setMode( KFile::File | KFile::ExistingOnly );
    dia.exec();

    loadScript( dia.selectedURL().path() );
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
    Debug::Block b( __PRETTY_FUNCTION__ );

    if ( !m_base->directoryListView->selectedItem() ) return ;

    QString name = m_base->directoryListView->selectedItem()->text( 0 );
    QFile file( m_scripts[name].url.path() );

    if ( file.isWritable() )
        file.open( IO_ReadWrite );
    else {
        KMessageBox::information( 0, i18n( "File is not writable, opening in read-only mode." ) );
        file.open( IO_ReadOnly );
    }

    KTextEdit* editor = new KTextEdit();
    kapp->setTopWidget( editor );
    editor->setCaption( kapp->makeStdCaption( i18n( "Edit Script" ) ) );

    QTextStream stream( &file );
    editor->setText( stream.read() );
    editor->setTextFormat( QTextEdit::PlainText );
    editor->resize( 640, 480 );
    editor->show();
}


void
ScriptManager::slotRunScript()
{
    Debug::Block b( __PRETTY_FUNCTION__ );

    if ( !m_base->directoryListView->selectedItem() ) return ;

    QListViewItem* li = m_base->directoryListView->selectedItem();
    QString name = li->text( 0 );

    // Return when this script is already running
    if ( m_scripts[name].process ) return;

    KURL url = m_scripts[name].url;
    KProcess* script = new KProcess( this );
    *script << url.path();
    script->setWorkingDirectory( amaroK::saveLocation( "scripts/" ) );

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
    Debug::Block b( __PRETTY_FUNCTION__ );

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
    Debug::Block b( __PRETTY_FUNCTION__ );

    if ( !m_base->directoryListView->selectedItem() ) return;

    QString name = m_base->directoryListView->selectedItem()->text( 0 );
    if ( !m_scripts[name].process ) return;

    KURL url = m_scripts[name].url;
    QDir::setCurrent( url.directory() );

    QString command( "configure\n" );
    m_scripts[name].process->writeStdin( command.latin1(), command.length() );

    debug() << "Starting script configuration." << endl;
}


void
ScriptManager::scriptFinished( KProcess* process ) //SLOT
{
    Debug::Block b( __PRETTY_FUNCTION__ );

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
    Debug::Block b( __PRETTY_FUNCTION__ );

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
    Debug::Block b( __PRETTY_FUNCTION__ );

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
ScriptManager::saveScripts()
{
    Debug::Block b( __PRETTY_FUNCTION__ );

    QStringList paths;
    ScriptMap::Iterator it;
    for ( it = m_scripts.begin(); it != m_scripts.end(); ++it )
        paths << it.data().url.path();

    KConfig* config = kapp->config();
    config->setGroup( "ScriptManager" );
    config->writePathEntry( "Scripts", paths );
}


void
ScriptManager::restoreScripts()
{
    Debug::Block b( __PRETTY_FUNCTION__ );

    KConfig* config = kapp->config();
    config->setGroup( "ScriptManager" );
    QStringList paths = config->readPathListEntry( "Scripts" );

    QStringList::Iterator it;
    for ( it = paths.begin(); it != paths.end(); ++it )
        loadScript( *it );
}


void
ScriptManager::engineStateChanged( Engine::State state )
{
    DEBUG_FUNC_INFO

    switch ( state )
    {
        case Engine::Empty:
            notifyScripts( "EngineStateChange: empty" );
            break;

        case Engine::Idle:
            notifyScripts( "EngineStateChange: idle" );
            break;

        case Engine::Paused:
            notifyScripts( "EngineStateChange: paused" );
            break;

        case Engine::Playing:
            notifyScripts( "EngineStateChange: playing" );
            break;
    }
}


#include "scriptmanager.moc"
