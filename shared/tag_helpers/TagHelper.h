/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#ifndef TAGHELPER_H
#define TAGHELPER_H

#define MIN_COVER_SIZE 1024 // Minimum size for an embedded cover to be loaded

#include "MetaValues.h"
#include "FileType.h"
#include "amarokshared_export.h"

#include <fileref.h>
#include <tag.h>
#include <id3v1tag.h>

namespace Meta
{
    namespace Tag
    {
        class AMAROKSHARED_EXPORT TagHelper
        {
            public:
                enum UIDType
                {
                    UIDInvalid     = 0,
                    UIDAFT         = 3
                };

                enum FMPS
                {
                    FMPSPlayCount  = 0,
                    FMPSRating     = 1,
                    FMPSScore      = 2
                };

                TagHelper( TagLib::Tag *tag, Amarok::FileType fileType );
                explicit TagHelper( TagLib::ID3v1::Tag *tag, Amarok::FileType fileType );
                virtual ~TagHelper();

                /**
                 * Read all supported tags from file.
                 */
                virtual Meta::FieldHash tags() const;
                /**
                 * Write changed metadata to file.
                 * Return true if something written.
                 */
                virtual bool setTags( const Meta::FieldHash &changes );

                /**
                 * Render tags. Used in UID generator.
                 */
                virtual TagLib::ByteVector render() const;

                /**
                 * Check If file contains embedded cover.
                 */
                virtual bool hasEmbeddedCover() const;

                /**
                 * Read embedded cover from file.
                 */
                virtual QImage embeddedCover() const;

                /**
                 * Add or update cover in file. Will be set as FrontCover.
                 * Return true If something written.
                 */
                virtual bool setEmbeddedCover( const QImage &cover );

                /**
                 * Return file type.
                 */
                Amarok::FileType fileType() const;

                /**
                 * Returns combination of:
                 * title, album, artist, genre and comment fields.
                 * Used to encoding detection.
                 */
                QByteArray testString() const;

            protected:
                /**
                 * Split UID url.
                 * @return Pair of uid type and uid. Uid type can be used to
                 * get field name where to write It (owner in case of ID3v2).
                 */
                QPair< UIDType, QString > splitUID( const QString &uidUrl ) const;
                /**
                 * Check If @arg uid correct UID for specified @arg type.
                 */
                bool isValidUID( const QString &uid, const UIDType type ) const;
                /**
                 * Returns field name for Meta::val* value.
                 */
                TagLib::String fieldName( const qint64 field ) const;
                /**
                 * Returns Meta::val* value corresponds to field name.
                 */
                qint64 fieldName( const TagLib::String &field ) const;
                /**
                 * Split DiscNr field on Disc number and Disc count.
                 */
                QPair< int, int > splitDiscNr( const QString &value ) const;
                /**
                 * Returns field name for specified UID type.
                 */
                TagLib::String uidFieldName( const UIDType type ) const;
                /**
                 * Returns field name for specified FMPS value.
                 */
                TagLib::String fmpsFieldName( const FMPS field ) const;

                QHash< quint64, TagLib::String > m_fieldMap;
                QHash< FMPS,    TagLib::String > m_fmpsFieldMap;
                QHash< UIDType, TagLib::String > m_uidFieldMap;

            private:
                TagLib::Tag *m_tag;
                Amarok::FileType m_fileType;
        };

        /**
         * Returns TagHelper for specified @arg fileref.
         * @arg forceCreation If true: selector will force tag creation.
         * @return TagHelper or NULL if file doesn't have tags.
         * Should be deleted by user after use.
         */
        AMAROKSHARED_EXPORT TagHelper *selectHelper( const TagLib::FileRef &fileref, bool forceCreation = false );
    }
}

#endif // TAGHELPER_H
