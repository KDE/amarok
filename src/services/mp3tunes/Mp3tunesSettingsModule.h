/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef MP3TUNESSETTINGSMODULE_H
#define MP3TUNESSETTINGSMODULE_H

#include "Mp3tunesConfig.h"

#include <kcmodule.h>

namespace Ui { class Mp3tunesConfigWidget; }

/**
A KCM module for configuring the Mp3tunes service

	@author
*/
class Mp3tunesSettingsModule : public KCModule
{
    Q_OBJECT
public:
    explicit Mp3tunesSettingsModule( QWidget *parent = 0, const QVariantList &args = QVariantList() );

    ~Mp3tunesSettingsModule();

    virtual void save();
    virtual void load();
    virtual void defaults();


private slots:

    void settingsChanged();

private:

    Mp3tunesConfig m_config;
    Ui::Mp3tunesConfigWidget * m_configDialog;

};

#endif
