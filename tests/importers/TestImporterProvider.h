/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#ifndef TEST_IMPORTER_PROVIDER_H
#define TEST_IMPORTER_PROVIDER_H

#include "ImporterMocks.h"

class TestImporterProvider : public ImporterMocks
{
    Q_OBJECT

private Q_SLOTS:
    void constructorShouldSetConfigAndManager();
    void constructorShouldSetUidIfNotSet();
    void idShouldReturnConfiguredId();
    void descriptionShouldDelegateToManager();
    void iconShouldDelegateToManager();
    void nameShouldReturnConfiguredName();
    void nameShouldNotCrashIfNameIsNotConfigured();
    void isConfigurableShouldReturnTrue();
    void configWidgetShouldDelegateToManager();
    void reconfigureShouldEmitSignal();
    void reconfigureShouldNotEmitSignalOnDifferentUid();
    void defaultPreferenceShouldReturnNoByDefault();
};

#endif // TEST_IMPORTER_PROVIDER_H
