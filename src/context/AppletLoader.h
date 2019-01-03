/****************************************************************************************
 * Copyright (c) 2017 Malte Veerman <malte.veerman@gmail.com>                             *
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

#ifndef APPLETLOADER_H
#define APPLETLOADER_H

#include <QObject>
#include <QList>

class KPluginMetaData;

namespace Context
{

class AppletLoader : public QObject
{
    Q_OBJECT

public:
    explicit AppletLoader(QObject *parent = nullptr);
    ~AppletLoader();

    QList<KPluginMetaData> applets() const;
    QList<KPluginMetaData> enabledApplets() const;
    void findApplets();

signals:
    void finished(QList<KPluginMetaData>);

private:
    QList<KPluginMetaData> m_applets;
};

}

#endif // APPLETLOADER_H
