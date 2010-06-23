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

#ifndef AMAROK_NETWORK_SCRIPT_H
#define AMAROK_NETWORK_SCRIPT_H

#include "NetworkAccessManagerProxy.h"

#include <QObject>
#include <QtScript>

class AmarokDownloadHelper;

class AmarokNetworkScript : public QObject
{
    Q_OBJECT

    public:
        AmarokNetworkScript( QScriptEngine* ScriptEngine );
        ~AmarokNetworkScript();
};

class Downloader : public QObject
{
    Q_OBJECT

    public:
        Downloader( QScriptEngine* scriptEngine );
        ~Downloader();

    private:
        static QScriptValue dataDownloader_prototype_ctor( QScriptContext* context, QScriptEngine* engine );
        static QScriptValue stringDownloader_prototype_ctor( QScriptContext* context, QScriptEngine* engine );
        static QScriptValue init( QScriptContext* context, QScriptEngine* engine, bool stringResult );


        QScriptEngine* m_scriptEngine;
};

Q_DECLARE_METATYPE( Downloader* )

// this internal class manages multiple downloads from a script.
// keeps track of each unique download
class AmarokDownloadHelper : public QObject
{
    Q_OBJECT

    static AmarokDownloadHelper *s_instance;
    
public:
    AmarokDownloadHelper();

    static AmarokDownloadHelper *instance();
    
    // called by the wrapper class to register a new download
    void newStringDownload( const KUrl &url, QScriptEngine* engine, QScriptValue obj, QString encoding = "UTF-8" );
    void newDataDownload( const KUrl &url, QScriptEngine* engine, QScriptValue obj );

private slots:
    void resultString( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
    void resultData( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
    
private:
    QHash< KUrl, QScriptEngine* > m_engines;
    QHash< KUrl, QScriptValue > m_values;
    QHash< KUrl, QString > m_encodings;

    void cleanUp( const KUrl &url );
};

#endif
