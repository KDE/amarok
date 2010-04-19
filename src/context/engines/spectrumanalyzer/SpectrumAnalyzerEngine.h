/****************************************************************************************
 * Copyright (c) 2010 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
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

#ifndef AMAROK_SPECTRUMANALYZER_ENGINE
#define AMAROK_SPECTRUMANALYZER_ENGINE

#include <phonon/audiodataoutput.h>
#include <phonon/path.h>

#include "ContextObserver.h"
#include "context/DataEngine.h"
#include "core/meta/Meta.h"

/**
*    This class provides spectrum information from the currently playing song
*/

class SpectrumAnalyzerEngine : public Context::DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT
  
    public:

        /**
        *   Creates an spectrum analyzer engine to get audio Data
        */
        SpectrumAnalyzerEngine( QObject* parent, const QList<QVariant>& args );

        /**
        *   Destructor
        */
        ~SpectrumAnalyzerEngine();

        /**
        *   @returns all sources this engine gets data from
        */
        QStringList sources() const;

        /**
        *   Receives a message from (?! the applet ?!)
        */
        void message( const Context::ContextState& state );

        /**
        *   Is called by the applet controler when the track has changed
        */
        using Observer::metadataChanged;
        void metadataChanged( Meta::TrackPtr track );

    protected:

        /**
        *   Applet requests data
        */
        bool sourceRequestEvent( const QString& name );

    private slots:

        /**
        *   Updates the data of the engine for the spectrum analyzer applet
        */
        void receiveData( const QMap<Phonon::AudioDataOutput::Channel,QVector<qint16> > &data );

    private:

        /**
        *   Updates the data for the spectrum analyzer applet
        */
        void update();

        Phonon::AudioDataOutput                                        *m_audioDataOutput;      //!< The audio Data Output to get data
        QMap< Phonon::AudioDataOutput::Channel, QVector< qint16 > >     m_audioData;            //!< Audio data to be passed to the applet
        QStringList                                                     m_sources;              //!< Sources connected to this engine
        bool                                                            m_requested;            //!< Ia an update requested
        bool                                                            m_dataHasChanged;       //!< Has the data changed
        Meta::TrackPtr                                                  m_currentTrack;         //!< Track currently played

};

Q_DECLARE_METATYPE ( QVector< qint16 > )
K_EXPORT_AMAROK_DATAENGINE( spectrumanalyzer, SpectrumAnalyzerEngine )

#endif
