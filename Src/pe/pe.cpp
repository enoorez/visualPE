
#include "pe.h"



/** ��һ��ֵ����һ�����ȶ�����ֵ */
DWORD ToAligentSize(DWORD nSize , DWORD nAligent)
{
    // �����˶��ٱ����ڴ����,�������ٱ�,���ж��ٱ��ڴ���뵥λ ;
    // ��ͷ�Ƿ񳬳��ڴ����,��������һ���ڴ���뵥λ
    if(nSize%nAligent != 0)
        return (nSize / nAligent + 1)*nAligent;
    return nSize;
}




PE::PE(void *pFileData, bool bIsFileAlign)
    :m_pData(pFileData)
    ,m_bIsFileAlign(bIsFileAlign)
{
}

pe_DosHeader *PE::getDosHeader()
{
    return (pe_DosHeader*)m_pData;
}

pe_NtHeader *PE::getNtHeader()
{
    if(m_pData==nullptr)
        return nullptr;
	return  (pe_NtHeader*)(getDosHeader()->e_lfanew + (int)m_pData);
}

pe_FileHeader *PE::getFileHeader()
{
    if(m_pData==nullptr)
        return nullptr;
    return &getNtHeader()->FileHeader;
}

pe_OptionHeader32 *PE::getOptionHeader32()
{
    if(m_pData==nullptr)
        return nullptr;

    return (pe_OptionHeader32*)&getNtHeader( )->OptionalHeader;
}


pe_OptionHeader64* PE::getOptionHeader64( )
{
    if( m_pData == nullptr )
        return nullptr;

    return (pe_OptionHeader64*)&getNtHeader( )->OptionalHeader;
}


pe_DataDirectroy *PE::getDataDirectory()
{
    if(m_pData == nullptr  )
        return nullptr;

    if( getOptionHeader32( )->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ) {
        return (pe_DataDirectroy*)getOptionHeader32( )->DataDirectory;
    }
    else if( getOptionHeader32( )->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC ) {
        return (pe_DataDirectroy*)getOptionHeader64( )->DataDirectory;
    }
    return nullptr;
}

DWORD PE::RVAToOfs(DWORD dwRVA)
{
    if(m_pData==nullptr)
        return 0;
    //��ʼ�������β��Ұ���RVA��ַ������
    //��ȡ��׼ͷָ��,�Ի�ȡ������Ŀ
    //��ȡ������Ŀ
    DWORD	dwSecTotal = getFileHeader()->NumberOfSections;

    //��ȡ��һ������
    PIMAGE_SECTION_HEADER	pScn = getSectionHeader();

    //��������
    for(DWORD i = 0; i < dwSecTotal; i++)
    {
        if(dwRVA >= pScn->VirtualAddress
           && dwRVA < pScn->VirtualAddress + pScn->Misc.VirtualSize)
        {
            // rva ת �ļ�ƫ�ƹ�ʽ:
            // rva - ��������rva + ���������ļ�ƫ��
            return dwRVA - pScn->VirtualAddress + pScn->PointerToRawData;
        }
        ++pScn;
    }
    return 0;
}

pe_SectionHeader* PE::getSectionHeader()
{
	if(m_pData == nullptr)
		return nullptr;

	return IMAGE_FIRST_SECTION(getNtHeader());
}


void* PE::getDescriptorTableRva( int nIndex )
{
    int     tableOffset = 0;
    getDataDirectory( )[ nIndex ].VirtualAddress;

    if( m_bIsFileAlign ) {
        tableOffset = RVAToOfs( getDataDirectory( )[ nIndex ].VirtualAddress );
    }
    else {
        tableOffset = getDataDirectory( )[ nIndex ].VirtualAddress ;
    }

    return (void*)( tableOffset);
}


pe_ExportTable* PE::getExportTable( )
{
    void *p = getDescriptorTableRva( 0 );
    if( p == nullptr )
        return nullptr;
    return (pe_ExportTable*)( (int)p + (int)m_pData );
}

pe_ImportTable* PE::getImportTable( )
{
    void *p = getDescriptorTableRva( 1);
    if( p == nullptr )
        return nullptr;
    return (pe_ImportTable*)( (int)p + (int)m_pData );
}

DWORD PE::fileBase( ) 
{
    return (DWORD)m_pData;
}



bool PE::getExportTableFunctionName( DWORD indexOfAddress , string& name )
{
    pe_ExportTable* table = getExportTable( );
    if( table == nullptr )
        return false;

    if( indexOfAddress > table->NumberOfFunctions )
        return false;

    DWORD* nameTable = (DWORD*)( RVAToOfs( table->AddressOfNames ) + fileBase( ) );
    WORD*  orderTable = (WORD *)( RVAToOfs( table->AddressOfNameOrdinals ) + fileBase( ) );

    
    // ������ű�, �ж����޸ú���������
    for( DWORD i = 0; i < table->NumberOfNames; ++i ) {

        if( indexOfAddress == orderTable[ i ] ) {
            // ������
            int nameIndex = orderTable[ i ];
            // �õ���������RVA
            DWORD nameRva = RVAToOfs( nameTable[ nameIndex ] );
            // �õ�������
            name = (char*)( nameRva + fileBase( ) );
            return true;
        }
        
    }



    name.resize( 10 );
    sprintf( (char*)name.c_str( ) , "%04X" , indexOfAddress + table->Base );

    return true;
}

int PE::getImportIATSize( ) 
{
    pe_ImportTable* table = getImportTable( );
    if( table == nullptr )
        return 0;

    
    DWORD* pIAT = nullptr;
    pIAT = (DWORD*)( RVAToOfs( table->FirstThunk ) + fileBase( ) );
    DWORD* backup = pIAT;
    while( *pIAT ) {
        ++pIAT;
    };

    return (DWORD)pIAT - (DWORD)backup;
}

const char* PE::getImportDllName( int rva )
{
    IMAGE_IMPORT_BY_NAME* pName = (IMAGE_IMPORT_BY_NAME*)( RVAToOfs( rva ) + fileBase( ) );
    return (char*)pName->Name;
}

const char* PE::getImportFunctionName( int rva )
{
    static  char buff[ 12 ];
    if( rva & 0x80000000 ) {
        // ���
        sprintf_s( buff , "%04X" , rva & 0x80000000 );
    }
    else {
        return getImportDllName( rva & 0x7FFFFFFF );
    }
    return nullptr;
}

PE::operator DWORD( )
{
    return (DWORD)m_pData;
}

pe_Relacation* PE::getRelacationBlack( )
{
    void *p = getDescriptorTableRva( 5 );
    if( p == nullptr )
        return nullptr;
    return (pe_Relacation*)( (int)p + (int)m_pData );
}


pe_ResourceTable* PE::getResourcesTable( )
{
    void *p = getDescriptorTableRva( 2 );
    if( p == nullptr )
        return nullptr;
    return (pe_ResourceTable*)( (int)p + (int)m_pData );
}

const wchar_t* PE::getResourcesNameid( int rva )
{
    DWORD resRoot = (DWORD)getResourcesTable( );
    if( resRoot == 0 )
        return nullptr;

    PIMAGE_RESOURCE_DIR_STRING_U pName = (PIMAGE_RESOURCE_DIR_STRING_U)( rva + resRoot );
    return pName->NameString;
}
