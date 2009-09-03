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

#include "AmarokNetworkScript.h"

#include "App.h"
#include "Debug.h"

#include <QtScript>

AmarokDownloadHelper *AmarokDownloadHelper::s_instance = 0;

AmarokNetworkScript::AmarokNetworkScript( QScriptEngine* ScriptEngine )
: QObject( kapp )
{
    Q_UNUSED( ScriptEngine );
}

AmarokNetworkScript::~AmarokNetworkScript()
{
}

// Class Downloader

Downloader::Downloader( QScriptEngine* engine )
    : QObject( kapp ),
    m_scriptEngine( engine )
{
    DEBUG_BLOCK
    engine->setDefaultPrototype( qMetaTypeId<Downloader*>(), QScriptValue() );
    const QScriptValue stringCtor = engine->newFunction( stringDownloader_prototype_ctor );
    engine->globalObject().setProperty( "Downloader", stringCtor ); //kept for compat
    engine->globalObject().setProperty( "StringDownloader", stringCtor ); //added for clarity
    const QScriptValue dataCtor = engine->newFunction( dataDownloader_prototype_ctor );
    engine->globalObject().setProperty( "DataDownloader", dataCtor );

    
}

Downloader::~Downloader()
{
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
        return QScriptValue( engine, false );
    }
}

QScriptValue
Downloader::init( QScriptContext* context, QScriptEngine* engine, bool stringResult )
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
    
    KUrl tmpUrl;
    QUrl url = qscriptvalue_cast<QUrl>( context->argument( 0 ) );
    tmpUrl.setEncodedUrl( url.toEncoded() );
    

    // start download, and connect to it
    //FIXME: url is not working directly.
    KIO::Job* job = KIO::storedGet( tmpUrl, KIO::NoReload, KIO::HideProgressInfo );
    if( stringResult )
    {
        QString encoding = "UTF-8";
        if(context->argumentCount() == 3 ) // encoding specified
            encoding = context->argument( 2 ).toString();
        AmarokDownloadHelper::instance()->newStringDownload( job, engine, context->argument( 1 ), encoding );
    }
    else
        AmarokDownloadHelper::instance()->newDataDownload( job, engine, context->argument( 1 ) );
    // connect to a local slot to extract the qstring
    //qScriptConnect( job, SIGNAL( result( KJob* ) ), object, fetchResult( job ) );
    return object;
}

///////
// Class AmarokDownloadHelper
//////

AmarokDownloadHelper::AmarokDownloadHelper()
{
    s_instance = this;
}

void
AmarokDownloadHelper::newStringDownload( KJob* download, QScriptEngine* engine, QScriptValue obj, QString encoding )
{
    m_jobs[ download ] = obj ;
    m_engines[ download ] = engine;
    m_encodings[ download ] = encoding;
    connect( download, SIGNAL( result( KJob* ) ), this, SLOT( resultString( KJob* ) ) );
}

void
AmarokDownloadHelper::newDataDownload( KJob* download, QScriptEngine* engine, QScriptValue obj )
{
    m_jobs[ download ] = obj ;
    m_engines[ download ] = engine;
    connect( download, SIGNAL( result( KJob* ) ), this, SLOT( resultData( KJob* ) ) );
}

void
AmarokDownloadHelper::resultData( KJob* job )
{
    DEBUG_BLOCK

    QScriptValue obj = m_jobs[ job ];
    QScriptEngine* engine = m_engines[ job ];
    
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
    args <<  engine->toScriptValue( static_cast< KIO::StoredTransferJob* >( job )->data() );
    obj.call( obj, args );

    m_jobs.remove( job );
    m_engines.remove( job );
}


void
AmarokDownloadHelper::resultString( KJob* job )
{
    DEBUG_BLOCK

    KIO::StoredTransferJob* const storedJob = static_cast< KIO::StoredTransferJob* >( job );
    QScriptValue obj = m_jobs[ job ];
    QScriptEngine* engine = m_engines[ job ];
    QString encoding = m_encodings[ job ];

    QString data;
    
    if( encoding.isEmpty() )
        data = QString( storedJob->data() );
    else
    {
        QTextCodec* codec = QTextCodec::codecForName( encoding.toUtf8() );
        QTextCodec* utf8codec = QTextCodec::codecForName( "UTF-8" );
        QTextCodec::setCodecForCStrings( utf8codec );
        data = codec->toUnicode( storedJob->data() );
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
    args <<  QScriptValue( engine, data );
    obj.call( obj, args );

    m_jobs.remove( job );
    m_engines.remove( job );
    m_encodings.remove( job );
}

AmarokDownloadHelper*
AmarokDownloadHelper::instance()
{
    if( !s_instance )
        s_instance = new AmarokDownloadHelper();
    return s_instance;
}


#include "AmarokNetworkScript.moc"
