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

#include "meta/Meta.h"
#include "ui_EditFilterDialog.h"

#include <KDialog>

#include <QList>
#include <QVector>


class QStringList;
class QWidget;

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
        QList<QRadioButton*> m_checkActions;
        Ui::EditFilterDialog m_ui;
        
        bool m_appended;               // true if a filter appended
        int m_selectedIndex;           // the position of the selected keyword in the combobox
        QVector<QString> m_vector;     // the vector of the amarok filter keyword
        QString m_filterText;          // the resulting filter string
        QString m_previousFilterText;  // the previous resulting filter string
        QString m_strPrefixNOT;        // is empty if no NOT prefix is needed else it's "-"

        // Cache lists for completion
        QStringList m_artists;
        QStringList m_albums;
        QStringList m_composers;
        QStringList m_genres;

    private:
        void exclusiveSelectOf( int which );
        QString keywordConditionString(const QString& keyword) const;

    private slots:
        void selectedKeyword(int index);

        void minSpinChanged(int value);
        void maxSpinChanged(int value);

        void textWanted();
        void textWanted( const QStringList &completions );
        void valueWanted();

        void chooseCondition(int index);
        void chooseOneValue();
        void chooseMinMaxValue();

        void slotCheckAll();
        void slotCheckAtLeastOne();
        void slotCheckExactly();
        void slotCheckExclude();

        void slotCheckAND();
        void slotCheckOR();

        void assignPrefixNOT();
        
        void resultReady( const QString &collectionId, const Meta::AlbumList &albums );
        void resultReady( const QString &collectionId, const Meta::ArtistList &artists );
        void resultReady( const QString &collectionId, const Meta::ComposerList &composers );
        void resultReady( const QString &collectionId, const Meta::GenreList &genres );

    protected slots:
        virtual void slotDefault();
        virtual void slotUser1();
        virtual void slotUser2();
        virtual void slotOk();
};

#endif /* AMAROK_EDITFILTERDIALOG_H */
