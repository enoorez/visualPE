#include "hexeditdialog.h"
#include "qboxlayout.h"
#include "qlineedit.h"
#include "qlabel.h"
#include "QPushButton.h"
#include "qvalidator.h"


HexEditDialog::HexEditDialog( QWidget* parent )
    : QDialog( parent )
    , mOk( false )
{
    QDialog::setWindowTitle( tr( "hex text input dialog" ) );

    mLineEdit = new QLineEdit;
    mLineEdit->setMinimumSize( 640 , 30 );

    // ×Ö·ûºÍÊý×Ö
    QRegExp reg( "[a-fA-F0-9 ]+$" );
    QRegExpValidator *pValidator = new QRegExpValidator( this );
    pValidator->setRegExp( reg );

    mLineEdit->setValidator( pValidator );



    mLabel = new QLabel;
    mOkButton = new QPushButton;
    mOkButton->setText( tr( "ok" ) );
    mCencelButton = new QPushButton;
    mCencelButton->setText( tr( "cencel" ) );

    QHBoxLayout *hLay = new QHBoxLayout;
    hLay->addWidget( mLabel );
    hLay->addWidget( mOkButton );
    hLay->addWidget( mCencelButton );

    QVBoxLayout* vLay = new QVBoxLayout( this );
    vLay->addWidget( mLineEdit );
    vLay->addLayout( hLay );

    connect( mLineEdit ,
             SIGNAL( textChanged( const QString& ) ) ,
             this ,
             SLOT( onEditTextChanged( const QString& ) ) );

    connect( mLineEdit ,
             SIGNAL( returnPressed( ) ) ,
             this ,
             SLOT( onExit( ) )
             );

    connect( mOkButton ,
             SIGNAL( clicked( ) ) ,
             this ,
             SLOT( onExit( ) ) );

    connect( mCencelButton ,
             SIGNAL( clicked( ) ) ,
             this ,
             SLOT( onCancle( ) ) );

}

bool HexEditDialog::showHexEditDialog( int maxLineEditChar , QString& text , QWidget* parent )
{
   // HexEditDialog dlg( maxLineEditChar, parent );
    HexEditDialog dlg( parent );
    return dlg.exec( maxLineEditChar, text );
}

HexEditDialog::~HexEditDialog( )
{

}

bool HexEditDialog::exec( int maxLineEditChar , QString& text )
{
    mLineEdit->setMaxLength( maxLineEditChar );
    QString curMax;
    curMax.sprintf( "(0/%d)" , maxLineEditChar );
    mLabel->setText( curMax );


    curMax = ">";
    for( int i = 0; i < text.size( ); ++i ) {
        if( text[ i ] == ' ' ) {
            curMax += "NN ";
        }
    }
    curMax += "NN;0";

    mLineEdit->setInputMask( curMax );

    mLineEdit->setText( text );
    int n = QDialog::exec( );
    if( n )
        text = mText;
    return n;
}

void HexEditDialog::onEditTextChanged( const QString & text )
{
    QString buff;
    buff.sprintf( "(%d/%d)" , text.length( ) , mLineEdit->maxLength( ) );
    mLabel->setText( buff );
}

void HexEditDialog::onExit( )
{
    mText = mLineEdit->text( );
    done( 1 );
}

void HexEditDialog::onCancle( )
{
    done( 0 );
}
