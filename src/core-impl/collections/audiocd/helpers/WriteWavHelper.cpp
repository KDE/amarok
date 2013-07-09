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

#include "WriteWavHelper.h"

#include "core/support/Debug.h"

WriteWavHelper::WriteWavHelper( QString filename, int sampleRate, int channels )
              : m_out( filename )
              , m_sampleRate( sampleRate )
              , m_channels( channels )
              , m_ok( true )
{
    if ( !m_out.open( QIODevice::WriteOnly ) )
    {
        error() << "Can't open file" << filename;
        m_ok = false;
    }
}
WriteWavHelper::~WriteWavHelper()
{
    m_out.close();
}

void
WriteWavHelper::writeHeader( int bytes )
{
    // Info about wav header https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
    int bitsPerSample = 16;
    m_out.write("RIFF");            // ChunkID
    putBytes( bytes + 44 - 8, 4);   // ChunkSize
    m_out.write( "WAVEfmt " );      // Format
    putBytes( 16, 4 );              // Subchunk1ID
    putBytes( 1, 2 );               // Subchunk1 Size
    putBytes( m_channels, 2 );      // NumChannels
    putBytes( m_sampleRate, 4 );    // SampleRate
    putBytes( m_sampleRate * m_channels *  bitsPerSample / 8, 4 ); // ByteRate
    putBytes( m_channels * bitsPerSample / 8, 2 );                // BlockAlign
    putBytes( bitsPerSample, 2 );    // BitsPersample
    m_out.write( "data" );           // Subchunk2ID
    putBytes( bytes, 4 );            // Subchunk2 Size
}

void
WriteWavHelper::writeData( char* data, int size )
{
    m_out.write( data, size );
}

void
WriteWavHelper::putBytes( long num, int bytes )
{
    int i = 0;
    char c;
    while( bytes-- )
    {
        c = ( num >> ( i << 3 ) ) & 0xff;
        m_out.write( &c, 1 );
        i++;
    }
}