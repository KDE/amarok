/*****************************************************************************
* copyright            : (C) 2009 Leo Franchi <lfranchi@kde.org>             *
******************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef DYNAMIC_CUSTOM_BIAS_H
#define DYNAMIC_CUSTOM_BIAS_H

#include "amarok_export.h"
#include "DynamicBiasWidgets.h"
#include "Bias.h"

class QComboBox;
class QVBoxLayout;

namespace Dynamic
{

class CustomBias;
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
        virtual CustomBiasEntry* newCustomBias() = 0;

        /**
         * Creates a new custom bias from the saved settings in the xml doc.
         * The XML should be saved in CustomBiasEntry::xml().
         */
        virtual CustomBiasEntry* newCustomBias( QDomElement e ) = 0;

};

/**
 *  This is the object that the singleton CustomBias can register. A service, or anything
 *  else, can register a new CustomBiasEntry for the user to select as a type of Custom Bias.
 */
class AMAROK_EXPORT CustomBiasEntry : public QObject
{
    Q_OBJECT
    public:
        CustomBiasEntry() {}
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
        void setWeight( int weight );
        
    private:
        double m_weight;

};

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
        static void registerNewBiasFactory( CustomBiasFactory* );

        /**
         * Remove CustomBiasEntry from the list of bias types that the user can select.
         */
        static void removeBiasFactory( CustomBiasFactory* );

        /**
         * Tries to create a new CustomBias from given XML. Will search to see if any of the loaded biases can handle this bias type.
         */
        static CustomBias* fromXml( QDomElement e );

        /**
         * Returns all the current registered factories for this CustomBias
         */
        static QList< CustomBiasFactory* > currentFactories();

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
        void weightChanged( double );
        void biasFactoriesChanged();
        
    private:
        CustomBias();
        CustomBias( Dynamic::CustomBiasEntry* entry, double weight );
        void refreshWidgets();

        static QList< CustomBiasFactory* > s_biasFactories;
        static QList< CustomBias* > s_biases;
        CustomBiasEntry* m_currentEntry;

        double m_weight; // slider for percent

};

// this should not be subclassed by implementing biases. this will call the widget() function
// of the CustomBiasEntry set on the CustomBias. 
class AMAROK_EXPORT CustomBiasEntryWidget : public PlaylistBrowserNS::BiasWidget
{
    Q_OBJECT
    public:
        explicit CustomBiasEntryWidget( CustomBias*, QWidget* parent = 0 );

    signals:
        void weightChangedInt( int );
        
    public slots:
        void refreshBiasFactories();
    
    private slots:
        void selectionChanged( int index );
        void weightChanged( int amount );

    private:
        CustomBias* m_cbias;

        QGridLayout* m_layout;
        Amarok::Slider* m_weightSelection;
        QLabel*         m_weightLabel;
        QLabel*         m_withLabel;
        KComboBox*      m_fieldSelection;
};


}

Q_DECLARE_METATYPE( Dynamic::CustomBiasFactory* )
Q_DECLARE_METATYPE( Dynamic::CustomBiasEntry* )



#endif
