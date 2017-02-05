#ifndef TOKENLIST_H
#define TOKENLIST_H

#include <QFont>
#include <QColor>




class TokenList
{
public:

    // ��ɫ��
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


        int     mBegPos;    // ��ʼλ��
        int     mLen;       // ����
        QFont   mFont;      // ����
        QColor  mFontColor; //������ɫ
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
