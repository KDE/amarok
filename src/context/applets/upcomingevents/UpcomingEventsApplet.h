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
#include "network/NetworkAccessManagerProxy.h"
#include "UpcomingEventsWidget.h"
#include "ui_upcomingEventsGeneralSettings.h"
#include "ui_upcomingEventsVenueSettings.h"

class KConfigDialog;
class QGraphicsLinearLayout;
class QListWidgetItem;
class QXmlStreamReader;
class TextScrollingWidget;

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
     * \brief Initialization
     *
     * Initializes the UpcomingEventsApplet with default parameters
     */
    virtual void init();

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
    TextScrollingWidget *m_headerLabel;

    /**
     * The icon for the Upcoming Events applet
     */
    Plasma::IconWidget *m_settingsIcon;

    /**
     * The UI of the general settings page
     */
    Ui::upcomingEventsGeneralSettings ui_GeneralSettings;

    /**
     * The UI of the sticky venue settings page
     */
    Ui::upcomingEventsVenueSettings ui_VenueSettings;

    /**
     * The possible values are "ThisWeek", "ThisMonth", "ThisYear" or "AllEvents"
     */
    QString m_timeSpan;

    /**
     * The temporary m_timeSpan value, used to store or not the user choice
     */
    QString m_temp_timeSpan;

    /**
     * The scroll area is used as an embedded widget to be added in the applet
     */
    Plasma::ExtenderItem *m_scrollWidget;

    /**
     * The vertical layout for presenting the list of events
     */
    QGraphicsLinearLayout *m_layout;

private slots:
    /**
     * Connects the source to the Upcoming Events engine
     * and calls the dataUpdated function
     */
    void engineSourceAdded( const QString &source );

    /**
     * Show the settings windows
     */
    void configure();

    /**
     * Replace the former time span by the new one
     */
    void changeTimeSpan( const QString &span );

    /**
     * Save the time span choosen by the user
     */
    void saveTimeSpan();

    /**
     * Save all the upcoming events settings
     */
    void saveSettings();

private:
    enum VenueItemRoles
    {
        VenueIdRole = Qt::UserRole,
        VenueNameRole,
        VenueCityRole,
        VenueCountryRole,
        VenueStreetRole,
        VenuePhotoUrlRole,
        VenueUrlRole,
        VenueWebsiteRole
    };

    struct VenueData
    {
        int id;
        QString name;
        QString city;
    };

    void readVenueLocation( QXmlStreamReader &xml, QListWidgetItem *item );
    QList<VenueData> venueStringToDataList( const QStringList &list );
    QList<VenueData> m_favoriteVenues;

private slots:
    void searchVenue( const QString &text );
    void venueResults( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
    void venuePhotoResult( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
    void showVenueInfo( QListWidgetItem *item );
    void venueResultDoubleClicked( QListWidgetItem *item );
    void selectedVenueDoubleClicked( QListWidgetItem *item );
    void openUrl( const QString &url );
};

K_EXPORT_AMAROK_APPLET( upcomingEvents, UpcomingEventsApplet )

#endif // UPCOMINGEVENTSAPPLET_H
