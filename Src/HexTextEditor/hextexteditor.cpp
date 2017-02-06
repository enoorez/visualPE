#include "hextexteditor.h"
#include <QDebug>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include "qmenu.h"
#include "qapplication.h"
#include <QClipboard>
#include <QLineEdit>
#include <QInputDialog>
#include "hexeditdialog.h"

#define OFS_SEPATOR     3
#define OFS_LINENUMBER  6

HexTextEditor::HexTextEditor( QWidget* parent )
    :QAbstractScrollArea( parent )
    , mMaxVisualLine( 0 )
    , mMaxLine( 0 )
    , mCurLine( 0 )
    , mRollStep( 10 )
    , mFontMetrics( nullptr )
    , mColorHexText( Qt::darkBlue )
    , mBKColorHexText( Qt::white )
    , mColorLineNumer( Qt::blue )
    , mBKColorLineNumber( Qt::white )
    , mCharWidth( )
    , mColorSelect( QColor( 163 , 73 , 164 ) )
    , mMouseLbuttonStatus(false )
    , mSelectBeginIndex()
    , mSelectEndIndex()
    , mControlKeyStatus(false )
{
    setFont(QFont( "Consolas", 10)  );

    setMouseTracking( true );

    mScrollBar = new QScrollBar( this );
    mScrollBar->setRange( 0 , mMaxLine );

    connect( mScrollBar , SIGNAL( sliderMoved( int ) ) , this , SLOT( scrollbarMove( int ) ) );
    connect( mScrollBar , SIGNAL( valueChanged( int ) ) , this , SLOT( scrollbarMove( int ) ) );
    
}




int HexTextEditor::getNumerOfDigit( int num )
{
    int power = 0;
    while( num ) {
        num /= LINE_ITEM_COUNT;
        ++power;
    }
    return power;
}


void HexTextEditor::hexToString( const char* pHex , int nLen , char* pBuff )
{
    if( pHex == NULL || pBuff == nullptr )
        return ;

    for( int i = 0 ; i < nLen; ++i ) {

        sprintf( pBuff , "%02X " , (unsigned char)*pHex );
        pHex++;
        pBuff += 3;
    }

    *( --pBuff ) = 0; // �����ڶ����ǿո�,����ȥ��
}

void HexTextEditor::getHexString( const char* pHex , int nLen , char* pBuff )
{
    memcpy( pBuff , pHex , nLen );
    pBuff[ nLen ] = 0;
    while( nLen-- ) {
        if( *pBuff < 32 )
            *pBuff = '.';
        ++pBuff;
    }
}



//************************************
// ������: paintEvent
// ��  ��: ���ڻ����¼�
// ����ֵ: void
// ��  ��: QPaintEvent * event
//************************************
void HexTextEditor::paintEvent( QPaintEvent *event )
{
    event;

    QPainter painter( this->viewport( ) );


    // ������ˢ�ɰ�ɫ
    painter.fillRect( painter.viewport( ) , QBrush( mBKColorHexText ) );

   

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // ����ѡ������
    paintSelectRect( painter );

    if( mHexData.isEmpty( ) )
        return;


    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // �����к�
    paintLineNumber( painter );


    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // ����ʮ�������ַ���
    paintHexText( painter );


    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // �����ַ���
    paintString( painter );

}

// �����������¼�
void HexTextEditor::wheelEvent( QWheelEvent * e )
{
    if( mHexData.isEmpty( ) )
        return;

    int numDegrees = e->delta( ) / 8;
    int numSteps = numDegrees / 15;


    if( numSteps > 0 ) {
        if( (QApplication::keyboardModifiers() == Qt::ControlModifier) ){
            QFont font = this->font();
            font.setPixelSize(font.pixelSize() + 10);
            setFont(font);
            viewport()->repaint();
            return ;
        }
        if( mCurLine <= 0 )
            return;


        emit wheelUp( mRollStep );

        // �����ƶ�������
        for( int i = 0; i < mRollStep && mCurLine>0; ++i ) {
            //verticalScrollBar( )->triggerAction( QAbstractSlider::SliderSingleStepSub );
            mCurLine--;
        }
        mScrollBar->setSliderPosition( mCurLine );
    }
    else {
        if( (QApplication::keyboardModifiers() == Qt::ControlModifier) ){
            QFont font = this->font();
            font.setPixelSize(font.pixelSize() - 10);
            setFont(font);
            viewport()->repaint();
            return ;
        }

        if( mCurLine >= mMaxLine )
            return ;

        emit wheelDown( mRollStep );
        // �����ƶ�������
        for( int i = 0; i < mRollStep&& mCurLine <= mMaxLine; ++i ) {
            //verticalScrollBar( )->triggerAction( QAbstractSlider::SliderSingleStepAdd );
            mCurLine++;
        }
        mScrollBar->setSliderPosition( mCurLine );
    }

    //this->viewport( )->update( );
}

// ���ڴ�С�ı��¼�
void HexTextEditor::resizeEvent( QResizeEvent * event )
{
    event;
    mMaxVisualLine = viewport( )->height( ) / mLineHeight ;


    mScrollBar->move( viewport()->width() - 10 , 0 );
    mScrollBar->resize( 10 , viewport( )->height( ) );
}

// ����ƶ��¼�
void HexTextEditor::mouseMoveEvent( QMouseEvent * e )
{
    if(mHexData.isEmpty())
        return;

    QPoint pos = e->pos( );
    QPainter    painter( this->viewport( ) );

    
    if( isOnLineNumberColumn( e->pos( ) ) ) {
        setCursor(Qt::ForbiddenCursor);
        emit mouseOnLineNumer( e->pos( ) );
    }
    else if( isOnHexTextColumn( e->pos( ) ) ) {

        // ������ת�г��к�, �������ź�
        // 
        int line = ( pos.y( ) - mLineHeight - 5 ) / mLineHeight + mCurLine;
        int row = ( pos.x( ) - mLineNumberWidth + mCharWidth * 2 ) / mCharWidth / 3 ;
        setCursor( Qt::IBeamCursor );

        emit mouseOnHexText( line , row );
    }
    else if( isOnStringColumn( e->pos( ) ) ) {
        setCursor( Qt::ArrowCursor );
        QPoint pos = e->pos( );
        int line = ( pos.y( ) - mLineHeight  - 5 ) / mLineHeight + mCurLine;
        int row = ( pos.x( ) - mLineNumberWidth- mHexTextWidth + mCharWidth * 2 ) / mCharWidth / 3 ;

        emit mouseOnString( line , row );
    }
    

    if( mMouseLbuttonStatus ) {
        
        int line = ( pos.y( ) - mLineHeight - 5 ) / mLineHeight + mCurLine;
        int row = ( pos.x( ) - mLineNumberWidth + mCharWidth * 2 ) / mCharWidth / 3 ;

        mSelectEndIndex = line*LINE_ITEM_COUNT + row ;

        if( mSelectBeginIndex > mSelectEndIndex ) {
            setSelection( mSelectEndIndex , mSelectBeginIndex );
        }
        else
            setSelection( mSelectBeginIndex , mSelectEndIndex );

        viewport( )->repaint( );
    }
}

// ��갴�������¼�
void HexTextEditor::mousePressEvent( QMouseEvent * e )
{
    if( e->button( ) == Qt::LeftButton ) {
        if( isOnHexTextColumn( e->pos( ) ) ) {
            mSelect.clear( );
            mMouseLbuttonStatus = true;
            QPoint pos = e->pos( );
            int line = ( pos.y( ) - mLineHeight - 5 ) / mLineHeight + mCurLine;
            int row = ( pos.x( ) - mLineNumberWidth + mCharWidth * 2 ) / mCharWidth / 3 ;

            mSelectBeginIndex = line*LINE_ITEM_COUNT + row ;
        }
    }
}

// ��갴�������¼�
void HexTextEditor::mouseReleaseEvent( QMouseEvent * e )
{

    if( e->button( ) == Qt::LeftButton ) {
        if( isOnHexTextColumn( e->pos( ) ) ) {

\
            mMouseLbuttonStatus = false;

            // ����갴�µ�ת��Ϊ����
            QPoint pos = e->pos( );
            int line = ( pos.y( ) - mLineHeight - 5 ) / mLineHeight + mCurLine;
            int row = ( pos.x( ) - mLineNumberWidth + mCharWidth * 2 ) / mCharWidth / 3 ;

            mSelectEndIndex = line*LINE_ITEM_COUNT + row ;
            
            if( mSelectBeginIndex > mSelectBeginIndex ) {
                setSelection( mSelectEndIndex , mSelectBeginIndex );
            }
            else
                setSelection( mSelectBeginIndex , mSelectEndIndex );

            viewport( )->repaint( );
        }
    }
}


void HexTextEditor::keyReleaseEvent( QKeyEvent * e )
{
    QPoint  pos = QCursor::pos( ) ;
    pos = this->mapFromGlobal( pos );
    int key = e->key( );
    if( key & Qt::Key_Control ) {
        mControlKeyStatus = false;
    }
}


// ���̰��������¼�
void HexTextEditor::keyPressEvent( QKeyEvent * e )
{
    QPoint  pos = QCursor::pos( ) ;
    pos = this->mapFromGlobal( pos );
    int key = e->key( );
    if( e->modifiers( ) == Qt::ControlModifier ) {
        switch( key ) {
            case Qt::Key_Z: undo( ); break;
            case Qt::Key_Y: redo( ); break;
            case Qt::Key_G:menuGotoByteTriggered( ); break;
            case  Qt::Key_E:menuEditTriggered( ); break;
            case Qt::Key_C:menuCopyTriggered( ); break;
            case Qt::Key_A:menuSelectAllTriggered( ); break;
            case Qt::Key_Down:
            {
                if( mCurLine >= mMaxLine )
                    return ;

                emit wheelDown( mRollStep );
                // �����ƶ�������

                mCurLine++;
                mScrollBar->setSliderPosition( mCurLine );
            }
            break;
            case Qt::Key_Up:
            {
                if( mCurLine <= 0 )
                    return;

                emit wheelUp( mRollStep );

                // �����ƶ�������
                mCurLine--;
                mScrollBar->setSliderPosition( mCurLine );
            }
        }
    }

}

//************************************
// ������: contextMenuEvent
// ��  ��: �˵��¼�
// ����ֵ: void
// ��  ��: QContextMenuEvent * event
//************************************
void HexTextEditor::contextMenuEvent( QContextMenuEvent *event )
{
    if( mHexData.isEmpty( ) )
        return;

    event;
    QMenu *menu = new QMenu( this );

    if( !mSelect.isEmpty( ) ) {
        connect( menu->addAction( tr( "&copy" ) ) ,
                 SIGNAL( triggered( ) ) ,
                 this ,
                 SLOT( menuCopyTriggered( ) ) );

        connect( menu->addAction( tr( "&edit" ) ) ,
                 SIGNAL( triggered( ) ) ,
                 this ,
                 SLOT( menuEditTriggered( ) ) );
    }

    connect( menu->addAction( tr( "&all select" ) ) ,
             SIGNAL( triggered( ) ) ,
             this ,
             SLOT( menuSelectAllTriggered( ) ) );


    connect( menu->addAction( tr( "&goto address" ) ) ,
             SIGNAL( triggered( ) ) ,
             this ,
             SLOT( menuGotoByteTriggered( ) ) );

    connect( menu->addAction( tr( "&goto line" ) ) ,
             SIGNAL( triggered( ) ) ,
             this ,
             SLOT( menuGotoLineTriggered( ) ) );




    //�ò˵���ʾ��λ��������������
    menu->move( cursor( ).pos( ) ); 
    menu->show( );
}

void HexTextEditor::menuCopyTriggered( )
{
    if( mSelect.isEmpty( ) )
        return;

    QClipboard *clipboard = QApplication::clipboard( );   //��ȡϵͳ������ָ��  
    //QString originalText = clipboard->text( );         //��ȡ���������ı���Ϣ  

    // ��ȡѡ������
    QString buff;
    getSelectionText( buff );
    clipboard->setText( buff );                  //���ü���������</span>  

}

void HexTextEditor::menuSelectAllTriggered( )
{
    if( mHexData.isEmpty( ) )
        return;

    setSelection( 0 , mHexData.size( ) );
}

void HexTextEditor::menuGotoByteTriggered( )
{
    if( mHexData.isEmpty( ) )
        return;

    bool isOK;
    QString text = QInputDialog::getText( NULL , 
                                          tr("goto") ,
                                          tr( "input address" ) ,
                                          QLineEdit::Normal ,
                                          "" ,
                                          &isOK );
    if( isOK ) {
        int pos = text.toInt( );
        setSelection( pos , pos + 1 );
        setVisualPos( pos );
    }
}

void HexTextEditor::menuGotoLineTriggered( )
{
    if( mHexData.isEmpty( ) )
        return;

    bool isOK;
    QString text = QInputDialog::getText( NULL , 
                                          tr("goto") ,
                                          tr( "input linenumber" ) ,
                                          QLineEdit::Normal ,
                                          "" ,
                                          &isOK );
    if( isOK ) {
        int pos = text.toInt( );
        setVisualPos( pos*LINE_ITEM_COUNT );
    }
}


void HexTextEditor::menuEditTriggered( )
{
    if( mHexData.isEmpty( ) )
        return;

    // �����༭��
    QString text;
    getSelectionText( text );

    HexEditDialog dlg( this );
    if( !dlg.exec( ( mSelectEndIndex - mSelectBeginIndex ) * 3 - 1 , text ) )
        return;

    QStringList hexList = text.split( ' ');

    int j = mSelectBeginIndex < mSelectEndIndex ? mSelectBeginIndex : mSelectEndIndex;

    for( auto &i : hexList ) {
        
        edit( j , i.toInt( 0,16) );
        ++j;
    }

    viewport( )->repaint( );
}



void HexTextEditor::scrollbarMove( int nStep )
{
    if( nStep<0 || nStep>mMaxLine )
        return;

    if(nStep>mCurLine )
        emit    wheelDown( nStep - mCurLine );
    else
        emit    wheelUp( mCurLine - nStep );

    mCurLine = nStep;
    viewport( )->repaint( );
}

// ��������
void HexTextEditor::setFont(const QFont &font )
{
    /**
      * ����ĸı�, ��Ӱ�쵽�к���, ʮ�������ı������ַ������Ŀ��
      */
    QAbstractScrollArea::setFont( font ) ;
    if( mFontMetrics )
        delete mFontMetrics;

    mFontMetrics = new QFontMetrics( font );
    mLineHeight = mFontMetrics->height( ) ;
    mCharWidth = mFontMetrics->width( 'W' );

    // ����ʾ���������
    mMaxVisualLine = viewport( )->height( ) ;
    mMaxVisualLine /= mLineHeight;

    // �к����Ŀ��
    mLineNumberWidth = 6 * mCharWidth ;
    // ʮ�������ı����Ŀ��
    mHexTextWidth = LINE_ITEM_COUNT * 3 * mCharWidth + 5;
    // �ַ������Ŀ��
    mStringWidth = LINE_ITEM_COUNT * mCharWidth;

    // ���ò�������󴰿ں���С���ڴ�С
    setMinimumWidth( getCharWidth( ) * 10 + mHexTextWidth + mStringWidth );
    setMaximumWidth( getCharWidth( ) * 10 + mHexTextWidth + mStringWidth );
}


//************************************
// ������: setVisualPos
// ��  ��: ��һ����ַ���õ�����ʾ����
// ����ֵ: void
// ��  ��: int index
//************************************
void HexTextEditor::setVisualPos( int pos )
{
    pos /= LINE_ITEM_COUNT;
    if( pos < 0 )
        pos = 0;
    if( pos >= mMaxLine )
        pos = mMaxLine ;

    mCurLine = pos;
    viewport( )->repaint( );
}

int HexTextEditor::getVisualPos( ) const
{
    return mCurLine * LINE_ITEM_COUNT;
}


// ���ص�ǰ����ʾ�еĿ�ʼ��ַ
int HexTextEditor::getVisualLine( ) const
{
    return mCurLine;
}

// ��ָ��������Ϊ����ʾ(�����Ե�һ��)
void HexTextEditor::setVisualLine( int line )
{
    if( line<0 || line>mMaxLine )
        return;
    mCurLine = line;
    viewport( )->repaint( );
}


int HexTextEditor::positionToLineNumber( int pos ) const
{
    return pos / LINE_ITEM_COUNT;
}

int HexTextEditor::lineToPosition( int line ) const
{
    return line * LINE_ITEM_COUNT;
}


int HexTextEditor::mousePosToIndex( const QPoint& pos )
{
    if( isOnHexTextColumn( pos ) ) {

        int line = ( pos.y( ) - mLineHeight - 5 ) / mLineHeight + mCurLine;
        int row = ( pos.x( ) - mLineNumberWidth + mCharWidth * 2 ) / mCharWidth / 3 ;
        
        return line * LINE_ITEM_COUNT + row;
    }
    return -1;
}


bool HexTextEditor::posIsOnVisual( int pos ) const
{
    return lineIsOnVisual( positionToLineNumber( pos ) );
}

bool HexTextEditor::lineIsOnVisual( int line ) const
{
    return  line >= mCurLine && line < mCurLine + mMaxVisualLine;
}


// ��ȡ����ʾ�������
int HexTextEditor::getMaxVisualLine( ) const
{
    return mMaxVisualLine;
}

// ��ȡһ�еĸ߶�,�߶�������Ϊ��λ
int HexTextEditor::getLineHeight( ) const
{
    return mFontMetrics->height( );
}

// ��ȡһ���ַ��Ŀ��(������Ϊ��λ)
int HexTextEditor::getCharWidth( ) const
{
    return mFontMetrics->width( 'W' );
}

// ��ȡ��ǰ����������һ�ε�����
int HexTextEditor::getRollStep( ) const
{
    return mRollStep;
}

// ���õ�ǰ����������һ�ε�����
void HexTextEditor::setRollStep( int rollStep )
{
    mRollStep = rollStep;
}

// ����Ҫ��ʾ�Ķ���������
void HexTextEditor::setHexData( const QByteArray& data )
{
    mHexData = data;
    int nSize = 0;
    nSize = data.size( );
    if( data.size( ) % LINE_ITEM_COUNT >= 0 ) {
        mMaxLine = data.size( ) / LINE_ITEM_COUNT + 1;
    }
    else
        mMaxLine = data.size( ) ;
    mCurLine = 0;

    mScrollBar->setRange( 0 , mMaxLine );
    // �ػ�
    viewport( )->repaint( );
}


//************************************
// ������: getLineNumberColumnWidth
// ��  ��: ��ȡ�к������ܿ��,���������Ϊ��λ
// ����ֵ: int
//************************************
int HexTextEditor::getLineNumberColumnWidth( ) const
{
    return getNumerOfDigit( ( mCurLine + mMaxVisualLine ) * mLineHeight );
}


// ��䱻ѡ�е�����ı���ɫ
void HexTextEditor::paintSelectRect( QPainter& painter )
{
    int nStartY = mLineHeight + 5;
    QRect  rect;
    QBrush brush( mColorSelect );

    int nBegLine = 0 , nCount = 0;
    int lineCount = 0;
    for( auto &i : mSelect ) {

        if( i.mCount == 0 || i.mLineNumber-mCurLine > mMaxVisualLine)
            continue;

        nBegLine = i.mLineNumber;
        nCount = i.mCount;
        lineCount = nCount / 16;
       

        if( nCount > LINE_ITEM_COUNT ) { // �ַ���������һ�еĸ���
            nCount = LINE_ITEM_COUNT;
        }
      
        if( nBegLine < mCurLine ) {
            if( nBegLine + lineCount < mCurLine) {
                continue;
            }
            else {
                lineCount = nBegLine + lineCount - mCurLine;
                nBegLine = mCurLine;
            }
        }

        if( lineCount > mMaxVisualLine )
            lineCount = mMaxVisualLine;
        else if(lineCount==0 )
            lineCount = 1;


        // ����ʮ�������ı�ȥ�ı���
        painter.fillRect( mLineNumberWidth + mCharWidth * i.mRow * 3 ,
                          nStartY + mLineHeight * ( nBegLine - mCurLine ) ,
                          mCharWidth * nCount * 3 - mCharWidth , // �п�
                          mLineHeight * lineCount , // �и�
                          brush );


        // �����ַ������ı���
        painter.fillRect( mLineNumberWidth + mHexTextWidth + mCharWidth * i.mRow ,
                          nStartY + mLineHeight*( nBegLine - mCurLine ) ,
                          mCharWidth * nCount ,
                          mLineHeight* lineCount ,
                          brush );
    }
}

// ���Ƴ��к�
void HexTextEditor::paintLineNumber( QPainter& painter )
{
    int nLineNumerDigitCount = getNumerOfDigit( ( mCurLine + mMaxVisualLine ) * LINE_ITEM_COUNT );
    
    // ��ˢ
    painter.setBrush( QBrush( mBKColorLineNumber ) );
    // ���Ʊ���
    painter.fillRect( QRect( 0 , 0 , mLineNumberWidth , viewport( )->height( ) ) , QBrush( mBKColorLineNumber ) );

    QRect   pos( 0 , mLineHeight + 5 , mLineNumberWidth - 5 , mLineHeight );
    QString text;

    // 
    painter.setPen( mColorLineNumer );
    for( int i = 1; i <= mMaxVisualLine && i + mCurLine <= mMaxLine; ++i ) {

        text.sprintf( "%0*X" , nLineNumerDigitCount , ( i + mCurLine - 1 ) * LINE_ITEM_COUNT );

        painter.drawText( pos , Qt::AlignRight , text );
        pos.setY( pos.y( ) + mLineHeight ) ; // �л�����һ��
        pos.setBottom( pos.bottom( ) + mLineHeight );
    }
}

// ����ʮ�������ַ���
void HexTextEditor::paintHexText( QPainter& painter )
{
    // ����ʮ�������ı���
    /*
          |00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F|0123456789ABCDEF
          ------+-----------------------------------------------+----------------
          �к�  |                                               |
          */


    QBrush  brush;
    // ���Ʒָ���
    QPen    pen( QColor( 0 , 162 , 232 ) , 3 );
    painter.setPen( pen );
    painter.drawLine( 0 , mLineHeight + 4 , viewport( )->width( ) , mLineHeight + 4 );


    pen.setColor( Qt::black );
    // �����������
    painter.drawText( QPoint( mLineNumberWidth , mLineHeight ) ,
                      "00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F" );



    // ��ȡ��Ҫת���Ķ������ִ��Ŀ�ʼ��ַ
    int     currentByte = mCurLine * LINE_ITEM_COUNT;

    char    hexBuff[ 3 * LINE_ITEM_COUNT + 1 ];
    int     nIndex = mCurLine * LINE_ITEM_COUNT;
    char    *pData = mHexData.data( );

    int     nSize = 0;
    nSize = mHexData.size( ) - mCurLine * LINE_ITEM_COUNT;

    painter.setPen( mColorHexText );
    painter.setBrush( QBrush( mBKColorHexText ) );
    painter.setFont( this->font( ) );
    for( int i = mCurLine , line = 0; line < mMaxVisualLine && i < mMaxLine; ++i , ++line ) {

        hexToString( pData + nIndex ,
                     nSize >= LINE_ITEM_COUNT ? LINE_ITEM_COUNT : nSize ,
                     hexBuff );

        paintHexTextLine( painter ,
                          line , // �к�
                          0 ,    // �к�
                          hexBuff ,
                          ( nSize >= LINE_ITEM_COUNT ? LINE_ITEM_COUNT : nSize ) * 3 - 1 ,
                          currentByte + line * LINE_ITEM_COUNT );


        nIndex += LINE_ITEM_COUNT;
        nSize -= LINE_ITEM_COUNT;
    }
}


//************************************
// ������: paintHexTextLine
// ��  ��: ����һ��ʮ�������ַ���
// ����ֵ: void
// ��  ��: QPainter & painter 
// ��  ��: int line  �к�
// ��  ��: int row   �к�
// ��  ��: const char * pStr Ҫ���Ƶ��ַ���
// ��  ��: int nSize         �ַ����ĳ���
// ��  ��: int absolutePos   �����ַ����ڶ������ֽ����еľ��Ե�ַ(�������ֽ���ָ����mHexData)         
//************************************
void HexTextEditor::paintHexTextLine( QPainter& painter ,
                                      int line , 
                                      int row ,  
                                      const char * pStr , 
                                      int    nSize ,
                                      int    absolutePos )
{

    TokenList::Token* pToken = nullptr;
    char    buff[ LINE_ITEM_COUNT * 3 + 1 ];

    int     tokenPos = 0;
    nSize /= 3;


    for( int i = 0; i < nSize; ) {

        if( nullptr != ( pToken = mTokenList.getToken( i + absolutePos ) ) ) {

            painter.setPen( pToken->mFontColor );
            pToken->mFont.setPixelSize( this->font( ).pixelSize( ) );
            painter.setFont( pToken->mFont );
            strncpy( buff , pStr + i * 3 , pToken->mLen * 3 );
            drawText( painter ,
                      buff ,
                      pToken->mLen * 3 ,
                      e_hexText ,
                      line ,
                      row + i );

            // �����Ѿ�������Item��
            i += pToken->mLen;

            // �ָ�ԭ���Ļ�ˢ��������.
            painter.setPen( mColorHexText );
            painter.setBrush( QBrush( mBKColorHexText ) );
            painter.setFont( this->font( ) );
            continue;
        }

        tokenPos = i + absolutePos;
        while( nullptr == ( pToken = mTokenList.getToken( tokenPos ) ) ) {
            if( tokenPos >= nSize + absolutePos ) {
                ++tokenPos;
                break;
            }
            ++tokenPos;
        }


        int nBufflen = tokenPos - i - absolutePos;
        strncpy( buff , pStr + i * 3 , nBufflen * 3 );
        drawText( painter ,
                  buff ,
                  nBufflen * 3 ,
                  e_hexText ,
                  line ,
                  row + i );

        i += nBufflen;
    }

}


//************************************
// ������: drawText
// ��  ��: ��ָ�����ϵ�ָ����ָ���л���һ���ַ���
// ����ֵ: void
// ��  ��: QPainter & painter
// ��  ��: const char * pStr  �ַ���
// ��  ��: int nLen           �ַ�������
// ��  ��: ColumnCode column  Ҫ���Ƶ���
// ��  ��: int line           �к�
// ��  ��: int row            �к�
//************************************
void HexTextEditor::drawText( QPainter& painter ,
                              const char *pStr ,
                              int   nLen ,
                              ColumnCode column,
                              int line ,
                              int row )
{
    int startPos = 0;
    int scal = 0;
    switch( column ) {
        case e_lineNumber:startPos = 0; scal = 1; break;
        case e_hexText:startPos = mLineNumberWidth; scal = 3; break;
        case e_string:startPos = mLineNumberWidth + mHexTextWidth; scal = 1; break;
        default: return;
    }

    QRect pos( startPos + row*mCharWidth  * scal ,
               line*mLineHeight + ( mLineHeight + 5 ) ,
               nLen*mCharWidth ,
               mLineHeight );



    painter.drawText( pos , pStr );
}


// �����ַ�����
void HexTextEditor::paintString( QPainter& painter )
{
    // ����λ��
    QPen    pen;

    pen.setColor( Qt::black );
    // �����������
    painter.drawText( QPoint( mLineNumberWidth + mHexTextWidth , mLineHeight ) , "0123456789ABCDEF" );

    QRect   pos( mLineNumberWidth + mHexTextWidth ,
                 mLineHeight + 5 ,
                 mStringWidth ,
                 mLineHeight );


    char    hexBuff[ LINE_ITEM_COUNT * 3 + 1 ];

    int     nIndex = mCurLine * LINE_ITEM_COUNT;
    char    *pData = mHexData.data( );

    int     nSize = 0;
    nSize = mHexData.size( ) - mCurLine * LINE_ITEM_COUNT;

    for( int i = mCurLine , j = 1; j <= mMaxVisualLine && i < mMaxLine; ++i , ++j ) {

        getHexString( pData + nIndex ,
                      nSize >= LINE_ITEM_COUNT ? LINE_ITEM_COUNT : nSize ,
                      hexBuff );
        nIndex += LINE_ITEM_COUNT;
        nSize -= LINE_ITEM_COUNT;

        painter.drawText( pos , hexBuff );

        pos.setY( pos.y( ) + mLineHeight ) ; // �л�����һ��
        pos.setBottom( pos.bottom( ) + mLineHeight );
    }
}


//************************************
// ������: isOnHexTextColumn
// ��  ��: ����һ�������Ƿ���ͣ��ʮ�������ı�����Χ��
// ����ֵ: bool
// ��  ��: const QPoint & pos
//************************************
bool HexTextEditor::isOnHexTextColumn( const QPoint& pos )
{
    return   pos.y( ) >= mLineHeight + 5
        && pos.x( ) > mLineNumberWidth && pos.x( ) < mLineNumberWidth + mHexTextWidth;
}

//************************************
// ������: isOnLineNumberColumn
// ��  ��: ����һ�������Ƿ���ͣ���к����ķ�Χ��
// ����ֵ: bool
// ��  ��: const QPoint & pos
//************************************
bool HexTextEditor::isOnLineNumberColumn( const QPoint& pos )
{
    return pos.y( ) >= mLineHeight + 5
        && pos.x( ) < mLineNumberWidth;
}

//************************************
// ������: isOnStringColumn
// ��  ��: ����һ�������Ƿ���ͣ���ַ������ķ�Χ��
// ����ֵ: bool
// ��  ��: const QPoint & pos
//************************************
bool HexTextEditor::isOnStringColumn( const QPoint& pos )
{
    return  pos.y( ) >= mLineHeight + 5
        && pos.x( ) >= mLineNumberWidth + mHexTextWidth && pos.x( ) < viewport( )->width( ) - 20;
}

// ��ն��������ݺ���ɫ��
void HexTextEditor::cleaer( )
{
    mMaxLine = mCurLine = 0;
    mHexData.clear( );
    mTokenList.clear( );
    mSelect.clear( );
}


// ����ѡ������
void HexTextEditor::setSelection( int nBeginIndex , int nEndIndex )
{
    if( mHexData.isEmpty( ) )
        return ;

    if( nBeginIndex < 0
        || nEndIndex < 0
        || nEndIndex < nBeginIndex
        || nEndIndex - 1 > mHexData.size( ) )
        return;

    mSelect.clear( );
    SelectRangle select;

  
    // ѡ�е���
    if( nEndIndex / LINE_ITEM_COUNT == nBeginIndex / LINE_ITEM_COUNT ) { // �ж��Ƿ���ͬһ��
        select.mRow = nBeginIndex % LINE_ITEM_COUNT; // �������ʼ���ֵ���

        select.mCount = nEndIndex - nBeginIndex;// �����ѡ�и���

        select.mLineNumber = nBeginIndex / LINE_ITEM_COUNT; // �����ѡ��λ�õ��к�

        mSelect.push_back( select );

        nBeginIndex += select.mCount;
    }
    else {

        // ѡ�ж���ʱ , �����в�ֳ�������
        // ��ʼλ�ò���16����,������Ϊһ����.
        // ����λ�ò���16����,������Ϊһ����.
        // ��ʼ����ͽ������깹��һ���еĵ�����Ϊһ����.

        if( nBeginIndex % LINE_ITEM_COUNT != 0 ) {
            // �õ���ͷ����
            select.mRow = nBeginIndex % LINE_ITEM_COUNT; // �������ʼ���ֵ���
            select.mCount = ( LINE_ITEM_COUNT - select.mRow );// �����ѡ�и���
            select.mLineNumber = nBeginIndex / LINE_ITEM_COUNT; // �����ѡ��λ�õ��к�
            mSelect.push_back( select );

            nBeginIndex += LINE_ITEM_COUNT - select.mRow;
        }
        if( nEndIndex % LINE_ITEM_COUNT != 0 ) {

            // �õ���β����
            select.mRow = 0; // �������ֵ��к�
            select.mCount = nEndIndex % LINE_ITEM_COUNT; // �������ֵ�ѡ�и���
            select.mLineNumber = nEndIndex / LINE_ITEM_COUNT ;// �������ֵ��к�
            mSelect.push_back( select );
            nEndIndex -= select.mCount;
        }
    }
  

    if( nBeginIndex < nEndIndex ) {

        select.mLineNumber = nBeginIndex / LINE_ITEM_COUNT;
        select.mRow = 0; // ����ѡ��,��˿�ʼ����Ϊ0
        select.mCount = nEndIndex - nBeginIndex ; // ����ѡ��,��˸�����һ���еı���
        mSelect.push_back( select );
    }

    viewport( )->repaint( );
}

// ��ȡѡ��������ı�
bool HexTextEditor::getSelectionText( QString& selectText )
{
    if( mHexData.isEmpty( ) )
        return false;

    int count = 0;
    if( mSelect.isEmpty( ) )
        return false;

    for( auto& i : mSelect ) {
        count += i.mCount;
    }

    char* buff = new char[ count * 3 + 1 ];
    // ���кź��к�ת���ɻ�����������
    int nIndex = mSelect.begin( )->mLineNumber*LINE_ITEM_COUNT + mSelect.begin( )->mRow;

    hexToString( mHexData.data( ) + nIndex ,
                 count ,
                 buff
                 );

    selectText = buff;

    delete[ ] buff;
    return true;
}

// ���һ����ɫ��
void HexTextEditor::addToken( const TokenList::Token& token )
{
    if( mTokenList.addToken( token ) )
        viewport( )->repaint( );

}

// ������ɫ��
bool HexTextEditor::updateToken( const TokenList::Token& token )
{
    if( mTokenList.updateToken( token ) ) {
        viewport( )->repaint( );
        return true;
    }
    return false;
}

// �����ɫ��
void HexTextEditor::clearToken( )
{
    mTokenList.clear( );
}

bool HexTextEditor::edit( int nIndex , const char data )
{
    if( mHexData.isEmpty( ) || nIndex>=mHexData.size())
        return false;

    mEditUndoStack.push( nIndex , mHexData.data( )[ nIndex ] );
    mHexData.data( )[ nIndex ] = data;
    return true;
}

void HexTextEditor::undo( )
{
    HexEidtStack::NODE node ;
    if( mEditUndoStack.pop( node ) ) {

        char* pData = mHexData.data( );
        mEditRedoStack.push( node.index , pData[ node.index ] );

        pData[ node.index ] = (char)node.data;
        viewport( )->repaint( );
    }
}

// ��ʱ������
void HexTextEditor::redo( )
{
    HexEidtStack::NODE node ;
    if( mEditRedoStack.pop( node ) ) { 

        char* pData = mHexData.data( );
        mEditUndoStack.push( node.index , pData[ node.index ] );
        pData[ node.index ] = (char)node.data;
        viewport( )->repaint( );
    }
}

QByteArray HexTextEditor::getRowData( ) const
{
    return mHexData;
}


QColor HexTextEditor::getColorSelect() const
{
    return mColorSelect;
}

void HexTextEditor::setColorSelect(const QColor &colorSelect)
{
    mColorSelect = colorSelect;
}

QColor HexTextEditor::getBKColor() const
{
    return mBKColorHexText;
}

void HexTextEditor::setBKColor(const QColor &bKColor)
{
    mBKColorHexText = bKColor;
}

QColor HexTextEditor::getFontColor() const
{
    return mColorHexText;
}

void HexTextEditor::setFontColor(const QColor &fontColor)
{
    mColorHexText = fontColor;
}

QColor HexTextEditor::getBKColorLineNumber() const
{
    return mBKColorLineNumber;
}

void HexTextEditor::setBKColorLineNumber(const QColor &bKColorLineNumber)
{
    mBKColorLineNumber = bKColorLineNumber;
}

QColor HexTextEditor::getColorLineNumer() const
{
    return mColorLineNumer;
}

void HexTextEditor::setColorLineNumer(const QColor &colorLineNumer)
{
    mColorLineNumer = colorLineNumer;
}


