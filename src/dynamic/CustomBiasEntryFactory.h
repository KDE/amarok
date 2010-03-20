/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef AMAROK_CUSTOM_BIAS_ENTRY_FACTORY_H
#define AMAROK_CUSTOM_BIAS_ENTRY_FACTORY_H

#include "amarok_export.h"

#include <QString>
#include <QDomElement>

namespace Dynamic
{

class CustomBiasEntry;

/**
 * The factory which creates custom bias entries on demand. As the user can create any number
 * of biases from from the bias addition widget, new custom biass types need to be able to be
 * generated on command and at runtime.
 **/
class AMAROK_EXPORT CustomBiasEntryFactory
{
    public:
        CustomBiasEntryFactory() {}
        virtual ~CustomBiasEntryFactory() {}

        /**
         *   Returns the name of the type of bias. eg. "Last.fm Similar Artists"
         */
        virtual QString name() const = 0;

        /**
         * Returns an internal non-translatable name for this custom bias type.
         */
        virtual QString pluginName() const = 0;

        /**
         * Create the custom bias. The caller takes owner of the pointer
         */
        virtual CustomBiasEntry* newCustomBiasEntry() = 0;

        /**
         * Creates a new custom bias from the saved settings in the xml doc.
         * The XML should be saved in CustomBiasEntry::xml().
         */
        virtual CustomBiasEntry* newCustomBiasEntry( QDomElement e ) = 0;

};

}

Q_DECLARE_METATYPE( Dynamic::CustomBiasEntryFactory* )

#endif // end include guard
