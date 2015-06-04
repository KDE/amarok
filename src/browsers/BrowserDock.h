/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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
 
#ifndef BROWSERWIDGET_H
#define BROWSERWIDGET_H

#include "BrowserBreadcrumbWidget.h"
#include "BrowserCategoryList.h"
#include "BrowserMessageArea.h"
#include "widgets/AmarokDockWidget.h"

#include <KVBox>

#include <QWeakPointer>

/**
The base widget that contains all other browsers, organized in a dig down interface
*/
class BrowserDock : public AmarokDockWidget
{
    Q_OBJECT

public:
    BrowserDock( QWidget *parent );

    ~BrowserDock();

    BrowserCategoryList *list() const;
    void navigate( const QString &target );
    void polish();

private Q_SLOTS:
    void home();
    void paletteChanged( const QPalette &palette );

private:
    BrowserBreadcrumbWidget *m_breadcrumbWidget;
    QWeakPointer<BrowserCategoryList> m_categoryList;
    KVBox *m_mainWidget;
    BrowserMessageArea *m_messageArea;
};

#endif
