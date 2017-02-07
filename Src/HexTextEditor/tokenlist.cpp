#include "tokenlist.h"

TokenList::TokenList()
{

}


TokenList::Token* TokenList::getToken( int nPos )
{
    for( auto &i : mTokenList ) {

        if( i.isMe( nPos ) )
            return &i;
    }
    return nullptr;
}

#define  LINE_ITEM_COUNT 16
 
bool TokenList::addToken( const Token& token )
{
    Token* pToken = getToken( token.mBegPos );

    if( pToken ) {
        return false;
    }

    // token中, 有一些会占用几行的空间.
    // 这样给绘制带来非常大的不便, 因此, 在这里, 要将这些占用几行的token
    // 切割成小的token

    // 选中多行
    // 去掉头尾两段不以LINE_ITEM_COUNT个Item对齐的行
    int nBeginIndex = token.mBegPos;
    int nEndIndex = token.mLen + token.mBegPos;

    Token temp = token;
    if( nEndIndex / LINE_ITEM_COUNT - nBeginIndex / LINE_ITEM_COUNT > 0 ) {

        if( nBeginIndex % LINE_ITEM_COUNT != 0 ) {
            // 得到开头部分
            temp.mBegPos = nBeginIndex;
            temp.mLen = ( ( nBeginIndex / LINE_ITEM_COUNT + 1 ) * LINE_ITEM_COUNT ) - nBeginIndex;
            mTokenList << temp;
            nBeginIndex += temp.mLen;
        }
        if( nEndIndex%LINE_ITEM_COUNT != 0 ) {
            // 得到结束部分
            temp.mBegPos = nEndIndex / LINE_ITEM_COUNT * LINE_ITEM_COUNT ;
            temp.mLen = nEndIndex % 16;
            nEndIndex -= temp.mLen;
            mTokenList << temp;
        }
    }

    while( nBeginIndex < nEndIndex ) {

        temp.mBegPos = nBeginIndex;
        temp.mLen = nEndIndex - nBeginIndex>LINE_ITEM_COUNT ? LINE_ITEM_COUNT : nEndIndex - nBeginIndex;
        mTokenList << temp;
        nBeginIndex += temp.mLen;
    }
    return true;
}

void TokenList::clear( )
{
    mTokenList.clear( );
}

bool TokenList::updateToken( const Token& token )
{
    Token* pToken = getToken( token.mBegPos );

    if( pToken ) {
        return false;
    }

    for( auto& i : mTokenList ) {

        if( i.mId == token.mId ) {
            i.mFont = token.mFont;
            i.mFontColor = token.mFontColor;
        }
    }

    return true;
}


int TokenList::Token::mCount;

void TokenList::Token::operator()(int begpos, int len, QColor fontColor, QFont font)
{
    mBegPos = begpos;
    mLen = len;
    mFontColor = fontColor;
    mFont = font;
}
