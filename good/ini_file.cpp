

#include <stdio.h>
#include "good/file.h"
#include "good/ini_file.h"


#define INVALID_SECTION(s) (s == m_lSections.end())


// Disable obsolete warnings.
#pragma warning(disable: 4996)


namespace good
{


	//------------------------------------------------------------------------------------------------------------
	TIniFileError ini_file::save() const
	{
		// File contents will be:
		// junk (before 1rst section)
		// [Section.name](optionally ;junk)\n
		// Key = value(optionally ;junk)\n

		FILE* f = fopen(name.c_str(), "wb");

		if (f == NULL)
			return IniFileNotFound;

		fprintf(f, "%s", junkBeforeSections.c_str());
		for (const_iterator it = begin(); it != end(); ++it)
		{
			if (it->name.size() > 0)
				fprintf(f, "[%s]", it->name.c_str());
			if (it->junkAfterName.size() > 0)
			{
				fprintf(f, "%s", it->junkAfterName.c_str());
				if (it->eolAfterJunk)
					fprintf(f, "\n");
			}
			else
				fprintf(f, "\n");
			for (ini_section::const_iterator confIt = it->begin(); confIt != it->end(); ++confIt)
			{
#ifdef INI_FILE_DONT_TRIM_STRINGS
				fprintf(f, "%s=%s", confIt->key.c_str(), confIt->value.c_str());
#else
				fprintf(f, "%s = %s", confIt->key.c_str(), confIt->value.c_str());
#endif
				if (confIt->junk.size() > 0)
				{
					if (confIt->junkIsComment)
#ifdef INI_FILE_DONT_TRIM_STRINGS
						fprintf(f, " ;");
#else
						fprintf(f, ";");
#endif
					else
						fprintf(f, "\n");
					fprintf(f, "%s", confIt->junk.c_str());
					if (confIt->eolAterJunk)
						fprintf(f, "\n");
				}
				else
					fprintf(f, "\n");
			}
		}
		fclose(f);

		return IniFileNoError;
	}



	//------------------------------------------------------------------------------------------------------------
	TIniFileError ini_file::load()
	{
		// Read entire file into memory.
		size_t fsize = file::file_size(name.c_str());

		if (fsize == FILE_OPERATION_FAILED)
			return IniFileNotFound;

		if (fsize > MAX_INI_FILE_SIZE)
			return IniFileTooBig;
		
		clear();
		TIniFileError result = IniFileNoError;
		
		char* buf = (char*)malloc(fsize+1);
		DebugAssert(buf);

		size_t read = good::file::file_to_memory(name.c_str(), buf, fsize);
		DebugAssert(fsize == read);
		buf[fsize] = 0;

		char *section = NULL, *key = NULL, *value = NULL, *junk = NULL;
		char *line = buf, *first_junk = buf, *junk_after_section_name = NULL;
		
		int lineNumber = 1, section_end = 0;
		
		bool escapeKey = false, escapeValue = false, escapeSection = false;
		bool comment = false, junkIsComment = false;

		ini_string Key, Value, Junk;
		
		iterator currentSection = m_lSections.end();

		for (size_t pos = 0; pos < fsize; ++pos)
		{
			//----------------------------------------------------------------------------
			// Fast skip comments.
			//----------------------------------------------------------------------------
			if (comment && buf[pos] != '\n')
				continue;

			char c = buf[pos];

			//----------------------------------------------------------------------------
			// End of line.
			//----------------------------------------------------------------------------
			if (c == '\n')
			{
				lineNumber++;
				comment = false; // End of line is end of comment.
				section_end = 0;
				
				section = NULL;
				line = &buf[pos+1];

				// If there was key-value on this line (without comment), 
				// then junk will be continued on next line.
				if ( key && (junk == NULL) )
				{
					junk = line;
					junkIsComment = false;
					if (line > buf)
						*(line-1) = 0; // Make sure value stops at this line.
				}
			}

			//----------------------------------------------------------------------------
			// Escape character. Don't do nothing, process them later.
			//----------------------------------------------------------------------------
			else if (c == '\\')
			{
				if (value)
					escapeValue = true;
				else if (section)
					escapeSection = true;
				else //if (key)
					escapeKey = true;

				if (buf[pos+1] == 'x') // Unicode character (\x002B). Still not supported.
					pos +=3;
				if (buf[pos+1] == '\r') // "\\r\n"
					pos +=2;
				else // One of the following: \a \b \0 \t \r \n \\ \; \# \= \:
					++pos;
			}

			//----------------------------------------------------------------------------
			// Start of comment.
			//----------------------------------------------------------------------------
			else if ( (c == INI_FILE_COMMENT_CHARACTER_1)
#ifdef INI_FILE_COMMENT_CHARACTER_2
				|| (c == INI_FILE_COMMENT_CHARACTER_2)
#endif
				)
			{
				if (section)
				{
					section = NULL;
					DebugPrint("Invalid ini file %s at line %d, column %d: ", name.c_str(), lineNumber, &buf[pos] - line + 1);
					DebugPrint("no ']', end of section character.\n");
					result = IniFileBadSyntax;
#ifdef INI_FILE_STOP_ON_ERROR
					break;
#endif
				}
				else if ( key && (junk == NULL) )
				{
					buf[pos] = 0;
					junk = &buf[pos+1];
					junkIsComment = true;
				}
				comment = true;
			}

			//----------------------------------------------------------------------------
			// Start of a new section.
			//----------------------------------------------------------------------------
			else if (c == '[')
			{
				if ( (section == NULL) && (key != line) ) // Section is not started yet and there is no key-value separator before.
				{
					section_end++;
					section = &buf[pos+1];
				}
				else
				{
					if (section)
						section_end++; // There is need to be 1 more symbol for end of section now.

					DebugPrint("Invalid ini file %s at line %d, column %d: ", name.c_str(), lineNumber, &buf[pos] - line + 1);
					DebugPrint("'[' character in section name or value.\n");
					result = IniFileBadSyntax;
#ifdef INI_FILE_STOP_ON_ERROR
					break;
#endif
				}
			}

			//----------------------------------------------------------------------------
			// End of section.
			//----------------------------------------------------------------------------
			else if (c == ']')
			{				
				if (section)
				{
					if (--section_end > 0)
						continue;

					*(section - 1) = 0; // Stop everything before section name.

					// Save junk after section name.
					if (junk_after_section_name)
					{
						currentSection->junkAfterName = junk_after_section_name;
						currentSection->eolAfterJunk = false;
						junk_after_section_name = NULL;
					}

					// Save previous key-value-junk.
					if (junk)
					{
						if ( m_lSections.empty() )
						{
							currentSection = m_lSections.insert(m_lSections.end(), ini_section());
							junkBeforeSections = first_junk;
						}

						ini_string Key(key), Value(value);
#ifndef INI_FILE_DONT_TRIM_STRINGS
						Key.trim();
						Value.trim();
#endif
						currentSection->add(Key, Value, junk, junkIsComment, false);
						key = value = junk = NULL;
						junkIsComment = false;
					}


					buf[pos] = 0;
					ini_string Section(section);
#ifndef INI_FILE_DONT_TRIM_STRINGS
					Section.trim();
#endif
					if ( m_lSections.empty() )
						junkBeforeSections = first_junk;

					currentSection = m_lSections.insert(m_lSections.end(), ini_section());
					currentSection->name = Section;
					section = NULL;
					junk_after_section_name = &buf[pos+1];
					comment = true; // Force to be junk until end of line.
				}
				else
				{
					DebugPrint("Invalid ini file %s at line %d, column %d: ", name.c_str(), lineNumber, &buf[pos] - line + 1);
					DebugPrint("'[' character in section name or value.\n");
					result = IniFileBadSyntax;
#ifdef INI_FILE_STOP_ON_ERROR
					break;
#endif
				}
			}

			//----------------------------------------------------------------------------
			// Separator of key-value.
			//----------------------------------------------------------------------------
			else if ( (c == INI_FILE_KEY_VALUE_SEPARATOR_1)
#ifdef INI_FILE_KEY_VALUE_SEPARATOR_2
				|| (c == INI_FILE_KEY_VALUE_SEPARATOR_2)
#endif
				)
			{
				if (section == NULL)
				{
					if (key == NULL)
					{
						if (junk_after_section_name)
						{
							currentSection->junkAfterName = junk_after_section_name;
							currentSection->eolAfterJunk = true;
							junk_after_section_name = NULL;
						}

						if (line > buf)
							*(line-1) = 0; // Make sure last junk stops at this line.

						buf[pos] = 0;
						key = line;
						value = &buf[pos+1];
						junkIsComment = false;
						junk = NULL;
					}
					else
					{
						if (key != line) // Key-value-junk are on some previous line, save them.
						{
							if ( m_lSections.empty() )
							{
								currentSection = m_lSections.insert(m_lSections.end(), ini_section());
								junkBeforeSections = first_junk;
							}

							// Truncate junk by putting end of string to the end of last line.
							if (line > buf)
							{
								*(line-1) = 0;
								if (line == junk)
									junk = "";
							}
							
							ini_string Key(key), Value(value);
#ifndef INI_FILE_DONT_TRIM_STRINGS
							Key.trim();
							Value.trim();
#endif
							currentSection->add(Key, Value, junk, junkIsComment, true);

							buf[pos] = 0;
							key = line;
							value = &buf[pos+1];
							junk = NULL;
							junkIsComment = false;
						}
						else // Key-value-junk are on same line, = in value.
						{
							DebugPrint("Invalid ini file %s at line %d, column %d: ", name.c_str(), lineNumber, &buf[pos] - line + 1);
							DebugPrint("key-value separator in value.\n");
							result = IniFileBadSyntax;
	#ifdef INI_FILE_STOP_ON_ERROR
							break;
	#endif
						}
					}
				}
				else
				{
					DebugPrint("Invalid ini file %s at line %d, column %d: ", name.c_str(), lineNumber, &buf[pos] - line + 1);
					DebugPrint("key-value separator at section name.\n");
					result = IniFileBadSyntax;
#ifdef INI_FILE_STOP_ON_ERROR
					break;
#endif
				}
			}
		}


		// Save last junk after section name.
		if (junk_after_section_name)
		{
			currentSection->junkAfterName = junk_after_section_name;
			currentSection->eolAfterJunk = false;
			junk_after_section_name = NULL;
		}

		// Save last key-value-junk.
		else if (key)
		{
			if ( m_lSections.empty() )
			{
				currentSection = m_lSections.insert(m_lSections.end(), ini_section());
				junkBeforeSections = first_junk;
			}

			ini_string Key(key), Value(value);
#ifndef INI_FILE_DONT_TRIM_STRINGS
			Key.trim();
			Value.trim();
#endif
			currentSection->add(Key, Value, junk, junkIsComment, false);
		}


#ifdef INI_FILE_STOP_ON_ERROR
		if (result != IniFileNoError)
			clear();
		else
#endif
			m_pBuffer = buf;

		return result;
	}


} // namespace
