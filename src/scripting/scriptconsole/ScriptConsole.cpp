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
#define DEBUG_PREFIX "ScriptConsole"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "MainWindow.h"
#include "ScriptEditorDocument.h"
#include "ScriptConsoleDebugger.h"
#include "ScriptConsoleItem.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QListWidget>
#include <QKeyEvent>
#include <QMenuBar>
#include <QJSEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QToolBar>

#include <KMessageBox>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KLocalizedString>


using namespace AmarokScript;
using namespace ScriptConsoleNS;

QPointer<ScriptConsole> ScriptConsole::s_instance;

ScriptConsole*
ScriptConsole::instance()
{
    if( !s_instance )
        s_instance = new ScriptConsole( The::mainWindow() );
    return s_instance.data();
}

//private

ScriptConsole::ScriptConsole( QWidget *parent )
    : QMainWindow( parent, Qt::Window )
{
    m_editor = KTextEditor::Editor::instance();
    if ( !m_editor )
    {
        KMessageBox::error( nullptr, i18n("A KDE text-editor component could not be found.\n"
                                   "Please check your KDE installation.  Exiting the console!") );
        deleteLater();
        return;
    }
    m_scriptListDock = new ScriptListDockWidget( this );
    m_debugger = new ScriptConsoleDebugger( this );

    setDockNestingEnabled( true );
    setWindowTitle( i18n( "Script Console" ) );
    setObjectName( QStringLiteral("scriptconsole") );

    m_codeWidget = getWidget( QStringLiteral("Code"), ScriptConsoleDebugger::CodeWidget );
    addDockWidget( Qt::BottomDockWidgetArea, m_codeWidget );
    QList<QDockWidget*> debugWidgets = QList<QDockWidget*>()
                    << getWidget( i18n( "Console" ), ScriptConsoleDebugger::ConsoleWidget )
                    << getWidget( i18n( "Error" ), ScriptConsoleDebugger::ErrorLogWidget )
                    << getWidget( i18n( "Debug Output" ), ScriptConsoleDebugger::DebugOutputWidget )
                    << getWidget( i18n( "Loaded Scripts" ), ScriptConsoleDebugger::ScriptsWidget )
                    << getWidget( i18n( "Breakpoints" ), ScriptConsoleDebugger::BreakpointsWidget )
                    << getWidget( i18n( "Stack" ), ScriptConsoleDebugger::StackWidget )
                    << getWidget( i18n( "Locals" ), ScriptConsoleDebugger::LocalsWidget );
    foreach( QDockWidget *widget, debugWidgets )
    {
      addDockWidget( Qt::BottomDockWidgetArea, widget );
    }
    addDockWidget( Qt::BottomDockWidgetArea, m_scriptListDock );
    tabifyDockWidget( debugWidgets[0], debugWidgets[1] );
    tabifyDockWidget( debugWidgets[1], debugWidgets[2] );
    tabifyDockWidget( debugWidgets[3], debugWidgets[4] );
    tabifyDockWidget( debugWidgets[5], debugWidgets[6] );

    QMenuBar *bar = new QMenuBar( this );
    setMenuBar( bar );
    bar->addMenu( m_debugger->createStandardMenu( this ) );
    QToolBar *toolBar = m_debugger->createStandardToolBar( this );
    QAction *action = new QAction( i18n( "Stop" ), this );
    action->setIcon( QApplication::style()->standardIcon( QStyle::SP_MediaStop ) );
    connect( action, &QAction::toggled, this, &ScriptConsole::slotAbortEvaluation );
    toolBar->addAction( action );
    action = new QAction( QIcon::fromTheme( QStringLiteral("media-playback-start") ), i18n("Execute Script"), this );
    action->setShortcut( Qt::CTRL + Qt::Key_Enter );
    connect( action, &QAction::triggered, this, &ScriptConsole::slotExecuteNewScript );
    toolBar->addAction( action );
    action = new QAction( QIcon::fromTheme( QStringLiteral("document-new") ), i18n( "&New Script" ), this );
    action->setShortcut( Qt::CTRL + Qt::Key_N );
    toolBar->addAction( action );
    connect( action, &QAction::triggered, this, &ScriptConsole::slotNewScript );
    action = new QAction( QIcon::fromTheme( QStringLiteral("edit-delete") ), i18n( "&Delete Script" ), this );
    toolBar->addAction( action );
    connect( action, &QAction::triggered, m_scriptListDock, &ScriptListDockWidget::removeCurrentScript );
    action = new QAction( i18n( "&Clear All Scripts" ), this );
    toolBar->addAction( action );
    connect( action, &QAction::triggered, m_scriptListDock, &ScriptListDockWidget::clear );
    action = new QAction( i18n("Previous Script"), this );
    action->setShortcut( QKeySequence::MoveToPreviousPage );
    connect( action, &QAction::triggered, m_scriptListDock, &ScriptListDockWidget::prev );
    toolBar->addAction( action );
    action = new QAction( i18n("Next Script"), this );
    action->setShortcut( QKeySequence::MoveToNextPage );
    connect( action, &QAction::triggered, m_scriptListDock, &ScriptListDockWidget::next );
    toolBar->addAction( action );

    addToolBar( toolBar );

    QMenu *viewMenu = new QMenu( this );
    viewMenu->setTitle( i18n( "&View" ) );
    foreach( QDockWidget *dockWidget, findChildren<QDockWidget*>() )
    {
        if( dockWidget->parentWidget() == this )
            viewMenu->addAction( dockWidget->toggleViewAction() );
    }
    menuBar()->addMenu( viewMenu );

    addDockWidget( Qt::BottomDockWidgetArea, m_scriptListDock );
    connect( m_scriptListDock, &ScriptListDockWidget::edit, this, &ScriptConsole::slotEditScript );
    connect( m_scriptListDock, &ScriptListDockWidget::currentItemChanged, this, &ScriptConsole::setCurrentScriptItem );

    QListWidgetItem *item = new QListWidgetItem( "The Amarok Script Console allows you to easily execute"
                                                "JavaScript with access to all functions\nand methods you would"
                                                "have in an Amarok script.\nInformation on scripting for Amarok is"
                                                "available at:\nhttp://community.kde.org/Amarok/Development#Scripting"
                                                "\nExecute code: CTRL-Enter\nBack in code history: Page Up"
                                                "\nForward in code history: Page Down"
                                                "See the debugger manual at: <link here>"
                                               , 0 );
    item->setFlags( Qt::NoItemFlags );
    m_scriptListDock->addItem( item );

    QSettings settings( QStringLiteral("KDE"), QStringLiteral("Amarok") );
    settings.beginGroup( QStringLiteral("ScriptConsole") );
    restoreGeometry( settings.value(QStringLiteral("geometry")).toByteArray() );
    m_savePath = settings.value(QStringLiteral("savepath")).toString();
    settings.endGroup();

    if( m_savePath.isEmpty() )
        m_savePath = Amarok::saveLocation(QStringLiteral("scriptconsole"));

    slotNewScript();
    connect( m_debugger, &ScriptConsoleDebugger::evaluationSuspended, this, &ScriptConsole::slotEvaluationSuspended );
    connect( m_debugger, &ScriptConsoleDebugger::evaluationResumed, this, &ScriptConsole::slotEvaluationResumed );
    show();
    raise();
}

void
ScriptConsole::slotExecuteNewScript()
{
    if( m_scriptItem->document()->text().isEmpty() )
        return;
    /* TODO - QJSEngine does not have syntax checker. Evaluate alternatives
    QScriptSyntaxCheckResult syntaxResult = m_scriptItem->engine()->checkSyntax( m_scriptItem->document()->text() );
    if( QScriptSyntaxCheckResult::Valid != syntaxResult.state() )
    {
        debug() << "Syntax error: " << syntaxResult.errorLineNumber() << syntaxResult.errorMessage();
        KTextEditor::View *view = dynamic_cast<KTextEditor::View*>( m_codeWidget->widget() );
        ScriptEditorDocument::highlight( view, syntaxResult.errorLineNumber(), QColor( 255, 0, 0 ) );
        int response = KMessageBox::warningContinueCancel( this, i18n( "Syntax error at line %1, continue anyway?\nError: %2",
                                                  syntaxResult.errorLineNumber(), syntaxResult.errorMessage() ),
                                            i18n( "Syntax Error" ) );
        if( response == KMessageBox::Cancel )
            return;
    }
    */

    m_scriptItem->document()->save();
    m_codeWidget->setWidget( m_debugger->widget( ScriptConsoleDebugger::CodeWidget ) );
    m_scriptItem->start( false );
}

void
ScriptConsole::closeEvent( QCloseEvent *event )
{
    QSettings settings( QStringLiteral("KDE"), QStringLiteral("Amarok") );
    settings.beginGroup( QStringLiteral("ScriptConsole") );
    settings.setValue( QStringLiteral("geometry"), saveGeometry() );
    settings.setValue( QStringLiteral("savepath"), m_savePath );
    settings.endGroup();
    QMainWindow::closeEvent( event );
    deleteLater();
}

void
ScriptConsole::slotEditScript( ScriptConsoleItem *item )
{
    if( m_scriptItem->running() && KMessageBox::warningContinueCancel( this, i18n( "This will stop this script! Continue?" ), QString(), KStandardGuiItem::cont()
                                        , KStandardGuiItem::cancel(), QStringLiteral("stopRunningScriptWarning") ) == KMessageBox::Cancel )
        return;

    item->pause();
    setCurrentScriptItem( item );
}

ScriptConsoleItem*
ScriptConsole::createScriptItem( const QString &script )
{
    if( ( m_savePath.isEmpty() || !QDir( m_savePath ).exists() )
        && ( m_savePath = QFileDialog::getExistingDirectory(this, i18n( "Choose where to save your scripts" ), QStringLiteral("~"),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks) ).isEmpty() )
        return 0;

    QString scriptPath;
    QString scriptName;
    do
    {
        scriptName = QStringLiteral( "Script-%1" ).arg( qrand() );
        scriptPath =  QStringLiteral( "%1/%2" ).arg( m_savePath, scriptName );
    } while ( QDir( scriptPath ).exists() );
    QDir().mkdir( scriptPath );

    ScriptEditorDocument *document = new ScriptEditorDocument( this, m_editor->createDocument( 0 ) );
    document->setText( script );
    ScriptConsoleItem *scriptItem = new ScriptConsoleItem( this, scriptName, QStringLiteral("Generic"), scriptPath, document );
    return scriptItem;
}

ScriptConsole::~ScriptConsole()
{
    m_debugger->detach();
}

void
ScriptConsole::slotEvaluationSuspended()
{
    if( !m_scriptItem )
    {
        slotNewScript();
        return;
    }
    debug() << "Is Running() " << m_scriptItem->running();
    debug() << "Engine isError()" << m_scriptItem->engineResult().isError();
    // TODO - Inspect if translations work with real debugger signals
    //if( m_scriptItem->engine() && m_scriptItem->engine()->uncaughtException().isValid() )
    if( m_scriptItem->engine() && m_scriptItem->engineResult().isError() )
        return;

    KTextEditor::View *view = m_scriptItem->createEditorView( m_codeWidget );
    view->installEventFilter( this );
    view->document()->installEventFilter( this );
    m_codeWidget->setWidget( view );
}

void
ScriptConsole::slotEvaluationResumed()
{
    debug() << "Is running() " << m_scriptItem->running();
    debug() << "Engine isError()" << m_scriptItem->engineResult().isError();
    if( !m_scriptItem->engine() || !m_scriptItem->running() )
        return;

    KTextEditor::View *view = m_scriptItem->createEditorView( m_codeWidget );
    view->installEventFilter( this );
    m_codeWidget->setWidget( view );
}

void
ScriptConsole::slotAbortEvaluation()
{
    m_scriptItem->pause();
}


QDockWidget*
ScriptConsole::getWidget( const QString &title, ScriptConsoleDebugger::DebuggerWidget widget )
{
    QDockWidget *debugWidget = new QDockWidget( title, this );
    debugWidget->setWidget( m_debugger->widget( widget ) );
    return debugWidget;
}

void
ScriptConsole::setCurrentScriptItem( ScriptConsoleItem *item )
{
    if( !item || m_scriptItem.data() == item )
        return;
    m_debugger->detach();
    m_debugger->attachTo( item->engine() );
    m_scriptItem = item;
    if( item->engine() && item->running() )
    {
        m_codeWidget->setWidget( m_debugger->widget( ScriptConsoleDebugger::CodeWidget ) );
    }
    else
    {
        KTextEditor::View *view = item->createEditorView( m_codeWidget );
        view->installEventFilter( this );
        m_codeWidget->setWidget( view );
    }
}

void
ScriptConsole::slotNewScript()
{
    ScriptConsoleItem *item = createScriptItem( QLatin1String("") );
    m_scriptListDock->addScript( item );
    setCurrentScriptItem( item );
}

bool
ScriptConsole::eventFilter( QObject *watched, QEvent *event )
{
    Q_UNUSED( watched )
    if( event->type() == QEvent::KeyPress )
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>( event );
        if( keyEvent == QKeySequence::MoveToNextPage )
        {
            m_scriptListDock->next();
            return true;
        }
        else if( keyEvent == QKeySequence::MoveToPreviousPage )
        {
            m_scriptListDock->prev();
            return true;
        }
    }
    return false;
}

ScriptListDockWidget::ScriptListDockWidget( QWidget *parent )
: QDockWidget( i18n( "Scripts" ), parent )
{
    QWidget *widget = new BoxWidget( true, this );
    setWidget( widget );
    m_scriptListWidget = new QListWidget( widget );
    m_scriptListWidget->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    connect( m_scriptListWidget, &QListWidget::doubleClicked,
             this, &ScriptListDockWidget::slotDoubleClicked );
    connect( m_scriptListWidget, &QListWidget::currentItemChanged,
             this, &ScriptListDockWidget::slotCurrentItemChanged );
}

void
ScriptListDockWidget::addScript( ScriptConsoleItem *script )
{
    if( !script )
        return;

    QListWidgetItem *item = new QListWidgetItem( script->name(), 0 );
    item->setData( ScriptRole, QVariant::fromValue<ScriptConsoleItem*>( script ) );
    m_scriptListWidget->addItem( item );
    m_scriptListWidget->setCurrentItem( item );
}

void
ScriptListDockWidget::removeCurrentScript()
{
    QListWidgetItem *item = m_scriptListWidget->takeItem( m_scriptListWidget->currentRow() );
    ScriptConsoleItem *scriptItem = qvariant_cast<ScriptConsoleItem*>( item->data( ScriptRole ) );
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
    delete item;
}

void
ScriptListDockWidget::slotCurrentItemChanged( QListWidgetItem *newItem, QListWidgetItem *oldItem )
{
    Q_UNUSED( oldItem )
    Q_EMIT currentItemChanged( newItem ? qvariant_cast<ScriptConsoleItem*>( newItem->data(ScriptRole) ) : nullptr );
}

void
ScriptListDockWidget::slotDoubleClicked( const QModelIndex &index )
{
    Q_EMIT edit( qvariant_cast<ScriptConsoleItem*>( index.data(ScriptRole) ) );
}

void
ScriptListDockWidget::clear()
{
    if( sender() && KMessageBox::warningContinueCancel( nullptr, i18n("Are you absolutely certain?") ) == KMessageBox::Cancel )
        return;
    for( int i = 0; i<m_scriptListWidget->count(); ++i )
        qvariant_cast<ScriptConsoleItem*>( m_scriptListWidget->item( i )->data( ScriptRole ) )->deleteLater();
    m_scriptListWidget->clear();

}

void
ScriptListDockWidget::addItem( QListWidgetItem *item )
{
    m_scriptListWidget->addItem( item );
}

ScriptListDockWidget::~ScriptListDockWidget()
{
    clear();
}

void
ScriptListDockWidget::next()
{
    int currentRow = m_scriptListWidget->currentRow();
    m_scriptListWidget->setCurrentRow( currentRow > 1 ? currentRow - 1 : currentRow );
}

void
ScriptListDockWidget::prev()
{
    int currentRow = m_scriptListWidget->currentRow();
    m_scriptListWidget->setCurrentRow( currentRow + 1 < m_scriptListWidget->count() ? currentRow + 1 : currentRow );
}
