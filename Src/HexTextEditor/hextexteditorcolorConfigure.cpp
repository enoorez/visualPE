#include "hextexteditorcolorConfigure.h"

#include <QHBoxLayout>
#include <QLabel>
#include "qpushbutton.h"
#include <QColorDialog>
#include "qpainter.h"
#include "hextexteditor.h"
#include "mainwindow.h"

QHBoxLayout*    HexTextEditorColorConfigureDlg::addColorAreaAndButton(
    const QString& labelText , ColorArea* btn )
{
    QHBoxLayout* hbl = new QHBoxLayout;
    QLabel  *label = new QLabel;
    label->setText( labelText );
    btn->setMinimumWidth( 30 );
    hbl->addWidget( label );
    hbl->addWidget( btn );

    return hbl;
}


HexTextEditorColorConfigureDlg::HexTextEditorColorConfigureDlg( MainWindow *parent )
    :QDialog( parent ) ,
    mWindow( parent )
{
    QVBoxLayout *vbl = new QVBoxLayout( this );

    mLinenumber = new ColorArea( this , mWindow->mHexTextEditor->getColorLineNumer( ) );
    mLinenumberBackground = new ColorArea( this , mWindow->mHexTextEditor->getBKColorLineNumber( ) );
    mHexText = new ColorArea( this , mWindow->mHexTextEditor->getFontColor( ) );
    mHexTextBackground = new ColorArea( this , mWindow->mHexTextEditor->getBKColor( ) );
    mHexTextSelect = new ColorArea( this , mWindow->mHexTextEditor->getColorSelect( ) );


    vbl->addLayout( addColorAreaAndButton( tr( "lineNumber Color" ) , mLinenumber ) );
    vbl->addLayout( addColorAreaAndButton( tr( "lineNumber Background Color" ) , mLinenumberBackground ) );
    vbl->addLayout( addColorAreaAndButton( tr( "hex text color" ) , mHexText ) );
    vbl->addLayout( addColorAreaAndButton( tr( "hex text background color" ) , mHexTextBackground ) );
    vbl->addLayout( addColorAreaAndButton( tr( "hex text slection section color" ) , mHexTextSelect ) );


    // 连接按钮点击事件
    connect( mLinenumber ,
             SIGNAL( clicked( const QColor& ) ) ,
             this ,
             SLOT( onLinenumberCorlorClick( const QColor& ) ) );
    connect( mLinenumberBackground ,
             SIGNAL( clicked( const QColor& ) ) ,
             this ,
             SLOT( onLinenumberBackgroundCorlorClick( const QColor& ) ) );
    connect( mHexText ,
             SIGNAL( clicked( const QColor& ) ) ,
             this ,
             SLOT( onHexTextColorClick( const QColor& ) ) );
    connect( mHexTextBackground ,
             SIGNAL( clicked( const QColor& ) ) ,
             this ,
             SLOT( onHexTextBackgroundColorClick( const QColor& ) ) );
    connect( mHexTextSelect ,
             SIGNAL( clicked( const QColor& ) ) ,
             this ,
             SLOT( onHexTextSelectColorClick( const QColor& ) ) );



}

void HexTextEditorColorConfigureDlg::onLinenumberCorlorClick( const QColor & c )
{
    mWindow->mHexTextEditor->setColorLineNumer( c );
}

void HexTextEditorColorConfigureDlg::onLinenumberBackgroundCorlorClick( const QColor & c )
{
    mWindow->mHexTextEditor->setBKColorLineNumber( c );
}

void HexTextEditorColorConfigureDlg::onHexTextColorClick( const QColor & c )
{
    mWindow->mHexTextEditor->setFontColor( c );
}

void HexTextEditorColorConfigureDlg::onHexTextBackgroundColorClick( const QColor & c )
{
    mWindow->mHexTextEditor->setBKColor( c );
}

void HexTextEditorColorConfigureDlg::onHexTextSelectColorClick( const QColor & c )
{
    mWindow->mHexTextEditor->setColorSelect( c );
}


ColorArea::ColorArea( QWidget *parent , const QColor &color )
    :QFrame( parent ) ,
    mBackgroundColor( color )
{ }

void ColorArea::setBackgroundColor( const QColor &color )
{
    mBackgroundColor = color;
    this->repaint( );
}

void ColorArea::paintEvent( QPaintEvent * )
{
    QPainter    painter( this );

    painter.fillRect( QRect( 0 , 0 , size( ).width( ) , size( ).height( ) ) ,
                      QBrush( mBackgroundColor ) );
}

void ColorArea::mouseReleaseEvent( QMouseEvent *event )
{
    if( event->button( ) == Qt::LeftButton ) {
        mBackgroundColor = QColorDialog::getColor( );
        emit clicked( mBackgroundColor );
        repaint( );
    }
}


