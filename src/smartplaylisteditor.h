// (c) Pierpaolo Di Panfilo 2004
// (c) Alexandre Pereira de Oliveira 2005
// (c) 2006 Peter C. Ndikuwera <pndiku@gmail.com>
// See COPYING file for licensing information

#ifndef SMARTPLAYLISTEDITOR_H
#define SMARTPLAYLISTEDITOR_H

#include <kdialog.h> //baseclass
#include <qdom.h>
#include <khbox.h>       //baseclass
#include <q3ptrlist.h>    //definition required
//Added by qt3to4:
#include <QLabel>
#include <klineedit.h>   //inline function

class KComboBox;
class QSpinBox;
class KLineEdit;
class QCheckBox;
class Q3DateEdit;
class QLabel;
class QToolButton;
class Q3VGroupBox;

class CriteriaEditor;

class SmartPlaylistEditor : public KDialog
{
Q_OBJECT
    friend class CriteriaEditor;

    public:
        SmartPlaylistEditor( QString name, QWidget *parent, const char *name=0 );
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
        Q3VGroupBox *m_criteriaAnyGroupBox;
        Q3VGroupBox *m_criteriaAllGroupBox;

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

        Q3PtrList<CriteriaEditor> m_criteriaEditorAnyList;
        Q3PtrList<CriteriaEditor> m_criteriaEditorAllList;
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
        Q3DateEdit *m_dateEdit1;
        Q3DateEdit *m_dateEdit2;
        KComboBox *m_dateCombo;
        QLabel *m_rangeLabel;
	KComboBox *m_lengthCombo;
};

inline int
CriteriaEditor::indexToRating( int index )
{
    if ( index <= 9 && index >= 1 ) return index + 1;
    if ( index == 0 ) return index;
    return -1;
}

inline int
CriteriaEditor::ratingToIndex( int rating )
{
    if ( rating <= 10 && rating >= 2 ) return rating - 1;
    if ( rating == 0 ) return rating;
    return -1;
}

#endif
