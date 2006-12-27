/***************************************************************************
    copyright            : (C) 2006 by Martin Aumueller
    email                : aumuell@reserv.at

    copyright            : (C) 2005 by Andy Leadbetter
    email                : andrew.leadbetter@gmail.com
                           (original mp4 implementation)
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#include "wavfile.h"

#include <taglib/tfile.h>
#include <taglib/audioproperties.h>
#include <taglib/tag.h>

namespace TagLib {
////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Wav::File::File(const char *file,
        bool readProperties,
        Properties::ReadStyle propertiesStyle,
        FILE *fp)
        : TagLib::File(file)
        , wavtag( NULL )
        , properties( NULL )
{

    //   debug ("Wav::File: create new file object.");
    //debug ( file );

    /**
     * Create the Wav file.
     */

    if(fp)
        wavfile = fp;
    else
        wavfile = fopen(file, "rb");

    if( isOpen() )
    {
        read(readProperties, propertiesStyle );
    }
}

Wav::File::~File()
{
    if(wavfile)
        fclose(wavfile);
    delete properties;
}

TagLib::Tag *Wav::File::tag() const
{
    return NULL;
}

TagLib::Tag *Wav::File::getWavTag() const
{
    return NULL;
}

Wav::Properties *Wav::File::audioProperties() const
{
    return properties;
}

bool Wav::File::save()
{
    return false;
}

bool Wav::File::isOpen()
{
    return wavfile != NULL;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Wav::File::read(bool readProperties, Properties::ReadStyle propertiesStyle)
{
    properties =  new Wav::Properties(propertiesStyle);

    if (wavfile != NULL) {
        if(readProperties)
        {
            // Parse bitrate etc.
            properties->readWavProperties( wavfile );
        }
    }
}

}
