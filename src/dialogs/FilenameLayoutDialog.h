/****************************************************************************************
 * Copyright (c) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef FILENAMELAYOUTDIALOG_H
#define FILENAMELAYOUTDIALOG_H

#include "amarok_export.h"
#include "ui_FilenameLayoutDialog.h"

#include <QWidget>


static const QStringList typeElements = ( QStringList()
<< ""
<< "%ignore"
<< "%track"
<< "%title"
<< "%artist"
<< "%composer"
<< "%year"
<< "%album"
<< "%comment"
<< "%genre"
<< "%filetype"
<< "%folder"
<< "%initial"
<< "%discnumber"
<< " "
<< "/"
<< "."
<< "-"
<< "_" );

class TokenDropTarget;

//Holds the TokenLayoutWidget and TokenPool and handles their interaction. Also holds a number of case and substitution options for the filename scheme.
class FilenameLayoutDialog
    : public QWidget
    , private Ui::FilenameLayoutDialog
{
    Q_OBJECT

    public:

        enum Type
        {
              Unknown = 0
            , Ignore
            , Track
            , Title
            , Artist
            , Composer
            , Year
            , Album
            , Comment
            , Genre
            , FileType
            , Folder
            , Initial
            , DiscNumber
            , Space
            , Slash
            , Dot
            , Dash
            , Underscore
        };


        AMAROK_EXPORT explicit FilenameLayoutDialog( QWidget *parent = 0, bool isOrganizeCollection = 0 ); // Could I have exported the whole class? I don't see how
        AMAROK_EXPORT QString getParsableScheme();
        int getCaseOptions();
        int getWhitespaceOptions();
        int getUnderscoreOptions();

    public slots:
        void onAccept();

    signals:
        void schemeChanged();

    private slots:
        void editStateEnable( bool checked );
        void toggleAdvancedMode();
    
    private:
        void setAdvancedMode( bool isAdvanced );
        QString parsableScheme() const;
        void inferScheme( const QString &scheme );
        
        bool m_isOrganizeCollection;
        bool m_advancedMode;
        TokenDropTarget *m_dropTarget;
        
        QList<QRadioButton*> m_caseEditRadioButtons;
};

#endif    //FILENAMELAYOUTDIALOG_H

