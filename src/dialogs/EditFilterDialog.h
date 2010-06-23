/****************************************************************************************
 * Copyright (c) 2006 Giovanni Venturi <giovanni@kde-it.org>                            *
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
 
#ifndef AMAROK_EDITFILTERDIALOG_H
#define AMAROK_EDITFILTERDIALOG_H

#include "core/meta/Meta.h"
#include "ui_EditFilterDialog.h"

#include <KDialog>

#include <QList>

class QWidget;
class QSpinBox;
class QDateEdit;

class EditFilterDialog : public KDialog
{
    Q_OBJECT

    public:
        explicit EditFilterDialog( QWidget* parent, const QString &text = QString() );
        ~EditFilterDialog();

        QString filter() const;

    signals:
        void filterChanged( const QString &filter );

    private:
        Ui::EditFilterDialog m_ui;

        bool m_appended;               // true if a filter appended
        QString m_filterText;          // the resulting filter string
        QString m_previousFilterText;  // the previous resulting filter string

        // Cache lists for completion
        QStringList m_artists;
        QStringList m_albums;
        QStringList m_composers;
        QStringList m_genres;
        QStringList m_labels;

        QString keywordConditionDate(const QString& keyword) const;
        QString keywordConditionNumeric(const QString& keyword) const;
        QString keywordConditionText(const QString& keyword) const;

    private slots:
        void selectedAttribute( const QString &attr );

        void minSpinChanged(int value);
        void maxSpinChanged(int value);

        void dateWanted();
        void textWanted( const KIcon &icon = KIcon() );
        void textWanted( const QStringList &completions, const KIcon &icon = KIcon() );
        void valueWanted();

        void chooseCondition(int index);
        void chooseOneValue();
        void chooseMinMaxValue();
        
        void resultReady( const QString &collectionId, const Meta::AlbumList &albums );
        void resultReady( const QString &collectionId, const Meta::ArtistList &artists );
        void resultReady( const QString &collectionId, const Meta::ComposerList &composers );
        void resultReady( const QString &collectionId, const Meta::GenreList &genres );
        void resultReady( const QString &collectionId, const Meta::LabelList &labels );

    protected slots:
        virtual void slotDefault();
        virtual void slotUser1();
        virtual void slotUser2();
        virtual void slotOk();
};

#endif /* AMAROK_EDITFILTERDIALOG_H */
