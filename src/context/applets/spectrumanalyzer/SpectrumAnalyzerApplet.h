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
#include "FastFourierTransformation.h"

#include <phonon/audiodataoutput.h>
#include <Plasma/IconWidget>
#include <plasma/theme.h>
#include <qgraphicsview.h>
#include <QGLPixelBuffer>
#include <QAction>

#include "AnalyzerGlWidget.h"
#include <ui_spectrumAnalyzerSettings.h>

class TextScrollingWidget;

namespace Plasma
{
    class IconWidget;
}

class SpectrumAnalyzerApplet: public Context::Applet
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
        *   Slot to inform the applet of a new track playing
        */
        virtual void started();

        /**
        *   Slot to inform the applet that playback has been stopped
        */
        virtual void stopped();

        /**
        *   connects the applet to the source as data engine
        *   @arg source name of the engine to connect to
        */
        void connectSource( const QString &source );

        /**
        *   Called by the config dialog when the user clicks ok
        */
        void saveSettings();

        /**
        *   Called by the AnalyzerGlWidget if a key was pressed
        *   @arg key Key that was pressed
        */
        void keyPressed( int key );

    protected:
        /**
        *   Populates the config dialog with stuff
        *   @arg parent the config dialog
        */
        void createConfigurationInterface( KConfigDialog *parent );

    private:
        qreal                                                           m_visualHeight;         //!< Height of the applet if the analyzer is running
        TextScrollingWidget                                            *m_headerText;           //!< Header of the applet
        QGraphicsTextItem                                              *m_errorText;            //!< Error text for OpenGL errors
        bool                                                            m_running;              //!< is the playback running
        int                                                             m_numChannels;          //!< Channel count
        QMap< Phonon::AudioDataOutput::Channel, QVector< qint16 > >     m_audioData;            //!< audio data for each channel
        QPixmap                                                         m_glPixmap;             //!< Pixmap to paint the OpenGL scene into
        QGLFormat                                                       m_glFormat;             //!< Format settings of the OpenGL contexts
        AnalyzerGlWidget                                               *m_glWidget;             //!< VSXu OpenGL Widget
        QGLPixelBuffer                                                 *m_glBuffer;             //!< OpenGL Pixel buffer for attached rendering
        QString                                                         m_artist;               //!< Current artist
        QString                                                         m_title;                //!< Current title
        Plasma::IconWidget                                             *m_settingsIcon;         //!< Icon for the settings
        Plasma::IconWidget                                             *m_powerIcon;            //!< Icon for Power on/off
        Plasma::IconWidget                                             *m_detachIcon;           //!< Icon for De/Attach
        Plasma::IconWidget                                             *m_fullscreenIcon;       //!< Icon for Fullscreen mode
        Plasma::IconWidget                                             *m_modeIcon;             //!< Icon for switching mode
        Ui::spectrumAnalyzerSettings                                    ui_Settings;            //!< settings dialog
        bool                                                            m_glError;              //!< Did we have an OpenGL error?
        QString                                                         m_glErrorText;          //!< Text of the OpenGL error
        bool                                                            m_detached;             //!< Is the render OpenGL window detached
        bool                                                            m_power;                //!< Applet power status
        bool                                                            m_fullscreen;           //!< are we in fullscreen mode
        bool                                                            m_cutLowFrequencys;     //!< whether or not low frequencies should be excluded

        /**
        *   Does a Fast Fourier Transformation on the Audio Data
        *   @arg audioData audio Data to do the transformation on
        */
        void transformAudioData( QVector<float> &audioData );

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

        /**
        *   Switches the analyzer mode to the next available one
        */
        void nextMode();
};

Q_DECLARE_METATYPE ( QVector< qint16 > )
AMAROK_EXPORT_APPLET( spectrumanalyzer, SpectrumAnalyzerApplet )

#endif
