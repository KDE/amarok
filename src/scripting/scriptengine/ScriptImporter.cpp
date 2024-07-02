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
#include "qtbindings/Core.h"
#include "qtbindings/Sql.h"
#ifdef WITH_QT_UITOOLS
 #include "qtbindings/UiTools.h"
#endif
#include "qtbindings/Gui.h"
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
    m_engine->importModule( QStringLiteral("amarok/") + src );
}

bool
ScriptImporter::loadQtBinding( const QString& binding )
{
    if (m_qtScriptCompat) {
        QJSValue scriptObj;

        /* Export QT classes for script only if requested */
        if (binding == QStringLiteral("qt.core")) {
            debug() << __PRETTY_FUNCTION__ << "QT Bindings[qt.core] imported";
            QtBindings::Core::ByteArray::installJSType( m_engine );
            QtBindings::Core::CoreApplication::installJSType( m_engine );
            QtBindings::Core::Dir::installJSType( m_engine );
            QtBindings::Core::FileInfo::installJSType( m_engine );
            QtBindings::Core::File::installJSType( m_engine );
            QtBindings::Core::IODevice::installJSType( m_engine );
            QtBindings::Core::Locale::installJSType( m_engine );
            QtBindings::Core::Resource::installJSType( m_engine );
            QtBindings::Core::TextCodec::installJSType( m_engine );
            QtBindings::Core::TextStream::installJSType( m_engine );
            QtBindings::Core::Translator::installJSType( m_engine );
            QtBindings::Core::Url::installJSType( m_engine );
        } else if (binding == QStringLiteral("qt.network")) {
            QString message(binding +
                            QStringLiteral(" not available in Qt5 and no wrapper yet for this Amarok version"));
            warning() << __PRETTY_FUNCTION__ << message;
            m_engine->evaluate(QStringLiteral("console.warn(") + message + QStringLiteral(")"));
        } else if (binding == QStringLiteral("qt.xml")) {
            QString message(binding +
                            QStringLiteral(" not available in Qt5 and no wrapper yet for this Amarok version"));
            warning() << __PRETTY_FUNCTION__ << message;
            m_engine->evaluate(QStringLiteral("console.warn(") + message + QStringLiteral(")"));
        } else if (binding == QStringLiteral("qt.gui")) {
            debug() << __PRETTY_FUNCTION__ << "QT Bindings[qt.gui] imported";
            QtBindings::Gui::CheckBox::installJSType( m_engine );
            QtBindings::Gui::Label::installJSType( m_engine );
            QtBindings::Gui::DialogButtonBox::installJSType( m_engine );
        } else if (binding == QStringLiteral("qt.sql")) {
            debug() << __PRETTY_FUNCTION__ << "QT Bindings[qt.sql] imported";
            QtBindings::Sql::SqlQuery::installJSType( m_engine );
#ifdef WITH_QT_UITOOLS
        } else if (binding == QStringLiteral("qt.uitools")) {
            debug() << __PRETTY_FUNCTION__ << "QT Bindings[qt.uitools] imported";
            QtBindings::UiTools::UiLoader::installJSType( m_engine );
#endif
        } else {
            error() << __PRETTY_FUNCTION__ << "Requested QT binding not available: " << binding;
            return false;
        }
        return true;
    }
    error() << __PRETTY_FUNCTION__ << "Loading Qt bindings in scripts not enabled.!";
    return false;
}

bool
ScriptImporter::include( const QString& relativeFilename )
{
    QUrl includeUrl = KIO::upUrl(m_scriptUrl);
    includeUrl = includeUrl.adjusted(QUrl::StripTrailingSlash);
    includeUrl.setPath(includeUrl.path() + QLatin1Char('/') + (relativeFilename));
    QFile file(includeUrl.toLocalFile());
    warning() << "Include file: " << file.fileName();
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        warning() << "cannot open the include file: " << file.fileName();
        return false;
    }
    QString importScript(QString(file.readAll()));
    if (m_qtScriptCompat) {
        //QTBUG-69408 - const is not supported by ES5. Replace it with 'var'
        QRegularExpression removeConst(
                QStringLiteral("const ([_$a-zA-Z\xA0-\uFFFF][_$a-zA-Z0-9\xA0-\uFFFF]*) *="),
                QRegularExpression::DotMatchesEverythingOption );
        importScript.replace( removeConst, QStringLiteral("var \\1 ="));
    }
    QJSValue result = m_engine->evaluate(importScript, relativeFilename);
    if (result.isError()) {
        error() << "Uncaught exception at " << result.property(QStringLiteral("name")).toString() << ":";
        error() << result.property(QStringLiteral("filename")).toString() << ":" << result.property(QStringLiteral("lineNumber")).toInt();
        error() << result.property(QStringLiteral("message")).toString();
        error() << result.property(QStringLiteral("stack")).toString();
        return false;
    }
    return true;
}

QStringList
ScriptImporter::availableBindings() const
{
    return QStringList()
    << QStringLiteral("qt.core")
    << QStringLiteral("qt.network")
    << QStringLiteral("qt.xml")
    << QStringLiteral("qt.gui")
    << QStringLiteral("qt.sql")
#ifdef WITH_QT_UITOOLS
    << QStringLiteral("qt.uitools")
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

