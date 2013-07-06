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

#include <QScriptEngine>
#include <QTextCodec>

AmarokDownloadHelper *AmarokDownloadHelper::s_instance = 0;

AmarokNetworkScript::AmarokNetworkScript( QScriptEngine *engine )
    : QObject( engine )
{
    QScriptValue scriptObject = engine->newQObject( this, QScriptEngine::AutoOwnership,
                                                    QScriptEngine::ExcludeSuperClassContents );
    engine->globalObject().property( "Amarok" ).setProperty( "Network", scriptObject );
}

// Class Downloader

Downloader::Downloader( QScriptEngine* engine )
    : QObject( engine )
    , m_scriptEngine( engine )
{
    engine->setDefaultPrototype( qMetaTypeId<Downloader*>(), QScriptValue() );
    const QScriptValue stringCtor = engine->newFunction( stringDownloader_prototype_ctor );
    engine->globalObject().setProperty( "Downloader", stringCtor ); //kept for compat
    engine->globalObject().setProperty( "StringDownloader", stringCtor ); //added for clarity
    const QScriptValue dataCtor = engine->newFunction( dataDownloader_prototype_ctor );
    engine->globalObject().setProperty( "DataDownloader", dataCtor );
}

QScriptValue
Downloader::stringDownloader_prototype_ctor( QScriptContext* context, QScriptEngine* engine )
{
    return init( context, engine, true );
}

QScriptValue
Downloader::dataDownloader_prototype_ctor( QScriptContext* context, QScriptEngine* engine )
{
    if( engine->importedExtensions().contains( "qt.core" ) )
        return init( context, engine, false );
    else
    {
        context->throwError( i18nc("do not translate 'DataDownloader' or 'qt.core'", "qt.core must be loaded to use DataDownloader" ) );
        return engine->toScriptValue( false );
    }
}

QScriptValue
Downloader::init( QScriptContext* context, QScriptEngine *engine, bool stringResult )
{
    // from QtScript API docs
    DEBUG_BLOCK
    QScriptValue object;

    if( context->isCalledAsConstructor() )
        object = context->thisObject();
    else
    {
        object = engine->newObject();
        object.setPrototype( context->callee().property("prototype") );
    }

    if( context->argumentCount() < 2 )
    {
        debug() << "ERROR! Constructor not called with enough arguments:" << context->argumentCount();
        return object;
    }

    if( !context->argument( 1 ).isFunction() ) //TODO: check QUrl
    {
        debug() << "ERROR! Constructor not called with a QUrl and function!";
        return object;
    }

    KUrl url( qscriptvalue_cast<QUrl>( context->argument( 0 ) ) );

    // start download, and connect to it
    //FIXME: url is not working directly.
    if( stringResult )
    {
        QString encoding = "UTF-8";
        if( context->argumentCount() == 3 ) // encoding specified
            encoding = context->argument( 2 ).toString();
        AmarokDownloadHelper::instance()->newStringDownload( url, engine, context->argument( 1 ), encoding );
    }
    else
        AmarokDownloadHelper::instance()->newDataDownload( url, engine, context->argument( 1 ) );
    // connect to a local slot to extract the qstring
    //qScriptConnect( job, SIGNAL(result(KJob*)), object, fetchResult( job ) );
    return object;
}

///////
// Class AmarokDownloadHelper
//////

AmarokDownloadHelper::AmarokDownloadHelper()
{
    s_instance = this;
    connect( The::networkAccessManager(), SIGNAL(requestRedirected(KUrl,KUrl)),
             this, SLOT(requestRedirected(KUrl,KUrl)) );
}

void
AmarokDownloadHelper::newStringDownload( const KUrl &url, QScriptEngine* engine, QScriptValue obj, QString encoding )
{
    m_encodings[ url ] = encoding;
    newDownload( url, engine, obj, SLOT(resultString(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
AmarokDownloadHelper::newDataDownload( const KUrl &url, QScriptEngine* engine, QScriptValue obj )
{
    newDownload( url, engine, obj, SLOT(resultData(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
AmarokDownloadHelper::newDownload( const KUrl &url, QScriptEngine* engine, QScriptValue obj, const char *slot )
{
    m_values[ url ] = obj;
    m_engines[ url ] = engine;

    The::networkAccessManager()->getData( url, this, slot );
}

void
AmarokDownloadHelper::requestRedirected( const KUrl &sourceUrl, const KUrl &targetUrl )
{
    DEBUG_BLOCK

    // Move all entries from "url" to "targetUrl".
    updateUrl< QScriptEngine* >( m_engines, sourceUrl, targetUrl );
    updateUrl< QScriptValue >( m_values, sourceUrl, targetUrl );
    updateUrl< QString >( m_encodings, sourceUrl, targetUrl );
}

void
AmarokDownloadHelper::resultData( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( !m_values.contains( url ) )
        return;

    if( e.code != QNetworkReply::NoError )
        warning() << "Error fetching data:" << e.description;

    QScriptValue obj = m_values.value( url );
    QScriptEngine* engine = m_engines.value( url );
    cleanUp( url );

    // now send the data to the associated script object
    if( !obj.isFunction() )
    {
        debug() << "script object is valid but not a function!!";
        return;
    }

    if( !engine )
    {
        debug() << "stored script engine is not valid!";
        return;
    }

    QScriptValueList args;
    args << engine->toScriptValue( data );
    obj.call( obj, args );
}


void
AmarokDownloadHelper::resultString( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( !m_values.contains( url ) )
        return;

    if( e.code != QNetworkReply::NoError )
        warning() << "Error fetching string:" << e.description;

    QScriptValue obj = m_values.value( url );
    QScriptEngine* engine = m_engines.value( url );
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
        QTextCodec* utf8codec = QTextCodec::codecForName( "UTF-8" );
        QTextCodec::setCodecForCStrings( utf8codec );
        str = codec->toUnicode( data );
    }

    // now send the data to the associated script object
    if( !obj.isFunction() )
    {
        debug() << "script object is valid but not a function!!";
        return;
    }

    if( !engine )
    {
        debug() << "stored script engine is not valid!";
        return;
    }

    QScriptValueList args;
    args << engine->toScriptValue( str );
    obj.call( obj, args );
}

void
AmarokDownloadHelper::cleanUp( const KUrl &url )
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
