/*
 * Copyright 2018  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ANALYZERWORKER_H
#define ANALYZERWORKER_H

#include "AnalyzerBase.h"

#include <phonon/audiodataoutput.h>

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QTime>

#include <complex>
#include <fftw3.h>


class QTimer;

namespace Analyzer
{

/**
 * Base worker class for all analyzers
 * All compute heavy tasks should be offloaded to this.
 */
class Worker : public QObject
{
    friend class Base;

    Q_OBJECT

public:
    const static int PROCESSING_INTERVAL = 5; // Interval between new data lookups
    const static int DATA_BUFFER_SIZE = 8; // Higher values increase latency, lower values increase risk of missing frames

    Worker();
    ~Worker() override;

    void playbackStateChanged();
    void stopTimers();

protected:
    /**
     * @return The current scope data.
     */
    const QVector<double>& scope() const { return m_currentScope; }

    /**
     * This function is being called after new scope data is ready.
     * Get the scope to be analyzed by calling scope().
     * Subclasses must implement this function.
     */
    virtual void analyze() = 0;

private:
    struct BandInfo
    {
        double lowerFreq;
        double midFreq;
        double upperFreq;
        double lowerK;
        double midK;
        double upperK;
        int scopeIndex;
    };

    /**
     * This function is thread-safe.
     */
    void receiveData( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > &newData );

    // None of the following functions are thread-safe. Only connect with queued connections to them.
    void processData();
    void applyWindowFunction();
    void makeScope();
    void setSampleSize( uint size );
    void setWindowFunction( Base::WindowFunction windowFunction );
    void setScopeSize( int size );
    void calculateExpFactor( qreal minFreq, qreal maxFreq, int sampleRate );
    void resetDemo() { m_demoT = 201; }

    /**
     * Override this function for your custom idle animation.
     */
    virtual void demo();

    fftw_plan m_plan;
    mutable QMutex m_rawInMutex;
    QList<double> m_rawIn;
    double *m_in;
    std::complex<double> *m_out;
    QVector<double> m_currentScope;
    QVector<BandInfo> m_interpolatedScopeBands;
    QVector<BandInfo> m_notInterpolatedScopeBands;
    uint m_size;
    double m_expFactor;
    Base::WindowFunction m_windowFunction;
    int m_expectedDataTime;
    int m_demoT;
    QElapsedTimer m_lastUpdate;
    QTimer *m_demoTimer;
    QTimer *m_processTimer;
};

}

#endif // ANALYZERWORKER_H
