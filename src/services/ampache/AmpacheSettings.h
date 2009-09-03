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
 
#ifndef AMPACHESETTINGS_H
#define AMPACHESETTINGS_H

#include "AmpacheConfig.h"

#include <kcmodule.h>

namespace Ui { class AmpacheConfigWidget; }

/**
Class for handling settings for Ampache services

	@author 
*/
class AmpacheSettings : public KCModule
{
    Q_OBJECT
public:
    explicit AmpacheSettings( QWidget *parent = 0, const QVariantList &args = QVariantList() );

    ~AmpacheSettings();

    virtual void save();
    virtual void load();
    virtual void defaults();

private:

    AmpacheConfig m_config;
    Ui::AmpacheConfigWidget * m_configDialog;

private slots:

    void add();
    void remove();
    void modify();
    void selectedItemChanged ( const QString & name );
    void serverNameChanged(const QString & text);
};

#endif
