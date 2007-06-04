/***************************************************************************
begin                : 2004/02/07
copyright            : (C) Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
                      const QString &header = QString(), bool manage=true);
        void showPageByName( const QByteArray& page );

    protected slots:
        void updateButtons();
        void updateSettings();
        void updateWidgets();
        void updateWidgetsDefault();

    private slots:

    protected:
        bool hasChanged();
        bool isDefault();

    private:
        QList<ConfigDialogBase*> m_pageList;
};


#endif // AMAROK2CONFIGDIALOG_H
