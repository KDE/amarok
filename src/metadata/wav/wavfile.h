// (C) 2005 Martin Aumueller <aumuell@reserv.at>
// portions are (C) 2005 Umesh Shankar <ushankar@cs.berkeley.edu>
//          and (C) 2005 Andy Leadbetter <andrew.leadbetter@gmail.com>
//
// See COPYING file for licensing information

#ifndef TAGLIB_WAVFILE_H
#define TAGLIB_WAVFILE_H

#include <taglib/tfile.h>
#include "wavproperties.h"

namespace TagLib { 

    namespace Wav {

        class Tag;

        class File : public TagLib::File
        {
            public:
                /*!
                 * Contructs a Wav file from \a file.  If \a readProperties is true the
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
                 * Returns the Wav::Properties for this file.  If no audio properties
                 * were read then this will return a null pointer.
                 */
                virtual Wav::Properties *audioProperties() const;

                /*!
                 * Save the file.  
                 * This is the same as calling save(AllTags);
                 *
                 * \note As of now, saving Wav tags is not supported.
                 */
                virtual bool save();

                void read(bool readProperties, Properties::ReadStyle propertiesStyle);

                TagLib::Tag *getWavTag() const;

                bool isWavFile() const;

            protected:
                File(const File &);
                File &operator=(const File &);
                bool isOpen();


                TagLib::Tag *wavtag;
                Wav::Properties *properties;

                FILE *wavfile;
        };
    }
}

#endif
