/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "ScriptsConfig"

#include "ScriptsConfig.h"

#include "amarokconfig.h"
#include "configdialog/ConfigDialog.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "scripting/scriptmanager/ScriptManager.h"
#include "ScriptSelector.h"
#include "ui_ScriptsConfig.h"

#include <KMessageBox>
//#include <KNS3/QtQuickDialogWrapper>
#include <KPluginMetaData>
#include <KTar>
#include <KZip>

#include <QFileDialog>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTimer>
#include <QVBoxLayout>

ScriptsConfig::ScriptsConfig( Amarok2ConfigDialog *parent )
    : ConfigDialogBase( parent )
    , m_configChanged( false )
    , m_parent( parent )
    , m_oldSelector( nullptr )
{
    DEBUG_BLOCK
    Ui::ScriptsConfig gui;
    gui.setupUi( this );

    m_uninstallButton = gui.uninstallButton;
    m_timer = new QTimer(this);
    connect( m_timer, &QTimer::timeout, this, &ScriptsConfig::slotUpdateScripts );
    m_timer->setInterval( 200 );

    // Load config // Disable script updating and KNewStuff for now. TODO review situation post 3.0
    /*gui.kcfg_AutoUpdateScripts->setChecked( AmarokConfig::autoUpdateScripts() );
    gui.manageButton->setIcon( QIcon::fromTheme( QStringLiteral("get-hot-new-stuff-amarok") ) );
    connect( gui.manageButton, &QAbstractButton::clicked,
             this, &ScriptsConfig::slotManageScripts );*/
    connect( gui.installButton, &QAbstractButton::clicked,
             this, &ScriptsConfig::installLocalScript );

    m_selector = gui.scriptSelector;
    m_verticalLayout = gui.verticalLayout;
    slotReloadScriptSelector();

    //connect( gui.reloadButton, &QAbstractButton::clicked, m_timer, QOverload<>::of(&QTimer::start) ); //TODO post-3.0
    connect( gui.uninstallButton, &QAbstractButton::clicked, this, &ScriptsConfig::slotUninstallScript );

    connect( ScriptManager::instance(), &ScriptManager::scriptsChanged,
             this, &ScriptsConfig::slotReloadScriptSelector );

    this->setEnabled( AmarokConfig::enableScripts() );
}

ScriptsConfig::~ScriptsConfig()
{}

void
ScriptsConfig::slotManageScripts()
{
    QStringList updateScriptsList;
    //KNS3::QtQuickDialogWrapper dialog(QStringLiteral("amarok.knsrc"), this);
    //dialog.exec();

    //if( !dialog.installedEntries().isEmpty() || !dialog.changedEntries().isEmpty() )
    //    m_timer->start();
}

void
ScriptsConfig::updateSettings()
{
    DEBUG_BLOCK
    if( m_configChanged )
    {
        m_selector->save();
        ScriptManager::instance()->configChanged( true );
    }
}

bool
ScriptsConfig::hasChanged()
{
    return m_configChanged;
}

bool
ScriptsConfig::isDefault()
{
    return false;
}

void
ScriptsConfig::slotConfigChanged( bool changed )
{
    m_configChanged = changed;
    if( changed )
        debug() << "config changed";
}

void
ScriptsConfig::installLocalScript()
{
    DEBUG_BLOCK
    // where's this config stored anyway, use amarokconfig instead?
    // the script can actually be updated if you get the folder name right
    int response =  KMessageBox::warningContinueCancel( this, i18n( "Manually installed scripts "
                                        "cannot be automatically updated, continue?" ), QString(), KStandardGuiItem::cont()
                                        , KStandardGuiItem::cancel(), QStringLiteral("manualScriptInstallWarning") );
    if( response == KMessageBox::Cancel )
        return;

    QString filePath = QFileDialog::getOpenFileName( this, i18n( "Select Archived Script" ) );
    if( filePath.isEmpty() )
        return;

    QString fileName = QFileInfo( filePath ).fileName();
    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForFile( filePath );
    QScopedPointer<KArchive> archive;
    if( mimeType.inherits( QStringLiteral("application/zip") ) )
        archive.reset( new KZip( filePath ) );
    else
        archive.reset( new KTar( filePath ) );

    if( !archive || !archive->open( QIODevice::ReadOnly ) )
    {
        KMessageBox::error( this, i18n( "Invalid Archive" ) );
        return;
    }

    QString destination = QStandardPaths::writableLocation( QStandardPaths::GenericDataLocation ) + QStringLiteral("/amarok/scripts/");
    const KArchiveDirectory* const archiveDir = archive->directory();
    const QDir dir( destination );
    bool json = true;
    const KArchiveFile *metadataFile = findScriptMetadataFile( archiveDir, false );
    if( !metadataFile ) // no json, trying spec
    {
        debug() << "No script JSON found, trying spec/desktop";
        json = false;
        metadataFile = findScriptMetadataFile( archiveDir, true );
    }

    if( !metadataFile )
    {
        KMessageBox::error( this, i18n( "Invalid Script File" ) );
        return;
    }

    QTemporaryFile tempFile;
    tempFile.open();
    QIODevice *device = metadataFile->createDevice();
    tempFile.write( device->readAll() );
    delete device;
    tempFile.close();

    KPluginMetaData newScriptInfo;
    if( json )
        newScriptInfo = KPluginMetaData( tempFile.fileName() );
    else
        newScriptInfo = ScriptManager::createMetadataFromSpec( tempFile.fileName() );
    if( !newScriptInfo.isValid() )
    {
        debug() << "Invalid script: problem loading" << ( json ? "json" : "spec" );
        KMessageBox::error( this, i18n( "Invalid Script File" ) );
        return;
    }

    if( ScriptManager::instance()->m_scripts.contains( newScriptInfo.pluginId() ) )
    {
        QString existingVersion = ScriptManager::instance()->m_scripts[ newScriptInfo.pluginId() ]->info().version();
        QString message = i18n( "Another script with the name %1 already exists\nExisting Script's "
                                "Version: %2\nSelected Script's Version: %3", newScriptInfo.name()
                                , existingVersion, newScriptInfo.version() );
        KMessageBox::error( this, message );
        return;
    }
    bool archiveContainsDir = archiveDir->entries().length() && archiveDir->entry( archiveDir->entries().first() )->isDirectory();
    if( !archiveContainsDir )
        destination += fileName;
    QString suffix = QStringLiteral("");
    while( dir.exists( destination + suffix ) && !archiveContainsDir )
    {
        suffix = QString::number( suffix.toInt() + 1 );
    }
    destination += suffix;
    dir.mkpath( destination );
    debug() << "script being copied to" << destination;
    archiveDir->copyTo( destination );
    KMessageBox::information( this, i18n( "The script %1 was successfully installed", newScriptInfo.name() ) );
    m_timer->start();
}

void
ScriptsConfig::slotReloadScriptSelector()
{
    DEBUG_BLOCK
    m_oldSelector = m_selector;
    m_selector = new ScriptSelector( this );
    QString key = QStringLiteral( "Generic" );
    m_selector->addScripts( ScriptManager::instance()->scripts( key ),
                            i18n("Generic") );

    key = QStringLiteral( "Lyrics" );
    m_selector->addScripts( ScriptManager::instance()->scripts( key ),
                            i18n("Lyrics") );

    key = QStringLiteral( "Scriptable Service" );
    m_selector->addScripts( ScriptManager::instance()->scripts( key ),
                            i18n("Scriptable Service") );
    connect( m_selector, &ScriptSelector::changed, this, &ScriptsConfig::slotConfigChanged );
    connect( m_selector, &ScriptSelector::filtered, m_uninstallButton, &QPushButton::setDisabled );
    connect( m_selector, &ScriptSelector::changed,
             qobject_cast<Amarok2ConfigDialog*>(m_parent), &Amarok2ConfigDialog::updateButtons );

    m_verticalLayout->insertWidget( 0, m_selector );
    m_verticalLayout->removeWidget( m_oldSelector );

    m_selector->setFilter( m_oldSelector->filter() );
    QTimer::singleShot( 0, this, &ScriptsConfig::restoreScrollBar );
}

void
ScriptsConfig::restoreScrollBar()
{
    if( !m_oldSelector )
        return;
    m_selector->setVerticalPosition( m_oldSelector->verticalPosition() );
    m_oldSelector->deleteLater();
}

void
ScriptsConfig::slotUpdateScripts()
{
    m_timer->stop();
    ScriptManager::instance()->updateAllScripts();
}

void
ScriptsConfig::slotUninstallScript()
{
    DEBUG_BLOCK
    if( !ScriptManager::instance()->m_scripts.contains( m_selector->currentItem() ) )
        return;

    ScriptItem *item = ScriptManager::instance()->m_scripts.value( m_selector->currentItem() );
    /*
    int response = KMessageBox::warningContinueCancel( this, i18n( "You are advised to only uninstall manually "
                                                                    "installed scripts using this button." ) );
    if( response == KMessageBox::Cancel )
        return;
    */

    QFileInfo metadataFile( item->metadataPath() );
    qDebug() << "About to remove folder " << metadataFile.path();
    QDir( metadataFile.path() ).removeRecursively();
    m_timer->start();
}

const KArchiveFile*
ScriptsConfig::findScriptMetadataFile( const KArchiveDirectory *dir, const bool spec ) const
{
    for( const QString &entry : dir->entries() )
    {
        if( dir->entry( entry )->isFile() )
        {
            if( (entry == QLatin1String("script.spec") && spec ) || ( entry == QLatin1String("script.json") && !spec ) )
                return static_cast<const KArchiveFile*>( dir->entry( entry ) );
        }
        else
        {
            if( entry != QLatin1String(".") && entry != QLatin1String("..") )
            {
                const KArchiveDirectory *subDir = static_cast<const KArchiveDirectory*>( dir->entry( entry ) );
                if( subDir )
                {
                    const KArchiveFile *file = findScriptMetadataFile( subDir, spec );
                    if( !file )
                        continue;
                    return file;
                }
            }
        }
    }
    return nullptr;
}
