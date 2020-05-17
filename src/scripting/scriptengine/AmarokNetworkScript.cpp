/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#define DEBUG_PREFIX "AmarokNetworkScript"

#include "AmarokNetworkScript.h"

#include "App.h"
#include "core/support/Debug.h"

#include <QJSEngine>
#include <QTextCodec>

#include <KLocalizedString>

using namespace AmarokScript;

AmarokDownloadHelper *AmarokDownloadHelper::s_instance = nullptr;

Downloader::Downloader( QJSEngine* engine )
    : QObject( engine )
    , m_scriptEngine( engine )
{
    QJSValue scriptObj = engine->newQObject( this );
    //engine->setDefaultPrototype( qMetaTypeId<Downloader*>(), QJSValue() );
    scriptObj.setPrototype( QJSValue() );
    const QJSValue stringCtor = scriptObj.property( "stringDownloader_prototype_ctor" );
    engine->globalObject().setProperty( QStringLiteral("Downloader"), stringCtor ); //kept for compat
    engine->globalObject().setProperty( QStringLiteral("StringDownloader"), stringCtor ); //added for clarity
    const QJSValue dataCtor = scriptObj.property( "dataDownloader_prototype_ctor" );
    engine->globalObject().setProperty( QStringLiteral("DataDownloader"), dataCtor );
}

QJSValue
Downloader::stringDownloader_prototype_ctor( QJSValueList arguments )
{
    return init( arguments, m_scriptEngine, true );
}

QJSValue
Downloader::dataDownloader_prototype_ctor( QJSValueList arguments )
{
    //if( engine->importedExtensions().contains( QStringLiteral("qt.core") ) )
        return init( arguments, m_scriptEngine, false );
    /*
    else
    {
        context->throwError( i18nc("do not translate 'DataDownloader' or 'qt.core'", "qt.core must be loaded to use DataDownloader" ) );
        return engine->toScriptValue( false );
    }
    */
}

QJSValue
Downloader::init( QJSValueList arguments, QJSEngine *engine, bool stringResult )
{
    // from QtScript API docs
    DEBUG_BLOCK
    QJSValue object;

    /* TODO - Check if its possible to reflect if Constructor or not
    if( context->isCalledAsConstructor() )
        object = context->thisObject();
    else
    {
        object = engine->newObject();
        object.setPrototype( context->callee().property(QStringLiteral("prototype")) );
    }
    */
    object = engine->newObject();

    if( arguments.size() < 2 )
    {
        debug() << "ERROR! Constructor not called with enough arguments:" << arguments.size();
        return object;
    }

    if( !arguments.at( 1 ).isCallable() ) //TODO: check QUrl
    {
        debug() << "ERROR! Constructor not called with a Url and function!";
        return object;
    }

    QUrl url = QUrl::fromEncoded( arguments.at( 0 ).toString().toLatin1(), QUrl::StrictMode );

    if( !url.isValid() )
    {
        debug() << "ERROR! Constructor not called with a valid Url!";
        return object;
    }

    // start download, and connect to it
    if( stringResult )
    {
        QString encoding = QStringLiteral("UTF-8");
        if( arguments.size() == 3 ) // encoding specified
            encoding = arguments.at( 2 ).toString();
        AmarokDownloadHelper::instance()->newStringDownload( url, engine, arguments.at( 1 ), encoding );
    }
    else
        AmarokDownloadHelper::instance()->newDataDownload( url, engine, arguments.at( 1 ) );
    return object;
}

///////
// Class AmarokDownloadHelper
//////

AmarokDownloadHelper::AmarokDownloadHelper()
{
    s_instance = this;
    connect( The::networkAccessManager(), &NetworkAccessManagerProxy::requestRedirectedUrl,
             this, &AmarokDownloadHelper::requestRedirected );
}

void
AmarokDownloadHelper::newStringDownload( const QUrl &url, QJSEngine* engine, const QJSValue &obj, const QString &encoding )
{
    m_encodings[ url ] = encoding;
    newDownload( url, engine, obj, &AmarokDownloadHelper::resultString );
}

void
AmarokDownloadHelper::newDataDownload( const QUrl &url, QJSEngine* engine, const QJSValue &obj )
{
    newDownload( url, engine, obj, &AmarokDownloadHelper::resultData );
}

void
AmarokDownloadHelper::requestRedirected( const QUrl &sourceUrl, const QUrl &targetUrl )
{
    DEBUG_BLOCK

    // Move all entries from "url" to "targetUrl".
    updateUrl< QJSEngine* >( m_engines, sourceUrl, targetUrl );
    updateUrl< QJSValue >( m_values, sourceUrl, targetUrl );
    updateUrl< QString >( m_encodings, sourceUrl, targetUrl );
}

void
AmarokDownloadHelper::resultData( const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e )
{
    if( !m_values.contains( url ) )
        return;

    if( e.code != QNetworkReply::NoError )
        warning() << "Error fetching data:" << e.description;

    QJSValue obj = m_values.value( url );
    QJSEngine* engine = m_engines.value( url );
    cleanUp( url );

    // now send the data to the associated script object
    if( !obj.isCallable() )
    {
        debug() << "script object is valid but not a function!!";
        return;
    }

    if( !engine )
    {
        debug() << "stored script engine is not valid!";
        return;
    }

    QJSValueList args;
    args << engine->toScriptValue( data );
    obj.call( args );
}


void
AmarokDownloadHelper::resultString( const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e )
{
    if( !m_values.contains( url ) )
        return;

    if( e.code != QNetworkReply::NoError )
        warning() << "Error fetching string:" << e.description;

    QJSValue obj = m_values.value( url );
    QJSEngine* engine = m_engines.value( url );
    QString encoding = m_encodings.value( url );
    cleanUp( url );

    QString str;
    if( encoding.isEmpty() )
    {
        str = QString( data );
    }
    else
    {
        QTextCodec* codec = QTextCodec::codecForName( encoding.toUtf8() );
        str = codec->toUnicode( data );
    }

    // now send the data to the associated script object
    if( !obj.isCallable() )
    {
        debug() << "script object is valid but not a function!!";
        return;
    }

    if( !engine )
    {
        debug() << "stored script engine is not valid!";
        return;
    }

    QJSValueList args;
    args << engine->toScriptValue( str );
    obj.call( args );
}

void
AmarokDownloadHelper::cleanUp( const QUrl &url )
{
    m_values.remove( url );
    m_engines.remove( url );
    m_encodings.remove( url );
}

AmarokDownloadHelper*
AmarokDownloadHelper::instance()
{
    if( !s_instance )
        s_instance = new AmarokDownloadHelper();
    return s_instance;
}
