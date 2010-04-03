/****************************************************************************************
 * Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
 * Copyright (c) 2009-2010 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                *
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

#ifndef UPCOMING_EVENTS_APPLET_H
#define UPCOMING_EVENTS_APPLET_H

// Includes
#include "context/Applet.h"
#include "context/DataEngine.h"
#include "UpcomingEventsWidget.h"
#include <ui_upcomingEventsSettings.h>

class TextScrollingWidget;

// Qt
class QAction;
class QGraphicsLayoutItem;
class QGraphicsSimpleTextItem;
class QGraphicsTextItem;
class QVBoxLayout;
class DropPixmapItem;
class QLabel;

// KDE
class KConfigDialog;

namespace Plasma
{
    class IconWidget;
}


 /**
  * \class UpcomingEventsApplet UpcomingEventsApplet.h UpcomingEventsApplet.cpp
  * \brief The base class of the Upcoming Events applet.
  * 
  * UpcomingEventsApplet displays the upcoming events of the current artist from LastFm.
  */
class UpcomingEventsApplet : public Context::Applet
{
    Q_OBJECT

public:
    /**
     * \brief Constructor
     *
     * UpcomingEventsApplet constructor
     *
     * \param parent : the UpcomingEventsApplet parent (used by Context::Applet)
     * \param args : (used by Context::Applet)
     */
    UpcomingEventsApplet( QObject* parent, const QVariantList& args );

    /**
     * \brief Initialization
     *
     * Initializes the UpcomingEventsApplet with default parameters
     */
    void init();

    /**
     * \brief Paints the interface
     *
     * This method is called when the interface should be painted
     *
     * \param painter : the QPainter to use to do the paintiner
     * \param option : the style options object
     * \param contentsRect : the rect to paint within; automatically adjusted for
     *                     the background, if any
     */
    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );

    /**
     * Called when any of the geometry constraints have been updated.
     *
     * This is always called prior to painting and should be used as an
     * opportunity to layout the widget, calculate sizings, etc.
     *
     * Do not call update() from this method; an update() will be triggered
     * at the appropriate time for the applet.
     *
     * \param constraints : the type of constraints that were updated
     */
    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

protected:
    /**
     * Reimplement this method so provide a configuration interface,
     * parented to the supplied widget. Ownership of the widgets is passed
     * to the parent widget.
     *
     * \param parent : the dialog which is the parent of the configuration
     *               widgets
     */
    void createConfigurationInterface(KConfigDialog *parent);

public slots:
    /**
     * Updates the data from the Upcoming Events engine
     *
     * \param name : the name
     * \param data : the engine from where the data are received
     */
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:
    /**
     * Title of the applet (in the top bar)
     */
    TextScrollingWidget* m_headerLabel;

    /**
     * The icon for the Upcoming Events applet
     */
    Plasma::IconWidget *m_settingsIcon;

    /**
     * The GUI of the settings window
     */
    Ui::upcomingEventsSettings ui_Settings;

    /**
     * The possible values are "ThisWeek", "ThisMonth", "ThisYear" or "AllEvents"
     */
    QString m_timeSpan;

    /**
     * Enables the link to the event home page if true, disables else
     */
    bool m_enabledLinks;

    /**
     * The temporary m_timeSpan value, used to store or not the user choice
     */
    QString m_temp_timeSpan;

    /**
     * The temporary m_enabledLinks value, used to store or not the user choice
     */
    bool m_temp_enabledLinks;

    /**
     * All the widgets added in the applet
     */
    QList< UpcomingEventsWidget * > m_widgets;

    /**
     * The layout used to organize the widgets
     */
    QVBoxLayout * m_mainLayout;

    /**
     * The scroll area is used as an embedded widget to be added in the applet
     */
    QGraphicsProxyWidget * m_scrollProxy;

    /**
     * A scroll area in order to add scroll bars
     */
    QScrollArea * m_scroll;
    
private slots:
    /**
     * Connects the source to the Upcoming Events engine
     * and calls the dataUpdated function
     */
    void connectSource( const QString &source );

    /**
     * Show the settings windows
     */
    void configure();

    /**
     * Replace the former time span by the new one
     */
    void changeTimeSpan(QString span);

    /**
     * Sets the upcoming events as links
     */
    void setAddressAsLink(int state);

    /**
     * Save the time span choosen by the user
     */
    void saveTimeSpan();

    /**
     * Displays all the upcoming events addresses as links
     */
    void saveAddressAsLink();

    /**
     * Save all the upcoming events settings
     */
    void saveSettings();
};

K_EXPORT_AMAROK_APPLET( upcomingEvents, UpcomingEventsApplet )

#endif // UPCOMINGEVENTSAPPLET_H
