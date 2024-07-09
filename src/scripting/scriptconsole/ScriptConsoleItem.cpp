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

#define DEBUG_PREFIX "ScriptConsoleItem"

#include "ScriptConsoleItem.h"

#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "ScriptEditorDocument.h"

#include <KTextEditor/View>

#include <QFile>
#include <QFileInfo>
#include <QJSValue>
#include <QTextStream>
#include <QDir>
#include <QPlainTextEdit>


using namespace ScriptConsoleNS;

ScriptConsoleItem::ScriptConsoleItem( QObject *parent, const QString &name, const QString &category
                                    , const QString &path, ScriptEditorDocument *document )
: ScriptItem( parent, name, QStringLiteral("%1/main.js").arg(path), createSpecFile( name, category, path ) )
, m_clearOnDelete( false )
, m_viewFactory( document )
{
    document->setParent( this );
    document->save( url() );
    initializeScriptEngine();

    m_view = m_viewFactory->createView( nullptr );
    m_console= new QPlainTextEdit( nullptr );
    m_console->setReadOnly(true);
    m_output = new QPlainTextEdit( nullptr );
    m_output->setReadOnly(true);
    m_error = new QPlainTextEdit( nullptr );
    m_error->setReadOnly(true);

    connect( this, &ScriptConsoleItem::evaluated, this, &ScriptConsoleItem::updateOutputWidget);
    connect( this, &ScriptConsoleItem::signalHandlerException, this, &ScriptConsoleItem::updateErrorWidget);
}

ScriptConsoleItem::~ScriptConsoleItem()
{
    if( running() )
        stop();
    if( m_clearOnDelete || !AmarokConfig::saveSession() ) // TODO: implement session restore + user config option, export button
    {
        QFileInfo info( url().path() );
        QDir dir( info.path() );
        if( !dir.exists() )
            return;
        dir.remove( QStringLiteral("main.js") );
        dir.remove( QStringLiteral("script.spec") );
        if( !dir.rmdir( dir.absolutePath() ) )
            debug() << "Directory %1 not removed, contains other files";
    }
    if( m_view )
        m_view->deleteLater();
    if (m_console)
        m_console->deleteLater();
    if (m_output)
        m_output->deleteLater();
    if (m_error)
        m_error->deleteLater();
}

KPluginMetaData
ScriptConsoleItem::createSpecFile( const QString &name, const QString &category, const QString &path )
{
    QString specs = QStringLiteral( "[Desktop Entry]"
                            "\nIcon=\"\""
                            "\nType=script"
                            "\nServiceTypes=KPluginMetaData"
                            "\nName=%1"
                            "\nComment=Amarok console script"
                            "\nX-KDE-PluginInfo-Name=%1"
                            "\nX-KDE-PluginInfo-Version=1.0"
                            "\nX-KDE-PluginInfo-Category=%2"
                            "\nX-KDE-PluginInfo-Depends=Amarok2.0"
                            "\nX-KDE-PluginInfo-EnabledByDefault=false\n" ).arg( name, category );
    // TODO - Replace .desktop with new json format. Important: file extension matters
    QString specPath = QStringLiteral( "%1/script.desktop" ).arg( path );
    QFile file( specPath );
    if( !file.open( QIODevice::WriteOnly ) )
    {
        debug() << "Couldn't write to " << path;
        return KPluginMetaData();
    }
    QTextStream stream( &file );
    stream << specs;
    file.close();
    return KPluginMetaData( specPath );
}

bool
ScriptConsoleItem::start( bool silent )
{
    if( running() )
        return false;
    if( !info().isValid() )
    {
        debug() << "Invalid spec";
        return false;
    }
    m_viewFactory->save();
    Q_ASSERT( engine() );
    //engine()->pushContext();
    m_viewFactory->setReadWrite( false );
    bool result = ScriptItem::start( silent );
    m_viewFactory->setReadWrite( true );
    return result;
}

KTextEditor::View*
ScriptConsoleItem::getEditorView( QWidget *parent )
{
    m_view->setParent( parent );
    return m_view.data();
}

QWidget *
ScriptConsoleItem::getConsoleWidget( QWidget *parent )
{
    m_console->setParent( parent );
    return m_console.data();
}

void
ScriptConsoleItem::appendToConsoleWidget( const QString &msg )
{
    m_console->appendPlainText( msg );
}

QWidget *
ScriptConsoleItem::getOutputWdiget( QWidget *parent )
{
    m_output->setParent( parent );
    return m_output.data();
}

QWidget *
ScriptConsoleItem::getErrorWidget( QWidget *parent )
{
    m_error->setParent( parent );
    return m_error.data();
}

void
ScriptConsoleItem::initializeScriptEngine()
{
    ScriptItem::initializeScriptEngine();
}

void
ScriptConsoleItem::timerEvent( QTimerEvent *event )
{
    Q_UNUSED( event )
}


void
ScriptConsoleItem::setClearOnDeletion( bool clearOnDelete )
{
    m_clearOnDelete = clearOnDelete;
}

QString
ScriptConsoleItem::handleError( QJSValue *result )
{
    QString errorString = QStringLiteral( "Script Error: %1 (line: %2)" )
                        .arg( result->toString() )
                        .arg( result->property(QStringLiteral("lineNumber")).toInt() );

    return errorString;
}

void
ScriptConsoleItem::pause()
{
    if( !running() )
        return;
    ///engine()->popContext();
    ScriptItem::pause();
}

void
ScriptConsoleItem::updateOutputWidget( QString output )
{
    m_output->appendPlainText( output );
}

void
ScriptConsoleItem::updateErrorWidget( QJSValue error )
{
    m_error->appendPlainText( handleError( &error ) );
}
