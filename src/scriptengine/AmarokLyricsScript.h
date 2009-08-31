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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_LYRICS_SCRIPT_H
#define AMAROK_LYRICS_SCRIPT_H

#include <QObject>

class QScriptEngine;
class QByteArray;

namespace AmarokScript
{

class AmarokLyricsScript : public QObject 
{
    Q_OBJECT

public:
    AmarokLyricsScript( QScriptEngine* scriptEngine );
    ~AmarokLyricsScript();

public slots:
    void showLyrics( const QString& lyrics ) const;
    
    void showLyricsHtml( const QString& lyrics ) const;
    void showLyricsError( const QString& error ) const;

    QString escape( const QString& str );

    void setLyricsForTrack( const QString& trackUrl , const QString& lyrics ) const;
    QString toUtf8( const QByteArray& lyrics, const QString& encoding = "UTF-8" );
    QString QStringtoUtf8( const QString& lyrics, const QString& encoding = "UTF-8" );
    QByteArray fromUtf8( const QString& str, const QString& encoding );
    
signals:
    void fetchLyrics( const QString& artist, const QString& title, const QString& );
};

}

#endif
