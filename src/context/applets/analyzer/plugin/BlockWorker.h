/****************************************************************************************
 * Copyright (c) 2017 Malte Veerman <malte.veerman@gmail.com>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef BLOCKWORKER_H
#define BLOCKWORKER_H

#include "AnalyzerWorker.h"

#include <QMutex>
#include <QTime>
#include <QVector>


class BlockWorker : public Analyzer::Worker
{
    friend class BlockRenderer;

    Q_OBJECT

public:
    BlockWorker( int rows, int columns, qreal step, bool showFadebars );

    void setStep( qreal step ) { m_step = step; }
    void setRows( int rows );
    void setColumns( int columns );
    void setRefreshRate( qreal refreshRate ) { m_refreshTime = std::floor( 1000.0 / refreshRate ); }
    void setShowFadebars( bool showFadebars ) { m_showFadebars = showFadebars; }

signals:
    void finished();

protected:
    void analyze() Q_DECL_OVERRIDE;

private:
    struct Fadebar
    {
        int y;
        qreal intensity;
        Fadebar()
        {
            y = 0;
            intensity = 0.0;
        }
        Fadebar( int y, qreal intensity )
        {
            this->y = y;
            this->intensity = intensity;
        }
    };
    mutable QMutex m_mutex;  //used to lock m_store and m_fadebars
    QVector<double> m_store;  //current bar heights
    QVector<double> m_yscale;
    QVector<QList<Fadebar> > m_fadebars;
    qreal m_step;
    int m_rows;
    int m_columns;
    int m_refreshTime;
    bool m_showFadebars;
    QTime m_lastUpdate;
};

#endif //BLOCKWORKER_H
