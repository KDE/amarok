/******************************************************************************
 * Copyright (C) 2006 Giovanni Venturi <giovanni@kde-it.org>                  *
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
 
#ifndef AMAROK_EDITFILTERDIALOG_H
#define AMAROK_EDITFILTERDIALOG_H

#include <KDialog>

#include <QList>
#include <QVector>


class QGroupBox;
class QCheckBox;
class QComboBox;
class QLabel;
class QRadioButton;
class QSpinBox;
class QStringList;
class QVBoxLayout;
class QWidget;

class KLineEdit;

class EditFilterDialog : public KDialog
{
    Q_OBJECT
    public:
        EditFilterDialog( QWidget* parent, bool metaBundleKeywords, const QString &text = "" );
        ~EditFilterDialog();

        QString filter() const;

    signals:
        void filterChanged( const QString &filter );

    private:
        QVBoxLayout *m_mainLay;

        QCheckBox *m_prefixNOT;
        QComboBox *m_comboKeyword;
        KLineEdit *m_editKeyword;

        QGroupBox *m_groupBox;

        QComboBox *m_comboCondition;
        QLabel *m_filesizeLabel;
        QComboBox *m_comboUnitSize;

        QRadioButton *m_minMaxRadio;
        QSpinBox *m_spinMin1, *m_spinMin2;
        QLabel *m_andLabel;
        QSpinBox *m_spinMax1, *m_spinMax2;

        QGroupBox *m_groupBox2;
        QRadioButton *m_checkALL;
        QRadioButton *m_checkAtLeastOne;
        QRadioButton *m_checkExactly;
        QRadioButton *m_checkExclude;
        QList<QRadioButton*> m_actionCheck;

        QGroupBox *m_groupBox3;
        QRadioButton *m_checkAND;
        QRadioButton *m_checkOR;

        bool m_appended;               // true if a filter appended
        int m_selectedIndex;           // the position of the selected keyword in the combobox
        QVector<QString> m_vector; // the vector of the amarok filter keyword
        QString m_filterText;          // the resulting filter string
        QString m_previousFilterText;  // the previous resulting filter string
        QString m_strPrefixNOT;        // is empty if no NOT prefix is needed else it's "-"

    private:
        void exclusiveSelectOf( int which );
        QString keywordConditionString(const QString& keyword) const;
        void setMinMaxValueSpins();

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

    protected slots:
        virtual void slotDefault();
        virtual void slotUser1();
        virtual void slotUser2();
        virtual void slotOk();
};

#endif /* AMAROK_EDITFILTERDIALOG_H */
