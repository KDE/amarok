/****************************************************************************************
 * Copyright (c) 2004-2007 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2004 Frederik Holljen <fh@ez.no>                                       *
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

#ifndef NOTIFICATIONSCONFIG_H
#define NOTIFICATIONSCONFIG_H

#include "ui_NotificationsConfig.h"
#include "configdialog/ConfigDialogBase.h"
#include "widgets/Osd.h"

class OSDPreviewWidget;

class NotificationsConfig : public ConfigDialogBase, public Ui_NotificationsConfig
{
    Q_OBJECT

    public:
        explicit NotificationsConfig( Amarok2ConfigDialog* parent );
        virtual ~NotificationsConfig();

        bool hasChanged() override;
        bool isDefault() override;
        void updateSettings() override;

    Q_SIGNALS:
        void changed();

    private Q_SLOTS:
        void slotPositionChanged();
        void useCustomColorsToggled( bool );
        void setGrowlEnabled( bool );

    private:
        OSDPreviewWidget* m_osdPreview;
        
        OSDWidget::Alignment m_oldAlignment;
        uint m_oldYOffset;

        void hideEvent( QHideEvent* ) override;
        void showEvent( QShowEvent* ) override;
};

#endif
