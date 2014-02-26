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

#include <KMessageBox>
#include <KTextEditor/View>

#include <QFile>
#include <QFileInfo>
#include <QScriptEngine>
#include <QTextStream>
#include <QDir>
#include <QScriptEngineDebugger>
#include <QMainWindow>

using namespace ScriptConsoleNS;

ScriptConsoleItem::ScriptConsoleItem( QObject *parent, const QString &name, const QString &category
                                    , const QString &path, ScriptEditorDocument *document )
: ScriptItem( parent, name, QString("%1/main.js").arg(path), createSpecFile( name, category, path ) )
, m_clearOnDelete( false )
, m_viewFactory( document )
{
    document->setParent( this );
    document->save( url().path() );
    initializeScriptEngine();
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
        dir.remove( "main.js" );
        dir.remove( "script.spec" );
        if( !dir.rmdir( dir.absolutePath() ) )
            debug() << "Directory %1 not removed, contains other files";
    }
    if( m_view )
        m_view.data()->deleteLater();
}

KPluginInfo
ScriptConsoleItem::createSpecFile( const QString &name, const QString &category, const QString &path )
{
    QString specs = QString( "[Desktop Entry]"
                            "\nIcon=\"\""
                            "\nType=script"
                            "\nServiceTypes=KPluginInfo"
                            "\nName= %1"
                            "\nX-KDE-PluginInfo-Name=%1"
                            "\nX-KDE-PluginInfo-Version=1.0"
                            "\nX-KDE-PluginInfo-Category=%2"
                            "\nX-KDE-PluginInfo-Depends=Amarok2.0"
                            "\nX-KDE-PluginInfo-EnabledByDefault=false\n" ).arg( name ).arg( category );

    QString specPath = QString( "%1/script.spec" ).arg( path );
    QFile file( specPath );
    if( !file.open( QIODevice::WriteOnly ) )
    {
        debug() << "Couldn't write to " + path;
        return KPluginInfo();
    }
    QTextStream stream( &file );
    stream << specs;
    return KPluginInfo( specPath );
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
    engine()->pushContext();
    return ScriptItem::start( silent );
}

KTextEditor::View*
ScriptConsoleItem::createEditorView( QWidget *parent )
{
    if( !m_view )
        m_view = m_viewFactory->createView( parent );
    else
        m_view.data()->setParent( parent );
    return m_view.data();
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
ScriptConsoleItem::handleError( QScriptEngine *engine )
{
    QString errorString = QString( "Script Error: %1 (line: %2)" )
                        .arg( engine->uncaughtException().toString() )
                        .arg( engine->uncaughtExceptionLineNumber() );
    return errorString;
}

void
ScriptConsoleItem::pause()
{
    if( !running() )
        return;
    engine()->popContext();
    ScriptItem::pause();
}
