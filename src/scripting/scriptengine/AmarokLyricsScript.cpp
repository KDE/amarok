/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#define DEBUG_PREFIX "AmarokLyricsScript"

#include "AmarokLyricsScript.h"

#include "EngineController.h"
#include "scripting/scriptmanager/ScriptManager.h"
#include "lyrics/LyricsManager.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QApplication>
#include <QByteArray>
#include <QJSEngine>
#include <QTextCodec>

using namespace AmarokScript;


AmarokLyricsScript::AmarokLyricsScript( QJSEngine *engine )
    : QObject( engine )
{
    QJSValue scriptObject = engine->newQObject( this );
    engine->globalObject().property( "Amarok" ).setProperty( "Lyrics", scriptObject );
    connect( ScriptManager::instance(), &ScriptManager::fetchLyrics,
             this, &AmarokLyricsScript::fetchLyrics );
}

void
AmarokLyricsScript::showLyrics( const QString &lyrics ) const
{
    DEBUG_BLOCK
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
        return;
    LyricsManager::instance()->lyricsResult( lyrics.toUtf8(), track ) ;
}

void
AmarokLyricsScript::showLyricsHtml( const QString &lyrics ) const
{
    showLyrics( lyrics );
    /*
    DEBUG_BLOCK
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
        return;
    LyricsManager::instance()->lyricsResultHtml( lyrics, false );
    */
}

void
AmarokLyricsScript::showLyricsError( const QString &error ) const
{
    /*
    DEBUG_BLOCK
    LyricsManager::instance()->lyricsError( error );
    */
    Q_UNUSED( error );
}


void
AmarokLyricsScript::showLyricsNotFound( const QString &msg ) const
{
    /*
    DEBUG_BLOCK
    LyricsManager::instance()->lyricsNotFound( msg );
    */
    Q_UNUSED( msg );
}
 

QString
AmarokLyricsScript::escape( const QString &str )
{
    return str.toHtmlEscaped();
}

void
AmarokLyricsScript::setLyricsForTrack( const QString &trackUrl, const QString &lyrics ) const
{
    Q_UNUSED( trackUrl );
    Q_UNUSED( lyrics );
    /* TODO - convert method invocation below
    LyricsManager::instance()->setLyricsForTrack( trackUrl, lyrics );
    */
}

QString
AmarokLyricsScript::toUtf8( const QByteArray &lyrics, const QString &encoding )
{
    QTextCodec* codec = QTextCodec::codecForName( encoding.toUtf8() );
    if( !codec )
        return QString();
    return codec->toUnicode( lyrics );
}

QString
AmarokLyricsScript::QStringtoUtf8( const QString &lyrics, const QString &encoding )
{
    QTextCodec* codec = QTextCodec::codecForName( encoding.toUtf8() );
    if( !codec )
        return QString();
    return codec->toUnicode( lyrics.toLatin1() );
}

QByteArray
AmarokLyricsScript::fromUtf8( const QString &str, const QString &encoding )
{
    QTextCodec* codec = QTextCodec::codecForName( encoding.toUtf8() );
    if( !codec )
        return QByteArray();

    return codec->fromUnicode( str );
}
