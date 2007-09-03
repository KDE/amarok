// (c) Pierpaolo Di Panfilo 2004
// (c) Alexandre Pereira de Oliveira 2005
// (c) 2006 Peter C. Ndikuwera <pndiku@gmail.com>
// See COPYING file for licensing information

#ifndef SMARTPLAYLISTEDITOR_H
#define SMARTPLAYLISTEDITOR_H

#include <kdialogbase.h> //baseclass
#include <qdom.h>
#include <qhbox.h>       //baseclass
#include <qptrlist.h>    //definition required
#include <klineedit.h>   //inline function

class KComboBox;
class KIntSpinBox;
class KLineEdit;
class QCheckBox;
class QDateEdit;
class QLabel;
class QToolButton;
class QVGroupBox;

class CriteriaEditor;

class SmartPlaylistEditor : public KDialogBase
{
Q_OBJECT
    friend class CriteriaEditor;

    public:
        SmartPlaylistEditor( QString playlist_name, QWidget *parent, const char *name=0 );
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
        QVGroupBox *m_criteriaAnyGroupBox;
        QVGroupBox *m_criteriaAllGroupBox;

        //limit widgets
        QCheckBox *m_limitCheck;
        KIntSpinBox *m_limitSpin;
        //order by widgets
        QCheckBox *m_orderCheck;
        KComboBox *m_orderCombo;
        KComboBox *m_orderTypeCombo;
        //expand by
        QCheckBox *m_expandCheck;
        KComboBox *m_expandCombo;

        QPtrList<CriteriaEditor> m_criteriaEditorAnyList;
        QPtrList<CriteriaEditor> m_criteriaEditorAllList;
};



class CriteriaEditor : public QHBox
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

        void loadCriteriaList( int valueType, QString condition = QString::null );
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
        QHBox *m_editBox;
        KLineEdit *m_lineEdit;
        KComboBox *m_comboBox;
        KComboBox *m_comboBox2;
        KIntSpinBox *m_intSpinBox1;
        KIntSpinBox *m_intSpinBox2;
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
