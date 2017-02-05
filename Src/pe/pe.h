#ifndef PE_H
#define PE_H

#include <windows.h>
#include <string>
using std::string;

typedef IMAGE_RESOURCE_DIRECTORY        pe_ResDir;
typedef IMAGE_RESOURCE_DIRECTORY_ENTRY  pe_ResDirEntry;
typedef IMAGE_RESOURCE_DATA_ENTRY       pe_ResDataEntry;
typedef IMAGE_DOS_HEADER                pe_DosHeader;
typedef IMAGE_NT_HEADERS                pe_NtHeader;
typedef IMAGE_FILE_HEADER               pe_FileHeader;
typedef IMAGE_OPTIONAL_HEADER32         pe_OptionHeader32;
typedef IMAGE_OPTIONAL_HEADER64         pe_OptionHeader64;
typedef IMAGE_DATA_DIRECTORY            pe_DataDirectroy;
typedef IMAGE_SECTION_HEADER            pe_SectionHeader;
typedef IMAGE_EXPORT_DIRECTORY          pe_ExportTable;
typedef IMAGE_IMPORT_DESCRIPTOR         pe_ImportTable;
typedef IMAGE_BASE_RELOCATION           pe_Relacation;
typedef IMAGE_RESOURCE_DIRECTORY        pe_ResourceTable;
typedef IMAGE_RESOURCE_DIRECTORY        pe_ResourcesDirectory;
typedef IMAGE_RESOURCE_DIRECTORY_ENTRY  pe_ResourcesDirectoryEntry;
typedef IMAGE_RESOURCE_DATA_ENTRY       pe_ResourcesDataEntry;

typedef struct Typeoffset{
    WORD wOffset : 12;
    WORD wType : 4;
}Typeoffset , *PTypeoffset;


class PE
{
    void*   m_pData;
    bool    m_bIsFileAlign;
public:
    PE(void* pFileData,bool bIsFileAlign=true);

    operator DWORD( );
    DWORD               RVAToOfs( DWORD dwRVA );
    DWORD               fileBase( );

    pe_DosHeader*       getDosHeader();
    pe_NtHeader*        getNtHeader();
    pe_FileHeader*      getFileHeader();
    pe_OptionHeader32*  getOptionHeader32( );
    pe_OptionHeader64*  getOptionHeader64( );
    pe_DataDirectroy*   getDataDirectory();
    pe_SectionHeader*   getSectionHeader();

    void*               getDescriptorTableRva(int nIndex );
    pe_ExportTable*     getExportTable( );
    bool                getExportTableFunctionName( DWORD indexOfAddress , string& name );

    pe_ImportTable*     getImportTable( );
    int                 getImportIATSize( );
    const char *        getImportDllName( int rva);
    const char*         getImportFunctionName( int rva );
    
    pe_Relacation*      getRelacationBlack( );
    
    pe_ResourceTable*   getResourcesTable( );
    const wchar_t*      getResourcesNameid( int rva );
};

#endif // PE_H
