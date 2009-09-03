/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#ifndef MAGNATUNESETTINGSMODULE_H
#define MAGNATUNESETTINGSMODULE_H

#include "MagnatuneConfig.h"

#include <kcmodule.h>

namespace Ui { class MagnatuneConfigWidget; }

/**
A KCM module to configure the Magnatune service

	@author 
*/
class MagnatuneSettingsModule : public KCModule
{
    Q_OBJECT
public:
    explicit MagnatuneSettingsModule( QWidget *parent = 0, const QVariantList &args = QVariantList() );

    ~MagnatuneSettingsModule();

    virtual void save();
    virtual void load();
    virtual void defaults();

private slots:

    void settingsChanged();

private:

    MagnatuneConfig m_config;
    Ui::MagnatuneConfigWidget * m_configDialog;

};

#endif
