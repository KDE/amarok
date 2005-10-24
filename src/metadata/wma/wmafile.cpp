/***************************************************************************
copyright            : (C) 2005 by Umesh Shankar
email                : ushankar@cs.berkeley.edu
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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

#include "wmatag.h"
#include <tfile.h>
#include <audioproperties.h>

namespace TagLib {
////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

WMA::File::File(const char *file, bool readProperties,
        Properties::ReadStyle propertiesStyle) : TagLib::File(file)
{
    properties = NULL;
    wmaTag = NULL;
    isWMA = false;
    if(isOpen()) {
        wmaTag = new WMATag();
        read(readProperties, propertiesStyle);
    }
}

WMA::File::~File()
{ 
    delete wmaTag; 
}

TagLib::Tag *WMA::File::tag() const
{
    return wmaTag;
}

TagLib::WMA::WMATag *WMA::File::getWMATag() const
{
    return wmaTag;
}

WMA::Properties *WMA::File::audioProperties() const
{
    return properties;
}

bool WMA::File::save()
{
    //debug ("WMA::File: Saving not supported yet.");
    return false;
}

bool WMA::File::isValid() const
{
    fprintf(stderr, "WMA: isValid(): isWMA=%d, TagLib: isValid()=%d, isOpen()=%d\n",
            int(isWMA), int(TagLib::File::isValid()), int(isOpen()));
    return isWMA && TagLib::File::isValid();
}


////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void WMA::File::read(bool readProperties, Properties::ReadStyle propertiesStyle)
{
    fprintf(stderr, "WMA::FILE::read\n");
    if(readProperties)
        properties = new Properties(propertiesStyle);

    isWMA = asfReadHeader();
}
}

