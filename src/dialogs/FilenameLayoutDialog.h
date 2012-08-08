/****************************************************************************************
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2009 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
 * Copyright (c) 2012 Ralf Engels <ralf-engels@gmx.de>                                  *
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

class TokenDropTarget;
class QFileInfo;

//Holds the TokenLayoutWidget and TokenPool and handles their interaction. Also holds a number of case and substitution options for the filename scheme.
class AMAROK_EXPORT FilenameLayoutWidget : public QWidget, protected Ui::FilenameLayoutDialog
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


        explicit FilenameLayoutWidget( QWidget *parent = 0 );
        virtual ~FilenameLayoutWidget() {}

        QString getParsableScheme();

        void setScheme( const QString &scheme );

        /* accessors to Ui::FilenameLayoutWidget members */
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


    public slots:
        virtual void onAccept();

    signals:
        /** emitted when either the scheme, option checkboxes or the replace edits change */
        void schemeChanged();

    private slots:
        void editStateEnable( bool checked );
        void toggleAdvancedMode();

        void slotFormatPresetSelected( int );
        void slotAddFormat();
        void slotRemoveFormat();
        void slotUpdateFormat();
        void slotSaveFormatList();

    private:
        /** Set the advanced mode, blending out several "advanced" widgets */
        void setAdvancedMode( bool isAdvanced );
        QString parsableScheme() const;

        /** Fills the m_dropTarget according to the given string scheme. */
        void inferScheme( const QString &scheme );

        bool m_formatListModified;
        bool m_advancedMode;

    protected:

        /** Set's several configuration options.
            Don't move this function to the constructor. It calls virtuals. */
        void populateConfiguration();
        void populateFormatList();

        virtual Token* createToken(qint64 value) const;

        TokenDropTarget *m_dropTarget;

        /** The name of the category used for storing the configuration */
        QString m_configCategory;

        QList<QRadioButton*> m_caseEditRadioButtons;
};

/** This dialog is used in the OrganizeCollection */
class AMAROK_EXPORT OrganizeCollectionWidget : public FilenameLayoutWidget
{
    Q_OBJECT

    public:
        explicit OrganizeCollectionWidget( QWidget *parent = 0 );
        virtual ~OrganizeCollectionWidget() {}

        void setformatPresetVisible( bool visible ) { formatPresetWidget->setVisible( visible ); }
};

/** This dialog allows the user to define a filename scheme from which to guess tags. */
class AMAROK_EXPORT TagGuesserWidget : public FilenameLayoutWidget
{
    Q_OBJECT

    public:
        explicit TagGuesserWidget( QWidget *parent = 0 );
        virtual ~TagGuesserWidget() {}

        /** Sets the filename to show colored preview from. */
        void setFileName( const QString& fileName );

        /** Returns the fileName with added path. */
        QString getParsableFileName();

        int getCaseOptions();
        bool getWhitespaceOptions();
        bool getUnderscoreOptions();

    public slots:
        virtual void onAccept();

    private slots:
        /** Updates the result texts. */
        void updatePreview();

    protected:
        virtual Token* createToken(qint64 value) const;

        /** Adds the path (depending on cbUseFullPath and sbNestingLevel) to the fileInfo */
        QString parsableFileName( const QFileInfo &fileInfo ) const;

        /** Filename to guess from. */
        QString m_filename;
};


#endif    //FILENAMELAYOUTDIALOG_H

