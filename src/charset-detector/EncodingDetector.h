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

#ifndef AMAROK_ENCODING_DETECTOR_H
#define AMAROK_ENCODING_DETECTOR_H

#include <QObject>

class EncodingDetector : public QObject
{
    Q_OBJECT

    public:
        EncodingDetector( const char* str );
        ~EncodingDetector();

        QString encoding() const;
        bool    gotResult() const;
        //TODO: use the KDE4.2 API to do this
        void    startTableDetector();
        //TODO: improve the algorithm
        void    startFileNameDetector();
        void    startLocaleDetector();

    private:
        const   char* m_str;
        QString m_encoding;
        bool    m_gotResult;
};

#endif
