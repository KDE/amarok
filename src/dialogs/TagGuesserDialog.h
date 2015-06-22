/****************************************************************************************
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Daniel Dewald <Daniel.Dewald@time.shift.de>                       *
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

#ifndef TAGGUESSERDIALOG_H
#define TAGGUESSERDIALOG_H

#include "amarok_export.h"
#include "widgets/FilenameLayoutWidget.h"

#include "ui_TagGuessOptions.h"

#include <QDialog>

#include <QMap>
#include <QString>

class QLabel;

class TagGuessOptionWidget : public QWidget, public Ui::TagGuessOptions
{
    Q_OBJECT

    public:
        TagGuessOptionWidget( QWidget *parent = 0 );

        int getCaseOptions();
        bool getWhitespaceOptions();
        bool getUnderscoreOptions();

    Q_SIGNALS:
        void changed();

    private Q_SLOTS:
        // Handles the radiobuttons
        void editStateEnable( bool checked );

    private:
        QList<QRadioButton*> m_caseEditRadioButtons;
};



/** This widget allows the user to define a filename scheme from which to guess tags. */
class AMAROK_EXPORT TagGuesserWidget : public FilenameLayoutWidget
{
    Q_OBJECT

    public:
        explicit TagGuesserWidget( QWidget *parent = 0 );
        virtual ~TagGuesserWidget() {}

    protected:
        virtual Token* createToken(qint64 value) const;
};


class AMAROK_EXPORT TagGuesserDialog : public KDialog
{
    Q_OBJECT

    public:
        TagGuesserDialog( const QString &fileName, QWidget *parent = 0 );

        QMap<qint64,QString> guessedTags();

        /** Sets the filename to show colored preview from. */
        void setFileName( const QString& fileName );

    public Q_SLOTS:
        virtual void onAccept();

    private Q_SLOTS:
        /** Updates the result texts and the colored filename. */
        void updatePreview();

    private:

        /** @Returns a filename with the same number of directory
            levels as the scheme.
            Also removes the extension.
        */
        QString parsableFileName( const QFileInfo &fileInfo ) const;

        /** @Returns the fileName with added path. */
        QString getParsableFileName();

        /**
        *   @Returns a colored version of the filename
        */
        QString coloredFileName( QMap<qint64,QString> tags );

        /**
         * @Returns color name for specified metadata field
         */
        static QString fieldColor( qint64 field );


        /** Filename to guess from. */
        QString m_fileName;

        TagGuesserWidget* m_layoutWidget;
        QLabel* m_filenamePreview;
        TagGuessOptionWidget* m_optionsWidget;
};


#endif /* TAGGUESSERDIALOG_H */

