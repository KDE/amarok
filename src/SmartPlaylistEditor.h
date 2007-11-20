// (c) Pierpaolo Di Panfilo 2004
// (c) Alexandre Pereira de Oliveira 2005
// (c) 2006 Peter C. Ndikuwera <pndiku@gmail.com>
// (c) 2007 Seb Ruiz <ruiz@kde.org>
// See COPYING file for licensing information

#ifndef SMARTPLAYLISTEDITOR_H
#define SMARTPLAYLISTEDITOR_H

#include <KDialog> //baseclass
#include <klineedit.h>   //inline function
#include <KHBox>       //baseclass

#include <QDateEdit>
#include <QDomDocument>
#include <QDomElement>

class CriteriaEditor;

class KComboBox;
class KLineEdit;

class QGroupBox;
class QCheckBox;
class QLabel;
class QSpinBox;
class QToolButton;

class SmartPlaylistEditor : public KDialog
{
Q_OBJECT
    friend class CriteriaEditor;

    public:
        SmartPlaylistEditor( QString defaultName, QWidget *parent, const char *name=0 );
        SmartPlaylistEditor( QWidget *parent, QDomElement xml, const char *name=0 );

        QDomElement result();

        QString name() const { return m_nameLineEdit->text().replace( "\n", " " ); }

        enum CriteriaType { criteriaAll = 0, criteriaAny = 1 };

    public slots:
        void addCriteriaAny();
        void addCriteriaAny( QDomElement &xml);
        void removeCriteriaAny( CriteriaEditor *criteria);

        void addCriteriaAll();
        void addCriteriaAll( QDomElement &xml);
        void removeCriteriaAll( CriteriaEditor *criteria);

    private slots:
        void updateOrderTypes( int index );

    private:
        void init(QString defaultName);
        void updateMatchWidgets();

        KLineEdit *m_nameLineEdit;

        QCheckBox *m_matchAnyCheck;
        QCheckBox *m_matchAllCheck;

        // matching boxes
        QGroupBox *m_criteriaAnyGroupBox;
        QGroupBox *m_criteriaAllGroupBox;

        //limit widgets
        QCheckBox *m_limitCheck;
        QSpinBox *m_limitSpin;
        //order by widgets
        QCheckBox *m_orderCheck;
        KComboBox *m_orderCombo;
        KComboBox *m_orderTypeCombo;
        //expand by
        QCheckBox *m_expandCheck;
        KComboBox *m_expandCombo;

        QList<CriteriaEditor*> m_criteriaEditorAnyList;
        QList<CriteriaEditor*> m_criteriaEditorAllList;
};



class CriteriaEditor : public KHBox
{
Q_OBJECT
    public:
        CriteriaEditor( SmartPlaylistEditor *editor, QWidget *parent, int criteriaType, QDomElement criteria = QDomElement() );
        ~CriteriaEditor();
        QString getSearchCriteria();
        void setSearchCriteria( const QString &str );
        QDomElement getDomSearchCriteria( QDomDocument &doc );
        void enableRemove( bool );

    private slots:
        void slotRemoveCriteriaAny();
        void slotRemoveCriteriaAll();
        void slotAddCriteriaAny();
        void slotAddCriteriaAll();
        void slotFieldSelected( int );
        void loadEditWidgets();

    private:
        enum ValueType { String, AutoCompletionString, Number, Year, Date, Rating, Length };

        void loadCriteriaList( int valueType, QString condition = QString() );
        int getValueType( int fieldIndex );
        inline int indexToRating( int );
        inline int ratingToIndex( int );

        SmartPlaylistEditor *m_playlistEditor;
        int m_currentValueType;
        QString m_lastCriteria;

        KComboBox *m_fieldCombo;
        KComboBox *m_criteriaCombo;
        QToolButton *m_addButton;
        QToolButton *m_removeButton;

        //editing widgets
        KHBox *m_editBox;
        KLineEdit *m_lineEdit;
        KComboBox *m_comboBox;
        KComboBox *m_comboBox2;
        QSpinBox *m_intSpinBox1;
        QSpinBox *m_intSpinBox2;
        QDateEdit *m_dateEdit1;
        QDateEdit *m_dateEdit2;
        KComboBox *m_dateCombo;
        QLabel *m_rangeLabel;
	KComboBox *m_lengthCombo;
};

inline int
CriteriaEditor::indexToRating( int index )
{
    if ( index <= 10 && index >= 0 ) return index;
    return -1;
}

inline int
CriteriaEditor::ratingToIndex( int rating )
{
    if ( rating <= 10 && rating >= 0 ) return rating;
    return -1;
}

#endif
