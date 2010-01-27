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

#ifndef DYNAMIC_CUSTOM_BIAS_H
#define DYNAMIC_CUSTOM_BIAS_H

#include "amarok_export.h"
#include "CustomBiasEntry.h"
#include "CustomBiasEntryFactory.h"
#include "DynamicBiasWidgets.h"
#include "Bias.h"

class QComboBox;
class QVBoxLayout;

namespace Dynamic
{

class CustomBias;
/**
 * This is a meta bias that allows for different sorts of biases to be plugged
 * in, and the user to select from them. This bias is meant to be modeled on the
 * Proportional bias---so the pluggable biases are binary ones, that operate on
 * individual tracks only.
 *
 * There is only one bias type chosen by the user at a time per bias.
 */
class AMAROK_EXPORT CustomBias : public QObject, public Bias
{
    Q_OBJECT
    public:
        virtual ~CustomBias();

        /**
         * Add a new CustomBiasEntry to the registry. It will show up for users when then select the type of bias they want.
         */
        static void registerNewBiasFactory( CustomBiasEntryFactory* );

        /**
         * Remove CustomBiasEntry from the list of bias types that the user can select.
         */
        static void removeBiasFactory( CustomBiasEntryFactory* );

        /**
         * Tries to create a new CustomBias from given XML. Will search to see if any of the loaded biases can handle this bias type.
         */
        static CustomBias* fromXml( QDomElement e );

        /**
         * Returns all the current registered factories for this CustomBias
         */
        static QList< CustomBiasEntryFactory* > currentFactories();

        /// These are used to create new biases. They register the new bias with the static class
        static CustomBias* createBias(); /// so they can send it messages. 
        static CustomBias* createBias( Dynamic::CustomBiasEntry* entry, double weight );
        
        // reimplement from Dynamic::Bias
        virtual PlaylistBrowserNS::BiasWidget* widget(QWidget* parent = 0);
        virtual QDomElement xml() const;
   
        virtual double energy(const Meta::TrackList& playlist, const Meta::TrackList& context);
        virtual double reevaluate(double oldEnergy, const Meta::TrackList& oldPlaylist, Meta::TrackPtr newTrack, int newTrackPos, const Meta::TrackList& context);
        virtual bool hasCollectionFilterCapability();
        virtual CollectionFilterCapability* collectionFilterCapability();
        
        // these are used by the CustomBiasWidget. This public coupling sucks, but that's how daniel designed it...
        void setCurrentEntry( CustomBiasEntry* );
        CustomBiasEntry* currentEntry();

        virtual double weight() const { return m_weight; }
        void setWeight( double );

    signals:
        void biasFactoriesChanged();
        void biasChanged( Dynamic::Bias* );

    private slots:
        void customBiasChanged();
        
    private:
        CustomBias();
        CustomBias( Dynamic::CustomBiasEntry* entry, double weight );
        void refreshWidgets();

        // the static members deal with all currently managed biases
        static QList< CustomBiasEntryFactory* > s_biasFactories;
        /// if we are told to load a bias but don't yet have that factory registered e.g. on startup,
        /// remember that, so when we do get that factory we can set the proper bias type
        static QMap< QString, CustomBias* > s_failedMap;
        static QMap< QString, QDomElement > s_failedMapXml;
           
        static QList< CustomBias* > s_biases;
        
        CustomBiasEntry* m_currentEntry;

        double m_weight; // slider for percent

};

}

#endif
