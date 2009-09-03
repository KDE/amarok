/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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
 
#ifndef SCRIPTSELECTOR_H
#define SCRIPTSELECTOR_H

#include <KCategorizedView>
#include <KPluginSelector>
#include <ksharedconfig.h>
#include <KPluginInfo>

class ScriptSelector : public KPluginSelector
{
    Q_OBJECT

    public:
        ScriptSelector( QWidget * parent );

        ~ScriptSelector();
        QString currentItem();

        void addScripts(const QList<KPluginInfo> &pluginInfoList,
                     PluginLoadMethod pluginLoadMethod = ReadConfigFile,
                     const QString &categoryName = QString(),
                     const QString &categoryKey = QString(),
                     const KSharedConfig::Ptr &config = KSharedConfig::Ptr());

    private:
        KCategorizedView*          m_listView;
        QMap<int, QString>         m_scripts;
        int                        scriptCount;

};

#endif
