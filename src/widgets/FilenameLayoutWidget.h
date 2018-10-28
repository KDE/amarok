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

#ifndef FILENAMELAYOUTWIDGET_H
#define FILENAMELAYOUTWIDGET_H

#include "amarok_export.h"

#include <QWidget>

class Token;
class TokenPool;
class TokenDropTarget;

class QFileInfo;
class QStackedWidget;
class QLabel;
class QFrame;
class QComboBox;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;

class QLineEdit;

//Holds the TokenLayoutWidget and TokenPool and handles their interaction. Also holds a number of case and substitution options for the filename scheme.
class AMAROK_EXPORT FilenameLayoutWidget : public QWidget
{
    Q_OBJECT

    public:

        enum Type
        {
              Unknown = 0
            , Ignore
            , TrackNumber
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

    public Q_SLOTS:
        virtual void onAccept();

    Q_SIGNALS:
        /** emitted when either the scheme, option checkboxes or the replace edits change */
        void schemeChanged();

    private Q_SLOTS:
        void toggleAdvancedMode();

        /* Updates the update preset button */
        void slotUpdatePresetButton();
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

        bool m_advancedMode;

    protected:

        /** Set's several configuration options.
            Don't move this function to the constructor. It calls virtuals. */
        void populateConfiguration();

        /** Populates the preset combo box */
        void populateFormatList( const QString &custom );

        /** Saves the preset combo box */
        void saveFormatList() const;

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
        QLineEdit *m_filenameLayoutEdit;

        /** The name of the category used for storing the configuration */
        QString m_configCategory;
};


#endif    //FILENAMELAYOUTWIDGET_H

