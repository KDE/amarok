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

#include <QFile>
#include <QFileInfo>
#include <QScriptEngine>
#include <QTextStream>
#include <QDir>

using namespace ScriptConsole;

ScriptConsoleItem::ScriptConsoleItem( QObject *parent, const QString &name, const QString &category
                                    , const QString &path, ScriptEditorDocument *document )
: ScriptItem( parent, name, QString("%1/main.js").arg(path), createSpecFile( name, category, path ) )
, m_viewFactory( document )
, m_clearOnDelete( false )
{
    connect( this, SIGNAL(signalHandlerException(QScriptValue)), SIGNAL(logChanged()) );
    document->setParent( this );
    document->save( url().path() );
    m_scriptMethods = new ScriptingMethods( this );
    connect( m_scriptMethods, SIGNAL(output(QString)), this, SLOT(slotOutput(QString)) );
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
    if( !info().isValid() )
    {
        debug() << "Invalid spec";
        return false;
    }
    m_viewFactory->save();
    return ScriptItem::start( silent );
}

KTextEditor::View*
ScriptConsoleItem::createEditorView( QWidget *parent )
{
    if( engine() && engine()->isEvaluating() && KMessageBox::warningContinueCancel(0, i18n("Stop the current evaluation?")) == KMessageBox::Cancel )
        return 0;
    stop();
    return m_viewFactory->createView( parent );
}

void
ScriptConsoleItem::initializeScriptEngine()
{
    ScriptItem::initializeScriptEngine();
    QScriptValue scriptConsoleMethods = engine()->newQObject( m_scriptMethods, QScriptEngine::QtOwnership
                                                            , QScriptEngine::ExcludeSuperClassContents );
    engine()->globalObject().setProperty( "UndocumentedScriptConsoleInternals", scriptConsoleMethods );
    engine()->evaluate("function print(str){ return UndocumentedScriptConsoleInternals.print( str ); };");
}

void
ScriptConsoleItem::setClearOnDeletion( bool clearOnDelete )
{
    m_clearOnDelete = clearOnDelete;
}

void ScriptConsoleItem::slotOutput( const QString &string )
{
    m_output << string;
}

ScriptingMethods::ScriptingMethods( QObject *parent )
: QObject( parent )
{}

void
ScriptingMethods::print( const QScriptValue &value )
{
    emit output( value.toString() );
}
