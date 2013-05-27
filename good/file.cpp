#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#	include <direct.h> // For mkdir() function.
#else
#	include <fcntl.h>
#endif

#include "good/file.h"


namespace good
{

	//----------------------------------------------------------------------------------------------------------------
	size_t file::file_size(const char* szFileName)
	{
		FILE* f = fopen(szFileName, "r");

		if (f)
		{
			fseek(f, 0, SEEK_END);
			long int result = ftell(f);
			fclose(f);
			return result;
		}

		return FILE_OPERATION_FAILED;
	}


	//----------------------------------------------------------------------------------------------------------------
	size_t file::file_to_memory(const char* szFileName, void* pBuffer, size_t iBufferSize, long iPos)
	{
		FILE* f = fopen(szFileName, "rb");

		if (f)
		{
			fseek(f, iPos, SEEK_SET);
			size_t readen = fread(pBuffer, 1, iBufferSize, f);
			fclose(f);
			return readen;
		}

		return (size_t)FILE_OPERATION_FAILED;
	}


	//----------------------------------------------------------------------------------------------------------------
	bool file::make_folders( const char *szFileName )
	{
		char szFolderName[1024];
		int folderNameSize = 0;
		szFolderName[0] = 0;

		int iLen = (int)strlen(szFileName);

		int i = 0;

		while ( i < iLen )
		{
			while ( (i < iLen) && (szFileName[i] != PATH_SEPARATOR) )
			{
				szFolderName[folderNameSize++] = szFileName[i];
				i++;
			}

			if ( i == iLen )
				return true;

			i++;
			szFolderName[folderNameSize++] = PATH_SEPARATOR;//next
			szFolderName[folderNameSize] = 0;
			
			mkdir(szFolderName);
		}

		return true;
	}

} // namespace good
