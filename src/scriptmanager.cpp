/***************************************************************************
 *   Copyright (C) 2004-2007 by Mark Kretschmann <markey@web.de>           *
 *                 2005-2007 by Seb Ruiz <me@sebruiz.net>                  *
 *                      2006 by Alexandre Oliveira <aleprj@gmail.com>      *
 *                      2006 by Martin Ellis <martin.ellis@kdemail.net>    *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#define DEBUG_PREFIX "ScriptManager"

#include "amarok.h"
#include "amarokconfig.h"
#include "contextbrowser.h"
#include "debug.h"
#include "enginecontroller.h"
#include "metabundle.h"
#include "scriptmanager.h"
#include "statusbar.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QLabel>
#include <QPixmap>
#include <QSettings>
#include <QTextCodec>
#include <QTimer>

#include <kaboutapplicationdialog.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmenu.h>
#include <k3procio.h>
#include <kprotocolmanager.h>
#include <kpushbutton.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <ktar.h>
#include <ktextedit.h>
#include <kwm.h>


namespace Amarok {
    void closeOpenFiles(int out, int in, int err) {
        for(int i = sysconf(_SC_OPEN_MAX) - 1; i > 2; i--)
            if(i!=out && i!=in && i!=err)
                close(i);
    }

     /**
    * This constructor is needed so that the correct codec is used. K3ProcIO defaults
    * to latin1, while the scanner uses UTF-8.
    */
    ProcIO::ProcIO() : K3ProcIO( QTextCodec::codecForName( "UTF-8" ) ) {}

    QString
    proxyForUrl(const QString& url)
    {
        KUrl kurl( url );

        QString proxy;

        if ( KProtocolManager::proxyForUrl( kurl ) !=
                QString::fromLatin1( "DIRECT" ) ) {
            KProtocolManager::slaveProtocol ( kurl, proxy );
        }

        return proxy;
    }

    QString
    proxyForProtocol(const QString& protocol)
    {
        return KProtocolManager::proxyFor( protocol );
    }


}

////////////////////////////////////////////////////////////////////////////////
// class AmarokScriptNewStuff
////////////////////////////////////////////////////////////////////////////////

/**
 * GHNS Customised Download implementation.
 */
#if 0 //TODO: PORT to KNS2
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
#endif

////////////////////////////////////////////////////////////////////////////////
// class ScriptManager
////////////////////////////////////////////////////////////////////////////////

ScriptManager* ScriptManager::s_instance = 0;


ScriptManager::ScriptManager( QWidget *parent, const char *name )
        : KDialog( parent )
        , EngineObserver( EngineController::instance() )
        , m_gui( new Ui::ScriptManagerBase() )
{
    DEBUG_BLOCK
    setObjectName( name );
    setModal( false );
    setButtons( Close );
    setDefaultButton( Close );
    showButtonSeparator( true );


    s_instance = this;

    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n( "Script Manager" ) ) );

    // Gives the window a small title bar, and skips a taskbar entry
#ifdef Q_WS_X11
    KWM::setType( winId(), NET::Utility );
    KWM::setState( winId(), NET::SkipTaskbar );
#endif

    QWidget* main = new QWidget( this );
    m_gui->setupUi( main );

    setMainWidget( main );

    /// Category items
    m_generalCategory    = new QTreeWidgetItem( m_gui->treeWidget );
    m_lyricsCategory     = new QTreeWidgetItem( m_gui->treeWidget );
    m_scoreCategory      = new QTreeWidgetItem( m_gui->treeWidget );
    m_transcodeCategory  = new QTreeWidgetItem( m_gui->treeWidget );

    m_generalCategory  ->setText( 0, i18n( "General" ) );
    m_lyricsCategory   ->setText( 0, i18n( "Lyrics" ) );
    m_scoreCategory    ->setText( 0, i18n( "Score" ) );
    m_transcodeCategory->setText( 0, i18n( "Transcoding" ) );

    m_generalCategory  ->setFlags( Qt::ItemIsEnabled );
    m_lyricsCategory   ->setFlags( Qt::ItemIsEnabled );
    m_scoreCategory    ->setFlags( Qt::ItemIsEnabled );
    m_transcodeCategory->setFlags( Qt::ItemIsEnabled );

    m_generalCategory  ->setIcon( 0, SmallIcon( Amarok::icon( "files" ) ) );
    m_lyricsCategory   ->setIcon( 0, SmallIcon( Amarok::icon( "files" ) ) );
    m_scoreCategory    ->setIcon( 0, SmallIcon( Amarok::icon( "files" ) ) );
    m_transcodeCategory->setIcon( 0, SmallIcon( Amarok::icon( "files" ) ) );

    // Restore the open/closed state of the category items
    KConfigGroup config = Amarok::config( "ScriptManager" );
    m_generalCategory  ->setExpanded( config.readEntry( "General category open", false ) );
    m_lyricsCategory   ->setExpanded( config.readEntry( "Lyrics category open", false ) );
    m_scoreCategory    ->setExpanded( config.readEntry( "Score category State", false ) );
    m_transcodeCategory->setExpanded( config.readEntry( "Transcode category open", false ) );

    connect( m_gui->treeWidget, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ), SLOT( slotCurrentChanged( QTreeWidgetItem* ) ) );
    connect( m_gui->treeWidget, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ), SLOT( slotRunScript() ) );
    connect( m_gui->treeWidget, SIGNAL( customContextMenuRequested ( const QPoint& ) ), SLOT( slotShowContextMenu( const QPoint& ) ) );

    connect( m_gui->installButton,   SIGNAL( clicked() ), SLOT( slotInstallScript() ) );
    connect( m_gui->retrieveButton,  SIGNAL( clicked() ), SLOT( slotRetrieveScript() ) );
    connect( m_gui->uninstallButton, SIGNAL( clicked() ), SLOT( slotUninstallScript() ) );
    connect( m_gui->runButton,       SIGNAL( clicked() ), SLOT( slotRunScript() ) );
    connect( m_gui->stopButton,      SIGNAL( clicked() ), SLOT( slotStopScript() ) );
    connect( m_gui->configureButton, SIGNAL( clicked() ), SLOT( slotConfigureScript() ) );
    connect( m_gui->aboutButton,     SIGNAL( clicked() ), SLOT( slotAboutScript() ) );

    m_gui->installButton  ->setIcon( KIcon( Amarok::icon( "files" ) ) );
    m_gui->retrieveButton ->setIcon( KIcon( Amarok::icon( "download" ) ) );
    m_gui->uninstallButton->setIcon( KIcon( Amarok::icon( "remove" ) ) );
    m_gui->runButton      ->setIcon( KIcon( Amarok::icon( "play" ) ) );
    m_gui->stopButton     ->setIcon( KIcon( Amarok::icon( "stop" ) ) );
    m_gui->configureButton->setIcon( KIcon( Amarok::icon( "configure" ) ) );
    m_gui->aboutButton    ->setIcon( KIcon( Amarok::icon( "info" ) ) );

    QSize sz = sizeHint();
    setMinimumSize( qMax( 350, sz.width() ), qMax( 250, sz.height() ) );
    resize( sizeHint() );

//FIXME: contex tbrowser changes
//     connect( this, SIGNAL(lyricsScriptChanged()), ContextBrowser::instance(), SLOT( lyricsScriptChanged() ) );

    // Delay this call via eventloop, because it's a bit slow and would block
    QTimer::singleShot( 0, this, SLOT( findScripts() ) );
}


ScriptManager::~ScriptManager()
{
    DEBUG_BLOCK

    QStringList runningScripts;
    foreach( QString key, m_scripts.keys() ) {
        if( m_scripts[key].process ) {
            terminateProcess( &m_scripts[key].process );
            runningScripts << key;
        }
    }

    // Save config
    KConfigGroup config = Amarok::config( "ScriptManager" );
    config.writeEntry( "Running Scripts", runningScripts );

    // Save the open/closed state of the category items
    config.writeEntry( "General category open", m_generalCategory->isExpanded() );
    config.writeEntry( "Lyrics category open", m_lyricsCategory->isExpanded() );
    config.writeEntry( "Score category open", m_scoreCategory->isExpanded() );
    config.writeEntry( "Transcode category open", m_transcodeCategory->isExpanded() );

    s_instance = 0;
}


////////////////////////////////////////////////////////////////////////////////
// public
////////////////////////////////////////////////////////////////////////////////

bool
ScriptManager::runScript( const QString& name, bool silent )
{
    if( !m_scripts.contains( name ) )
        return false;

    m_gui->treeWidget->setCurrentItem( m_scripts[name].li );
    return slotRunScript( silent );
}


bool
ScriptManager::stopScript( const QString& name )
{
    if( !m_scripts.contains( name ) )
        return false;

    m_gui->treeWidget->setCurrentItem( m_scripts[name].li );
    slotStopScript();

    return true;
}


QStringList
ScriptManager::listRunningScripts()
{
    QStringList runningScripts;
    foreach( QString key, m_scripts.keys() )
        if( m_scripts[key].process )
            runningScripts << key;

    return runningScripts;
}


void
ScriptManager::customMenuClicked( const QString& message )
{
    notifyScripts( "customMenuClicked: " + message );
}


QString
ScriptManager::specForScript( const QString& name )
{
    if( !m_scripts.contains( name ) )
        return QString();
    QFileInfo info( m_scripts[name].url.path() );
    const QString specPath = info.path() + '/' + info.completeBaseName() + ".spec";

    return specPath;
}


void
ScriptManager::notifyFetchLyrics( const QString& artist, const QString& title )
{
    const QString args = QUrl::toPercentEncoding( artist ) + ' ' + QUrl::toPercentEncoding( title );
    notifyScripts( "fetchLyrics " + args );
}


void
ScriptManager::notifyFetchLyricsByUrl( const QString& url )
{
    notifyScripts( "fetchLyricsByUrl " + url );
}


void ScriptManager::notifyTranscode( const QString& srcUrl, const QString& filetype )
{
    notifyScripts( "transcode " + srcUrl + ' ' + filetype );
}


void
ScriptManager::requestNewScore( const QString &url, double prevscore, int playcount, int length, float percentage, const QString &reason )
{
    const QString script = ensureScoreScriptRunning();
    if( script.isNull() )
    {
        Amarok::StatusBar::instance()->longMessage(
            i18n( "No score scripts were found, or none of them worked. Automatic scoring will be disabled. Sorry." ),
            KDE::StatusBar::Sorry );
        return;
    }

    m_scripts[script].process->writeStdin(
        QString( "requestNewScore %6 %1 %2 %3 %4 %5" )
        .arg( prevscore )
        .arg( playcount )
        .arg( length )
        .arg( percentage )
        .arg( reason )
        .arg( QString( QUrl::toPercentEncoding( url ) ) ) ); //last because it might have %s
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::findScripts() //SLOT
{
    const QStringList allFiles = KGlobal::dirs()->findAllResources( "data", "amarok/scripts/*",KStandardDirs::Recursive );

    // Add found scripts to treeWidget:
    foreach( QString str, allFiles )
        if( QFileInfo( str ).isExecutable() )
            loadScript( str );

    // Handle auto-run:

    KConfigGroup config = Amarok::config( "ScriptManager" );
    const QStringList runningScripts = config.readEntry( "Running Scripts", QStringList() );

    foreach( QString str, runningScripts )
        if( m_scripts.contains( str ) ) {
            debug() << "Auto-running script: " << str << endl;
            m_gui->treeWidget->setCurrentItem( m_scripts[str].li );
            slotRunScript();
        }

//FIXME    m_gui->treeWidget->setCurrentItem( m_gui->treeWidget->firstChild() );
    slotCurrentChanged( m_gui->treeWidget->currentItem() );
}


void
ScriptManager::slotCurrentChanged( QTreeWidgetItem* item )
{
    const bool isCategory = item == m_generalCategory ||
                            item == m_lyricsCategory ||
                            item == m_scoreCategory ||
                            item == m_transcodeCategory;

    if( item && !isCategory ) {
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
    QString _path = path;

    if( path.isNull() ) {
        _path = KFileDialog::getOpenFileName( KUrl(),
            "*.amarokscript.tar *.amarokscript.tar.bz2 *.amarokscript.tar.gz|"
            + i18n( "Script Packages (*.amarokscript.tar, *.amarokscript.tar.bz2, *.amarokscript.tar.gz)" )
            , this );
        if( _path.isNull() ) return false;
    }

    KTar archive( _path );
    if( !archive.open( QIODevice::ReadOnly ) ) {
        KMessageBox::sorry( 0, i18n( "Could not read this package." ) );
        return false;
    }

    QString destination = Amarok::saveLocation( "scripts/" );
    const KArchiveDirectory* const archiveDir = archive.directory();

    // Prevent installing a script that's already installed
    const QString scriptFolder = destination + archiveDir->entries().first();
    if( QFile::exists( scriptFolder ) ) {
        KMessageBox::error( 0, i18n( "A script with the name '%1' is already installed. "
                                     "Please uninstall it first.", archiveDir->entries().first() ) );
        return false;
    }

    archiveDir->copyTo( destination );
    m_installSuccess = false;
    recurseInstall( archiveDir, destination );

    if( m_installSuccess ) {
        KMessageBox::information( 0, i18n( "Script successfully installed." ) );
        return true;
    }
    else {
        KMessageBox::sorry( 0, i18n( "<p>Script installation failed.</p>"
                                     "<p>The package did not contain an executable file. "
                                     "Please inform the package maintainer about this error.</p>" ) );

        // Delete directory recursively
        KIO::NetAccess::del( KUrl( scriptFolder ), 0 );
    }

    return false;
}


void
ScriptManager::recurseInstall( const KArchiveDirectory* archiveDir, const QString& destination )
{
    const QStringList entries = archiveDir->entries();

    foreach( QString entry, entries ) {
        const KArchiveEntry* const archEntry = archiveDir->entry( entry );

        if( archEntry->isDirectory() ) {
            const KArchiveDirectory* const dir = static_cast<const KArchiveDirectory*>( archEntry );
            recurseInstall( dir, destination + entry + '/' );
        }
        else {
            ::chmod( QFile::encodeName( destination + entry ), archEntry->permissions() );

            if( QFileInfo( destination + entry ).isExecutable() ) {
                loadScript( destination + entry );
                m_installSuccess = true;
            }
        }
    }
}


void
ScriptManager::slotRetrieveScript()
{
#if 0 //FIXME: PORT To KNS2
    // Delete KNewStuff's configuration entries. These entries reflect which scripts
    // are already installed. As we cannot yet keep them in sync after uninstalling
    // scripts, we deactivate the check marks entirely.
    Amarok::config()->deleteGroup( "KNewStuffStatus" );

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
#endif
}


void
ScriptManager::slotUninstallScript()
{
    const QString name = m_gui->treeWidget->currentItem()->text( 0 );

    if( KMessageBox::warningContinueCancel( this, i18n( "Are you sure you want to uninstall the script '%1'?", name ), i18n("Uninstall Script"), KGuiItem( i18n("Uninstall") ) ) == KMessageBox::Cancel )
        return;

    if( m_scripts.find( name ) == m_scripts.end() )
        return;

    const QString directory = m_scripts[name].url.directory();

    // Delete directory recursively
    const KUrl url = KUrl( directory );
    if( !KIO::NetAccess::del( url, 0 ) ) {
        KMessageBox::sorry( 0, i18n( "<p>Could not uninstall this script.</p><p>The ScriptManager can only uninstall scripts which have been installed as packages.</p>" ) );
        return;
    }

    QStringList keys;

    // Find all scripts that were in the uninstalled folder
    foreach( QString key, m_scripts.keys() )
        if( m_scripts[key].url.directory() == directory )
            keys << key;

    // Terminate script processes, remove entries from script list
    foreach( QString key, keys ) {
        delete m_scripts[key].li;
        terminateProcess( &m_scripts[key].process );
        m_scripts.remove( key );
    }
}


bool
ScriptManager::slotRunScript( bool silent )
{
    if( !m_gui->runButton->isEnabled() ) return false;

    QTreeWidgetItem* const li = m_gui->treeWidget->currentItem();
    const QString name = li->text( 0 );

    if( m_scripts[name].type == "lyrics" && lyricsScriptRunning() != QString::null ) {
        if( !silent )
            KMessageBox::sorry( 0, i18n( "Another lyrics script is already running. "
                                         "You may only run one lyrics script at a time." ) );
        return false;
    }

    if( m_scripts[name].type == "transcode" && transcodeScriptRunning() != QString::null ) {
        if( !silent )
            KMessageBox::sorry( 0, i18n( "Another transcode script is already running. "
                                         "You may only run one transcode script at a time." ) );
        return false;
    }

    // Don't start a script twice
    if( m_scripts[name].process ) return false;

    Amarok::ProcIO* script = new Amarok::ProcIO();
    script->setComm( static_cast<K3Process::Communication>( K3Process::All ) );
    const KUrl url = m_scripts[name].url;
    *script << url.path();
    script->setWorkingDirectory( Amarok::saveLocation( "scripts-data/" ) );

    connect( script, SIGNAL( receivedStderr( K3Process*, char*, int ) ), SLOT( slotReceivedStderr( K3Process*, char*, int ) ) );
    connect( script, SIGNAL( receivedStdout( K3Process*, char*, int ) ), SLOT( slotReceivedStdout( K3Process*, char*, int ) ) );
    connect( script, SIGNAL( processExited( K3Process* ) ), SLOT( scriptFinished( K3Process* ) ) );

    if( script->start( K3Process::NotifyOnExit ) )
    {
        if( m_scripts[name].type == "score" && !scoreScriptRunning().isNull() )
        {
            stopScript( scoreScriptRunning() );
            m_gui->treeWidget->setCurrentItem( li );
        }
        AmarokConfig::setLastScoreScript( name );
    }
    else
    {
        if( !silent )
            KMessageBox::sorry( 0, i18n( "<p>Could not start the script <i>%1</i>.</p>"
                                         "<p>Please make sure that the file has execute (+x) permissions.</p>", name ) );
        delete script;
        return false;
    }

    li->setIcon( 0, SmallIcon( Amarok::icon( "play" ) ) );
    debug() << "Running script: " << url.path() << endl;

    m_scripts[name].process = script;
    slotCurrentChanged( m_gui->treeWidget->currentItem() );
    if( m_scripts[name].type == "lyrics" )
        emit lyricsScriptChanged();

    return true;
}


void
ScriptManager::slotStopScript()
{
    QTreeWidgetItem* const li = m_gui->treeWidget->currentItem();
    const QString name = li->text( 0 );

    // Just a sanity check
    if( m_scripts.find( name ) == m_scripts.end() )
        return;

    terminateProcess( &m_scripts[name].process );
    m_scripts[name].log = QString::null;
    slotCurrentChanged( m_gui->treeWidget->currentItem() );

    li->setIcon( 0, QPixmap() );
}


void
ScriptManager::slotConfigureScript()
{
    const QString name = m_gui->treeWidget->currentItem()->text( 0 );
    if( !m_scripts[name].process ) return;

    const KUrl url = m_scripts[name].url;
    QDir::setCurrent( url.directory() );

    m_scripts[name].process->writeStdin( QString("configure") );
}


void
ScriptManager::slotAboutScript()
{
    const QString name = m_gui->treeWidget->currentItem()->text( 0 );
    QFile readme( m_scripts[name].url.directory( KUrl::AppendTrailingSlash ) + "README" );
    QFile license( m_scripts[name].url.directory( KUrl::AppendTrailingSlash) + "COPYING" );

    if( !readme.open( QIODevice::ReadOnly ) ) {
        KMessageBox::sorry( 0, i18n( "There is no information available for this script." ) );
        return;
    }

    KAboutData aboutData( name.toLatin1(), name.toLatin1(), "1.0", readme.readAll() );

    KAboutApplicationDialog* about = new KAboutApplicationDialog( &aboutData, this );
    about->setButtons( KDialog::Ok );
    about->setDefaultButton( KDialog::Ok );

    kapp->setTopWidget( about );
    about->setCaption( KDialog::makeStandardCaption( i18n( "About %1", name ) ) );

    about->setInitialSize( QSize( 500, 350 ) );
    about->show();
}


void
ScriptManager::slotShowContextMenu( const QPoint& pos )
{
    QTreeWidgetItem* item = m_gui->treeWidget->itemAt( pos );

    const bool isCategory = item == m_generalCategory ||
                            item == m_lyricsCategory ||
                            item == m_scoreCategory ||
                            item == m_transcodeCategory;

    if( !item || isCategory ) return;

    // Find the script entry in our map
    QString key;
    foreach( key, m_scripts.keys() )
        if( m_scripts[key].li == item ) break;

    enum { SHOW_LOG, EDIT };
    KMenu menu;
    menu.addTitle( i18n( "Debugging" ) );
    QAction* logAction = menu.addAction( KIcon( Amarok::icon( "clock" ) ), i18n( "Show Output &Log" ) );
    QAction* editAction = menu.addAction( KIcon( Amarok::icon( "edit" ) ), i18n( "&Edit" ) );
    logAction->setData( SHOW_LOG );
    editAction->setData( EDIT );

    logAction->setEnabled( m_scripts[key].process != 0 );

    QAction* choice = menu.exec( mapToGlobal( pos ) );
    if( !choice ) return;
    const int id = choice->data().toInt();

    switch( id )
    {
        case EDIT:
            KRun::runCommand( "kwrite " + m_scripts[key].url.path() );
            break;

        case SHOW_LOG:
            QString line;
            while( m_scripts[key].process->readln( line ) != -1 )
                m_scripts[key].log += line;

            KTextEdit* editor = new KTextEdit( m_scripts[key].log );
            kapp->setTopWidget( editor );
            editor->setCaption( KDialog::makeStandardCaption( i18n( "Output Log for %1" ).arg( key ) ) );
            editor->setReadOnly( true );

            QFont font( "fixed" );
            font.setFixedPitch( true );
            font.setStyleHint( QFont::TypeWriter );
            editor->setFont( font );

            editor->resize( 500, 380 );
            editor->show();
            break;
    }
}


/* This is just a workaround, some scripts crash for some people if stdout is not handled. */
void
ScriptManager::slotReceivedStdout( K3Process*, char* buf, int len )
{
    debug() << QString::fromLatin1( buf, len ) << endl;
}


void
ScriptManager::slotReceivedStderr( K3Process* process, char* buf, int len )
{
    // Look up script entry in our map
    ScriptMap::Iterator it;
    ScriptMap::Iterator end( m_scripts.end() );
    for( it = m_scripts.begin(); it != end; ++it )
        if( it.value().process == process ) break;

    const QString text = QString::fromLatin1( buf, len );
    error() << it.key() << ":\n" << text << endl;

    if( it.value().log.length() > 20000 )
        it.value().log = "==== LOG TRUNCATED HERE ====\n";
    it.value().log += text;
}


void
ScriptManager::scriptFinished( K3Process* process ) //SLOT
{
    // Look up script entry in our map
    ScriptMap::Iterator it;
    ScriptMap::Iterator end( m_scripts.end() );
    for( it = m_scripts.begin(); it != end; ++it )
        if( it.value().process == process ) break;

    // Check if there was an error on exit
    if( process->normalExit() && process->exitStatus() != 0 )
        KMessageBox::detailedError( 0, i18n( "The script '%1' exited with error code: %2", it.key(), process->exitStatus() )
                                           ,it.value().log );

    // Destroy script process
    delete it.value().process;
    it.value().process = 0;
    it.value().log.clear();
    it.value().li->setIcon( 0, QPixmap() );
    slotCurrentChanged( m_gui->treeWidget->currentItem() );
}


////////////////////////////////////////////////////////////////////////////////
// private
////////////////////////////////////////////////////////////////////////////////

QStringList
ScriptManager::scriptsOfType( const QString &type ) const
{
    QStringList scripts;
    foreach( QString key, m_scripts.keys() )
        if( m_scripts[key].type == type )
            scripts += key;

    return scripts;
}


QString
ScriptManager::scriptRunningOfType( const QString &type ) const
{
    foreach( QString key, m_scripts.keys() )
        if( m_scripts[key].process && m_scripts[key].type == type )
            return key;

    return QString();
}


QString
ScriptManager::ensureScoreScriptRunning()
{
    QString s = scoreScriptRunning();
    if( !s.isNull() )
        return s;

    if( runScript( AmarokConfig::lastScoreScript(), true /*silent*/ ) )
        return AmarokConfig::lastScoreScript();

    const QString def = i18n( "Score" ) + ": " + "Default";
    if( runScript( def, true ) )
        return def;

    const QStringList scripts = scoreScripts();
    for( QStringList::const_iterator it = scripts.begin(), end = scripts.end(); it != end; ++it )
        if( runScript( *it, true ) )
            return *it;

    return QString();
}


void
ScriptManager::terminateProcess( K3ProcIO** proc )
{
    if( *proc ) {
        (*proc)->kill(); // Sends SIGTERM
        (*proc)->detach();

        delete *proc;
        *proc = 0;
    }
}


void
ScriptManager::notifyScripts( const QString& message )
{
    foreach( ScriptItem item, m_scripts ) {
        K3ProcIO* const proc = item.process;
        if( proc ) proc->writeStdin( message );
    }
}


void
ScriptManager::loadScript( const QString& path )
{
    if( !path.isEmpty() ) {
        const KUrl url = KUrl( path );
        QString name = url.fileName();
        QString type = "generic";

        // Read and parse .spec file, if exists
        QFileInfo info( path );
        QTreeWidgetItem* li = 0;
        const QString specPath = info.path() + '/' + info.completeBaseName() + ".spec";
        if( QFile::exists( specPath ) ) {
            debug() << "Spec file found: " << specPath << endl;
            QSettings spec( specPath, QSettings::IniFormat );
            if( spec.contains( "name" ) )
                name = spec.value( "name" ).toString();
            if( spec.contains( "type" ) ) {
                type = spec.value( "type" ).toString();
                if( type == "lyrics" ) {
                    li = new QTreeWidgetItem( m_lyricsCategory );
                    li->setText( 0, name );
                }
                if( type == "transcode" ) {
                    li = new QTreeWidgetItem( m_transcodeCategory );
                    li->setText( 0, name );
                }
                if( type == "score" ) {
                    li = new QTreeWidgetItem( m_scoreCategory );
                    li->setText( 0, name );
                }
            }
        }

        if( !li ) {
            li = new QTreeWidgetItem( m_generalCategory );
            li->setText( 0, name );
        }

        li->setIcon( 0, QPixmap() );

        ScriptItem item;
        item.url = url;
        item.type = type;
        item.process = 0;
        item.li = li;

        m_scripts[name] = item;
        debug() << "Loaded: " << name << endl;

        slotCurrentChanged( m_gui->treeWidget->currentItem() );
    }
}


void
ScriptManager::engineStateChanged( Engine::State state, Engine::State /*oldState*/ )
{
    switch( state )
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
