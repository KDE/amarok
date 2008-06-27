/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
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

#ifndef AMAROK_OSD_SCRIPT_H
#define AMAROK_OSD_SCRIPT_H

#include <QImage>
#include <QObject>
#include <QtScript>

namespace Amarok
{

    class amarokOSDScript : public QObject
    {
        Q_OBJECT

        public:
            amarokOSDScript( QScriptEngine* ScriptEngine );
            ~amarokOSDScript();

        public slots:
            void showCurrentTrack();
            void show();
            void setDuration( int ms );
            void setTextColor( const QColor &color );
            void setBackgroundColor( const QColor &color );
            void setOffset( int y );
            void setImage( const QImage &image );
            void setScreen( int screen );
            void setText( const QString &text );
            void setDrawShadow( const bool b );
            void setRating( const short rating ); //what is this?
    };
}

#endif
