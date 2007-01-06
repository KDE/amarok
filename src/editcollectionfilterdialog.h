// (c) 2006 Giovanni Venturi <giovanni@kde-it.org>
// See COPYING file for licensing information.

#ifndef AMAROK_EDITCOLLECTIONFILTERDIALOG_H
#define AMAROK_EDITCOLLECTIONFILTERDIALOG_H

#include <qvaluelist.h>

#include <kdialogbase.h>

class QWidget;
class QVBoxLayout;
class QComboBox;
class QCheckBox;
class QLineEdit;
class QRadioButton;
class QGroupBox;
class QSpinBox;

class EditCollectionFilterDialog : public KDialogBase
{
    Q_OBJECT
    public:
        EditCollectionFilterDialog( QWidget* parent = 0, const QString &text = "");
        ~EditCollectionFilterDialog();

        QString filter() const;

    private:
        QVBoxLayout *m_mainLay;
        QLineEdit *m_filterRule;

        QCheckBox *m_prefixNOT;
        QComboBox *m_comboKeyword;
        QLineEdit *m_editKeyword;

        QGroupBox *m_groupBox;

        QRadioButton *m_keywordValueRadio;
        QComboBox *m_comboCondition;
        QSpinBox *m_spinValue1;
        QSpinBox *m_spinValue2;
        QComboBox *m_comboUnitSize;

        QRadioButton *m_minMaxRadio;
        QSpinBox *m_spinMin1, *m_spinMin2;
        QSpinBox *m_spinMax1, *m_spinMax2;

        QGroupBox *m_groupBox2;
        QRadioButton *m_checkALL;
        QRadioButton *m_checkAtLeastOne;
        QRadioButton *m_checkExactly;
        QRadioButton *m_checkExclude;
        QValueList<QRadioButton*> m_actionCheck;

        QGroupBox *m_groupBox3;
        QRadioButton *m_checkAND;
        QRadioButton *m_checkOR;

        bool m_appended;               // true if a filter appended
        int m_selectedIndex;           // the position of the selected keyword in the combobox
        QString vector[22];            // the vector of the amarok filter keyword
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
        void valueWanted();

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

#endif /* AMAROK_EDITCOLLECTIONFILTERDIALOG_H */
