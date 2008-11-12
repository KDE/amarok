/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/
#ifndef FILENAMELAYOUTDIALOG_H
#define FILENAMELAYOUTDIALOG_H

#include "amarok_export.h"
#include "ui_FilenameLayoutDialog.h"

#include <QWidget>

//Holds the FilenameLayoutWidget and TokenListWidget and handles their interaction. Also holds a number of case and substitution options for the filename scheme.
class FilenameLayoutDialog
    : public QWidget
    , private Ui::FilenameLayoutDialog
{
    Q_OBJECT

    public:
        AMAROK_EXPORT FilenameLayoutDialog( QWidget *parent = 0, bool isOrganizeCollection = 0 );  //could I have exported the whole class? I don't see how
        AMAROK_EXPORT QString getParsableScheme();
        int getCaseOptions();
        int getWhitespaceOptions();
        int getUnderscoreOptions();

    private:
        QList< QRadioButton * > caseEditRadioButtons;
        void toBasicMode();
        void toAdvancedMode();
        bool m_isOrganizeCollection;

    private slots:
        void editStateEnable( bool checked );
        void onAccept();
        void toggleAdvancedMode();

    signals:
        void schemeChanged();
};

#endif    //FILENAMELAYOUTDIALOG_H

