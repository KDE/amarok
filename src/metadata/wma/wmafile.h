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

#ifndef TAGLIB_WMAFILE_H
#define TAGLIB_WMAFILE_H

#include <tfile.h>
#include "wmaproperties.h"

namespace TagLib { 

    namespace WMA {

        class WMATag;

        //! A WMA file class with some useful methods specific to WMA
        //! Most of the real parsing is ripped from MPlayer

        /*!
         * This implements the generic TagLib::File API
         */

        class File : public TagLib::File
        {
            public:
                /*!
                 * Contructs a WMA file from \a file.  If \a readProperties is true the
                 * file's audio properties will also be read using \a propertiesStyle.  If
                 * false, \a propertiesStyle is ignored.
                 *
                 * \deprecated This constructor will be dropped in favor of the one below
                 * in a future version.
                 */
                File(const char *file, bool readProperties = true,
                        Properties::ReadStyle propertiesStyle = Properties::Average);

                /*!
                 * Destroys this instance of the File.
                 */
                virtual ~File();

                /*!
                 * Returns a pointer to the WMATag class, which provides the
                 * basic TagLib::Tag fields
                 *
                 * \see WMATag()
                 */
                virtual Tag *tag() const;

                /*!
                 * Returns the WMA::Properties for this file.  If no audio properties
                 * were read then this will return a null pointer.
                 */
                virtual Properties *audioProperties() const;

                /*!
                 * Save the file.  
                 * This is the same as calling save(AllTags);
                 *
                 * \note As of now, saving WMA tags is not supported.
                 */
                virtual bool save();

                /*!
                 * Returns a pointer to the WMATag of the file.
                 *
                 * \note The Tag <b>is still</b> owned by the WMA::File and should not be
                 * deleted by the user.  It will be deleted when the file (object) is
                 * destroyed.
                 */
                WMATag *getWMATag() const;

                /*!
                 * Returns true if the file appears to be a WMA file.
                 */
                bool isValid() const;

            protected:
                File(const File &);
                File &operator=(const File &);

                void getWavHeader(int size);

                int asfReadHeader();

                void read(bool readProperties, Properties::ReadStyle propertiesStyle);

                WMATag *wmaTag;
                Properties *properties;
                bool isWMA;
        };
    }
}

#endif
