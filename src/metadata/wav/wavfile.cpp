// (C) 2005 Martin Aumueller <aumuell@reserv.at>
// portions are (C) 2005 Umesh Shankar <ushankar@cs.berkeley.edu>
//          and (C) 2005 Andy Leadbetter <andrew.leadbetter@gmail.com>
//
// See COPYING file for licensing information

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
