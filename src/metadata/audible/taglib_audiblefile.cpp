// (C) 2005 Martin Aumueller <aumuell@reserv.at>
// portions are (C) 2005 Umesh Shankar <ushankar@cs.berkeley.edu>
//          and (C) 2005 Andy Leadbetter <andrew.leadbetter@gmail.com>
//
// See COPYING file for licensing information

#include "taglib_audiblefile.h"

#include "audibletag.h"
#include <taglib/tfile.h>
#include <taglib/audioproperties.h>

namespace TagLib {
////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Audible::File::File(const char *file,
        bool readProperties,
        Properties::ReadStyle propertiesStyle,
        FILE *fp)
        : TagLib::File(file)
        , audibletag( NULL )
        , properties( NULL )
{

    //   debug ("Audible::File: create new file object.");
    //debug ( file );

    /**
     * Create the Audible file.
     */

    if(fp)
        audiblefile = fp;
    else
        audiblefile = fopen(file, "rb");

    if( isOpen() )
    {
        read(readProperties, propertiesStyle );
    }
}

Audible::File::~File()
{
    if(audiblefile)
        fclose(audiblefile);
    delete audibletag;
    delete properties;
}

TagLib::Tag *Audible::File::tag() const
{
    return audibletag;
}

TagLib::Audible::Tag *Audible::File::getAudibleTag() const
{
    return audibletag;
}

Audible::Properties *Audible::File::audioProperties() const
{
    return properties;
}

bool Audible::File::save()
{
    return false;
}

bool Audible::File::isOpen()
{
    return audiblefile != NULL;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Audible::File::read(bool readProperties, Properties::ReadStyle propertiesStyle)
{
    properties =  new Audible::Properties(propertiesStyle);
    audibletag = new Audible::Tag();

    if (audiblefile != NULL) {
        audibletag->readTags( audiblefile );
        int off = audibletag->getTagsEndOffset();
        //fprintf(stderr, "off=%d\n", off);

        if(readProperties)
        {
            // Parse bitrate etc.
            properties->readAudibleProperties( audiblefile, off );
        }
    }
}

}
