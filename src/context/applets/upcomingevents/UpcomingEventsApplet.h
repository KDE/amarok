/****************************************************************************************
 * Copyright (c) 2009 Joffrey Clavel <jclavel@clabert.info>                             *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
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

#include "context/Applet.h"
#include "context/DataEngine.h"



// for use when gui created
#include <ui_upcomingEventsSettings.h>

class QAction;
class QGraphicsSimpleTextItem;
class QGraphicsTextItem;
class TextScrollingWidget;
class KConfigDialog;
class DropPixmapItem;

namespace Plasma
{    
    class IconWidget;
}


 /**
  * UpcomingEventsApplet will display events from the Internet, relative to the current playing song.
  * @author Joffrey Clavel
  * @version 0.1
  */
class UpcomingEventsApplet : public Context::Applet
{
    Q_OBJECT

public:
    UpcomingEventsApplet( QObject* parent, const QVariantList& args );
    
    void init();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );
    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    bool hasHeightForWidth() const;
    qreal heightForWidth( qreal width ) const;

protected:
    void createConfigurationInterface(KConfigDialog *parent);

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:
    qreal m_aspectRatio;
    qreal m_headerAspectRatio;
    QSizeF m_size;

    /**
     * Title of the applet (in the top bar)
     */
    QGraphicsSimpleTextItem* m_headerLabel; 

    Plasma::IconWidget *m_settingsIcon;
    Ui::upcomingEventsSettings ui_Settings;

    QString m_timeSpan;
    bool m_enabledLinks;
    TextScrollingWidget* m_eventParticipants;
    DropPixmapItem* m_bigImage;
    QGraphicsSimpleTextItem* m_eventName;
    QGraphicsSimpleTextItem* m_eventDate;
    QGraphicsTextItem* m_url;
    
private slots:
    void connectSource( const QString &source );

    /**
     * Show the settings windows
     */
    void configure();
    void changeTimeSpan(QString span);
    void setAddressAsLink(int state);

};

K_EXPORT_AMAROK_APPLET( upcomingEvents, UpcomingEventsApplet )

#endif // UPCOMINGEVENTSAPPLET_H