#include "TypeTree.h"
#include "Src/HexTextEditor/hextexteditor.h"
#include <QTreeWidgetItem>
#include <QTreeWidget>


TypeTree::TypeTree( QTreeWidget* tree , HexTextEditor* edit )
    :mTree( tree ) , mEdit( edit )
{

}


TypeTree::~TypeTree( )
{ 
    clear( );
}




Field* TypeTree::addField( FieldType type ,
                           const QString& name , /* �ֶ��� */
                           int nIndex , /* �ֶ��ڻ������ڵ�ƫ�� */
                           int nSize , /* �ֶ�ռ���ֽ��� */
                           const QString& comment /* �ֶε�ע�� */ ,
                           const QVariant& value ,    // �ֶε�ֵ
                           Field* parent ,
                           TokenList::Token* pToken,
                           int userData,
                           int userDataSize,
                           int userDataType )
{
    // ���TreeWidget�ڵ�
    QTreeWidgetItem* item = new QTreeWidgetItem;

    // ���HexTextEditor��ɫ��
    TokenList::Token token( nIndex , nSize );
    if( pToken ) {
        token = *pToken;
        token.mBegPos = nIndex;
        token.mLen = nSize;
        mEdit->addToken( token );
    }
    Field*  field = new Field( type , name , comment , value , token , item , parent );

    // �����Ƿ��и��ڵ�ѡ���ԵĽ��в���
    if( parent ) {
        parent->item->addChild( item );
    }
    else
        mTree->addTopLevelItem( item );

    // ���ýڵ���ı���Ϣ
    item->setText( 0 , name );
    item->setData( 0 , Qt::UserRole , (QVariant)(int)field );

    QString buff ;
    if( value.type( ) == QVariant::String ) {
        item->setText( 1 , value.toString() );
    }
    else if( value.type( ) == QVariant::Int ) {
        buff.sprintf( "%X" , value.toInt( ) );
        item->setText( 1 , buff);
    }
  
    
    if( userData != -1 ) {
        buff.sprintf( "%X" , userData );
        item->setText( 2 ,  buff);
        USERDATAONITEM* itemData = new USERDATAONITEM{ userDataType , userData , userDataSize };
        item->setData( 2 , Qt::UserRole , (int)itemData );
    }

    item->setText( 3 , comment );

    mFieldList << field;
    return field;
}

Field* TypeTree::findType( FieldType type , const QString& name )
{
    for( auto& i : mFieldList ) {
        if( i->type == type && i->name == name ) {
            return i;
        }
    }
    return nullptr;
}

void TypeTree::clear( )
{
    for( auto& i : mFieldList ) {
        delete i;
    }
    mFieldList.clear( );
    for( auto& i : mUserDataList ) {
        delete i;
    }
    mUserDataList.clear( );
}

void TypeTree::repain( )
{
    mTree->viewport( )->repaint( );
    mEdit->viewport( )->repaint( );
}

Field::Field(FieldType type , const QString& name , const QString& comment , const QVariant& value , TokenList::Token&token , QTreeWidgetItem* item /*= nullptr */ , Field* parent /*= nullptr */ )
    : parent( parent )
    , name( name )
    , comment( comment )
    , value( value )
    , token( token )
    , item( item )
    , type( type )
{ 
    if( parent ) {
        parent->subField.push_back( this );
    }
}

Field::~Field( )
{
    
}

Field* Field::findSub( const QString name )
{
    for( auto& i : subField ) {
        if( i->name == name )
            return i;
    }
    return nullptr;
}

Field* Field::createField( FieldType type,const QString& name , const QString& comment , const QVariant& value , TokenList::Token&token , QTreeWidgetItem* item /*= nullptr */ , Field* parent /*= nullptr */ )
{
    Field* field = findSub( name );
    if(field ) {
        return field;
    }

    return new Field(type, name , comment , value , token , item , parent );
}

void Field::itemOnTextEditRangle( int& index , int& nLen )
{
    index = token.mBegPos;
    nLen = token.mLen;
}
