/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef DYNAMIC_CUSTOM_BIAS_ENTRY_H
#define DYNAMIC_CUSTOM_BIAS_ENTRY_H

#include "amarok_export.h"
#include "DynamicBiasWidgets.h"
#include "Bias.h"

namespace Dynamic
{

/**
 *  This is the object that the singleton CustomBias can register. A service, or anything
 *  else, can register a new CustomBiasEntry for the user to select as a type of Custom Bias.
 */
class AMAROK_EXPORT CustomBiasEntry : public QObject
{
    Q_OBJECT
    public:
        CustomBiasEntry();
        virtual ~CustomBiasEntry() {}

        /**
         * Returns an internal non-translatable name for this custom bias type.
         * Must be the same as what the associated CustoPluginFactory::pluginName
         * returns.
         */
        virtual QString pluginName() const = 0;

        /**
         * Returns the widget that will configure this bias. It will be placed in
         * a vertical layout, and will already have its label be shown. Minimize
         * vertical space if possible.
         */
        virtual QWidget* configWidget( QWidget* parent ) = 0;

        /**
         * Returns an XML representation of the bias so it can be saved to disk.
         */
        virtual QDomElement xml( QDomDocument doc ) const = 0;

        /**
         * Returns if the given track satisfies this bias' conditions.
         */
        virtual bool trackSatisfies( const Meta::TrackPtr ) = 0;

        /**
         * Convenience method. Returns number of tracks that satisfy
         * from the list. Preferred when there are multiple tracks to
         * check to allow the Bias to do more local caching of expensive
         * (read: web) lookups.
         */
        virtual double numTracksThatSatisfy( const Meta::TrackList& ) = 0;

        /**
         * If your custom bias operates on the collection (that is, represents a subset of the collection
         * at any given point in time, and you also want to filter/control the initial generation of the
         * playlist, you can implement this capability.
         */
        virtual bool hasCollectionFilterCapability() = 0;

        /**
        * Returns a QSet< QByteArray > of track uids that match this bias. Used when building the
        * initial playlists, this must be implemented if your bias returns true for filterFromCollection.
        * See APIDOX of Bias.h for more explanation.
        *
        * As the currently set weight of the bias is stored in the parent CustomBias, the individual CustomBiasEntries
        * don't know the value, so can't return it. Use the passed in value to construct your CollectionFilterCapability
        * that you return to the BiasSolver.
        */
        virtual CollectionFilterCapability* collectionFilterCapability( double weight ) { Q_UNUSED( weight ); return 0; }

    signals:
        void biasChanged();
};

}


Q_DECLARE_METATYPE( Dynamic::CustomBiasEntry* )

#endif
