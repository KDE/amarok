/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef AMPACHESETTINGS_H
#define AMPACHESETTINGS_H

#include <kcmodule.h>

namespace Ui { class AmpacheConfigWidget; }

/**
Class for handling settings for Ampache services

	@author 
*/
class AmpacheSettings : public KCModule
{
public:
    AmpacheSettings( QWidget *parent = 0, const QVariantList &args = QVariantList() );

    ~AmpacheSettings();

    virtual void save();
    virtual void load();
    virtual void defaults();

private:

    Ui::AmpacheConfigWidget * m_configDialog;

};

#endif
