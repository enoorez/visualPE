#ifndef HEXEDITDIALOG_H 
#define HEXEDITDIALOG_H 

#include <QDialog>
class QPushButton;
class QLabel;
class QLineEdit;

class HexEditDialog : public QDialog
{
    Q_OBJECT
public:
    HexEditDialog(QWidget* parent = nullptr );
    ~HexEditDialog( );

    bool    exec( int maxLineEditChar , QString& text );

    static bool showHexEditDialog( int maxLineEditChar , QString& text , QWidget* parent = nullptr );
    
public slots:
    void onEditTextChanged( const QString & text );
    void onExit( );
    void onCancle( );

protected:
    QLineEdit*  mLineEdit;
    QLabel*     mLabel;
    QPushButton*mOkButton;
    QPushButton*mCencelButton;

    bool        mOk;
    QString     mText;
};



#endif // !HEXEDITDIALOG_H 
