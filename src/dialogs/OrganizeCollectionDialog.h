/****************************************************************************************
 * Copyright (c) 2006 Mike Diehl <madpenguin8@yahoo.com>                                *
 * Copyright (c) 2008 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef AMAROK_ORGANIZECOLLECTIONDIALOG_H
#define AMAROK_ORGANIZECOLLECTIONDIALOG_H

#include "amarok_export.h"
#include "core/meta/forward_declarations.h"

#include "widgets/FilenameLayoutWidget.h"
#include "ui_OrganizeCollectionOptions.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QWidget>

namespace Ui
{
    class OrganizeCollectionDialogBase;
}

class QLineEdit;
class TrackOrganizer;


/** A couple of options used in the filename layout dialog and the UmsCollection configuration dialog. */
class AMAROK_EXPORT OrganizeCollectionOptionWidget : public QGroupBox, public Ui::OrganizeCollectionsOptions
{
    Q_OBJECT

    public:
        explicit OrganizeCollectionOptionWidget( QWidget *parent = 0 );

        bool asciiOnly() const { return asciiCheck->isChecked(); }
        void setAsciiOnly( bool enable ) { asciiCheck->setChecked( enable ); }
        bool vfatCompatible() const { return vfatCheck->isChecked(); }
        void setVfatCompatible( bool enable ) { vfatCheck->setChecked( enable ); }
        bool postfixThe() const { return ignoreTheCheck->isChecked(); }
        void setPostfixThe( bool enable ) { ignoreTheCheck->setChecked( enable ); }
        bool replaceSpaces() const { return spaceCheck->isChecked(); }
        void setReplaceSpaces( bool enable ) { spaceCheck->setChecked( enable ); }
        QString regexpText() const { return regexpEdit->text(); }
        void setRegexpText( const QString &text ) { regexpEdit->setText( text ); }
        QString replaceText() const { return replaceEdit->text(); }
        void setReplaceText( const QString &text ) { replaceEdit->setText( text ); }

    Q_SIGNALS:
        void optionsChanged();
};


/** A FilenameLayoutWidget that set's the needed tokens for organizing a collection.
    Note: This widget is also used in the UmsCollection dialog at
    src/core-impl/collections/umsCollection/UmsCollection.cpp */
class AMAROK_EXPORT OrganizeCollectionWidget : public FilenameLayoutWidget
{
    Q_OBJECT

    public:
        explicit OrganizeCollectionWidget( QWidget *parent = 0 );
        virtual ~OrganizeCollectionWidget() {}

    protected:
        QString buildFormatTip() const;
};


class AMAROK_EXPORT OrganizeCollectionDialog : public QDialog
{
    Q_OBJECT

    public:

        explicit OrganizeCollectionDialog( const Meta::TrackList &tracks,
                                           const QStringList &folders,
                                           const QString &targetExtension = QString(),
                                           QWidget *parent = 0,
                                           const char *name = 0,
                                           bool modal = true,
                                           const QString &caption = QString(),
                                           QFlags<QDialogButtonBox::StandardButton> buttonMask = QDialogButtonBox::Ok|QDialogButtonBox::Cancel );

        ~OrganizeCollectionDialog();

        QMap<Meta::TrackPtr, QString> getDestinations();
        bool overwriteDestinations() const;

    public Q_SLOTS:
        void slotUpdatePreview();
        void slotDialogAccepted();

    private Q_SLOTS:
        void processPreviewPaths();
        void previewNextBatch();
        void slotOverwriteModeChanged();

    private:
        QString buildFormatString() const;

        Ui::OrganizeCollectionDialogBase *ui;

        TrackOrganizer *m_trackOrganizer;
        Meta::TrackList m_allTracks;
        QString m_targetFileExtension;

        QStringList m_originals;
        QStringList m_previews;
        QString m_previewPrefix;
        bool m_conflict;

    private Q_SLOTS:
        void slotEnableOk( const QString & currentCollectionRoot );
};

#endif  //AMAROK_ORGANIZECOLLECTIONDIALOG_H
