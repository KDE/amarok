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
         *   Returns the name of the type of bias. eg. "Last.fm"
         */
        virtual QString name() = 0;

        /**
         * Returns the widget that will configure this bias. It will be placed in
         * a vertical layout, and will already have its label be shown. Minimize
         * vertical space if possible.
         */
        virtual QWidget* configWidget() = 0;

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
        CustomBias();
        ~CustomBias() {}


        static CustomBias* self()
        {
            if( !s_self )
                s_self = new CustomBias();

            return s_self;
        }
        
        // reimplement from Dynamic::Bias
        virtual PlaylistBrowserNS::BiasWidget* widget(QWidget* parent = 0);
        virtual QDomElement xml() const;
   
        virtual double energy(const Meta::TrackList& playlist, const Meta::TrackList& context);
        virtual double reevaluate(double oldEnergy, const Meta::TrackList& oldPlaylist, Meta::TrackPtr newTrack, int newTrackPos, const Meta::TrackList& context);

        /**
         * Add a new CustomBiasEntry to the registry. It will show up for users when then select the type of bias they want.
         */
        void registerNewBiasEntry( CustomBiasEntry* );

        /**
         * Remove CustomBiasEntry from the list of bias types that the user can select.
         */
        void removeBiasEntry( CustomBiasEntry* );

        
        // these are used by the CustomBiasWidget. This public coupling sucks, but that's how daniel designed it...
        void setCurrentEntry( CustomBiasEntry* );
        QList< CustomBiasEntry* > currentEntries();

        void setWeight( double );
        
    private:
        QList< CustomBiasEntry* > m_biasEntries;
        CustomBiasEntry* m_currentEntry;

        double m_weight; // slider for percent

        static CustomBias* s_self;
};

// this should not be subclassed by implementing biases. this will call the widget() function
// of the CustomBiasEntry set on the CustomBias. 
class AMAROK_EXPORT CustomBiasEntryWidget : public PlaylistBrowserNS::BiasWidget
{
    Q_OBJECT
    public:
        explicit CustomBiasEntryWidget( CustomBias*, QWidget* parent = 0 );


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

Q_DECLARE_METATYPE( Dynamic::CustomBiasEntry* )



#endif
