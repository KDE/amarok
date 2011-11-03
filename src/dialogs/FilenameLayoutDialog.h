/****************************************************************************************
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2009 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
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
#include "TagGuesser.h"

#include <QWidget>

static const QStringList typeElements = ( QStringList()
<< ""
<< "%ignore%"
<< "%track%"
<< "%title%"
<< "%artist%"
<< "%composer%"
<< "%year%"
<< "%album%"
<< "%albumartist%"
<< "%comment%"
<< "%genre%"
<< "%filetype%"
<< "%folder%"
<< "%initial%"
<< "%discnumber%"
<< " "
<< "/"
<< "."
<< "-"
<< "_" );

class TokenDropTarget;
class QFileInfo;

//Holds the TokenLayoutWidget and TokenPool and handles their interaction. Also holds a number of case and substitution options for the filename scheme.
class AMAROK_EXPORT FilenameLayoutDialog
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
            , AlbumArtist
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


         explicit FilenameLayoutDialog( QWidget *parent = 0, bool isOrganizeCollection = 0 );
        ~FilenameLayoutDialog() {}

        QString getParsableScheme();
        QString getParsableFileName();
        int getCaseOptions();
        bool getWhitespaceOptions();
        bool getUnderscoreOptions();

        /**
        *   Sets the filename to show colored preview from
        */
        void setFileName( QString FileName );

        void setScheme( const QString &scheme );

        /* accessors to Ui::FilenameLayoutDialog members */
        bool asciiOnly() const { return asciiCheck->isChecked(); }
        void setAsciiOnly( bool enable ) { asciiCheck->setChecked( enable ); }
        bool vfatCompatible() const { return vfatCheck->isChecked(); }
        void setVfatCompatible( bool enable ) { vfatCheck->setChecked( enable ); }
        bool ignoreThe() const { return ignoreTheCheck->isChecked(); }
        void setIgnoreThe( bool enable ) { ignoreTheCheck->setChecked( enable ); }
        bool replaceSpaces() const { return spaceCheck->isChecked(); }
        void setReplaceSpaces( bool enable ) { spaceCheck->setChecked( enable ); }
        QString regexpText() const { return regexpEdit->text(); }
        void setRegexpText( const QString &text ) { regexpEdit->setText( text ); }
        QString replaceText() const { return replaceEdit->text(); }
        void setReplaceText( const QString &text ) { replaceEdit->setText( text ); }

        void setformatPresetVisible( bool visible ) { formatPresetWidget->setVisible( visible ); }

    public slots:
        void onAccept();

    signals:
        /** emitted when either the scheme, option checkboxes or the replace edits change */
        void schemeChanged();

    private slots:
        void editStateEnable( bool checked );
        void toggleAdvancedMode();
        void updatePreview();

        void slotFormatPresetSelected( int );
        void slotAddFormat();
        void slotRemoveFormat();
        void slotUpdateFormat();
        void slotSaveFormatList();

    private:
        void initOrganizeCollection();
        void initTagGuesser();
        void setAdvancedMode( bool isAdvanced );
        QString parsableScheme() const;
        QString parsableFileName( const QFileInfo &fileInfo ) const;
        void inferScheme( const QString &scheme );
        void populateFormatList();

        QString m_configCategory;
        QString m_filename;                         //!< Filename to guess from
        bool m_isOrganizeCollection;
        bool m_formatListModified;
        bool m_advancedMode;
        TokenDropTarget *m_dropTarget;
        QColor m_color_Track;
        QColor m_color_Title;
        QColor m_color_Artist;
        QColor m_color_Composer;
        QColor m_color_Year;
        QColor m_color_Album;
        QColor m_color_AlbumArtist;
        QColor m_color_Comment;
        QColor m_color_Genre;

        QList<QRadioButton*> m_caseEditRadioButtons;
};

#endif    //FILENAMELAYOUTDIALOG_H

