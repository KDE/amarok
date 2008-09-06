/******************************************************************************
* Copyright (C) 2008 Leo Franchi <lfranchi@kde.org>                          *
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

#ifndef AMAROK_LYRICS_SCRIPT_H
#define AMAROK_LYRICS_SCRIPT_H

#include <QObject>

class QScriptEngine;

namespace AmarokScript
{

class AmarokLyricsScript : public QObject 
{
    Q_OBJECT

public:
    AmarokLyricsScript( QScriptEngine* scriptEngine );
    ~AmarokLyricsScript();

public slots:
    void showLyrics( QString lyrics, const QString& encoding = "UTF-8") const;
    void showLyricsHtml( QString lyrics, const QString& encoding = "UTF-8" ) const;
    void setLyricsForTrack( const QString& trackUrl , const QString& lyrics ) const;

    
signals:
    void fetchLyrics( const QString& artist, const QString& title, const QString& );
};

}

#endif
