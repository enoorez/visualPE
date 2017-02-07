#ifndef TOKENLIST_H
#define TOKENLIST_H

#include <QFont>
#include <QColor>




class TokenList
{
public:

    // 着色器
    class Token
    {

    public:
        Token( int begpos = 0 ,
               int len = 0 ,
               QColor fontColor = QColor( Qt::black ) ,
               QFont font = QFont( "Consolas" , 10 ) )
               : mBegPos( begpos ) ,
               mLen( len ) ,
               mFont( font ) ,
               mFontColor( fontColor )

        {
            mId = mCount++;
        }
        void operator()( int begpos,
                         int len ,
                         QColor fontColor = QColor( Qt::black ) ,
                         QFont font = QFont( "Consolas" , 10 ) );

        

        int     next( )
        {
            return mBegPos + mLen;
        }

        bool    isMe( int nPos )
        {
            return mBegPos <= nPos && nPos < mBegPos + mLen;
        }

        friend bool    operator==( const Token& l , const Token& r )
        {
            return l.mBegPos == r.mBegPos && l.mLen >= r.mLen;
        }

        friend bool    operator<( const Token& l,  const Token& r )
        {
            return l.mBegPos < r.mBegPos;
        }


        int     mBegPos;    // 开始位置
        int     mLen;       // 长度
        QFont   mFont;      // 字体
        QColor  mFontColor; //字体颜色
        int     mId;
        static  int mCount;
    };


public:
    TokenList();

    bool        addToken(const Token& token);
    bool        updateToken( const Token& token );
    Token*      getToken( int nPos );
    void        clear( );

private:

    QList<Token>    mTokenList;
};

#endif // TOKENLIST_H
