/****************************************************************************************
 * Copyright (C) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_METATAGLIB_H
#define AMAROK_METATAGLIB_H

#include "MetaValues.h"
#include "amarokshared_export.h"

#include <QString>

/* This file exists because we need to share the implementation between
 * amaroklib and amarokcollectionscanner (which doesn't link to amaroklib).
 */
namespace Meta
{
    namespace Tag
    {
        AMAROKSHARED_EXPORT Meta::FieldHash readTags( const QString &path );

        /**
         * Writes tags stored in @param changes back to file. Respects
         * AmarokConfig::writeBack() and AmarokConfig::writeBackStatistics().
         *
         * If you are about to call this from the main thread, you should really think
         * of using WriteTagsJob instead.
         *
         * Changed in 2.8: this method no longer checks AmarokConfig::writeBack()
         *
         * @param path path of the file to write the tags to
         * @param changes Meta:val* key to value map of tags to write
         * @param writeStatistics whether to include statistics-related tags when writing
         *
         * @see WriteTagsJob
         */
        AMAROKSHARED_EXPORT void writeTags( const QString &path,
                                            const Meta::FieldHash &changes,
                                            bool writeStatistics );

        // the utilities don't need to handle images
        AMAROKSHARED_EXPORT QImage embeddedCover( const QString &path );

        /**
         * Writes embedded cover back to file. Overwrites any possible existing covers.
         * This function doesn't take any configuration any account.
         *
         * If you are about to call this from the main thread, you should really think
         * of using WriteTagsJob instead.
         *
         * @see WriteTagsJob
         */
        AMAROKSHARED_EXPORT void setEmbeddedCover( const QString &path, const QImage &cover );
    }
}

#endif // AMAROK_METATAGLIB_H
