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

#ifndef SERVICE_SPECTRUMANALYZER_APPLET_H
#define SERVICE_SPECTRUMANALYZER_APPLET_H

#include <kconfigdialog.h>
#include "context/Applet.h"
#include "context/DataEngine.h"
#include "context/widgets/TextScrollingWidget.h"
#include "core/engine/EngineObserver.h"
#include <phonon/audiodataoutput.h>
#include <Plasma/IconWidget>
#include <plasma/theme.h>
#include <qgraphicsview.h>
#include <QGLPixelBuffer>

#include "AnalyzerGlWidget.h"

class TextScrollingWidget;

namespace Plasma
{
    class IconWidget;
}

class SpectrumAnalyzerApplet: public Context::Applet, public Engine::EngineObserver
{
    Q_OBJECT

    public:
        /**
        *   Contructs an Spectrum Analyzer Applet
        *   @arg parent parent of the applet (applet controller)
        *   @arg args argument passed to the applet
        */
        SpectrumAnalyzerApplet( QObject* parent, const QVariantList& args );

        /**
        *   Destructor
        */
        ~SpectrumAnalyzerApplet();

        /**
        *   Called by the applet contoller to initialize the applet
        */
        void init();

        /**
        *   Called by the applet controller to inform of playback
        */
        virtual void engineNewTrackPlaying();

        /**
        *   Called by the applet controller to inform of playback end
        */
        virtual void enginePlaybackEnded( qint64 finalPosition, qint64 trackLength, PlaybackEndedReason reason );

        /**
        *   paints the applet
        *   @arg painter painter to paint with
        *   @arg option painting options
        *   @arg contentsRect the Rect constrainting the painting
        */
        void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

        /**
        *   Called on resize of the applet window
        *   @arg constraints new contraints of the window
        */
        void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    public slots:
        /**
        *   Updates the data in the applet
        *   @arg name the name of the sender?
        *   @arg data The data received
        */
        void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );

        /**
        *   connects the applet to the source as data engine
        *   @arg source name of the engine to connect to
        */
        void connectSource( const QString &source );

        /**
        *   Called by the config dialog when the user clicks ok
        */
        void saveSettings();

    protected:
        /**
        *   Populates the config dialog with stuff
        *   @arg parent the config dialog
        */
        void createConfigurationInterface( KConfigDialog *parent );

    private:
        TextScrollingWidget                                            *m_headerText;           //!< Header of the applet
        QGraphicsTextItem                                              *m_errorText;            //!< Error text for OpenGL errors
        bool                                                            m_running;              //!< is the playback running
        int                                                             m_numChannels;          //!< Channel count
        QMap< Phonon::AudioDataOutput::Channel, QVector< qint16 > >     m_audioData;            //!< audio data for each channel
        QGraphicsPixmapItem                                            *m_glLabel;              //!< Graphics item to paint the OpenGL scene into
        QGLFormat                                                       m_glFormat;             //!< Format settings of the OpenGL contexts
        AnalyzerGlWidget                                               *m_glWidget;             //!< VSXu OpenGL Widget
        QGLPixelBuffer                                                 *m_glBuffer;             //!< OpenGL Pixel buffer for attached rendering
        QString                                                         m_artist;               //!< Current artist
        QString                                                         m_title;                //!< Current title
        Plasma::IconWidget                                             *m_settingsIcon;         //!< Icon for the settings
        Plasma::IconWidget                                             *m_powerIcon;            //!< Icon for Power on/off
        Plasma::IconWidget                                             *m_detachIcon;           //!< Icon for De/Attach
        Plasma::IconWidget                                             *m_fullscreenIcon;       //!< Icon for Fullscreen mode
        bool                                                            m_glError;              //!< Did we have an OpenGL error?
        QString                                                         m_glErrorText;          //!< Text of the OpenGL error
        bool                                                            m_detached;             //!< Is the render OpenGL window detached
        bool                                                            m_power;                //!< Applet power status
        bool                                                            m_fullscreen;           //!< are we in fullscreen mode

    private slots:

        /**
        *   Renders the OpenGL Scene and puts it into the Graphics Pixmap Item
        */
        void updateOpenGLScene();

        /**
        *   Detaches the OpenGL window
        *   @arg fullscreen whether we should detach to fullscreen mode
        */
        void detach( bool fullscreen );

        /**
        *   Ataches the OpenGL window
        */
        void attach();

        /**
        *   Toogles between on and off
        */
        void togglePower();

        /**
        *   Toggles between detached and attached mode
        */
        void toggleDetach();

        /**
        *   toggle fullscreen mode
        */
        void toggleFullscreen();
};

Q_DECLARE_METATYPE ( QVector< qint16 > )
K_EXPORT_AMAROK_APPLET( spectrumanalyzer, SpectrumAnalyzerApplet )

#endif
