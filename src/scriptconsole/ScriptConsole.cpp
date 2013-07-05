/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#include "ScriptConsole.h"

#include "MainWindow.h"
#include "ScriptEditorDocument.h"
#include "ScriptItemDelegate.h"
#include "ScriptConsoleItem.h"

#include <QFileDialog>
#include <QListWidget>
#include <QScriptEngine>
#include <QShortcut>
#include <QSettings>
#include <QSplitter>
#include <QTemporaryFile>

#include <KAction>
#include <KMessageBox>
#include <KStandardDirs>
#include <KTextEditor/ContainerInterface>
#include <KTextEditor/Editor>
#include <KTextEditor/EditorChooser>
#include <KTextEditor/View>

using namespace AmarokScript;
using namespace ScriptConsole;

QWeakPointer<ScriptConsoleDialog> ScriptConsoleDialog::s_instance;

ScriptConsoleDialog*
ScriptConsoleDialog::instance()
{
    if( !s_instance )
        s_instance = new ScriptConsoleDialog( The::mainWindow() );
    return s_instance.data();
}

//private

ScriptConsoleDialog::ScriptConsoleDialog( QWidget *parent )
    : KDialog( parent, 0 )
    , m_currentItemIndex( -1 )
{
    setModal( false );
    setCaption( i18n( "Script Console" ) );
    setButtons( None );
    setWindowFlags( Qt::Window );
    setupUi( mainWidget() );
    m_editor = KTextEditor::EditorChooser::editor();
    if ( !m_editor )
    {
        KMessageBox::error(0, i18n("A KDE text-editor component could not be found.\n"
                                   "Please check your KDE installation."));
        return;
    }
    m_editor->readConfig( KGlobal::config().data() );
    KTextEditor::ContainerInterface *iface = qobject_cast<KTextEditor::ContainerInterface*>( m_editor );
    if (iface)
        iface->setContainer( parent );
    ScriptEditorDocument *document = new ScriptEditorDocument( this, m_editor->createDocument( this ) );
    m_view = document->createView( m_splitter );
    m_splitter->insertWidget( 1, m_view );
    m_splitter->setStretchFactor( 1, 5 );
    setMinimumSize( 600, 600 );
    show();

    ScriptItemDelegate *delegate = new ScriptItemDelegate( m_outputListWidget );
    m_outputListWidget->setItemDelegate( delegate );
    m_outputListWidget->setEditTriggers( QAbstractItemView::DoubleClicked );
    m_outputListWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    QListWidgetItem *item = new QListWidgetItem( "The Amarok Script Console allows you to easily execute"
                                                "JavaScript with access to all functions\nand methods you would"
                                                "have in an Amarok script.\nInformation on scripting for Amarok is"
                                                "available at:\nhttp://community.kde.org/Amarok/Development#Scripting"
                                                "\nExecute code: CTRL-Enter\nBack in code history: Page Up"
                                                "\nForward in code history: Page Down"
                                               , m_outputListWidget );
    item->setFlags( Qt::NoItemFlags );

    connect( delegate, SIGNAL(killAndClearButtonClicked(QModelIndex)), SLOT(slotKillAndClearScript(QModelIndex)) );
    connect( delegate, SIGNAL(toggleRunButtonClicked(QModelIndex)), SLOT(slotToggleScript(QModelIndex)) );

    KAction *action = new KAction( i18n("Execute Script"), this );
    action->setShortcut( Qt::CTRL + Qt::Key_Enter );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotExecuteNewScript()) );
    m_executeButton->setDefaultAction( action );
    addAction( action );
    action = new KAction( i18n("Kill and Clear All Scripts"), this );
    action->setShortcut( Qt::CTRL + Qt::Key_X );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotKillAllAndClear()) );
    m_killAndClearButton->setDefaultAction( action );
    addAction( action );
    action = new KAction( i18n("Previous Script"), this );
    action->setShortcut( QKeySequence::MoveToPreviousPage );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotBackHistory()) );
    addAction( action );
    action = new KAction( i18n("Next Script"), this );
    action->setShortcut( QKeySequence::MoveToNextPage );
    connect( action, SIGNAL(triggered(bool)), SLOT(slotForwardHistory()) );
    addAction( action );

    connect( m_outputListWidget->model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), SLOT(dataChanged()) );

    QSettings settings( "KDE", "Amarok" );
    settings.beginGroup( "ScriptConsole" );
    restoreGeometry( settings.value("geometry").toByteArray() );
    m_splitter->restoreState( settings.value("splitter").toByteArray() );
    m_savePath = settings.value("savepath").toString();
    settings.endGroup();

    if( m_savePath.isEmpty() )
    {
        m_savePath = KUrl( KStandardDirs::locate( "data", "amarok/scriptconsole/" ) ).path();
    }
}

void
ScriptConsoleDialog::slotExecuteNewScript()
{
    QString script = m_view->document()->text();
    if( script.isEmpty() )
        return;
    ScriptItem *item = addItem( script );
    if( item )
        item->start( false );
}

void
ScriptConsoleDialog::slotKillAllAndClear()
{
    if( KMessageBox::warningContinueCancel( 0, i18n("Are you absolutely certain?") ) == KMessageBox::Cancel )
        return;
    m_outputListWidget->clear();
}

void
ScriptConsoleDialog::keyPressEvent(QKeyEvent* event)
{
    if( event->key() == Qt::Key_Escape )
        return;
    KDialog::keyPressEvent( event );
}

void
ScriptConsoleDialog::closeEvent( QCloseEvent *event )
{
    QSettings settings( "Amarok", "Script Console" );
    settings.beginGroup( "ScriptConsole" );
    settings.setValue( "geometry", saveGeometry() );
    settings.setValue( "splitter", m_splitter->saveState() );
    settings.setValue( "savepath", m_savePath );
    settings.endGroup();
    KDialog::closeEvent( event );
    deleteLater();
}

void
ScriptConsoleDialog::slotKillAndClearScript( const QModelIndex &index )
{
    ScriptConsoleItem *scriptItem = qvariant_cast<ScriptConsoleItem*>( index.data( ScriptItemDelegate::Script ) );
    switch( KMessageBox::warningYesNoCancel( this, i18n( "Remove script file from disk?" ), i18n( "Remove Script" ) ) )
    {
        case KMessageBox::Cancel:
            return;
        case KMessageBox::Yes:
            scriptItem->setClearOnDeletion( true );
        default:
            break;
    }
    scriptItem->stop();
    scriptItem->deleteLater();
    delete m_outputListWidget->takeItem( index.row() );
}

void
ScriptConsoleDialog::slotToggleScript( const QModelIndex &index )
{
    ScriptConsoleItem *item = qvariant_cast<ScriptConsoleItem*>( index.data( ScriptItemDelegate::Script ) );
    if( !item )
        return;
    if( item->engine() && item->engine()->isEvaluating() )
        item->stop();
    else
        item->start();
    m_outputListWidget->repaint();
}

void
ScriptConsoleDialog::slotBackHistory()
{
    if( m_currentItemIndex == 1 || m_outputListWidget->count() < 2 )
        return;
    if( !m_view->document()->text().isEmpty() )
    {
        if( m_currentItemIndex == -1 || qvariant_cast<ScriptConsoleItem*>(
                m_outputListWidget->item( m_currentItemIndex )->data( ScriptItemDelegate::Script ) )->document()->text() != m_view->document()->text() )
            if( KMessageBox::questionYesNo( 0, i18n("Discard the current script?") ) == KMessageBox::No )
                return;
    }
    --m_currentItemIndex;
    if( m_currentItemIndex < 0 )
        m_currentItemIndex = m_outputListWidget->count()-1;
    ScriptConsoleItem* item = qvariant_cast<ScriptConsoleItem*>(
                m_outputListWidget->item( m_currentItemIndex )->data( ScriptItemDelegate::Script ) );
    m_view->document()->clear();
    m_view->document()->setText( item->document()->text() );
}

void
ScriptConsoleDialog::slotForwardHistory()
{
    if( m_currentItemIndex < 0 || m_currentItemIndex == m_outputListWidget->count() )
        return;
    if( !m_view->document()->text().isEmpty() )
    {
        if( qvariant_cast<ScriptConsoleItem*>( m_outputListWidget->item( m_currentItemIndex )->data( ScriptItemDelegate::Script ) )->document()->text() != m_view->document()->text() )
        {
            if( KMessageBox::questionYesNo( 0, i18n("Discard the current script?") ) == KMessageBox::No )
                return;
        }
    }
    if( ++m_currentItemIndex == m_outputListWidget->count() )
        m_view->document()->clear();
    else
    {
        ScriptConsoleItem *item = qvariant_cast<ScriptConsoleItem*>(
                                        m_outputListWidget->item( m_currentItemIndex )->data( ScriptItemDelegate::Script ) );
        m_view->document()->setText( item->document()->text() );
    }
}

void
ScriptConsoleDialog::dataChanged()
{
    m_currentItemIndex = -1;
}


ScriptConsoleItem*
ScriptConsoleDialog::addItem( const QString &script )
{
    if( ( m_savePath.isEmpty() || !QDir( m_savePath ).exists() )
        && ( m_savePath = QFileDialog::getExistingDirectory(this, i18n( "Choose where to save your scripts" ), "~",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks) ).isEmpty() )
        return 0;

    QString scriptPath;
    QString scriptName;
    do{
        scriptName = QString( "Script-%1" ).arg( qrand() );
        scriptPath =  QString( "%1/%2" ).arg( m_savePath ).arg( scriptName );
    }while ( QDir( scriptPath ).exists() );
    QDir().mkdir( scriptPath );

    QListWidgetItem *item = new QListWidgetItem( m_outputListWidget );
    ScriptEditorDocument *document = new ScriptEditorDocument( this, m_editor->createDocument( 0 ) );
    document->setText( script );
    ScriptConsoleItem *scriptItem = new ScriptConsoleItem( this, scriptName, "Generic"
    , scriptPath, document );
    item->setFlags( item->flags() | Qt::ItemIsEditable );
    item->setData( ScriptItemDelegate::Script, QVariant::fromValue<ScriptConsoleItem*>(scriptItem) );
    m_outputListWidget->scrollToItem( item );
    connect( scriptItem, SIGNAL(logChanged()), m_outputListWidget, SLOT(update()) );
    connect( scriptItem, SIGNAL(evaluated(QString)), m_outputListWidget, SLOT(update()) );
    m_view->document()->clear();
    return scriptItem;
}

ScriptConsoleDialog::~ScriptConsoleDialog()
{}
