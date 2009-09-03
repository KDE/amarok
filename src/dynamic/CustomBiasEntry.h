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

class CustomBiasEntry;

/**
 * The factory which creates custom bias entries on demand. As the user can create any number
 * of biases from from the bias addition widget, new custom biass types need to be able to be
 * generated on command and at runtime.
 **/
class AMAROK_EXPORT CustomBiasFactory
{
    public:
        CustomBiasFactory() {}
        virtual ~CustomBiasFactory() {}

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
        virtual CustomBiasEntry* newCustomBias( double weight ) = 0;

        /**
         * Creates a new custom bias from the saved settings in the xml doc.
         * The XML should be saved in CustomBiasEntry::xml().
         */
        virtual CustomBiasEntry* newCustomBias( QDomElement e , double weight ) = 0;

};

/**
 *  This is the object that the singleton CustomBias can register. A service, or anything
 *  else, can register a new CustomBiasEntry for the user to select as a type of Custom Bias.
 */
class AMAROK_EXPORT CustomBiasEntry : public QObject
{
    Q_OBJECT
    public:
        CustomBiasEntry( double wieght );
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
        * See APIDOX for Bias.h for more explanation.
        */
        virtual CollectionFilterCapability* collectionFilterCapability() { return 0; }

        double weight();

    public slots:
        // takes an int 0-100 as it is connected to the slider
        void setWeight( int weight );

    private:
        double m_weight;

};

}


Q_DECLARE_METATYPE( Dynamic::CustomBiasFactory* )
Q_DECLARE_METATYPE( Dynamic::CustomBiasEntry* )

#endif
