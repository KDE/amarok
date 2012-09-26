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
// #include "ui_FilenameLayoutDialog.h"
#include "ui_TagGuessOptions.h"
#include "ui_FilenameLayoutOptions.h"

#include <QWidget>

class Token;
class TokenPool;
class TokenDropTarget;
class QFileInfo;
class QStackedWidget;
class QLabel;
class QComboBox;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;

class AMAROK_EXPORT TagGuessOptionWidget : public QWidget, public Ui::TagGuessOptions
{
    Q_OBJECT

    public:
        TagGuessOptionWidget( QWidget *parent = 0 );

        int getCaseOptions();
        bool getWhitespaceOptions();
        bool getUnderscoreOptions();

    signals:
        void optionsChanged();

    private slots:
        // Handles the radiobuttons
        void editStateEnable( bool checked );

    private:
        QList<QRadioButton*> m_caseEditRadioButtons;
};


/** A couple of options used in the filename layout dialog. */
class AMAROK_EXPORT FilenameLayoutOptionWidget : public QWidget, public Ui::FilenameLayoutOptions
{
    Q_OBJECT

    public:
        FilenameLayoutOptionWidget( QWidget *parent = 0 );

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

    signals:
        void optionsChanged();
};


//Holds the TokenLayoutWidget and TokenPool and handles their interaction. Also holds a number of case and substitution options for the filename scheme.
class AMAROK_EXPORT FilenameLayoutWidget : public QWidget //, protected Ui::FilenameLayoutDialog
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
            , CollectionRoot
        };


        explicit FilenameLayoutWidget( QWidget *parent = 0 );
        virtual ~FilenameLayoutWidget() {}

        QString getParsableScheme() const;

        void setScheme( const QString &scheme );


    public slots:
        virtual void onAccept();

    signals:
        /** emitted when either the scheme, option checkboxes or the replace edits change */
        void schemeChanged();

    private slots:
        void toggleAdvancedMode();

        /* Updates the update preset button */
        void slotUpdatePresetButton();
        void slotSaveFormatList();
        void slotFormatPresetSelected( int );
        void slotAddFormat();
        void slotRemoveFormat();
        void slotUpdateFormat();

    private:
        /** Set the advanced mode, blending out several "advanced" widgets */
        void setAdvancedMode( bool isAdvanced );

        /* Iterates over the elements of the TokenLayoutWidget bar
           (really over the elements of a QList that stores the indexes
           of the tokens) and generates a string that TagGuesser can digest. */
        QString dropTargetScheme() const;

        /** Fills the m_dropTarget according to the given string scheme. */
        void inferScheme( const QString &scheme );

        bool m_formatListModified;
        bool m_advancedMode;

    protected:

        /** Set's several configuration options.
            Don't move this function to the constructor. It calls virtuals. */
        void populateConfiguration();

        /** Populates the preset combo box */
        void populateFormatList();

        virtual Token* createToken(qint64 value) const;

        /** Returns a styled token to be used in as pre and
            postfix on the schema editing line. */
        virtual Token* createStaticToken(qint64 value) const;

        QVBoxLayout *m_mainLayout;

        QComboBox *m_presetCombo;
        QPushButton *m_addPresetButton;
        QPushButton *m_updatePresetButton;
        QPushButton *m_removePresetButton;

        QPushButton *m_advancedButton;

        TokenPool *m_tokenPool;
        QStackedWidget *m_schemeStack;
        QHBoxLayout *m_schemaLineLayout;
        TokenDropTarget *m_dropTarget;

        QLabel *m_syntaxLabel;
        QFrame *m_filenameLayout;
        KLineEdit *m_filenameLayoutEdit;


        /** The name of the category used for storing the configuration */
        QString m_configCategory;
};

/** This dialog is used in the OrganizeCollection */
class AMAROK_EXPORT OrganizeCollectionWidget : public FilenameLayoutWidget
{
    Q_OBJECT

    public:
        explicit OrganizeCollectionWidget( QWidget *parent = 0 );
        virtual ~OrganizeCollectionWidget() {}

        // void setformatPresetVisible( bool visible ) { formatPresetWidget->setVisible( visible ); }

        FilenameLayoutOptionWidget* m_optionsWidget;
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

        /** Returns the current guessed tags */
        QMap<qint64,QString> guessedTags();

    public slots:
        virtual void onAccept();

    private slots:
        /** Updates the result texts. */
        void updatePreview();

    protected:
        virtual Token* createToken(qint64 value) const;

        /** returns a filename with the same number of directory
            levels as the scheme.
            Also removes the extension.
        */
        QString parsableFileName( const QFileInfo &fileInfo ) const;

        /** Returns the fileName with added path. */
        QString getParsableFileName();

        /** Filename to guess from. */
        QString m_filename;

        QLabel* m_filenamePreview;
        TagGuessOptionWidget* m_optionsWidget;
};


#endif    //FILENAMELAYOUTDIALOG_H

