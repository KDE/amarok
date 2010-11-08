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

#ifndef ANALYZERGLWIDGET_H
#define ANALYZERGLWIDGET_H

#include <QGLWidget>
#include <QKeyEvent>

class AnalyzerGlWidget: public QGLWidget
{
    Q_OBJECT

    enum AnalyzerMode
    {
        Bars = 0,
        Wave,
        Waterfall,
        Waves3D,
        Channels3D
    };

    public:
        /**
        *   Widget to display OpenGL VSXu window
        */
        AnalyzerGlWidget( QGLFormat format, QColor fillColor );

        /**
        *   Destructor
        */
        ~AnalyzerGlWidget();

        /**
        *   Does resizing work on the OpenGL scene
        */
        virtual void resizeGLScene( int width, int height );

        /**
        *   initializes all OpenGL stuff
        */
        virtual void initializeGLScene();

        /**
        *   does the actual OpenGL paint stuff
        */
        virtual void paintGLScene();

        /**
        *   Sets the mode of the analyzer
        *   @arg mode analyzer mode to use
        */
        void setMode( int mode );

        /**
        *   @returns the current mode of the analyzer
        */
        int getMode();

        /**
        *   Sets the accuracy (e.g. bars per frequency) of the analyzer
        */
        void setAccuracy( float barsPerFrequency ) { m_barsPerFrequency = barsPerFrequency; };

        /**
        *   @returns the accuracy (e.g. bars per frequency) of the analyzer
        */
        float getAccuracy() { return m_barsPerFrequency; };

        /**
        *   Sets the frequency values
        *   @arg frequencyValues values of the diffenrent frequencys
        */
        void setFrequencyValues( QVector<int> frequencyValues );

        /**
        *   @returns if peaks should be shown
        */
        bool getPeaksStatus() { return m_showPeaks; };

        /**
        *   @returns if wave should be shown
        */
        bool getWaveStatus() { return m_showWave; };

        /**
        *   Sets if peaks should be shown
        *   @arg value Value to set
        */
        void setPeaksStatus( bool value ) { m_showPeaks = value; };

        /**
        *   Sets if Wave should be shown
        *   @arg value Value to set
        */
        void setWaveStatus( bool value ) { m_showWave = value; };

        /**
        *   Sets peak sinkrate
        *   @arg sinkrate Value to set
        */
        void setSinkrate( float sinkrate ) { m_peakSinkRate = sinkrate; };

        /**
        *   Gets peak sinkrate
        */
        float getSinkrate() { return m_peakSinkRate; };

    signals:
        /**
        *   Is emitted when a key was pressed on this widget
        *   @arg key Key that was pressed
        */
        void keyPressed( int key );

        /**
        *   Is emitted when the widget is hidden
        */
        void hidden();

    private:
        QColor                          m_fillColor;                //!< Color to fill the scene with
        AnalyzerMode                    m_mode;                     //!< Analyzer mode \see AnalyzerMode
        QVector<int>                    m_frequencyValues;          //!< strength of the different frequencys
        QVector<int>                    m_peaks;                    //!< peak position of the last paint
        QList< QVector<int> >           m_lastValues;               //!< last values for 3D Waves
        bool                            m_showPeaks;                //!< should peaks be shown
        bool                            m_showWave;                 //!< show wave overlay in bars mode
        int                             m_peakSinkRate;             //!< sink rate of peaks
        float                           m_barsPerFrequency;         //!< bars per frequency

        /**
        *   Calls resizeGLScene()
        *   \see resizeGLScene
        */
        virtual void resizeGL( int width, int height );
        virtual void paintGL();

        /**
        *   paints a bar analyzer
        *   @arg values frequency values to use
        */
        void paintBars( QVector<int> values );

        /**
        *   paints a wave analyzer
        *   @arg values frequency values to use
        */
        void paintWave( QVector<int> values );

        /**
        *   paints a waterfall analyzer
        *   @arg values frequency values to use
        */
        void paintWaterfall( QVector<int> values );

        /**
        *   paints a 3D wave analyzer
        *   @arg values frequency values to use
        */
        void paint3DWaves( QVector<int> values );

        /**
        *   paints a 3D channel analyzer
        *   @arg values frequency values to use
        */
        void paint3DChannels( QMap<int,QVector<int> > values );

        /**
        *   Does a spline interpolation
        *   @returns set with new splines
        *   @arg splines splines to interpolate from
        *   @arg size size of new value vector
        */
        QVector<int> interpolateSpline( QVector<int> splines, int size );

        /**
        *   @returns a waterfall diagram color for a value
        *   @arg value value to calculate color for
        */
        QVector<GLubyte> getValueColor( int value );

    protected:
        /**
        *   Overrides the normal key handling of QWidget
        *   @arg event key press event
        */
        void keyPressEvent( QKeyEvent * event );

        /**
        *   Overrides the normal hidden handling of QWidget
        *   @arg event hide event
        */
        void hideEvent ( QHideEvent * event );
};

#endif
