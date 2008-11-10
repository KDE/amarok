/*
 *   KSysGuard, the KDE System Guard
 *
 *   Copyright 1999 - 2001 Chris Schlaeger <cs@kde.org>
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

#ifndef PLASMA_SIGNALPLOTTER_H
#define PLASMA_SIGNALPLOTTER_H

#include <QtGui/QFont>
#include <QtGui/QGraphicsWidget>
#include <plasma/plasma_export.h>

namespace Plasma
{

class SignalPlotterPrivate;

struct PlotColor
{
    QColor color;
    QColor darkColor;
};

/**
 * @class SignalPlotter plasma/widgets/signalplotter.h <Plasma/Widgets/SignalPlotter>
 *
 * @short Provides a signal plotter for plasma.
 */
class PLASMA_EXPORT SignalPlotter : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString unit READ unit WRITE setUnit)
    Q_PROPERTY(qreal scale READ scaledBy WRITE scale) // Note: The naming of the functions here is poor
    Q_PROPERTY(bool useAutoRange READ useAutoRange WRITE setUseAutoRange)
    Q_PROPERTY(uint horizontalScale READ horizontalScale WRITE setHorizontalScale)
    Q_PROPERTY(bool showVerticalLines READ showVerticalLines WRITE setShowVerticalLines)
    Q_PROPERTY(QColor verticalLinesColor READ verticalLinesColor WRITE setVerticalLinesColor)
    Q_PROPERTY(uint verticalLinesDistance READ verticalLinesDistance WRITE setVerticalLinesDistance)
    Q_PROPERTY(bool verticalLinesScroll READ verticalLinesScroll WRITE setVerticalLinesScroll)
    Q_PROPERTY(bool showHorizontalLines READ showHorizontalLines WRITE setShowHorizontalLines)
    Q_PROPERTY(QColor horizontalLinesColor READ horizontalLinesColor WRITE setHorizontalLinesColor)
    Q_PROPERTY(QColor fontColor READ fontColor WRITE setFontColor)
    Q_PROPERTY(QFont font READ font WRITE setFont)
    Q_PROPERTY(uint horizontalLinesCount READ horizontalLinesCount WRITE setHorizontalLinesCount)
    Q_PROPERTY(bool showLabels READ showLabels WRITE setShowLabels)
    Q_PROPERTY(bool showTopBar READ showTopBar WRITE setShowTopBar)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
    Q_PROPERTY(QString svgBackground READ svgBackground WRITE setSvgBackground)
    Q_PROPERTY(bool thinFrame WRITE setThinFrame)
    Q_PROPERTY(bool stackPlots READ stackPlots WRITE setStackPlots)

public:
    SignalPlotter(QGraphicsItem *parent = 0);
    ~SignalPlotter();

    /**
     * Add a new line to the graph plotter, with the specified color.
     * Note that the order you add the plots must be the same order that
     * the same data is given in (unless you reorder the plots).
     * @param color the color to use for this plot
     */
    void addPlot(const QColor &color);

    /**
     * Add data to the graph, and advance the graph by one time period.
     * The data must be given as a list in the same order that the plots were
     * added (or consequently reordered).
     * @param samples a list with the new value for each plot
     */
    void addSample(const QList<double> &samples);

    /**
     * Reorder the plots into the order given.  For example:
     * \code
     *  KSignalPlotter *s = KSignalPlotter(parent);
     *  s->addPlot(Qt::Blue);
     *  s->addPlot(Qt::Green);
     *  QList neworder;
     *  neworder << 1 << 0;
     *  reorderPlots(newOrder);
     *  //Now the order is Green then Blue
     * \endcode
     * @param newOrder a list with the new position of each plot
     */
    void reorderPlots(const QList<uint>& newOrder);

    /**
     * Removes the plot at the specified index.
     * @param pos the index of the plot to be removed
     */
    void removePlot(uint pos);

    /**
     * Return the list of plot colors, in the order that the plots
     * were added (or later reordered).
     * @return a list containing the color of every plot
     */
    QList<PlotColor> &plotColors();

    /**
     * Set the title of the graph. Drawn in the top left.
     * @param title the title to use in the plotter
     */
    void setTitle(const QString &title);

    /**
     * Get the title of the graph.  Drawn in the top left.
     * @return the title to use in the plotter
     */
    QString title() const;

    /**
     *  Set the units.  Drawn on the vertical axis of the graph.
     *  Must be already translated into the local language.
     *  @param unit the unit string to use
     */
    void setUnit(const QString &unit);

    /**
     * Return the units used on the vertical axis of the graph.
     * @return the unit string used
     */
    QString unit() const;

    /**
     *  Scale all the values down by the given amount. This is useful
     *  when the data is given in, say, kilobytes, but you set the
     *  units as megabytes.  Thus you would have to call this with @p value
     *  set to 1024. This affects all the data already entered.
     *  @param delta the factor used to scale down the values
     */
    void scale(qreal delta);

    /**
     * Amount scaled down by. @see scale
     * @return the factor used to scale down the values
     */
    qreal scaledBy() const;

    /**
     * Set the minimum and maximum values on the vertical axis
     * automatically from the data available.
     * @param value true if the plotter should calculate its own
     * min and max values, otherwise false
     */
    void setUseAutoRange(bool value);

    /**
     * Whether the vertical axis range is set automatically.
     * @return true if the plotter calculates its own min and max
     * values, otherwise false
     */
    bool useAutoRange() const;

    /**
     * Change the minimum and maximum values drawn on the graph.
     * Note that these values are sanitised.  For example, if you
     * set the minimum as 3, and the maximum as 97, then the graph
     * would be drawn between 0 and 100.  The algorithm to determine
     * this "nice range" attempts to minimize the number of non-zero
     * digits.
     *
     * Use setAutoRange instead to determine the range automatically
     * from the data.
     * @param min the minimum value to use for the vertical axis
     * @param max the maximum value to use for the vertical axis
     */
    void setVerticalRange(double min, double max);

    /**
     * Get the min value of the vertical axis.  @see changeRange
     * @return the minimum value to use for the vertical axis
     */
    double verticalMinValue() const;

    /**
     * Get the max value of the vertical axis.  @see changeRange
     * @return the maximum value to use for the vertical axis
     */
    double verticalMaxValue() const;

    /**
     * Set the number of pixels horizontally between data points
     * @param scale the number of pixel to draw between two data points
     */
    void setHorizontalScale(uint scale);

    /**
     * The number of pixels horizontally between data points
     * @return the number of pixel drawn between two data points
     */
    uint horizontalScale() const;

    /**
     * Whether to draw the vertical grid lines
     * @param value true if the lines should be drawn, otherwise false
     */
    void setShowVerticalLines(bool value);

    /**
     * Whether the vertical grid lines will be drawn
     * @return true if the lines will be drawn, otherwise false
     */
    bool showVerticalLines() const;

    /**
     * The color of the vertical grid lines
     * @param color the color used to draw the vertical grid lines
     */
    void setVerticalLinesColor(const QColor &color);

    /**
     * The color of the vertical grid lines
     * @return the color used to draw the vertical grid lines
     */
    QColor verticalLinesColor() const;

    /**
     * The horizontal distance between the vertical grid lines
     * @param distance the distance between two vertical grid lines
     */
    void setVerticalLinesDistance(uint distance);

    /**
     * The horizontal distance between the vertical grid lines
     * @return the distance between two vertical grid lines
     */
    uint verticalLinesDistance() const;

    /**
     * Whether the vertical lines move with the data
     * @param value true if the vertical lines should move with the data
     */
    void setVerticalLinesScroll(bool value);

    /**
     * Whether the vertical lines move with the data
     * @return true if the vertical lines will move with the data
     */
    bool verticalLinesScroll() const;

    /**
     * Whether to draw the horizontal grid lines
     * @param value true if the lines should be drawn, otherwise false
     */
    void setShowHorizontalLines(bool value);
    /**
     * Whether to draw the horizontal grid lines
     * @return true if the lines will be drawn, otherwise false
     */
    bool showHorizontalLines() const;

    /**
     * The color of the horizontal grid lines
     * @param color the color used to draw the horizontal grid lines
     */
    void setHorizontalLinesColor(const QColor &color);

    /**
     * The color of the horizontal grid lines
     * @return the color used to draw the horizontal grid lines
     */
    QColor horizontalLinesColor() const;

    /**
     * The color of the font used for the axis
     * @param color the color used to draw the text of the vertical values
     */
    void setFontColor(const QColor &color);

    /**
     * The color of the font used for the axis
     * @return the color used to draw the text of the vertical values
     */
    QColor fontColor() const;

    /**
     * The font used for the axis
     * @param font the font used to draw the text of the vertical values
     */
    void setFont(const QFont &font);

    /**
     * The font used for the axis
     * @return the font used to draw the text of the vertical values
     */
    QFont font() const;

    /**
     * The number of horizontal lines to draw.  Doesn't include the top
     * most and bottom most lines.
     * @param count the number of horizontal lines to draw
     */
    void setHorizontalLinesCount(uint count);

    /**
     * The number of horizontal lines to draw.  Doesn't include the top
     * most and bottom most lines.
     * @return the number of horizontal lines that will be drawn
     */
    uint horizontalLinesCount() const;

    /**
     * Whether to show the vertical axis labels
     * @param value true if the values for the vertical axis should get drawn
     */
    void setShowLabels(bool value);

    /**
     * Whether to show the vertical axis labels
     * @return true if the values for the vertical axis will get drawn
     */
    bool showLabels() const;

    /**
     * Whether to show the title etc at the top.  Even if set, it
     * won't be shown if there isn't room
     * @param value true if the topbar should be shown
     */
    void setShowTopBar(bool value);

    /**
     * Whether to show the title etc at the top.  Even if set, it
     * won't be shown if there isn't room
     * @return true if the topbar will be shown
     */
    bool showTopBar() const;

    /**
     * The color to set the background.  This might not be seen
     * if an svg is also set.
     * @param color the color to use for the plotter background
     */
    void setBackgroundColor(const QColor &color);

    /**
     * The color to set the background.  This might not be seen
     * if an svg is also set.
     * @return the color of the plotter background
     */
    QColor backgroundColor() const;

    /**
     * The filename of the svg background.  Set to empty to disable
     * again.
     * @param filename the SVG file to use as a background image
     */
    void setSvgBackground(const QString &filename);

    /**
     * The filename of the svg background.  Set to empty to disable
     * again.
     * @return the file used as a background image
     */
    QString svgBackground();

    /**
     * Return the last value that we have for plot i. Returns 0 if not known.
     * @param i the plot we like to have the last value from
     * @return the last value of this plot or 0 if not found
     */
    double lastValue(uint i) const;

    /**
     * Return a translated string like: "34 %" or "100 KB" for plot i
     * @param i the plot we like to have the value as string from
     * @return the last value of this plot as a string
     */
    QString lastValueAsString(uint i) const;

    /**
     * Whether to show a white line on the left and bottom of the widget,
     * for a 3D effect
     * @param set true if the frame should get drawn
     */
    void setThinFrame(bool set);

    /**
     * Whether to stack the plots on top of each other.  The first plot
     * added will be at the bottom.  The next plot will be drawn on top,
     * and so on.
     * @param stack true if the plots should be stacked
     */
    void setStackPlots(bool stack);

    /**
     * Whether to stack the plots.  @see setStackPlots
     * @return true if the plots will be stacked
     */
    bool stackPlots() const;

    /**
     * Render the graph to the specified width and height, and return it
     * as an image.  This is useful, for example, if you draw a small version
     * of the graph, but then want to show a large version in a tooltip etc
     * @param width the width of the snapshot
     * @param height the height of the snapshot
     * @return a snapshot of the plotter as an image
     */
    QPixmap getSnapshotImage(uint width, uint height);

    /**
     * Overwritten to be notified of size changes. Needed to update the
     * data buffers that are used to store the samples.
     */
    virtual void setGeometry(const QRectF &geometry);

protected:
    void updateDataBuffers();
    void calculateNiceRange();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    void drawWidget(QPainter *p, uint w, uint height, int horizontalScale);
    void drawBackground(QPainter *p, int w, int h);
    void drawThinFrame(QPainter *p, int w, int h);
    void drawTopBarFrame(QPainter *p, int separatorX, int height);
    void drawTopBarContents(QPainter *p, int x, int width, int height);
    void drawVerticalLines(QPainter *p, int top, int w, int h);
    void drawPlots(QPainter *p, int top, int w, int h, int horizontalScale);
    void drawAxisText(QPainter *p, int top, int h);
    void drawHorizontalLines(QPainter *p, int top, int w, int h);

private:
    SignalPlotterPrivate *const d;
};

} // Plasma namespace

#endif
