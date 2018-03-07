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

#ifndef TEST_IMPORTER_MANAGER
#define TEST_IMPORTER_MANAGER

#include "ImporterMocks.h"

class TestImporterManager : public ImporterMocks
{
    Q_OBJECT

private Q_SLOTS:
    void initShouldLoadSettings();
    void creatingProviderShouldSetConfigAndParent();
    void creatingProviderShouldSaveSettings();
    void creatingProviderShouldSaveGeneratedId();
    void creatingConfigWidgetShouldDelegate();
    void createConfigWidgetShouldNotCrashOnNull();
    void createProviderShouldNotCrashOnNull();
    void createProviderShouldReplaceProviderIfExists();
    void createProviderShouldRegisterProvider();
    void forgetProviderShouldUnregisterProvider();
    void forgetProviderShouldForgetConfig();
    void forgetProviderShouldHangleInvalidId();
    void forgetProviderShouldNotCauseOtherProvidersToBeForgotten();
    void managerShouldHandleMultipleProviders();
};

#endif // TEST_IMPORTER_MANAGER
