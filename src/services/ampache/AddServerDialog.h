/****************************************************************************************
 * Copyright (c) 2010     Ian Monroe <ian@monroe.nu>                                   *
 *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef ADDSERVERDIALOG_H
#define ADDSERVERDIALOG_H

#include <QDialog>
#include <QPointer>

namespace Ui
{
    class NewServerWidget;
}

class AmpacheAccountLogin;

class AddServerDialog : public QDialog
{
    Q_OBJECT
    public:
        AddServerDialog();
        ~AddServerDialog() override;
        QString name();
        QString password();
        QString username();
        QUrl url();
    private Q_SLOTS:
        void anyTextEdited();
        void verifyData();
        void loginResult();
    private:
        Ui::NewServerWidget* m_widgets;
        QPointer<AmpacheAccountLogin> m_login;
};

#endif // ADDSERVERDIALOG_H
