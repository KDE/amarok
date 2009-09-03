/****************************************************************************************
 * Copyright (c) 2007 Richard J. Moore <rich@kde.org>                                   *
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

#ifndef PLASMA_UILOADER_H
#define PLASMA_UILOADER_H

#include <KSharedPtr>

#include <plasma/applet.h>

class QGraphicsWidget;

class UiLoader : public QSharedData
{
public:
    UiLoader();
    virtual ~UiLoader();

    QStringList availableWidgets() const;
    QGraphicsWidget *createWidget(const QString &className, QGraphicsWidget *parent = 0);

private:
    typedef QGraphicsWidget *(*widgetCreator)(QGraphicsWidget*);
    QHash<QString, widgetCreator> m_widgetCtors;
};


#endif // PLASMA_UILOADER_H
