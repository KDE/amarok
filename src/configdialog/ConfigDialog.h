/****************************************************************************************
 * Copyright (c) 2004-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK2CONFIGDIALOG_H
#define AMAROK2CONFIGDIALOG_H

#include "ConfigDialogBase.h"

#include <kconfigdialog.h>


class Amarok2ConfigDialog : public KConfigDialog
{
    Q_OBJECT

    public:
        Amarok2ConfigDialog( QWidget *parent, const char* name, KConfigSkeleton *config );
        ~Amarok2ConfigDialog();

        void addPage( ConfigDialogBase *page, const QString &itemName, const QString &pixmapName,
                      const QString &header = QString(), bool manage = true );

    public slots:
        /**
         * Shows the config dialog and sets the current page to @page (class name of the dialog).
         */
        void show( QString page );

        /**
         * Updates the state of the Apply button. Useful for widgets that are not managed by KConfigXT.
         */
        void updateButtons();

    protected slots:
        void updateSettings();
        void updateWidgets();
        void updateWidgetsDefault();

    private slots:

    protected:
        bool hasChanged();
        bool isDefault();

    private:
        QList<ConfigDialogBase*> m_pageList;
        QMap<ConfigDialogBase*, KPageWidgetItem*> m_pageMap;

        static QString s_currentPage;
};


#endif // AMAROK2CONFIGDIALOG_H
