/***************************************************************************
    copyright            : (C) 2005 by Martin Aumueller
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
