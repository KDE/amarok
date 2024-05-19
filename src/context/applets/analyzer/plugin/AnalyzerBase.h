/****************************************************************************************
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2009 Martin Sandsmark <martin.sandsmark@kde.org>                       *
 * Copyright (c) 2013 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef ANALYZERBASE_H
#define ANALYZERBASE_H

#ifdef __FreeBSD__
#include <sys/types.h>
#endif

#include <phonon/audiodataoutput.h>

#include <QMap>
#include <QQuickFramebufferObject>
#include <QThread>
#include <QVector>

#include <KConfigGroup>


namespace Analyzer
{
    class Worker;

    class Base : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(qreal minFrequency READ minFreq WRITE setMinFreq NOTIFY minFreqChanged)
    Q_PROPERTY(qreal maxFrequency READ maxFreq WRITE setMaxFreq NOTIFY maxFreqChanged)
    Q_PROPERTY(WindowFunction windowFunction READ windowFunction WRITE setWindowFunction NOTIFY windowFunctionChanged)
    Q_PROPERTY(qreal minimumFrequency READ minFreq WRITE setMinFreq NOTIFY minFreqChanged)
    Q_PROPERTY(qreal maximumFrequency READ maxFreq WRITE setMaxFreq NOTIFY maxFreqChanged)
    Q_PROPERTY(int sampleSize READ sampleSize WRITE setSampleSize NOTIFY sampleSizeChanged)

public:
    enum WindowFunction
    {
        Rectangular,
        Hann,
        Nuttall,
        Lanczos,
        Sine
    };
    Q_ENUM(WindowFunction)

    static const int DEMO_INTERVAL = 20; // ~50 fps

    ~Base() override;

    qreal maxFreq() const { return m_maxFreq; }
    void setMaxFreq( qreal maxFreq );
    qreal minFreq() const { return m_minFreq; }
    void setMinFreq( qreal minFreq );
    WindowFunction windowFunction() const;
    void setWindowFunction( WindowFunction windowFunction );
    int sampleSize() const;
    void setSampleSize( uint sampleSize );

    /**
     * Returns the worker object associated with this analyzer.
     */
    const Worker* worker() const;

Q_SIGNALS:
    void minFreqChanged();
    void maxFreqChanged();
    void scopeSizeChanged( uint );
    void windowFunctionChanged( WindowFunction );
    void sampleSizeChanged( uint );
    void calculateExpFactorNeeded( qreal, qreal, uint );

protected:
    Base( QQuickItem* );

    /**
     * Creates a new worker instance.
     * Subclasses must implement this function.
     * All compute heavy tasks should be offloaded to the created worker.
     * If you make any connections with your worker, remember to make them queued connections.
     * Do not set a parent for the worker. Base will take ownership of it.
     */
    virtual Worker* createWorker() const = 0;

    /**
     * Returns the standard KConfigGroup for all analyzers.
     * You can reimplement this function, if you want your subclass to have a different config.
     */
    virtual KConfigGroup config() const;

    /**
     * Use this function to set the size for the scope computed by the worker.
     */
    void setScopeSize( int size );

    /**
     * Use this function to pause audio data processing and analyzer updating
     */
    void drawNeedChanged( const bool );
private:
    void connectSignals();
    void disconnectSignals();
    void refreshSampleRate();

    double m_minFreq, m_maxFreq;
    int m_sampleRate;
    int m_scopeSize;

    Worker *m_worker;
    QThread m_workerThread;
};


} //END namespace Analyzer

#endif
