/******************************************************************************
 * Copyright (C) 2008 Leo Franchi <lfranchi@kde.org>                          *
 *               2008 Peter ZHOU  <peterzhoulei@gmail.com>                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "AmarokLyricsScript.h"

#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"
#include "LyricsManager.h"
#include "Meta.h"
#include "ScriptManager.h"

#include <KApplication>

namespace AmarokScript
{

AmarokLyricsScript::AmarokLyricsScript( QScriptEngine* scriptEngine )
    : QObject( kapp )
{
    Q_UNUSED( scriptEngine )
    connect( ScriptManager::instance(), SIGNAL( fetchLyrics( const QString&, const QString&, const QString& ) ), this, SIGNAL( fetchLyrics( const QString&, const QString&, const QString& ) ) );
}

AmarokLyricsScript::~AmarokLyricsScript()
{
}

void
AmarokLyricsScript::showLyrics( const QString& lyrics ) const
{
    DEBUG_BLOCK
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
        return;
    //debug() << "got lyrics: " << lyrics << " and track: " << track;
//    debug() << lyrics;
    LyricsManager::self()->lyricsResult( lyrics, false );
}

void
AmarokLyricsScript::showLyricsHtml( const QString& lyrics ) const
{
    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( !track )
        return;
    //debug() << "got lyrics: " << lyrics << " and track: " << track;
    LyricsManager::self()->lyricsResultHtml( lyrics, false );
}

void
AmarokLyricsScript::setLyricsForTrack( const QString& trackUrl, const QString& lyrics ) const
{
    Q_UNUSED( trackUrl );
    Q_UNUSED( lyrics );
}

QString
AmarokLyricsScript::toUtf8( const QByteArray& lyrics, const QString& encoding )
{
    QTextCodec* codec = QTextCodec::codecForName( encoding.toUtf8() );
    QTextCodec* utf8codec = QTextCodec::codecForName( "UTF-8" );
    QTextCodec::setCodecForCStrings( utf8codec );
    return codec->toUnicode( lyrics );
}

QString
AmarokLyricsScript::QStringtoUtf8( const QString& lyrics, const QString& encoding )
{
    QTextCodec* codec = QTextCodec::codecForName( encoding.toUtf8() );
    QTextCodec* utf8codec = QTextCodec::codecForName( "UTF-8" );
    QTextCodec::setCodecForCStrings( utf8codec );
    return codec->toUnicode( lyrics.toLatin1() );
}

QByteArray
AmarokLyricsScript::fromUtf8( const QString& str, const QString& encoding )
{
    QTextCodec* codec = QTextCodec::codecForName( encoding.toUtf8() );
    return codec->fromUnicode( str );
}

}

#include "AmarokLyricsScript.moc"
