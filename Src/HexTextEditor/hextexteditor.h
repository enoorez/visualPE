/*
 * 十六进制文本编辑器
 * 编辑器由三个文本编辑框组成:
 *  1. 行号框
 *  2. 十六进制文本框
 *  3. ASCII码框
 * 十六进制文本编辑器提供的功能有:
 *  1. 将传入的二进制数据以文本的形式显示
 *  2. 提供修改的功能

 *  部件会发出的信号:
     void    wheelUp( int nLine );    滚动条向上滚动
     void    wheelDown( int nLine );  滚动条向下滚动的那个
     void    mouseOnLineNumer( const QPoint& pos ); 鼠标在行号上
     void    mouseOnHexText( const QPoint& pos );   鼠标在十六进制字符串上
     void    mouseOnString( const QPoint& pos );    鼠标在字符串上
 */
#ifndef HEXTEXTEDITOR_H
#define HEXTEXTEDITOR_H
#include <QAbstractScrollArea>
#include <QWheelEvent>
#include "tokenlist.h"
#include "qscrollbar.h"

class QScrollBar;


#define LINE_ITEM_COUNT 16


// 选中区域结构
typedef struct SelectRangle
{
    int     mLineNumber;
    int     mRow;    // 列号
    int     mCount;  // 选中个数

    void    setRangle( int beginIndex , int endIndex )
    {
        mLineNumber = beginIndex/LINE_ITEM_COUNT;
        mRow = beginIndex % LINE_ITEM_COUNT;
        mCount = endIndex - beginIndex > 16 ? 16 : endIndex - beginIndex;
    }

}SelectRangle;


enum ColumnCode
{
    e_lineNumber, // 行号栏
    e_hexText,// 十六进制文本栏
    e_string,// 字符串栏
};

class HexEidtStack
{
public:
    typedef struct NODE
    {
        int    index;
        int    data;
    };

    bool    isEmpty( )const {return mData.isEmpty(); }
    int     size( )const{return mData.size ();};
    void    clear( ){mData.clear();};
    void    push( int index,unsigned char data ) {mData.push_back(NODE{index,data});}
    bool    pop(NODE& data )
    { 
        if( mData.isEmpty( ) )
            return false;
        data = mData.last( );
        mData.erase( --mData.end( ) );
        return true;
    };

protected:
    QList<NODE> mData;

};

class HexTextEditor : public QAbstractScrollArea
{
    Q_OBJECT

public:
    HexTextEditor( QWidget* parent );


protected:
    /*
     * 行号栏的宽度会随着行号的大小变大而变宽.
     */
    int             getLineNumberColumnWidth( )const;

    /**
      * 行号栏 | 十六进制文本栏 | 字符串栏
      * 一共三个窗口栏, 每个窗口之间的空隙为5个像素点.
      */
    bool            isOnHexTextColumn( const QPoint& pos );
    bool            isOnLineNumberColumn( const QPoint& pos );
    bool            isOnStringColumn( const QPoint& pos );

    virtual void    paintSelectRect( QPainter& painter );
    virtual void    paintLineNumber( QPainter& painter );
    virtual void    paintHexText( QPainter& painter );
    virtual void    paintString( QPainter& painter );
    virtual void    paintHexTextLine( QPainter& painter ,
                                      int line, int row, // 可以在坐标处开始画
                                      const char * pStr ,   // 可以画的字符串(默认是47个字符)
                                      int    nSize ,/*字符串的字符个数*/
                                      int   nIndex  // 字符串首地址在整个缓冲区索引
                                      );
    void            drawText( QPainter& painter ,
                              const char *pStr ,
                              int   nLen ,
                              ColumnCode column ,
                              int line ,
                              int row );

    virtual void    paintEvent( QPaintEvent * event );
    virtual void    wheelEvent( QWheelEvent * e );
    virtual void    resizeEvent( QResizeEvent * event );
    virtual void    mouseMoveEvent( QMouseEvent * e );
    virtual void    mousePressEvent( QMouseEvent * e );
    virtual void    mouseReleaseEvent( QMouseEvent * e );
    virtual void    keyPressEvent( QKeyEvent * e );
    virtual void    keyReleaseEvent( QKeyEvent * e );
    virtual void    contextMenuEvent( QContextMenuEvent *event );




public:
    static int      getNumerOfDigit( int num );
    static void     hexToString( const char* pHex , int nLen , char* pBuff );
    static void     getHexString( const char* pHex , int nLen , char* pBuff );


signals:
    void            wheelUp( int nLine );  // 滚动条向上滚动时
    void            wheelDown( int nLine );// 滚动条向下滚动时
    void            mouseOnLineNumer( const QPoint& pos ); // 鼠标经过行号栏时
    void            mouseOnHexText( int  line , int row ); // 鼠标经过十六进制文本栏时
    void            mouseOnString( int  line , int row );  // 鼠标经过字符串栏时


public slots:
    void            menuCopyTriggered( );  // 复制十六进制文本
    void            menuSelectAllTriggered( ); // 全选
    void            menuGotoByteTriggered(); // 跳转到指定地址
    void            menuGotoLineTriggered( ); // 跳转到指定行
    void            menuEditTriggered( ); // 编辑
    void            scrollbarMove( int nStep );

public:

    // 设置字体
    void            setFont(const QFont& font );
    void            setVisualPos(int pos );
    int             getVisualPos( )const;

    int             getVisualLine( )const; 
    void            setVisualLine( int line );

    bool            posIsOnVisual( int pos )const;
    bool            lineIsOnVisual( int line)const;

    int             positionToLineNumber(int pos )const;
    int             lineToPosition( int line )const;

    int             mousePosToIndex(const QPoint& pos );


    int             getMaxVisualLine( )const; 

    int             getLineHeight( )const;
    int             getCharWidth( )const ;

    // 设置滚动条滚动一次的行数
    int             getRollStep( ) const;
    void            setRollStep( int rollStep );

    void            setHexData( const QByteArray& Data );


    // 设置选中区域参数为: [开始地址,结束地址)
    void            setSelection( int nBeginIndex , int nEndIndex );
    bool            getSelectionText(QString& selectText );
    void            addToken( const TokenList::Token& token );
    bool            updateToken( const TokenList::Token& token );
    void            clearToken( );
    void            cleaer( );


public:

    bool            edit( int nIndex , const char data );
    void            undo( ); // 撤销
    void            redo( ); // 重做

protected:
    QByteArray      mHexData;   // 二进制数据(用于显示)


    QList<SelectRangle>    mSelect; // 被选中的行

    int             mCurLine;  // 编辑器可视部分的行号
    int             mMaxLine;  // 文本内容实际的最大行数
    int             mMaxVisualLine; // 编辑器能可视部分的最大行数

    int             mRollStep ; // 滚动条每次滚动的函数

    int             mLineHeight; // 行高
    int             mCharWidth; // 字宽

    QColor          mColorLineNumer; // 行号的颜色 
    QColor          mBKColorLineNumber;// 行号的背景颜色
    QColor          mFontColor; // 十六进制文本字体颜色
    QColor          mBKColor; // 十六进制文本栏的背景颜色
    QColor          mColorSelect; // 选中区域的背景颜色

    QFontMetrics*   mFontMetrics; // 字体信息

    TokenList       mTokenList; // 着色器


private:
    bool            mControlKeyStatus;
    HexEidtStack    mEditUndoStack; 
    HexEidtStack    mEditRedoStack;

    QScrollBar*     mScrollBar;
    bool            mMouseLbuttonStatus; //true : 鼠标弹起, false: 鼠标按下
    int             mSelectBeginIndex; // 选中区域的开始字节
    int             mSelectEndIndex;// 选中区域的结束字节

    int             mLineNumberWidth; // 行号栏的宽度
    int             mHexTextWidth; // 十六进制文本栏的宽度
    int             mStringWidth; // 字符串栏的宽度
};

#endif // HEXTEXTEDITOR_H
