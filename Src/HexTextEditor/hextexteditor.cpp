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

    *( --pBuff ) = 0; // 倒数第二个是空格,将它去掉
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
// 函数名: paintEvent
// 功  能: 窗口绘制事件
// 返回值: void
// 参  数: QPaintEvent * event
//************************************
void HexTextEditor::paintEvent( QPaintEvent *event )
{
    event;

    QPainter painter( this->viewport( ) );


    // 将背景刷成白色
    painter.fillRect( painter.viewport( ) , QBrush( mBKColorHexText ) );

   

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // 绘制选中区域
    paintSelectRect( painter );

    if( mHexData.isEmpty( ) )
        return;


    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // 绘制行号
    paintLineNumber( painter );


    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // 绘制十六进制字符串
    paintHexText( painter );


    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // 绘制字符串
    paintString( painter );

}

// 滚动条滚动事件
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

        // 向下移动滚动条
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
        // 向上移动滚动条
        for( int i = 0; i < mRollStep&& mCurLine <= mMaxLine; ++i ) {
            //verticalScrollBar( )->triggerAction( QAbstractSlider::SliderSingleStepAdd );
            mCurLine++;
        }
        mScrollBar->setSliderPosition( mCurLine );
    }

    //this->viewport( )->update( );
}

// 窗口大小改变事件
void HexTextEditor::resizeEvent( QResizeEvent * event )
{
    event;
    mMaxVisualLine = viewport( )->height( ) / mLineHeight ;


    mScrollBar->move( viewport()->width() - 10 , 0 );
    mScrollBar->resize( 10 , viewport( )->height( ) );
}

// 鼠标移动事件
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

        // 将坐标转行成行号, 并发出信号
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

// 鼠标按键按下事件
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

// 鼠标按键弹起事件
void HexTextEditor::mouseReleaseEvent( QMouseEvent * e )
{

    if( e->button( ) == Qt::LeftButton ) {
        if( isOnHexTextColumn( e->pos( ) ) ) {

\
            mMouseLbuttonStatus = false;

            // 将鼠标按下点转换为行列
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


// 键盘按键按下事件
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
                // 向上移动滚动条

                mCurLine++;
                mScrollBar->setSliderPosition( mCurLine );
            }
            break;
            case Qt::Key_Up:
            {
                if( mCurLine <= 0 )
                    return;

                emit wheelUp( mRollStep );

                // 向下移动滚动条
                mCurLine--;
                mScrollBar->setSliderPosition( mCurLine );
            }
        }
    }

}

//************************************
// 函数名: contextMenuEvent
// 功  能: 菜单事件
// 返回值: void
// 参  数: QContextMenuEvent * event
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




    //让菜单显示的位置在鼠标的坐标上
    menu->move( cursor( ).pos( ) ); 
    menu->show( );
}

void HexTextEditor::menuCopyTriggered( )
{
    if( mSelect.isEmpty( ) )
        return;

    QClipboard *clipboard = QApplication::clipboard( );   //获取系统剪贴板指针  
    //QString originalText = clipboard->text( );         //获取剪贴板上文本信息  

    // 获取选中内容
    QString buff;
    getSelectionText( buff );
    clipboard->setText( buff );                  //设置剪贴板内容</span>  

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

    // 弹出编辑框
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

// 设置字体
void HexTextEditor::setFont(const QFont &font )
{
    /**
      * 字体的改变, 会影响到行号栏, 十六进制文本栏和字符串栏的宽度
      */
    QAbstractScrollArea::setFont( font ) ;
    if( mFontMetrics )
        delete mFontMetrics;

    mFontMetrics = new QFontMetrics( font );
    mLineHeight = mFontMetrics->height( ) ;
    mCharWidth = mFontMetrics->width( 'W' );

    // 可显示的最大行数
    mMaxVisualLine = viewport( )->height( ) ;
    mMaxVisualLine /= mLineHeight;

    // 行号栏的宽度
    mLineNumberWidth = 6 * mCharWidth ;
    // 十六进制文本栏的宽度
    mHexTextWidth = LINE_ITEM_COUNT * 3 * mCharWidth + 5;
    // 字符串栏的宽度
    mStringWidth = LINE_ITEM_COUNT * mCharWidth;

    // 设置部件的最大窗口和最小窗口大小
    setMinimumWidth( getCharWidth( ) * 10 + mHexTextWidth + mStringWidth );
    setMaximumWidth( getCharWidth( ) * 10 + mHexTextWidth + mStringWidth );
}


//************************************
// 函数名: setVisualPos
// 功  能: 将一个地址设置到在显示区域
// 返回值: void
// 参  数: int index
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


// 返回当前可显示行的开始地址
int HexTextEditor::getVisualLine( ) const
{
    return mCurLine;
}

// 将指定行设置为可显示(并处以第一行)
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


// 获取可显示的最大函数
int HexTextEditor::getMaxVisualLine( ) const
{
    return mMaxVisualLine;
}

// 获取一行的高度,高度以像素为单位
int HexTextEditor::getLineHeight( ) const
{
    return mFontMetrics->height( );
}

// 获取一个字符的宽度(以像素为单位)
int HexTextEditor::getCharWidth( ) const
{
    return mFontMetrics->width( 'W' );
}

// 获取当前滚动条滚动一次的行数
int HexTextEditor::getRollStep( ) const
{
    return mRollStep;
}

// 设置当前滚动条滚动一次的行数
void HexTextEditor::setRollStep( int rollStep )
{
    mRollStep = rollStep;
}

// 设置要显示的二进制数据
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
    // 重绘
    viewport( )->repaint( );
}


//************************************
// 函数名: getLineNumberColumnWidth
// 功  能: 获取行号栏的总宽度,宽度以像素为单位
// 返回值: int
//************************************
int HexTextEditor::getLineNumberColumnWidth( ) const
{
    return getNumerOfDigit( ( mCurLine + mMaxVisualLine ) * mLineHeight );
}


// 填充被选中的区域的背景色
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
       

        if( nCount > LINE_ITEM_COUNT ) { // 字符个数超出一行的个数
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


        // 绘制十六进制文本去的背景
        painter.fillRect( mLineNumberWidth + mCharWidth * i.mRow * 3 ,
                          nStartY + mLineHeight * ( nBegLine - mCurLine ) ,
                          mCharWidth * nCount * 3 - mCharWidth , // 行宽
                          mLineHeight * lineCount , // 行高
                          brush );


        // 绘制字符串区的背景
        painter.fillRect( mLineNumberWidth + mHexTextWidth + mCharWidth * i.mRow ,
                          nStartY + mLineHeight*( nBegLine - mCurLine ) ,
                          mCharWidth * nCount ,
                          mLineHeight* lineCount ,
                          brush );
    }
}

// 绘制出行号
void HexTextEditor::paintLineNumber( QPainter& painter )
{
    int nLineNumerDigitCount = getNumerOfDigit( ( mCurLine + mMaxVisualLine ) * LINE_ITEM_COUNT );
    
    // 画刷
    painter.setBrush( QBrush( mBKColorLineNumber ) );
    // 绘制背景
    painter.fillRect( QRect( 0 , 0 , mLineNumberWidth , viewport( )->height( ) ) , QBrush( mBKColorLineNumber ) );

    QRect   pos( 0 , mLineHeight + 5 , mLineNumberWidth - 5 , mLineHeight );
    QString text;

    // 
    painter.setPen( mColorLineNumer );
    for( int i = 1; i <= mMaxVisualLine && i + mCurLine <= mMaxLine; ++i ) {

        text.sprintf( "%0*X" , nLineNumerDigitCount , ( i + mCurLine - 1 ) * LINE_ITEM_COUNT );

        painter.drawText( pos , Qt::AlignRight , text );
        pos.setY( pos.y( ) + mLineHeight ) ; // 切换到下一行
        pos.setBottom( pos.bottom( ) + mLineHeight );
    }
}

// 绘制十六进制字符串
void HexTextEditor::paintHexText( QPainter& painter )
{
    // 绘制十六进制文本框
    /*
          |00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F|0123456789ABCDEF
          ------+-----------------------------------------------+----------------
          行号  |                                               |
          */


    QBrush  brush;
    // 绘制分隔符
    QPen    pen( QColor( 0 , 162 , 232 ) , 3 );
    painter.setPen( pen );
    painter.drawLine( 0 , mLineHeight + 4 , viewport( )->width( ) , mLineHeight + 4 );


    pen.setColor( Qt::black );
    // 绘制数字序号
    painter.drawText( QPoint( mLineNumberWidth , mLineHeight ) ,
                      "00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F" );



    // 获取需要转换的二进制字串的开始地址
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
                          line , // 行号
                          0 ,    // 列号
                          hexBuff ,
                          ( nSize >= LINE_ITEM_COUNT ? LINE_ITEM_COUNT : nSize ) * 3 - 1 ,
                          currentByte + line * LINE_ITEM_COUNT );


        nIndex += LINE_ITEM_COUNT;
        nSize -= LINE_ITEM_COUNT;
    }
}


//************************************
// 函数名: paintHexTextLine
// 功  能: 绘制一行十六进制字符串
// 返回值: void
// 参  数: QPainter & painter 
// 参  数: int line  行号
// 参  数: int row   列号
// 参  数: const char * pStr 要绘制的字符串
// 参  数: int nSize         字符串的长度
// 参  数: int absolutePos   这行字符串在二进制字节流中的绝对地址(二进制字节流指的是mHexData)         
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

            // 增加已经成立的Item数
            i += pToken->mLen;

            // 恢复原来的画刷画笔字体.
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
// 函数名: drawText
// 功  能: 在指定栏上的指定行指定列绘制一串字符串
// 返回值: void
// 参  数: QPainter & painter
// 参  数: const char * pStr  字符串
// 参  数: int nLen           字符串个数
// 参  数: ColumnCode column  要绘制的栏
// 参  数: int line           行号
// 参  数: int row            列号
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


// 绘制字符串栏
void HexTextEditor::paintString( QPainter& painter )
{
    // 设置位置
    QPen    pen;

    pen.setColor( Qt::black );
    // 绘制数字序号
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

        pos.setY( pos.y( ) + mLineHeight ) ; // 切换到下一行
        pos.setBottom( pos.bottom( ) + mLineHeight );
    }
}


//************************************
// 函数名: isOnHexTextColumn
// 功  能: 测试一个坐标是否悬停在十六进制文本栏范围内
// 返回值: bool
// 参  数: const QPoint & pos
//************************************
bool HexTextEditor::isOnHexTextColumn( const QPoint& pos )
{
    return   pos.y( ) >= mLineHeight + 5
        && pos.x( ) > mLineNumberWidth && pos.x( ) < mLineNumberWidth + mHexTextWidth;
}

//************************************
// 函数名: isOnLineNumberColumn
// 功  能: 测试一个坐标是否悬停在行号栏的范围内
// 返回值: bool
// 参  数: const QPoint & pos
//************************************
bool HexTextEditor::isOnLineNumberColumn( const QPoint& pos )
{
    return pos.y( ) >= mLineHeight + 5
        && pos.x( ) < mLineNumberWidth;
}

//************************************
// 函数名: isOnStringColumn
// 功  能: 测试一个坐标是否悬停在字符串栏的范围内
// 返回值: bool
// 参  数: const QPoint & pos
//************************************
bool HexTextEditor::isOnStringColumn( const QPoint& pos )
{
    return  pos.y( ) >= mLineHeight + 5
        && pos.x( ) >= mLineNumberWidth + mHexTextWidth && pos.x( ) < viewport( )->width( ) - 20;
}

// 清空二进制数据和着色器
void HexTextEditor::cleaer( )
{
    mMaxLine = mCurLine = 0;
    mHexData.clear( );
    mTokenList.clear( );
    mSelect.clear( );
}


// 设置选中区域
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

  
    // 选中单行
    if( nEndIndex / LINE_ITEM_COUNT == nBeginIndex / LINE_ITEM_COUNT ) { // 判断是否是同一行
        select.mRow = nBeginIndex % LINE_ITEM_COUNT; // 计算出开始部分的列

        select.mCount = nEndIndex - nBeginIndex;// 计算出选中个数

        select.mLineNumber = nBeginIndex / LINE_ITEM_COUNT; // 计算出选中位置的行号

        mSelect.push_back( select );

        nBeginIndex += select.mCount;
    }
    else {

        // 选中多行时 , 将多行拆分成三部分
        // 开始位置不以16对齐,单独作为一部分.
        // 结束位置不以16对齐,单独作为一部分.
        // 开始坐标和结束坐标构成一整行的单独作为一部分.

        if( nBeginIndex % LINE_ITEM_COUNT != 0 ) {
            // 得到开头部分
            select.mRow = nBeginIndex % LINE_ITEM_COUNT; // 计算出开始部分的列
            select.mCount = ( LINE_ITEM_COUNT - select.mRow );// 计算出选中个数
            select.mLineNumber = nBeginIndex / LINE_ITEM_COUNT; // 计算出选中位置的行号
            mSelect.push_back( select );

            nBeginIndex += LINE_ITEM_COUNT - select.mRow;
        }
        if( nEndIndex % LINE_ITEM_COUNT != 0 ) {

            // 得到结尾部分
            select.mRow = 0; // 结束部分的列号
            select.mCount = nEndIndex % LINE_ITEM_COUNT; // 结束部分的选中个数
            select.mLineNumber = nEndIndex / LINE_ITEM_COUNT ;// 结束部分的行号
            mSelect.push_back( select );
            nEndIndex -= select.mCount;
        }
    }
  

    if( nBeginIndex < nEndIndex ) {

        select.mLineNumber = nBeginIndex / LINE_ITEM_COUNT;
        select.mRow = 0; // 整行选中,因此开始的列为0
        select.mCount = nEndIndex - nBeginIndex ; // 整行选中,因此个数是一整行的倍数
        mSelect.push_back( select );
    }

    viewport( )->repaint( );
}

// 获取选中区域的文本
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
    // 将行号和列号转换成缓冲区索引号
    int nIndex = mSelect.begin( )->mLineNumber*LINE_ITEM_COUNT + mSelect.begin( )->mRow;

    hexToString( mHexData.data( ) + nIndex ,
                 count ,
                 buff
                 );

    selectText = buff;

    delete[ ] buff;
    return true;
}

// 添加一个着色器
void HexTextEditor::addToken( const TokenList::Token& token )
{
    if( mTokenList.addToken( token ) )
        viewport( )->repaint( );

}

// 更新着色器
bool HexTextEditor::updateToken( const TokenList::Token& token )
{
    if( mTokenList.updateToken( token ) ) {
        viewport( )->repaint( );
        return true;
    }
    return false;
}

// 清空着色器
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

// 暂时不靠谱
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


