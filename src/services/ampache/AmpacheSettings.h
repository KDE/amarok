/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include <KCModule>

namespace Ui { class AmpacheConfigWidget; }

/**
Class for handling settings for Ampache services

	@author 
*/
class AmpacheSettings : public KCModule
{
    Q_OBJECT
public:
    explicit AmpacheSettings( QWidget *parent, const QVariantList &args );

    ~AmpacheSettings() override;

    void save() override;
    void load() override;
    void defaults() override;

private:

    AmpacheConfig m_config;
    Ui::AmpacheConfigWidget * m_configDialog;
    void loadList();
    int m_lastRowEdited;
    int m_lastColumnEdited;
private Q_SLOTS:

    void add();
    void remove();
    void serverNameChanged(const QString & text);
    void onCellDoubleClicked(int row, int column);
    void saveCellEdit(int row, int column);    
};

#endif
