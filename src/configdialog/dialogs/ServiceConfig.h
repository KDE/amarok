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
 
#ifndef SERVICECONFIGSCREEN_H
#define SERVICECONFIGSCREEN_H

#include "ConfigDialogBase.h"
#include "services/ServicePluginManager.h"

#include <KPluginSelector>


/**
A widget that allows configuration of services

	@author 
*/
class ServiceConfig : public ConfigDialogBase
{
    Q_OBJECT
            
public:
    ServiceConfig( QWidget * parent );

    ~ServiceConfig();

    virtual void updateSettings();
    virtual bool hasChanged();
    virtual bool isDefault();

public slots:

    void slotConfigChanged( bool changed );
    void slotConfigComitted( const QByteArray & name );

private:
    ServicePluginManager * m_servicePluginManager;
    KPluginSelector * m_serviceSelector;

    bool m_configChanged;
    QStringList m_changedServices;
};

#endif
