/****************************************************************************************
 * Copyright (c) 2010 Rainer Sigle <rainer.sigle@web.de>                                *
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

#ifndef TABS_APPLET_H
#define TABS_APPLET_H

// Amarok
#include "context/engines/tabs/TabsInfo.h"
#include "context/Applet.h"
#include "context/DataEngine.h"

#include <ui_TabsSettings.h>
#include <ui_ReloadEditDialog.h>

// Qt
#include <QGraphicsLinearLayout>
#include <QWeakPointer>

class TabsView;

namespace Plasma
{
    class IconWidget;
}

class KConfigDialog;


/** Amarok context view applet to display tab-related information
 * for the currently playing song
 */
class TabsApplet : public Context::Applet
{
    Q_OBJECT
    public:
        TabsApplet( QObject* parent, const QVariantList& args );
        ~TabsApplet();

    public Q_SLOTS:

        virtual void init();
        void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

        /**
         * Saves the settings from the configuration dialog
         */
        void saveSettings();

    protected:
        void createConfigurationInterface( KConfigDialog *parent );

    private Q_SLOTS:
        void reloadTabs();
        void stopped();

    private:
        TabsView *m_tabsView;

        enum AppletState { InitState, StoppedState, FetchingState, TabState, NoTabsState };
        AppletState m_currentState;
        void updateInterface( const AppletState appletState );

        /**
         * Layout for the formatting of the applet contents
         */
        QGraphicsLinearLayout *m_layout;
        QWeakPointer<Plasma::IconWidget>  m_reloadIcon;

        bool m_fetchGuitar;
        bool m_fetchBass;

        bool m_showTabBrowser;
        Ui::TabsSettings ui_Settings;
};

AMAROK_EXPORT_APPLET( tabs, TabsApplet )
Q_DECLARE_METATYPE ( TabsInfo *)

#endif /* Tabs_APPLET_H */
