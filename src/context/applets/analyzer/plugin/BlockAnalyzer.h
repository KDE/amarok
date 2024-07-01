/****************************************************************************************
 * Copyright (c) 2003-2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2005-2013 Mark Kretschmann <kretschmann@kde.org>                       *
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

#ifndef BLOCKANALYZER_H
#define BLOCKANALYZER_H

#include "AnalyzerBase.h"

#include <QImage>
#include <QPixmap>
#include <QQuickWindow>
#include <QSharedPointer>
#include <QSize>

class QPalette;

class BlockAnalyzer : public Analyzer::Base
{
    friend class BlockRenderer;

    Q_OBJECT
    Q_PROPERTY(FallSpeed fallSpeed READ fallSpeed WRITE setFallSpeed NOTIFY fallSpeedChanged)
    Q_PROPERTY(int columnWidth READ columnWidth WRITE setColumnWidth NOTIFY columnWidthChanged)
    Q_PROPERTY(bool showFadebars READ showFadebars WRITE setShowFadebars NOTIFY showFadebarsChanged)

public:
    enum FallSpeed
    {
        VerySlow = 0,
        Slow = 1,
        Medium = 2,
        Fast = 3,
        VeryFast = 4
    };
    Q_ENUM( FallSpeed )

    explicit BlockAnalyzer( QQuickItem *parent = nullptr );

    Renderer* createRenderer() const override;

    FallSpeed fallSpeed() const { return m_fallSpeed; }
    void setFallSpeed( FallSpeed fallSpeed );
    int columnWidth() const { return m_columnWidth; }
    void setColumnWidth( int columnWidth );
    bool showFadebars() const { return m_showFadebars; }
    void setShowFadebars( bool showFadebars );

    // Signed ints because most of what we compare them against are ints
    static const int BLOCK_HEIGHT = 2;
    static const int FADE_SIZE    = 90;

Q_SIGNALS:
    void fallSpeedChanged();
    void columnWidthChanged();
    void showFadebarsChanged( bool );
    void stepChanged( qreal );
    void rowsChanged( int );
    void columnsChanged( int );
    void refreshRateChanged( qreal );

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;
#else
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#endif
    Analyzer::Worker* createWorker() const override;
    virtual void paletteChange( const QPalette& );

    void drawBackground( const QPalette &palette );
    void determineStep();

private:
    void newWindow( QQuickWindow *window );

    int m_columns, m_rows;      //number of rows and columns of blocks
    int m_columnWidth;
    bool m_showFadebars;
    QPixmap m_barPixmap;
    QPixmap m_topBarPixmap;
    QPixmap m_backgroundPixmap;
    QVector<QPixmap> m_fadeBarsPixmaps;
    bool m_pixmapsChanged;

    qreal m_step; //rows to fall per frame
    FallSpeed m_fallSpeed;
};

#endif
