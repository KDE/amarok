/***************************************************************************
 *   Copyright (C) 2004-2007 by Mark Kretschmann <markey@web.de>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef ENGINECONFIG_H
#define ENGINECONFIG_H

#include "ConfigDialogBase.h"
#include <QMap>

class Q3GroupBox;
class QComboBox;

namespace Amarok { class PluginConfig; }


class EngineConfig : public ConfigDialogBase
{
    Q_OBJECT

    public:
        EngineConfig( QWidget* parent );
        virtual ~EngineConfig();

        virtual bool hasChanged();
        virtual bool isDefault();
        virtual void updateSettings();
        virtual void updateWidgets();
        virtual void updateWidgetsDefault();

    private slots:
        virtual void aboutEngine();

    private:
        void soundSystemChanged();

        Q3GroupBox* m_engineConfigFrame;
        QComboBox* m_soundSystem;
        Amarok::PluginConfig *m_engineConfig;

        QMap<QString, QString> m_pluginName;
        QMap<QString, QString> m_pluginAmarokName;
};


#endif


