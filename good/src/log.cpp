#include <stdio.h>
#include <string.h>

#include "good/defines.h"
#include "good/log.h"


namespace good
{


bool log::bLogToStdOut = true;
bool log::bLogToStdErr = true;
TLogLevel log::iLogLevel = ELogLevelWarning;
TLogLevel log::iFileLogLevel = ELogLevelWarning;
TLogLevel log::iStdErrLevel = ELogLevelError;
FILE* log::m_fLog = NULL;

bool log::m_aLogFields[ELogFormatTotal] = { false };


//----------------------------------------------------------------------------------------------------------------
char szLogMessage[GOOD_LOG_MAX_MSG_SIZE];
int iStartSize = 0;


//----------------------------------------------------------------------------------------------------------------
bool log::set_prefix( const char* szPrefix )
{
    int iSize = strnlen(szPrefix, GOOD_LOG_MAX_MSG_SIZE-1);
    if ( iSize == GOOD_LOG_MAX_MSG_SIZE-1 )
        return false;

#ifdef GOOD_MULTI_THREAD
    good::lock cLock(m_cMutex);
#endif
    strncpy(szLogMessage, szPrefix, iSize);
    iStartSize = iSize;
    return true;
}


//----------------------------------------------------------------------------------------------------------------
bool log::start_log_to_file( const char* szFile, bool bAppend /*= false*/ )
{
    m_fLog = fopen(szFile, bAppend ? "w+" : "w");
    return (m_fLog);
}


//----------------------------------------------------------------------------------------------------------------
void log::stop_log_to_file()
{
#ifdef GOOD_MULTI_THREAD
    good::lock cLock(m_cMutex);
#endif
    if ( m_fLog )
    {
        fflush(m_fLog);
        fclose(m_fLog);
        m_fLog = NULL;
    }
}


//----------------------------------------------------------------------------------------------------------------
int log::format( TLogLevel iLevel, char* szOutput, int iOutputSize, const char* szFmt, ... )
{
    if ( iLevel < iLogLevel )
        return 0;

    va_list argptr;
    va_start(argptr, szFmt);
    int iResult = format_va_list(szOutput, iOutputSize, szFmt, argptr);
    va_end(argptr);

    return iResult;
}


//----------------------------------------------------------------------------------------------------------------
int log::printf( TLogLevel iLevel, const char* szFmt, ... )
{
    if ( iLevel < iLogLevel )
        return 0;

    va_list argptr;
    va_start(argptr, szFmt);
    int iResult = format_va_list(szLogMessage, GOOD_LOG_MAX_MSG_SIZE, szFmt, argptr);
    va_end(argptr);

    if ( iResult )
    {
        // Log to stdout or stderr. TODO: FILE* array[iLevel] for setLogToStdOut() & setLogToStdErr().
        FILE* fOut = ( bLogToStdErr && (iLevel >= iStdErrLevel) ) ? stderr : (bLogToStdOut ? stdout : NULL );
        if ( fOut )
            fputs(szLogMessage, fOut);
        // Log to file.
        if ( m_fLog )
            fputs(szLogMessage, m_fLog);
    }
    return iResult;
}


//----------------------------------------------------------------------------------------------------------------
void log::print( TLogLevel iLevel, const char* szFinal )
{
    if ( iLevel < iLogLevel )
        return;

    // Log to stdout or stderr. TODO: FILE* array[iLevel] for setLogToStdOut() & setLogToStdErr().
    FILE* fOut = ( bLogToStdErr && (iLevel >= iStdErrLevel) ) ? stderr : (bLogToStdOut ? stdout : NULL );
    if ( fOut )
    {
        fputs(szFinal, fOut);
#ifdef GOOD_LOG_FLUSH
        fflush(fOut);
#endif
    }

    // Log to file.
    if ( m_fLog )
    {
        fputs(szFinal, m_fLog);
#ifdef GOOD_LOG_FLUSH
        fflush(m_fLog);
#endif
    }
}


//----------------------------------------------------------------------------------------------------------------
int log::format_va_list( char* szOutput, int iOutputSize, const char* szFmt, va_list argptr )
{
    --iOutputSize; // Save 1 position for trailing 0.
    if ( szOutput != szLogMessage )
    {
        int iSize = MAX2(iOutputSize, iStartSize);
        strncpy(szOutput, szLogMessage, iSize);
    }

    if ( iStartSize >= iOutputSize )
    {
#ifdef GOOD_LOG_USE_ENDL
        szOutput[iOutputSize-1] = '\n';
#endif
        szOutput[iOutputSize] = 0;
        return iOutputSize;
    }

    int iTotal = vsnprintf(&szOutput[iStartSize], iOutputSize-iStartSize, szFmt, argptr);

    iTotal += iStartSize;
    if ( iTotal > iOutputSize )
#ifdef GOOD_LOG_USE_ENDL
        iTotal = iOutputSize-1;
    szOutput[iTotal++] = '\n';
#else
        iTotal = iOutputSize;
#endif
    szLogMessage[iTotal] = 0;
    return iTotal;
}


} // namespace good
