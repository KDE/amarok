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

#ifndef AMAROK_TRACKINFO_SCRIPT_H
#define AMAROK_TRACKINFO_SCRIPT_H

#include <QObject>
#include <QtScript>

namespace Amarok
{

    class AmarokTrackInfoScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokTrackInfoScript( QScriptEngine* ScriptEngine );
            ~AmarokTrackInfoScript();

            Q_PROPERTY( int SampleRate READ getSampleRate );
            Q_PROPERTY( int Bitrate READ getBitrate );
            Q_PROPERTY( int Rating WRITE setRating READ getRating );

        public slots:

        private:
            int getSampleRate();
            int getBitrate();
            int getRating();
            void setRating( int Rating );
    };
}

#endif
