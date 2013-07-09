/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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

#ifndef WRITEWAVHELPER_H
#define WRITEWAVHELPER_H

#include <QString>
#include <QFile>

/**
 * Writes wav-file.
 */
class WriteWavHelper
{
public:
    WriteWavHelper( QString filename, int sampleRate = 44100, int channels = 2 );

    ~WriteWavHelper();
    /** @bytes -- size of data from data section in bytes */
    void writeHeader( int bytes );
    void writeData( char* data, int size );

private:
    void putBytes( long num, int bytes );

    QFile m_out;
    int m_sampleRate;
    int m_channels;
    bool m_ok;
};
#endif