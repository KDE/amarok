/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#include "ScriptImporter.h"

#include "App.h"
#include "AmarokBookmarkScript.h"
#include "qtbindings/CoreByteArray.h"
#include "qtbindings/CoreCoreApplication.h"
#include "qtbindings/CoreDir.h"
#include "qtbindings/CoreFileInfo.h"
#include "qtbindings/CoreLocale.h"
#include "qtbindings/CoreResource.h"
#include "qtbindings/CoreTextCodec.h"
#include "qtbindings/CoreTextStream.h"
#include "qtbindings/CoreTranslator.h"
#include "qtbindings/CoreUrl.h"
#include "qtbindings/SqlSqlQuery.h"
#include "AmarokCollectionViewScript.h"
#include "config.h"
#include "core/support/Debug.h"
#include "ScriptingDefines.h"
#include "AmarokPlaylistManagerScript.h"
#include "scripting/scriptmanager/ScriptManager.h"

#include <KIO/Global>

#include <QFile>
#include <QSet>
#include <QUrl>
#include <QTextStream>
#include <QCheckBox>
#include <QLabel>
#include <QRegularExpression>
#include <QTranslator>
#ifdef WITH_QT_UITOOLS
#include <QUiLoader>
#endif

using namespace AmarokScript;

ScriptImporter::ScriptImporter( AmarokScriptEngine *scriptEngine, const QUrl &url )
    : QObject( scriptEngine )
    , m_scriptUrl( url )
    , m_engine(scriptEngine )
{
    QJSValue scriptObject = scriptEngine->newQObject( this );
    scriptEngine->globalObject().setProperty( QStringLiteral("Importer"), scriptObject );
}

void
ScriptImporter::loadExtension( const QString& src )
{
    DEBUG_BLOCK
    /* TODO - Use commented code once QT versions >= 5.12
    m_engine->importModule( "amarok/" + src );
    */
    QFile extensionFile( "amarok/" + src );
    m_engine->evaluate( QTextStream(&extensionFile ).readAll() );
}

bool
ScriptImporter::loadQtBinding( const QString& binding )
{
    QJSValue scriptObj;

    /* Export QT classes for script only if requested */
    if (binding == "qt.core" ) {
        debug() << __PRETTY_FUNCTION__ <<  "QT Bindings[qt.core] imported";
        // Wrapped QObjects
        QtBindings::Core::ByteArray::installJSType( m_engine );

        QtBindings::Core::Dir::installJSType( m_engine );
        QtBindings::Core::Locale::installJSType( m_engine );
        QtBindings::Core::TextCodec::installJSType( m_engine );
        QtBindings::Core::FileInfo::installJSType( m_engine );
        QtBindings::Core::Resource::installJSType( m_engine );
        QtBindings::Core::TextStream::installJSType( m_engine );
        QtBindings::Core::Url::installJSType( m_engine );

        /*
        qRegisterMetaType<QtBindings::Core::ByteArray*>("ByteArray*");
        qRegisterMetaType<QtBindings::Core::ByteArray*>("const ByteArray*");
        qRegisterMetaType<QtBindings::Core::ByteArray>("ByteArray");
        qRegisterMetaType<QByteArray>("QByteArray");
        */

        // Native QObjects
        QtBindings::Core::CoreApplication::installJSType( m_engine );

        m_engine->globalObject().setProperty("QFile", m_engine->newQMetaObject<QFile>());
        m_engine->globalObject().setProperty("QIODevice", m_engine->newQMetaObject<QIODevice>());

        QtBindings::Core::Translator::installJSType( m_engine );
    } else if (binding == "qt.network" ) {
        QString message( binding + " not available in Qt5 and no wrapper yet for this Amarok version" );
        warning() << __PRETTY_FUNCTION__ <<  message;
        m_engine->evaluate("console.warn(" + message + ")");
    } else if (binding == "qt.xml" ) {
        QString message( binding + " not available in Qt5 and no wrapper yet for this Amarok version" );
        warning() << __PRETTY_FUNCTION__ <<  message;
        m_engine->evaluate("console.warn(" + message + ")");
    } else if (binding == "qt.gui" ) {
        debug() << __PRETTY_FUNCTION__ <<  "QT Bindings[qt.gui] imported";
        m_engine->globalObject().setProperty("QCheckBox", m_engine->newQMetaObject<QCheckBox>());
        m_engine->globalObject().setProperty("QLabel", m_engine->newQMetaObject<QLabel>());
    } else if (binding == "qt.sql" ) {
        debug() << __PRETTY_FUNCTION__ <<  "QT Bindings[qt.sql] imported";
        m_engine->globalObject().setProperty("QSqlQuery", m_engine->newQMetaObject<QtBindings::Sql::SqlQuery>());
#ifdef WITH_QT_UITOOLS
    } else if (binding == "qt.uitools" ) {
        debug() << __PRETTY_FUNCTION__ <<  "QT Bindings[qt.uitools] imported";
        m_engine->globalObject().setProperty("QUiLoader", m_engine->newQMetaObject<QUiLoader>());
#endif
    } else {
        error() << __PRETTY_FUNCTION__ << "Loading Qt bindings in scripts not available in Qt5!";
        return false;
    }

    return false;
}

bool
ScriptImporter::include( const QString& relativeFilename )
{
    QUrl includeUrl = KIO::upUrl(m_scriptUrl);
    includeUrl = includeUrl.adjusted(QUrl::StripTrailingSlash);
    includeUrl.setPath(includeUrl.path() + QLatin1Char('/') + ( relativeFilename ));
    QFile file( includeUrl.toLocalFile() );
    warning() << "Include file: " << file.fileName();
    if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        warning() << "cannot open the include file: " << file.fileName();
        return false;
    }
    //QTBUG-69408 - const from imported is undefined. So remove qualifier
    QString importScript(QString( file.readAll() ).remove( QRegularExpression("^const ", QRegularExpression::MultilineOption)) );
    QJSValue result = m_engine->evaluate(importScript, relativeFilename);
    /*
    QJSValue result = m_engine->evaluate( QString( file.readAll() )
            .remove( QRegularExpression("^const ", QRegularExpression::MultilineOption) )
            , relativeFilename );
    */
    if (result.isError()) {
        error() << "Uncaught exception at " << result.property("name").toString() << ":";
        error() << result.property("filename").toString() << ":" << result.property("lineNumber").toInt();
        error() << result.property("message").toString();
        error() << result.property("stack").toString();
        return false;
    }
    return true;
}

QStringList
ScriptImporter::availableBindings() const
{
    return QStringList()
    << "qt.core"
    << "qt.network"
    << "qt.xml"
    << "qt.gui"
    << "qt.sql"
#ifdef WITH_QT_UITOOLS
    << "qt.uitools"
#endif
    ;
}

bool
ScriptImporter::loadAmarokBinding( const QString &name )
{
    if( name == QLatin1String("bookmarks") )
        new AmarokBookmarkScript(m_engine );
    else if( name == QLatin1String("collectionview") )
        new AmarokCollectionViewScript(m_engine, ScriptManager::instance()->scriptNameForEngine(m_engine ) );
    else if( name == QLatin1String("playlistmanager") )
        new AmarokPlaylistManagerScript(m_engine );
    else
    {
        warning() << "\"" << name << "\" doesn't exist!";
        return false;
    }
    return true;
}

