/***************************************************************************
 *   Copyright (C) 2004-2006 by Mark Kretschmann <markey@web.de>           *
 *                      2005 by Seb Ruiz <me@sebruiz.net>                  *
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
#include "scriptmanagerbase.h"
#include "statusbar.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <qcheckbox.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qfont.h>
#include <qlabel.h>
#include <qtextcodec.h>
#include <qtimer.h>

#include <kaboutdialog.h>
#include <kapplication.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kprocio.h>
#include <kprotocolmanager.h>
#include <kpushbutton.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <ktar.h>
#include <ktextedit.h>
#include <kwin.h>

#include <knewstuff/downloaddialog.h> // knewstuff script fetching
#include <knewstuff/engine.h>         // "
#include <knewstuff/knewstuff.h>      // "
#include <knewstuff/provider.h>       // "


namespace Amarok {
    void closeOpenFiles(int out, int in, int err) {
        for(int i = sysconf(_SC_OPEN_MAX) - 1; i > 2; i--)
            if(i!=out && i!=in && i!=err)
                close(i);
    }

     /**
    * This constructor is needed so that the correct codec is used. KProcIO defaults
    * to latin1, while the scanner uses UTF-8.
    */
    ProcIO::ProcIO() : KProcIO( QTextCodec::codecForName( "UTF-8" ) ) {}

    QString
    proxyForUrl(const QString& url)
    {
        KURL kurl( url );

        QString proxy;

        if ( KProtocolManager::proxyForURL( kurl ) != 
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
        : KDialogBase( parent, name, false, QString::null, Close, Close, true )
        , EngineObserver( EngineController::instance() )
        , m_gui( new ScriptManagerBase( this ) )
{
    DEBUG_BLOCK

    s_instance = this;

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Script Manager" ) ) );

    // Gives the window a small title bar, and skips a taskbar entry
    KWin::setType( winId(), NET::Utility );
    KWin::setState( winId(), NET::SkipTaskbar );

    setMainWidget( m_gui );
    m_gui->listView->setRootIsDecorated( true );
    m_gui->listView->setFullWidth( true );
    m_gui->listView->setShowSortIndicator( true );


    /// Category items
    m_generalCategory    = new KListViewItem( m_gui->listView, i18n( "General" ) );
    m_lyricsCategory     = new KListViewItem( m_gui->listView, i18n( "Lyrics" ) );
    m_scoreCategory      = new KListViewItem( m_gui->listView, i18n( "Score" ) );
    m_transcodeCategory  = new KListViewItem( m_gui->listView, i18n( "Transcoding" ) );

    m_generalCategory  ->setSelectable( false );
    m_lyricsCategory   ->setSelectable( false );
    m_scoreCategory    ->setSelectable( false );
    m_transcodeCategory->setSelectable( false );

    m_generalCategory  ->setPixmap( 0, SmallIcon( Amarok::icon( "files" ) ) );
    m_lyricsCategory   ->setPixmap( 0, SmallIcon( Amarok::icon( "files" ) ) );
    m_scoreCategory    ->setPixmap( 0, SmallIcon( Amarok::icon( "files" ) ) );
    m_transcodeCategory->setPixmap( 0, SmallIcon( Amarok::icon( "files" ) ) );

    // Restore the open/closed state of the category items
    KConfig* const config = Amarok::config( "ScriptManager" );
    m_generalCategory  ->setOpen( config->readBoolEntry( "General category open" ) );
    m_lyricsCategory   ->setOpen( config->readBoolEntry( "Lyrics category open" ) );
    m_scoreCategory    ->setOpen( config->readBoolEntry( "Score category State" ) );
    m_transcodeCategory->setOpen( config->readBoolEntry( "Transcode category open" ) );

    connect( m_gui->listView, SIGNAL( currentChanged( QListViewItem* ) ), SLOT( slotCurrentChanged( QListViewItem* ) ) );
    connect( m_gui->listView, SIGNAL( doubleClicked ( QListViewItem*, const QPoint&, int ) ), SLOT( slotRunScript() ) );
    connect( m_gui->listView, SIGNAL( contextMenuRequested ( QListViewItem*, const QPoint&, int ) ), SLOT( slotShowContextMenu( QListViewItem*, const QPoint& ) ) );

    connect( m_gui->installButton,   SIGNAL( clicked() ), SLOT( slotInstallScript() ) );
    connect( m_gui->retrieveButton,  SIGNAL( clicked() ), SLOT( slotRetrieveScript() ) );
    connect( m_gui->uninstallButton, SIGNAL( clicked() ), SLOT( slotUninstallScript() ) );
    connect( m_gui->runButton,       SIGNAL( clicked() ), SLOT( slotRunScript() ) );
    connect( m_gui->stopButton,      SIGNAL( clicked() ), SLOT( slotStopScript() ) );
    connect( m_gui->configureButton, SIGNAL( clicked() ), SLOT( slotConfigureScript() ) );
    connect( m_gui->aboutButton,     SIGNAL( clicked() ), SLOT( slotAboutScript() ) );

    m_gui->installButton  ->setIconSet( SmallIconSet( Amarok::icon( "files" ) ) );
    m_gui->retrieveButton ->setIconSet( SmallIconSet( Amarok::icon( "download" ) ) );
    m_gui->uninstallButton->setIconSet( SmallIconSet( Amarok::icon( "remove" ) ) );
    m_gui->runButton      ->setIconSet( SmallIconSet( Amarok::icon( "play" ) ) );
    m_gui->stopButton     ->setIconSet( SmallIconSet( Amarok::icon( "stop" ) ) );
    m_gui->configureButton->setIconSet( SmallIconSet( Amarok::icon( "configure" ) ) );
    m_gui->aboutButton    ->setIconSet( SmallIconSet( Amarok::icon( "info" ) ) );

    QSize sz = sizeHint();
    setMinimumSize( kMax( 350, sz.width() ), kMax( 250, sz.height() ) );
    resize( sizeHint() );

    connect( this, SIGNAL(lyricsScriptChanged()), ContextBrowser::instance(), SLOT( lyricsScriptChanged() ) );

    // Delay this call via eventloop, because it's a bit slow and would block
    QTimer::singleShot( 0, this, SLOT( findScripts() ) );
}


ScriptManager::~ScriptManager()
{
    DEBUG_BLOCK

    QStringList runningScripts;
    ScriptMap::Iterator it;
    ScriptMap::Iterator end( m_scripts.end() );
    for( it = m_scripts.begin(); it != end; ++it ) {
        if( it.data().process ) {
            terminateProcess( &it.data().process );
            runningScripts << it.key();
        }
    }

    // Save config
    KConfig* const config = Amarok::config( "ScriptManager" );
    config->writeEntry( "Running Scripts", runningScripts );

    // Save the open/closed state of the category items
    config->writeEntry( "General category open", m_generalCategory->isOpen() );
    config->writeEntry( "Lyrics category open", m_lyricsCategory->isOpen() );
    config->writeEntry( "Score category open", m_scoreCategory->isOpen() );
    config->writeEntry( "Transcode category open", m_transcodeCategory->isOpen() );

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

    m_gui->listView->setCurrentItem( m_scripts[name].li );
    return slotRunScript( silent );
}


bool
ScriptManager::stopScript( const QString& name )
{
    if( !m_scripts.contains( name ) )
        return false;

    m_gui->listView->setCurrentItem( m_scripts[name].li );
    slotStopScript();

    return true;
}


QStringList
ScriptManager::listRunningScripts()
{
    QStringList runningScripts;
    foreachType( ScriptMap, m_scripts )
        if( it.data().process )
            runningScripts << it.key();

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
    const QString specPath = info.dirPath() + '/' + info.baseName( true ) + ".spec";

    return specPath;
}


void
ScriptManager::notifyFetchLyrics( const QString& artist, const QString& title )
{
    const QString args = KURL::encode_string( artist ) + ' ' + KURL::encode_string( title );
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
ScriptManager::notifyPlaylistChange( const QString& change)
{
   notifyScripts( "playlistChange: " + change );
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
        .arg( KURL::encode_string( url ) ) ); //last because it might have %s
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::findScripts() //SLOT
{
    const QStringList allFiles = kapp->dirs()->findAllResources( "data", "amarok/scripts/*", true );

    // Add found scripts to listview:
    {
        foreach( allFiles )
            if( QFileInfo( *it ).isExecutable() )
                loadScript( *it );
    }

    // Handle auto-run:

    KConfig* const config = Amarok::config( "ScriptManager" );
    const QStringList runningScripts = config->readListEntry( "Running Scripts" );

    {
        foreach( runningScripts )
            if( m_scripts.contains( *it ) ) {
                debug() << "Auto-running script: " << *it << endl;
                m_gui->listView->setCurrentItem( m_scripts[*it].li );
                slotRunScript();
            }
    }

    m_gui->listView->setCurrentItem( m_gui->listView->firstChild() );
    slotCurrentChanged( m_gui->listView->currentItem() );
}


void
ScriptManager::slotCurrentChanged( QListViewItem* item )
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
        _path = KFileDialog::getOpenFileName( QString::null,
            "*.amarokscript.tar *.amarokscript.tar.bz2 *.amarokscript.tar.gz|"
            + i18n( "Script Packages (*.amarokscript.tar, *.amarokscript.tar.bz2, *.amarokscript.tar.gz)" )
            , this
            , i18n( "Select Script Package" ) );
        if( _path.isNull() ) return false;
    }

    KTar archive( _path );
    if( !archive.open( IO_ReadOnly ) ) {
        KMessageBox::sorry( 0, i18n( "Could not read this package." ) );
        return false;
    }

    QString destination = Amarok::saveLocation( "scripts/" );
    const KArchiveDirectory* const archiveDir = archive.directory();

    // Prevent installing a script that's already installed
    const QString scriptFolder = destination + archiveDir->entries().first();
    if( QFile::exists( scriptFolder ) ) {
        KMessageBox::error( 0, i18n( "A script with the name '%1' is already installed. "
                                     "Please uninstall it first." ).arg( archiveDir->entries().first() ) );
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
        KIO::NetAccess::del( KURL::fromPathOrURL( scriptFolder ), 0 );
    }

    return false;
}


void
ScriptManager::recurseInstall( const KArchiveDirectory* archiveDir, const QString& destination )
{
    const QStringList entries = archiveDir->entries();

    foreach( entries ) {
        const QString entry = *it;
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
}


void
ScriptManager::slotUninstallScript()
{
    const QString name = m_gui->listView->currentItem()->text( 0 );

    if( KMessageBox::warningContinueCancel( 0, i18n( "Are you sure you want to uninstall the script '%1'?" ).arg( name ), i18n("Uninstall Script"), i18n("Uninstall") ) == KMessageBox::Cancel )
        return;

    if( m_scripts.find( name ) == m_scripts.end() )
        return;

    KURL scriptDirURL( m_scripts[name].url.upURL() );

    // find if the script is installed in the global or local scripts directory
    KURL scriptsDirURL;
    QStringList dirs = KGlobal::dirs()->findDirs( "data", "amarok/scripts/" );
    for ( QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it ) {
        scriptsDirURL = KURL::fromPathOrURL( *it );
        if ( scriptsDirURL.isParentOf( scriptDirURL ) )
            break;
    }

    // find the begining of this script directory tree
    KURL scriptDirUpURL = scriptDirURL.upURL();
    while ( ! scriptsDirURL.equals( scriptDirUpURL, true ) && scriptsDirURL.isParentOf( scriptDirUpURL ) ) {
        scriptDirURL = scriptDirUpURL;
        scriptDirUpURL = scriptDirURL.upURL();
    }

    // Delete script directory recursively
    if( !KIO::NetAccess::del( scriptDirURL, 0 ) ) {
        KMessageBox::sorry( 0, i18n( "<p>Could not uninstall this script.</p><p>The ScriptManager can only uninstall scripts which have been installed as packages.</p>" ) ); // only true when not running as root (which is reasonable)
        return;
    }

    QStringList keys;

    // Find all scripts that were in the uninstalled directory
    {
        foreachType( ScriptMap, m_scripts )
            if( scriptDirURL.isParentOf( it.data().url ) )
                keys << it.key();
    }

    // Terminate script processes, remove entries from script list
    {
        foreach( keys ) {
            delete m_scripts[*it].li;
            terminateProcess( &m_scripts[*it].process );
            m_scripts.erase( *it );
        }
    }
}


bool
ScriptManager::slotRunScript( bool silent )
{
    if( !m_gui->runButton->isEnabled() ) return false;

    QListViewItem* const li = m_gui->listView->currentItem();
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
    script->setComm( static_cast<KProcess::Communication>( KProcess::All ) );
    const KURL url = m_scripts[name].url;
    *script << url.path();
    script->setWorkingDirectory( Amarok::saveLocation( "scripts-data/" ) );

    connect( script, SIGNAL( receivedStderr( KProcess*, char*, int ) ), SLOT( slotReceivedStderr( KProcess*, char*, int ) ) );
    connect( script, SIGNAL( receivedStdout( KProcess*, char*, int ) ), SLOT( slotReceivedStdout( KProcess*, char*, int ) ) );
    connect( script, SIGNAL( processExited( KProcess* ) ), SLOT( scriptFinished( KProcess* ) ) );

    if( script->start( KProcess::NotifyOnExit ) )
    {
        if( m_scripts[name].type == "score" && !scoreScriptRunning().isNull() )
        {
            stopScript( scoreScriptRunning() );
            m_gui->listView->setCurrentItem( li );
        }
        AmarokConfig::setLastScoreScript( name );
    }
    else
    {
        if( !silent )
            KMessageBox::sorry( 0, i18n( "<p>Could not start the script <i>%1</i>.</p>"
                                         "<p>Please make sure that the file has execute (+x) permissions.</p>" ).arg( name ) );
        delete script;
        return false;
    }

    li->setPixmap( 0, SmallIcon( Amarok::icon( "play" ) ) );
    debug() << "Running script: " << url.path() << endl;

    m_scripts[name].process = script;
    slotCurrentChanged( m_gui->listView->currentItem() );
    if( m_scripts[name].type == "lyrics" )
        emit lyricsScriptChanged();

    return true;
}


void
ScriptManager::slotStopScript()
{
    QListViewItem* const li = m_gui->listView->currentItem();
    const QString name = li->text( 0 );

    // Just a sanity check
    if( m_scripts.find( name ) == m_scripts.end() )
        return;

    terminateProcess( &m_scripts[name].process );
    m_scripts[name].log = QString::null;
    slotCurrentChanged( m_gui->listView->currentItem() );

    li->setPixmap( 0, QPixmap() );
}


void
ScriptManager::slotConfigureScript()
{
    const QString name = m_gui->listView->currentItem()->text( 0 );
    if( !m_scripts[name].process ) return;

    const KURL url = m_scripts[name].url;
    QDir::setCurrent( url.directory() );

    m_scripts[name].process->writeStdin( "configure" );
}


void
ScriptManager::slotAboutScript()
{
    const QString name = m_gui->listView->currentItem()->text( 0 );
    QFile readme( m_scripts[name].url.directory( false ) + "README" );
    QFile license( m_scripts[name].url.directory( false ) + "COPYING" );

    if( !readme.open( IO_ReadOnly ) ) {
        KMessageBox::sorry( 0, i18n( "There is no information available for this script." ) );
        return;
    }

    KAboutDialog* about = new KAboutDialog( KAboutDialog::AbtTabbed|KAboutDialog::AbtProduct,
                                            QString::null,
                                            KDialogBase::Ok, KDialogBase::Ok, this );
    kapp->setTopWidget( about );
    about->setCaption( kapp->makeStdCaption( i18n( "About %1" ).arg( name ) ) );
    about->setProduct( "", "", "", "" );
    // Get rid of the confusing KDE version text
    QLabel* product = static_cast<QLabel*>( about->mainWidget()->child( "version" ) );
    if( product ) product->setText( i18n( "%1 Amarok Script" ).arg( name ) );

    about->addTextPage( i18n( "About" ), readme.readAll(), true );
    if( license.open( IO_ReadOnly ) )
        about->addLicensePage( i18n( "License" ), license.readAll() );

    about->setInitialSize( QSize( 500, 350 ) );
    about->show();
}


void
ScriptManager::slotShowContextMenu( QListViewItem* item, const QPoint& pos )
{
    const bool isCategory = item == m_generalCategory ||
                            item == m_lyricsCategory ||
                            item == m_scoreCategory ||
                            item == m_transcodeCategory;

    if( !item || isCategory ) return;

    // Look up script entry in our map
    ScriptMap::Iterator it;
    ScriptMap::Iterator end( m_scripts.end() );
    for( it = m_scripts.begin(); it != end; ++it )
        if( it.data().li == item ) break;

    enum { SHOW_LOG, EDIT };
    KPopupMenu menu;
    menu.insertTitle( i18n( "Debugging" ) );
    menu.insertItem( SmallIconSet( Amarok::icon( "clock" ) ), i18n( "Show Output &Log" ), SHOW_LOG );
    menu.insertItem( SmallIconSet( Amarok::icon( "edit" ) ), i18n( "&Edit" ), EDIT );
    menu.setItemEnabled( SHOW_LOG, it.data().process );
    const int id = menu.exec( pos );

    switch( id )
    {
        case EDIT:
            KRun::runCommand( "kwrite " + KProcess::quote(it.data().url.path()) );
            break;

        case SHOW_LOG:
            QString line;
            while( it.data().process->readln( line ) != -1 )
                it.data().log += line;

            KTextEdit* editor = new KTextEdit( it.data().log );
            kapp->setTopWidget( editor );
            editor->setCaption( kapp->makeStdCaption( i18n( "Output Log for %1" ).arg( it.key() ) ) );
            editor->setReadOnly( true );

            QFont font( "fixed" );
            font.setFixedPitch( true );
            font.setStyleHint( QFont::TypeWriter );
            editor->setFont( font );

            editor->setTextFormat( QTextEdit::PlainText );
            editor->resize( 500, 380 );
            editor->show();
            break;
    }
}


/* This is just a workaround, some scripts crash for some people if stdout is not handled. */
void
ScriptManager::slotReceivedStdout( KProcess*, char* buf, int len )
{
    debug() << QString::fromLatin1( buf, len ) << endl;
}


void
ScriptManager::slotReceivedStderr( KProcess* process, char* buf, int len )
{
    // Look up script entry in our map
    ScriptMap::Iterator it;
    ScriptMap::Iterator end( m_scripts.end() );
    for( it = m_scripts.begin(); it != end; ++it )
        if( it.data().process == process ) break;

    const QString text = QString::fromLatin1( buf, len );
    error() << it.key() << ":\n" << text << endl;

    if( it.data().log.length() > 20000 )
        it.data().log = "==== LOG TRUNCATED HERE ====\n";
    it.data().log += text;
}


void
ScriptManager::scriptFinished( KProcess* process ) //SLOT
{
    // Look up script entry in our map
    ScriptMap::Iterator it;
    ScriptMap::Iterator end( m_scripts.end() );
    for( it = m_scripts.begin(); it != end; ++it )
        if( it.data().process == process ) break;

    // Check if there was an error on exit
    if( process->normalExit() && process->exitStatus() != 0 )
        KMessageBox::detailedError( 0, i18n( "The script '%1' exited with error code: %2" )
                                           .arg( it.key() ).arg( process->exitStatus() )
                                           ,it.data().log );

    // Destroy script process
    delete it.data().process;
    it.data().process = 0;
    it.data().log = QString::null;
    it.data().li->setPixmap( 0, QPixmap() );
    slotCurrentChanged( m_gui->listView->currentItem() );
}


////////////////////////////////////////////////////////////////////////////////
// private
////////////////////////////////////////////////////////////////////////////////

QStringList
ScriptManager::scriptsOfType( const QString &type ) const
{
    QStringList scripts;
    foreachType( ScriptMap, m_scripts )
        if( it.data().type == type )
            scripts += it.key();

    return scripts;
}


QString
ScriptManager::scriptRunningOfType( const QString &type ) const
{
    foreachType( ScriptMap, m_scripts )
        if( it.data().process )
            if( it.data().type == type )
                return it.key();

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
ScriptManager::terminateProcess( KProcIO** proc )
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
    foreachType( ScriptMap, m_scripts ) {
        KProcIO* const proc = it.data().process;
        if( proc ) proc->writeStdin( message );
    }
}


void
ScriptManager::loadScript( const QString& path )
{
    if( !path.isEmpty() ) {
        const KURL url = KURL::fromPathOrURL( path );
        QString name = url.fileName();
        QString type = "generic";

        // Read and parse .spec file, if exists
        QFileInfo info( path );
        KListViewItem* li = 0;
        const QString specPath = info.dirPath() + '/' + info.baseName( true ) + ".spec";
        if( QFile::exists( specPath ) ) {
            KConfig spec( specPath, true, false );
            if( spec.hasKey( "name" ) )
                name = spec.readEntry( "name" );
            if( spec.hasKey( "type" ) ) {
                type = spec.readEntry( "type" );
                if( type == "lyrics" )
                    li = new KListViewItem( m_lyricsCategory, name );
                if( type == "transcode" )
                    li = new KListViewItem( m_transcodeCategory, name );
                if( type == "score" )
                    li = new KListViewItem( m_scoreCategory, name );
            }
        }

        if( !li )
            li = new KListViewItem( m_generalCategory, name );

        li->setPixmap( 0, QPixmap() );

        ScriptItem item;
        item.url = url;
        item.type = type;
        item.process = 0;
        item.li = li;

        m_scripts[name] = item;
        debug() << "Loaded: " << name << endl;

        slotCurrentChanged( m_gui->listView->currentItem() );
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
