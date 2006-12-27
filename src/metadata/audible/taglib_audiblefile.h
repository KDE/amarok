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

#ifndef TAGLIB_AUDIBLEFILE_H
#define TAGLIB_AUDIBLEFILE_H

#include <taglib/tfile.h>
#include "audibleproperties.h"
#include "audibletag.h"

namespace TagLib { 

    namespace Audible {

        class Tag;

        class File : public TagLib::File
        {
            public:
                /*!
                 * Contructs a Audible file from \a file.  If \a readProperties is true the
                 * file's audio properties will also be read using \a propertiesStyle.  If
                 * false, \a propertiesStyle is ignored.
                 */
                File(const char *file, bool readProperties = true,
                        Properties::ReadStyle propertiesStyle = Properties::Average,
                        FILE *fp=NULL);

                /*!
                 * Destroys this instance of the File.
                 */
                virtual ~File();


                virtual TagLib::Tag *tag() const;

                /*!
                 * Returns the Audible::Properties for this file.  If no audio properties
                 * were read then this will return a null pointer.
                 */
                virtual Audible::Properties *audioProperties() const;

                /*!
                 * Save the file.  
                 * This is the same as calling save(AllTags);
                 *
                 * \note As of now, saving Audible tags is not supported.
                 */
                virtual bool save();

                void read(bool readProperties, Properties::ReadStyle propertiesStyle);

                Audible::Tag *getAudibleTag() const;

                bool isAudibleFile() const;

            protected:
                File(const File &);
                File &operator=(const File &);
                bool isOpen();


                Audible::Tag *audibletag;
                Audible::Properties *properties;

                FILE *audiblefile;
        };
    }
}

#endif
