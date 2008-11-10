/*
 *   KSysGuard, the KDE System Guard
 *
 *   Copyright 1999 - 2002 Chris Schlaeger <cs@kde.org>
 *   Copyright 2006 John Tapsell <tapsell@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.

 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "signalplotter.h"

#include <math.h>
#include <string.h>

#include <QList>
#include <QPalette>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QPainterPath>
#include <QtGui/QPolygon>

#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KApplication>
#include <KStandardDirs>

#include <plasma/svg.h>

namespace Plasma
{

class SignalPlotterPrivate
{
    public:
        SignalPlotterPrivate()
            : svgBackground(0)
        { }

        ~SignalPlotterPrivate()
        {
        }

    int precision;
    uint samples;
    uint bezierCurveOffset;

    double scaledBy;
    double verticalMin;
    double verticalMax;
    double niceVertMin;
    double niceVertMax;
    double niceVertRange;

    bool fillPlots;
    bool showLabels;
    bool showTopBar;
    bool stackPlots;
    bool useAutoRange;
    bool showThinFrame;

    bool showVerticalLines;
    bool verticalLinesScroll;
    uint verticalLinesOffset;
    uint verticalLinesDistance;
    QColor verticalLinesColor;

    bool showHorizontalLines;
    uint horizontalScale;
    uint horizontalLinesCount;
    QColor horizontalLinesColor;

    Svg *svgBackground;
    QString svgFilename;

    QColor fontColor;
    QColor backgroundColor;
    QPixmap backgroundPixmap;

    QFont font;
    QString title;
    QString unit;

    QList<PlotColor> plotColors;
    QList<QList<double> > plotData;
};

SignalPlotter::SignalPlotter(QGraphicsItem *parent)
    : QGraphicsWidget(parent),
      d(new SignalPlotterPrivate)
{
    d->precision = 0;
    d->bezierCurveOffset = 0;
    d->samples = 0;
    d->verticalMin = d->verticalMax = 0.0;
    d->niceVertMin = d->niceVertMax = 0.0;
    d->niceVertRange = 0;
    d->useAutoRange = true;
    d->scaledBy = 1;
    d->showThinFrame = true;

    // Anything smaller than this does not make sense.
    setMinimumSize(QSizeF(16, 16));

    d->showVerticalLines = true;
    d->verticalLinesColor = QColor("black");
    d->verticalLinesDistance = 30;
    d->verticalLinesScroll = true;
    d->verticalLinesOffset = 0;
    d->horizontalScale = 1;

    d->showHorizontalLines = true;
    d->horizontalLinesColor = QColor("black");
    d->horizontalLinesCount = 5;

    d->showLabels = true;
    d->showTopBar = true;
    d->stackPlots = true;
    d->fillPlots = true;

    d->svgBackground = 0;
    d->backgroundColor = QColor(0, 0, 0);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

SignalPlotter::~SignalPlotter()
{
    delete d;
}

QString SignalPlotter::unit() const
{
    return d->unit;
}
void SignalPlotter::setUnit(const QString &unit)
{
    d->unit= unit;
}

void SignalPlotter::addPlot(const QColor &color)
{
    // When we add a new plot, go back and set the data for this plot to 0 for
    // all the other times. This is because it makes it easier for moveSensors.
    foreach (QList<double> data, d->plotData) {
        data.append(0);
    }
    PlotColor newColor;
    newColor.color = color;
    newColor.darkColor = color.dark(150);
    d->plotColors.append(newColor);
}

void SignalPlotter::addSample(const QList<double>& sampleBuf)
{
    if (d->samples < 4) {
        // It might be possible, under some race conditions, for addSample
        // to be called before d->samples is set. This is just to be safe.
        kDebug() << "Error - d->samples is only " << d->samples;
        updateDataBuffers();
        kDebug() << "d->samples is now " << d->samples;
        if (d->samples < 4) {
            return;
        }
    }
    d->plotData.prepend(sampleBuf);
    Q_ASSERT(sampleBuf.count() == d->plotColors.count());
    if ((uint)d->plotData.size() > d->samples) {
        d->plotData.removeLast(); // we have too many.  Remove the last item
        if ((uint)d->plotData.size() > d->samples) {
            // If we still have too many, then we have resized the widget.
            // Remove one more.  That way we will slowly resize to the new size
            d->plotData.removeLast();
        }
    }

    if (d->bezierCurveOffset >= 2) {
        d->bezierCurveOffset = 0;
    } else {
        d->bezierCurveOffset++;
    }

    Q_ASSERT((uint)d->plotData.size() >= d->bezierCurveOffset);

    // If the vertical lines are scrolling, increment the offset
    // so they move with the data.
    if (d->verticalLinesScroll) {
        d->verticalLinesOffset =
            (d->verticalLinesOffset + d->horizontalScale) % d->verticalLinesDistance;
    }
    update();
}

void SignalPlotter::reorderPlots(const QList<uint>& newOrder)
{
    if (newOrder.count() != d->plotColors.count()) {
        kDebug() << "neworder has " << newOrder.count()
                 << " and plot colors is " << d->plotColors.count();
        return;
    }
    foreach (QList<double> data, d->plotData) {
        if (newOrder.count() != data.count()) {
            kDebug() << "Serious problem in move sample.  plotdata[i] has "
                     << data.count() << " and neworder has " << newOrder.count();
        } else {
            QList<double> newPlot;
            for (int i = 0; i < newOrder.count(); i++) {
                int newIndex = newOrder[i];
                newPlot.append(data.at(newIndex));
            }
            data = newPlot;
        }
    }
    QList<PlotColor> newPlotColors;
    for (int i = 0; i < newOrder.count(); i++) {
        int newIndex = newOrder[i];
        PlotColor newColor = d->plotColors.at(newIndex);
        newPlotColors.append(newColor);
    }
    d->plotColors = newPlotColors;
}

void SignalPlotter::setVerticalRange(double min, double max)
{
    d->verticalMin = min;
    d->verticalMax = max;
    calculateNiceRange();
}

QList<PlotColor> &SignalPlotter::plotColors()
{
    return d->plotColors;
}

void SignalPlotter::removePlot(uint pos)
{
    if (pos >= (uint)d->plotColors.size()) {
        return;
    }
    d->plotColors.removeAt(pos);

    foreach (QList<double> data, d->plotData) {
        if ((uint)data.size() >= pos) {
            data.removeAt(pos);
        }
    }
}

void SignalPlotter::scale(qreal delta)
{
    if (d->scaledBy == delta) {
        return;
    }
    d->scaledBy = delta;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
    calculateNiceRange();
}

qreal SignalPlotter::scaledBy() const
{
    return d->scaledBy;
}

void SignalPlotter::setTitle(const QString &title)
{
    if (d->title == title) {
        return;
    }
    d->title = title;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

QString SignalPlotter::title() const
{
    return d->title;
}

void SignalPlotter::setUseAutoRange(bool value)
{
    d->useAutoRange = value;
    calculateNiceRange();
    // this change will be detected in paint and the image cache regenerated
}

bool SignalPlotter::useAutoRange() const
{
    return d->useAutoRange;
}

double SignalPlotter::verticalMinValue() const
{
    return d->verticalMin;
}

double SignalPlotter::verticalMaxValue() const
{
    return d->verticalMax;
}

void SignalPlotter::setHorizontalScale(uint scale)
{
    if (scale == d->horizontalScale) {
        return;
    }

    d->horizontalScale = scale;
    updateDataBuffers();
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

uint SignalPlotter::horizontalScale() const
{
    return d->horizontalScale;
}

void SignalPlotter::setShowVerticalLines(bool value)
{
    if (d->showVerticalLines == value) {
        return;
    }
    d->showVerticalLines = value;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

bool SignalPlotter::showVerticalLines() const
{
    return d->showVerticalLines;
}

void SignalPlotter::setVerticalLinesColor(const QColor &color)
{
    if (d->verticalLinesColor == color) {
        return;
    }
    d->verticalLinesColor = color;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

QColor SignalPlotter::verticalLinesColor() const
{
    return d->verticalLinesColor;
}

void SignalPlotter::setVerticalLinesDistance(uint distance)
{
    if (distance == d->verticalLinesDistance) {
        return;
    }
    d->verticalLinesDistance = distance;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

uint SignalPlotter::verticalLinesDistance() const
{
    return d->verticalLinesDistance;
}

void SignalPlotter::setVerticalLinesScroll(bool value)
{
    if (value == d->verticalLinesScroll) {
        return;
    }
    d->verticalLinesScroll = value;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

bool SignalPlotter::verticalLinesScroll() const
{
    return d->verticalLinesScroll;
}

void SignalPlotter::setShowHorizontalLines(bool value)
{
    if (value == d->showHorizontalLines) {
        return;
    }
    d->showHorizontalLines = value;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

bool SignalPlotter::showHorizontalLines() const
{
    return d->showHorizontalLines;
}

void SignalPlotter::setFontColor(const QColor &color)
{
    d->fontColor = color;
}

QColor SignalPlotter::fontColor() const
{
    return d->fontColor;
}

void SignalPlotter::setHorizontalLinesColor(const QColor &color)
{
    if (color == d->horizontalLinesColor) {
        return;
    }
    d->horizontalLinesColor = color;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

QColor SignalPlotter::horizontalLinesColor() const
{
    return d->horizontalLinesColor;
}

void SignalPlotter::setHorizontalLinesCount(uint count)
{
    if (count == d->horizontalLinesCount) {
        return;
    }
    d->horizontalLinesCount = count;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
    calculateNiceRange();
}

uint SignalPlotter::horizontalLinesCount() const
{
    return d->horizontalLinesCount;
}

void SignalPlotter::setShowLabels(bool value)
{
    if (value == d->showLabels) {
        return;
    }
    d->showLabels = value;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

bool SignalPlotter::showLabels() const
{
    return d->showLabels;
}

void SignalPlotter::setShowTopBar(bool value)
{
    if (d->showTopBar == value) {
        return;
    }
    d->showTopBar = value;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

bool SignalPlotter::showTopBar() const
{
    return d->showTopBar;
}

void SignalPlotter::setFont(const QFont &font)
{
    d->font = font;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

QFont SignalPlotter::font() const
{
    return d->font;
}

QString SignalPlotter::svgBackground()
{
    return d->svgFilename;
}

void SignalPlotter::setSvgBackground(const QString &filename)
{
    if (d->svgFilename == filename) {
        return;
    }

    if (!filename.isEmpty() && filename[0] == '/') {
        KStandardDirs *kstd = KGlobal::dirs();
        d->svgFilename = kstd->findResource("data", "ksysguard/" + filename);
    } else {
        d->svgFilename = filename;
    }

    if (!d->svgFilename.isEmpty()) {
        if (d->svgBackground) {
            delete d->svgBackground;
        }
        d->svgBackground = new Svg(this);
        d->svgBackground->setImagePath(d->svgFilename);
    }

}

void SignalPlotter::setBackgroundColor(const QColor &color)
{
    if (color == d->backgroundColor) {
        return;
    }
    d->backgroundColor = color;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

QColor SignalPlotter::backgroundColor() const
{
    return d->backgroundColor;
}

void SignalPlotter::setThinFrame(bool set)
{
    if (d->showThinFrame == set) {
        return;
    }
    d->showThinFrame = set;
    d->backgroundPixmap = QPixmap(); // we changed a paint setting, so reset the cache
}

void SignalPlotter::setStackPlots(bool stack)
{
    d->stackPlots = stack;
    d->fillPlots = stack;
}

bool SignalPlotter::stackPlots() const
{
    return d->stackPlots;
}

void SignalPlotter::updateDataBuffers()
{
    // This is called when the widget has resized
    //
    // Determine new number of samples first.
    //  +0.5 to ensure rounding up
    //  +4 for extra data points so there is
    //     1) no wasted space and
    //     2) no loss of precision when drawing the first data point.
    d->samples = static_cast<uint>(((size().width() - 2) /
                                      d->horizontalScale) + 4.5);
}

QPixmap SignalPlotter::getSnapshotImage(uint w, uint height)
{
    uint horizontalStep = (uint)((1.0 * w / size().width()) + 0.5); // get the closest integer horizontal step
    uint newWidth = (uint) (horizontalStep * size().width());
    QPixmap image = QPixmap(newWidth, height);
    QPainter p(&image);
    drawWidget(&p, newWidth, height, newWidth);
    p.end();
    return image;
}

void SignalPlotter::setGeometry(const QRectF &geometry)
{
    // First update our size, then update the data buffers accordingly.
    QGraphicsWidget::setGeometry(geometry);
    updateDataBuffers();
}

void SignalPlotter::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    uint w = (uint) size().width();
    uint h = (uint) size().height();

    // Do not do repaints when the widget is not yet setup properly.
    if (w <= 2) {
        return;
    }

    drawWidget(painter, w, h, d->horizontalScale);
}

void SignalPlotter::drawWidget(QPainter *p, uint w, uint height, int horizontalScale)
{
    uint h = height; // h will become the height of just the bit we draw the plots in
    p->setFont(d->font);

    uint fontheight = p->fontMetrics().height();
    if (d->verticalMin < d->niceVertMin ||
        d->verticalMax > d->niceVertMax ||
        d->verticalMax < (d->niceVertRange * 0.75 + d->niceVertMin) ||
        d->niceVertRange == 0) {
        calculateNiceRange();
    }
    QPen pen;
    pen.setWidth(1);
    pen.setCapStyle(Qt::RoundCap);
    p->setPen(pen);

    uint top = p->pen().width() / 2; // The y position of the top of the graph.  Basically this is one more than the height of the top bar
    h-= top;

    // Check if there's enough room to actually show a top bar.
    // Must be enough room for a bar at the top, plus horizontal
    // lines each of a size with room for a scale.
    bool showTopBar = d->showTopBar &&  h > (fontheight/*top bar size*/ +5/*smallest reasonable size for a graph*/);
    if (showTopBar) {
        top += fontheight; // The top bar has the same height as fontheight. Thus the top of the graph is at fontheight
        h -= fontheight;
    }
    if (d->backgroundPixmap.isNull() ||
        (uint)d->backgroundPixmap.size().height() != height ||
        (uint)d->backgroundPixmap.size().width() != w) {
        // recreate on resize etc
        d->backgroundPixmap = QPixmap(w, height);
        QPainter pCache(&d->backgroundPixmap);
        pCache.setRenderHint(QPainter::Antialiasing, false);
        pCache.setFont(d->font);

        drawBackground(&pCache, w, height);

        if (d->showThinFrame) {
            drawThinFrame(&pCache, w, height);
            // We have a 'frame' in the bottom and right - so subtract them from the view
            h--;
            w--;
            pCache.setClipRect(0, 0, w, height-1);
        }

        if (showTopBar) {
            int separatorX = w / 2;
            drawTopBarFrame(&pCache, separatorX, top);
        }

        // Draw scope-like grid vertical lines if it doesn't move.
        // If it does move, draw it in the dynamic part of the code.
        if (!d->verticalLinesScroll && d->showVerticalLines && w > 60) {
            drawVerticalLines(&pCache, top, w, h);
        }

        if (d->showHorizontalLines) {
            drawHorizontalLines(&pCache, top, w, h);
        }

    } else {
        if (d->showThinFrame) {
            // We have a 'frame' in the bottom and right - so subtract them from the view
            h--;
            w--;
        }
    }
    p->drawPixmap(0, 0, d->backgroundPixmap);
    p->setRenderHint(QPainter::Antialiasing, true);

    if (showTopBar) {
        int separatorX = w / 2;
        int topBarWidth = w - separatorX -2;
        drawTopBarContents(p, separatorX, topBarWidth, top -1);
    }

    p->setClipRect(0, top, w, h);
    // Draw scope-like grid vertical lines
    if (d->verticalLinesScroll && d->showVerticalLines && w > 60) {
        drawVerticalLines(p, top, w, h);
    }

    drawPlots(p, top, w, h, horizontalScale);

    if (d->showLabels && w > 60 && h > (fontheight + 1)) {
        // if there's room to draw the labels, then draw them!
        drawAxisText(p, top, h);
    }
}

void SignalPlotter::drawBackground(QPainter *p, int w, int h)
{
    p->fillRect(0, 0, w, h, d->backgroundColor);
    if (d->svgBackground) {
        d->svgBackground->resize(w, h);
        d->svgBackground->paint(p, 0, 0);
    }
}

void SignalPlotter::drawThinFrame(QPainter *p, int w, int h)
{
    // Draw white line along the bottom and the right side of the
    // widget to create a 3D like look.
    p->setPen(kapp->palette().color(QPalette::Light));
    p->drawLine(0, h - 1, w - 1, h - 1);
    p->drawLine(w - 1, 0, w - 1, h - 1);
}

void SignalPlotter::calculateNiceRange()
{
    d->niceVertRange = d->verticalMax - d->verticalMin;
    // If the range is too small we will force it to 1.0 since it
    // looks a lot nicer.
    if (d->niceVertRange < 0.000001) {
        d->niceVertRange = 1.0;
    }

    d->niceVertMin = d->verticalMin;
    if (d->verticalMin != 0.0) {
        double dim = pow(10, floor(log10(fabs(d->verticalMin)))) / 2;
        if (d->verticalMin < 0.0) {
            d->niceVertMin = dim * floor(d->verticalMin / dim);
        } else {
            d->niceVertMin = dim * ceil(d->verticalMin / dim);
        }
        d->niceVertRange = d->verticalMax - d->niceVertMin;
        if (d->niceVertRange < 0.000001) {
            d->niceVertRange = 1.0;
        }
    }
    // Massage the range so that the grid shows some nice values.
    double step = d->niceVertRange / (d->scaledBy * (d->horizontalLinesCount + 1));
    int logdim = (int)floor(log10(step));
    double dim = pow((double)10.0, logdim) / 2;
    int a = (int)ceil(step / dim);
    if (logdim >= 0) {
        d->precision = 0;
    } else if (a % 2 == 0) {
        d->precision = -logdim;
    } else {
        d->precision = 1 - logdim;
    }
    d->niceVertRange = d->scaledBy * dim * a * (d->horizontalLinesCount + 1);
    d->niceVertMax = d->niceVertMin + d->niceVertRange;
}

void SignalPlotter::drawTopBarFrame(QPainter *p, int separatorX, int height)
{
    // Draw horizontal bar with current sensor values at top of display.
    // Remember that it has a height of 'height'. Thus the lowest pixel
    // it can draw on is height-1 since we count from 0.
    p->setPen(Qt::NoPen);
    p->setPen(d->fontColor);
    p->drawText(0, 1, separatorX, height, Qt::AlignCenter, d->title);
    p->setPen(d->horizontalLinesColor);
    p->drawLine(separatorX - 1, 1, separatorX - 1, height - 1);
}

void SignalPlotter::drawTopBarContents(QPainter *p, int x, int width, int height)
{
    // The height is the height of the contents, so this will be
    // one pixel less than the height of the topbar
    double bias = -d->niceVertMin;
    double scaleFac = width / d->niceVertRange;
    // The top bar shows the current values of all the plot data.
    // This iterates through each different plot and plots the newest data for each.
    if (!d->plotData.isEmpty()) {
        QList<double> newestData = d->plotData.first();
        for (int i = newestData.count()-1; i >= 0; --i) {
            double newest_datapoint = newestData.at(i);
            int start = x + (int)(bias * scaleFac);
            int end = x + (int)((bias += newest_datapoint) * scaleFac);
            int start2 = qMin(start, end);
            end = qMax(start, end);
            start = start2;

            // If the rect is wider than 2 pixels we draw only the last
            // pixels with the bright color. The rest is painted with
            // a 50% darker color.

            p->setPen(Qt::NoPen);
            QLinearGradient  linearGrad(QPointF(start, 1), QPointF(end, 1));
            linearGrad.setColorAt(0, d->plotColors[i].darkColor);
            linearGrad.setColorAt(1, d->plotColors[i].color);
            p->fillRect(start, 1, end - start, height-1, QBrush(linearGrad));
        }
    }
}

void SignalPlotter::drawVerticalLines(QPainter *p, int top, int w, int h)
{
    p->setPen(d->verticalLinesColor);
    for (int x = d->verticalLinesOffset; x < (w - 2); x += d->verticalLinesDistance) {
        p->drawLine(w - x, top, w - x, h + top -1);
    }
}

void SignalPlotter::drawPlots(QPainter *p, int top, int w, int h, int horizontalScale)
{
    Q_ASSERT(d->niceVertRange != 0);

    if (d->niceVertRange == 0) {
        d->niceVertRange = 1;
    }
    double scaleFac = (h - 1) / d->niceVertRange;

    int xPos = 0;
    QList< QList<double> >::Iterator it = d->plotData.begin();

    p->setPen(Qt::NoPen);
    // In autoRange mode we determine the range and plot the values in
    // one go. This is more efficiently than running through the
    // buffers twice but we do react on recently discarded samples as
    // well as new samples one plot too late. So the range is not
    // correct if the recently discarded samples are larger or smaller
    // than the current extreme values. But we can probably live with
    // this.

    // These values aren't used directly anywhere.  Instead we call
    // calculateNiceRange()  which massages these values into a nicer
    // values.  Rounding etc.  This means it's safe to change these values
    // without affecting any other drawings.
    if (d->useAutoRange) {
        d->verticalMin = d->verticalMax = 0.0;
    }

    // d->bezierCurveOffset is how many points we have at the start.
    // All the bezier curves are in groups of 3, with the first of the
    // next group being the last point of the previous group

    // Example, when d->bezierCurveOffset == 0, and we have data, then just
    // plot a normal bezier curve. (we will have at least 3 points in this case)
    // When d->bezierCurveOffset == 1, then we want a bezier curve that uses
    // the first data point and the second data point.  Then the next group
    // starts from the second data point.
    //
    // When d->bezierCurveOffset == 2, then we want a bezier curve that
    // uses the first, second and third data.
    for (uint i = 0; it != d->plotData.end() && i < d->samples; ++i) {
        QPen pen;
        pen.setWidth(1);
        pen.setCapStyle(Qt::FlatCap);

        // We will plot 1 bezier curve for every 3 points, with the 4th point
        // being the end of one bezier curve and the start of the second.
        // This does means the bezier curves will not join nicely, but it
        // should be better than nothing.
        QList<double> datapoints = *it;
        QList<double> prev_datapoints = datapoints;
        QList<double> prev_prev_datapoints = datapoints;
        QList<double> prev_prev_prev_datapoints = datapoints;

        if (i == 0 && d->bezierCurveOffset > 0) {
            // We are plotting an incomplete bezier curve - we don't have
            // all the data we want. Try to cope.
            xPos += horizontalScale * d->bezierCurveOffset;
            if (d->bezierCurveOffset == 1) {
                prev_datapoints = *it;
                ++it; // Now we are on the first element of the next group, if it exists
                if (it != d->plotData.end()) {
                    prev_prev_prev_datapoints = prev_prev_datapoints = *it;
                } else {
                    prev_prev_prev_datapoints = prev_prev_datapoints = prev_datapoints;
                }
            } else {
                // d->bezierCurveOffset must be 2 now
                prev_datapoints = *it;
                Q_ASSERT(it != d->plotData.end());
                ++it;
                prev_prev_datapoints = *it;
                Q_ASSERT(it != d->plotData.end());
                ++it; // Now we are on the first element of the next group, if it exists
                if (it != d->plotData.end()) {
                    prev_prev_prev_datapoints = *it;
                } else {
                    prev_prev_prev_datapoints = prev_prev_datapoints;
                }
            }
        } else {
            // We have a group of 3 points at least.  That's 1 start point and 2 control points.
            xPos += horizontalScale * 3;
            it++;
            if (it != d->plotData.end()) {
                prev_datapoints = *it;
                it++;
                if (it != d->plotData.end()) {
                    prev_prev_datapoints = *it;
                    it++;  // We are now on the next set of data points
                    if (it != d->plotData.end()) {
                        // We have this datapoint, so use it for our finish point
                        prev_prev_prev_datapoints = *it;
                    } else {
                        // We don't have the next set, so use our last control
                        // point as our finish point
                        prev_prev_prev_datapoints = prev_prev_datapoints;
                    }
                } else {
                    prev_prev_prev_datapoints = prev_prev_datapoints = prev_datapoints;
                }
            } else {
                prev_prev_prev_datapoints = prev_prev_datapoints = prev_datapoints = datapoints;
            }
        }

        float x0 = w - xPos + 3.0 * horizontalScale;
        float x1 = w - xPos + 2.0 * horizontalScale;
        float x2 = w - xPos + 1.0 * horizontalScale;
        float x3 = w - xPos;
        float y0 = h - 1 + top;
        float y1 = y0;
        float y2 = y0;
        float y3 = y0;

        int offset = 0; // Our line is 2 pixels thick.  This means that when we draw the area, we need to offset
        double max_y = 0;
        double min_y = 0;
        for (int j = qMin(datapoints.size(), d->plotColors.size()) - 1; j >=0; --j) {
            if (d->useAutoRange) {
                // If we use autorange, then we need to prepare the min and max values for _next_ time we paint.
                // If we are stacking the plots, then we need to add the maximums together.
                double current_maxvalue =
                    qMax(datapoints[j],
                         qMax(prev_datapoints[j],
                              qMax(prev_prev_datapoints[j],
                                   prev_prev_prev_datapoints[j])));
                double current_minvalue =
                    qMin(datapoints[j],
                         qMin(prev_datapoints[j],
                              qMin(prev_prev_datapoints[j],
                                   prev_prev_prev_datapoints[j])));
                d->verticalMax = qMax(d->verticalMax, current_maxvalue);
                d->verticalMin = qMin(d->verticalMin, current_maxvalue);
                if (d->stackPlots) {
                    max_y += current_maxvalue;
                    min_y += current_minvalue;
                }
            }

            // Draw polygon only if enough data points are available.
            if (j < prev_prev_prev_datapoints.count() &&
                 j < prev_prev_datapoints.count() &&
                 j < prev_datapoints.count()) {

                // The height of the whole widget is h+top->  The height of
                // the area we are plotting in is just h.
                // The y coordinate system starts from the top, so at the
                // bottom the y coordinate is h+top.
                // So to draw a point at value y', we need to put this at  h+top-y'
                float delta_y0;
                delta_y0 = (datapoints[j] - d->niceVertMin) * scaleFac;

                float delta_y1;
                delta_y1 = (prev_datapoints[j] - d->niceVertMin) * scaleFac;

                float delta_y2;
                delta_y2 = (prev_prev_datapoints[j] - d->niceVertMin) * scaleFac;

                float delta_y3;
                delta_y3 = (prev_prev_prev_datapoints[j] - d->niceVertMin) * scaleFac;

                QPainterPath path;
                if (d->stackPlots && offset) {
                    // we don't want the lines to overdraw each other.
                    // This isn't a great solution though :(
                    if (delta_y0 < 3) {
                        delta_y0=3;
                    }
                    if (delta_y1 < 3) {
                        delta_y1=3;
                    }
                    if (delta_y2 < 3) {
                        delta_y2=3;
                    }
                    if (delta_y3 < 3) {
                        delta_y3=3;
                    }
                }
                path.moveTo(x0, y0 - delta_y0);
                path.cubicTo(x1, y1 - delta_y1, x2, y2 - delta_y2, x3, y3 - delta_y3);

                if (d->fillPlots) {
                    QPainterPath path2(path);
                    QLinearGradient myGradient(0,(h - 1 + top), 0, (h - 1 + top) / 5);
                    Q_ASSERT(d->plotColors.size() >= j);
                    QColor c0(d->plotColors[j].darkColor);
                    QColor c1(d->plotColors[j].color);
                    c0.setAlpha(150);
                    c1.setAlpha(150);
                    myGradient.setColorAt(0, c0);
                    myGradient.setColorAt(1, c1);

                    path2.lineTo(x3, y3 - offset);
                    if (d->stackPlots) {
                        // offset is set to 1 after the first plot is drawn,
                        // so we don't trample on top of the 2pt thick line
                        path2.cubicTo(x2, y2 - offset, x1, y1 - offset, x0, y0 - offset);
                    } else {
                        path2.lineTo(x0, y0 - 1);
                    }
                    p->setBrush(myGradient);
                    p->setPen(Qt::NoPen);
                    p->drawPath(path2);
                }
                p->setBrush(Qt::NoBrush);
                Q_ASSERT(d->plotColors.size() >= j);
                pen.setColor(d->plotColors[j].color);
                p->setPen(pen);
                p->drawPath(path);

                if (d->stackPlots) {
                    // We can draw the plots stacked on top of each other.
                    // This means that say plot 0 has the value 2 and plot
                    // 1 has the value 3, then we plot plot 0 at 2 and plot 1 at 2+3 = 5.
                    y0 -= delta_y0;
                    y1 -= delta_y1;
                    y2 -= delta_y2;
                    y3 -= delta_y3;
                    offset = 1;  // see the comment further up for int offset;
                }
            }
            if (d->useAutoRange && d->stackPlots) {
                d->verticalMax = qMax(max_y, d->verticalMax);
                d->verticalMin = qMin(min_y, d->verticalMin);
            }
        }
    }
}

void SignalPlotter::drawAxisText(QPainter *p, int top, int h)
{
    // Draw horizontal lines and values. Lines are always drawn.
    // Values are only draw when width is greater than 60.
    QString val;

    // top = 0 or font.height depending on whether there's a topbar or not
    // h = graphing area.height - i.e. the actual space we have to draw inside
    // Note we are drawing from 0,0 as the top left corner. So we have to add on top
    // to get to the top of where we are drawing so top+h is the height of the widget.
    p->setPen(d->fontColor);
    double stepsize = d->niceVertRange / (d->scaledBy * (d->horizontalLinesCount + 1));
    int step =
        (int)ceil((d->horizontalLinesCount+1) *
                  (p->fontMetrics().height() + p->fontMetrics().leading() / 2.0) / h);
    if (step == 0) {
        step = 1;
    }
    for (int y = d->horizontalLinesCount + 1; y >= 1; y-= step) {
        int y_coord =
            top + (y * (h - 1)) / (d->horizontalLinesCount + 1); // Make sure it's y*h first to avoid rounding bugs
        if (y_coord - p->fontMetrics().ascent() < top) {
            // at most, only allow 4 pixels of the text to be covered up
            // by the top bar. Otherwise just don't bother to draw it
            continue;
        }
        double value;
        if ((uint)y == d->horizontalLinesCount + 1) {
            value = d->niceVertMin; // sometimes using the formulas gives us a value very slightly off
        } else {
            value = d->niceVertMax / d->scaledBy - y * stepsize;
        }

        QString number = KGlobal::locale()->formatNumber(value, d->precision);
        val = QString("%1 %2").arg(number, d->unit);
        p->drawText(6, y_coord - 3, val);
    }
}

void SignalPlotter::drawHorizontalLines(QPainter *p, int top, int w, int h)
{
    p->setPen(d->horizontalLinesColor);
    for (uint y = 0; y <= d->horizontalLinesCount + 1; y++) {
        // note that the y_coord starts from 0.  so we draw from pixel number 0 to h-1.  Thus the -1 in the y_coord
        int y_coord =  top + (y * (h - 1)) / (d->horizontalLinesCount + 1);  // Make sure it's y*h first to avoid rounding bugs
        p->drawLine(0, y_coord, w - 2, y_coord);
    }
}

double SignalPlotter::lastValue(uint i) const
{
    if (d->plotData.isEmpty() || d->plotData.first().size() <= (int)i) {
        return 0;
    }
    return d->plotData.first()[i];
}

QString SignalPlotter::lastValueAsString(uint i) const
{
    if (d->plotData.isEmpty()) {
        return QString();
    }
    double value = d->plotData.first()[i] / d->scaledBy; // retrieve the newest value for this plot then scale it correct
    QString number = KGlobal::locale()->formatNumber(value, (value >= 100)?0:2);
    return QString("%1 %2").arg(number, d->unit);
}

} // Plasma namespace
