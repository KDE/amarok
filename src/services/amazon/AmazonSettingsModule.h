/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
 * The Amazon store in based upon the Magnatune store in Amarok,                        *
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#ifndef AMAZONSETTINGSMODULE_H
#define AMAZONSETTINGSMODULE_H

#include <kcmodule.h>

namespace Ui { class AmazonConfigWidget; }


/**
 * Represents the position of the different Amazon locales in the AmazonConfigWidget.
 */

enum Location
{
    AMAZON_FR = 0,
    AMAZON_DE,
    AMAZON_JP,
    AMAZON_UK,
    AMAZON_COM,
    AMAZON_IT,
    AMAZON_ES,
    AMAZON_NONE // user explicitly doesn't want to set a country
};

/**
A KCM module to configure the Amazon service

        @author Sven Krohlas <sven@asbest-online.de>
*/
class AmazonSettingsModule : public KCModule
{
    Q_OBJECT

public:
    explicit AmazonSettingsModule( QWidget *parent = 0, const QVariantList &args = QVariantList() );
    ~AmazonSettingsModule();

    virtual void save();
    virtual void load();
    virtual void defaults();

private Q_SLOTS:
    void settingsChanged();

private:
    Ui::AmazonConfigWidget *m_configDialog;
};

#endif // AMAZONSETTINGSMODULE_H
