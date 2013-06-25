#include <stdlib.h> // rand()

#include "good/string_buffer.h"

#include "chat.h"
#include "player.h"
#include "source_engine.h"
#include "type2string.h"


#define ChatError(...) CUtil::Message(NULL, __VA_ARGS__)

#if defined(DEBUG) || defined(_DEBUG)
#	define ChatMessage(...) CUtil::Message(NULL, __VA_ARGS__)
#else
#	define ChatMessage(...)
#endif



//----------------------------------------------------------------------------------------------------------------
extern char* szMainBuffer;
extern int iMainBufferSize;

good::string sComma(",");


//----------------------------------------------------------------------------------------------------------------
const good::string& PhraseToString( const CPhrase& cPhrase )
{
	static good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);
	sbBuffer.erase();

	bool bPrevOptional = false;
	for ( int iWord = 0; iWord < cPhrase.aWords.size(); ++iWord )
	{
		const CPhraseWord& cWord = cPhrase.aWords[iWord];

		if ( cWord.bOptional && !bPrevOptional )
			sbBuffer.append('(');
		if ( cWord.bCommaBefore )
			sbBuffer.append(", ");
		sbBuffer.append( cWord.sWord );
		if ( cWord.bCommaAfter )
			sbBuffer.append(',');
		if ( cWord.bOptional && !cWord.bNextOptional )
			sbBuffer.append(')');

		sbBuffer.append(' ');
		bPrevOptional = cWord.bOptional && cWord.bNextOptional;	// Check if optional belongs to same chunk.
	}

	sbBuffer[sbBuffer.size()-1] = cPhrase.chrPhraseEnd;
	return sbBuffer;
}



//----------------------------------------------------------------------------------------------------------------
good::vector<CPhrase> CChat::m_aMatchPhrases[EBotChatTotal]; // Phrases for commands used for matching.
good::vector<CPhrase> CChat::m_aPhrases[EBotChatTotal];      // Phrases for commands used for generation of commands.

good::vector<StringVector> CChat::m_aSynonims;               // Available synonims.

StringVector CChat::m_aVariables;                            // Available variable names ($player, $door, $button, etc).
good::vector<StringVector> CChat::m_aVariableValues;         // Available variable values (1, 2, opened, closed, weapon_...).


//----------------------------------------------------------------------------------------------------------------
const good::string& CChat::GetSynonim( const good::string& sSentence )
{
	for ( int iWord = 0; iWord < m_aSynonims.size(); ++iWord )
	{
		StringVector& aSynonims = m_aSynonims[iWord];

		// Check if sentence starts with some of this synonims.
		for ( int iSynonim = 0; iSynonim < aSynonims.size(); ++iSynonim )
		{
			const good::string& sSynonim = aSynonims[iSynonim];
			bool bBigger = sSentence.size() >  sSynonim.size();
			bool bEqual  = sSentence.size() == sSynonim.size();
			if ( ( (bBigger && (sSentence[sSynonim.size()] == ' ')) || bEqual ) && sSentence.starts_with(sSynonim) )
				return sSynonim;
		}
	}
	return sSentence;
}


//----------------------------------------------------------------------------------------------------------------
void CChat::AddSynonims( const good::string& sKey, const good::string& sValue )
{
	StringVector aSynonims;
	good::string sFirst = sKey.duplicate();
	sFirst.lower_case();
	
	aSynonims.push_back(sFirst);
	good::string sOthers(sValue, true);
	sOthers.escape();
	sOthers.lower_case();
	sOthers.split<good::vector>(aSynonims, '.', true);

	ChatMessage( "\t%s", CTypeToString::StringVectorToString(aSynonims).c_str() );

	m_aSynonims.push_back(aSynonims);
}


//----------------------------------------------------------------------------------------------------------------
bool CChat::AddChat( const good::string& sKey, const good::string& sValue )
{
	good::string sCommandName = sKey.duplicate();
	sCommandName.escape();
	sCommandName.lower_case();

	// Get command from key.
	int iCommand = CTypeToString::BotCommandFromString(sCommandName);
	if ( iCommand == -1 )
	{
		ChatError("Error, unknown command: %s.", sKey.c_str());
		return false;
	}

	good::string_buffer sbCommands( szMainBuffer, iMainBufferSize, false );
	sbCommands = sValue;
	sbCommands.escape();
	sbCommands.lower_case();
	if ( !sbCommands.ends_with('.') )
		sbCommands.append('.'); // Force to end with '.'

	good::vector<CPhrase> aPhrases;
	good::vector<CPhrase> aMatchPhrases;

	int iBegin = 0, iFirstPhrase = 0, iWordsBeforeParallelInFirstPhrase = 0, iParallelCount = 0;
	bool bCanBreak = true, bOptional = false, bPreviousParenthesis = false, bCommaBeforeWord = false;
	bool bParallel = false, bCanEndParallel = false, bEnd = false;
	for ( int iPos = 0; iPos < sbCommands.size(); ++iPos )
	{
		bool bQuestion = false, bExclamation = false, bSeparator = false;
		bool bWord = false, bComma = false;

		// Before word is added to phrases.
		switch ( sbCommands[iPos] ) // TODO: symbols from ini file.
		{
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			bWord = true;
			break;

		case ',':
		case ';':
			bWord = bComma = true;
			break;

		case '.':
		case '?':
		case '!':
			bWord = bEnd = true;
			if ( !bCanBreak )
			{
				ChatError("Error for '%s' command: sentence can't end inside <>", sKey.c_str());
				return false;
			}
			bExclamation = (sbCommands[iPos] == '!');
			bQuestion= (sbCommands[iPos] == '?');
			break;

		case '(':
			if ( bCanBreak && bParallel && !bCanEndParallel )
			{
				ChatError("Error for '%s' command: word can't be optional after separator.", sKey.c_str());
				return false;
			}
			bWord = true;
			break;

		case ')':
			bWord = true;
			break;

		case '<':
			if ( !bCanBreak )
			{
				ChatError("Error for '%s' command: invalid <> secuence.", sKey.c_str());
				return false;
			}
			bCanBreak = false;
			bWord = true;
			if ( !bParallel )
			{
				bParallel = true;
				iParallelCount = aPhrases.size() - iFirstPhrase;
				if ( iParallelCount == 0 )
				{
					iParallelCount = 1;
					iWordsBeforeParallelInFirstPhrase = 0;
				}
				else
					iWordsBeforeParallelInFirstPhrase = aPhrases[iFirstPhrase].aWords.size();
			}
			break;

		case '>':
			if ( bCanBreak )
			{
				ChatError("Error for '%s' command: invalid <> secuence.", sKey.c_str());
				return false;
			}

			// Check if parallel processing is terminated.
			if ( bParallel )
			{
				if ( bCanEndParallel )
				{
					bParallel = false;
					iWordsBeforeParallelInFirstPhrase = iParallelCount = 0;
				}
				else
					bCanEndParallel = true;
			}
			bWord = true;
			break;

		case '/':
			if ( !bParallel )
			{
				bParallel = true;
				iParallelCount = aPhrases.size() - iFirstPhrase;
				if ( iParallelCount == 0 )
				{
					iParallelCount = 1;
					iWordsBeforeParallelInFirstPhrase = 0;
				}
				else
					iWordsBeforeParallelInFirstPhrase = aPhrases[iFirstPhrase].aWords.size();
			}

			bWord = bSeparator = true;
			bCanEndParallel = false;
			break;

		default:
			break;
		}

		if ( bWord )
		{
			// We have a separated word. Check for synonims in matching phrases.
			if ( iPos-iBegin > 0 )
			{
				// Check if parallel processing is terminated.
				if ( bParallel && !bSeparator && bCanBreak )
				{
					if ( bCanEndParallel )
					{
						bParallel = false;
						iWordsBeforeParallelInFirstPhrase = iParallelCount = 0;
					}
					else
						bCanEndParallel = true;
				}

				// Get word from string.
				good::string sWord = sbCommands.substr(iBegin, iPos - iBegin);

				good::string sMatchWord;
				sMatchWord.assign(sWord, true);
				sMatchWord.lower_case();
				const good::string& sSynonim = GetSynonim(sMatchWord);

				if ( aPhrases.size() == iFirstPhrase ) // Add new phrase after '.' character.
				{
					aPhrases.push_back(CPhrase());
					aMatchPhrases.push_back(CPhrase());
				}

				int iFirst = iFirstPhrase;
				int iLast = aPhrases.size();
				if ( bParallel )
				{
					iFirst = aPhrases.size() - iParallelCount;
				}
				for ( int i = iFirst; i < iLast; ++i )
				{
					aPhrases[i].aWords.push_back( CPhraseWord(sWord.duplicate(), bOptional, bOptional, bCommaBeforeWord, false ) );
					aMatchPhrases[i].aWords.push_back( CPhraseWord(sSynonim.duplicate(), bOptional, bOptional, bCommaBeforeWord, false ) );
				}
				bCommaBeforeWord = bPreviousParenthesis = false;
			}
			iBegin = iPos+1;
		}

		if ( bComma )
		{
			if ( !bCommaBeforeWord )
			{
				int iFirst = iFirstPhrase;
				int iLast = aPhrases.size();
				if ( bParallel )
				{
					iFirst = aPhrases.size() - iParallelCount;
				}
				for ( int i = iFirst; i < iLast; ++i )
				{
					aPhrases[i].aWords.back().bCommaAfter = true;
					aMatchPhrases[i].aWords.back().bCommaAfter = true;
				}
			}
			else
			{
				bCommaBeforeWord = bPreviousParenthesis;
				bPreviousParenthesis = false;
			}
		}

		// After word is added.
		switch ( sbCommands[iPos] )
		{
		case '>':
			bPreviousParenthesis = false;
			bCanBreak = true;
			break;
		case '(':
			bOptional = bPreviousParenthesis = true;
			break;
		case ')':
			bPreviousParenthesis = bOptional = false;
			for ( int iPhrase = iFirstPhrase; iPhrase < aPhrases.size(); ++iPhrase )
			{
				aPhrases[iPhrase].aWords.back().bNextOptional = false;
				aMatchPhrases[iPhrase].aWords.back().bNextOptional = false;
			}
			break;
		case '/':
			{
				bPreviousParenthesis = false;

				// Copy all phrases we have so far.
				int iWordsBeforeSeparator = aPhrases[iFirstPhrase].aWords.size() - iWordsBeforeParallelInFirstPhrase;
				if ( iWordsBeforeSeparator == 0 ) // Case when there is a space before separator.
					iWordsBeforeSeparator = 1;

				for ( int iPhrase = iFirstPhrase; iPhrase < iFirstPhrase + iParallelCount; ++iPhrase )
				{
					CPhrase& cPhrase = aPhrases[iPhrase];
					CPhrase& cMatchPhrase = aMatchPhrases[iPhrase];

					CPhrase& cPhraseCopy = *aPhrases.insert( aPhrases.end(), CPhrase() );
					CPhrase& cMatchPhraseCopy = *aMatchPhrases.insert( aMatchPhrases.end(), CPhrase() );

					for ( int iWord = 0; iWord < cPhrase.aWords.size() - iWordsBeforeSeparator; ++iWord )
					{
						CPhraseWord& cWord = cPhrase.aWords[iWord];
						CPhraseWord cCopy = CPhraseWord(cWord.sWord.duplicate(), cWord.bOptional, cWord.bNextOptional, cWord.bCommaBefore, cWord.bCommaAfter);
						cPhraseCopy.aWords.push_back( cCopy );
						CPhraseWord cMatchCopy = CPhraseWord(cMatchPhrase.aWords[iWord].sWord.duplicate(), cWord.bOptional, cWord.bNextOptional, cWord.bCommaBefore, cWord.bCommaAfter);
						cMatchPhraseCopy.aWords.push_back( cMatchCopy );
					}
				}
				break;
			}
		}

		if ( bEnd )
		{
			if ( bParallel && !bCanEndParallel )
			{
				ChatError("Error for '%s' command: sentence can't end in separator.", sKey.c_str());
				return false;
			}
			for ( int iPhrase = iFirstPhrase; iPhrase < aPhrases.size(); ++iPhrase )
			{
				aPhrases[iPhrase].aWords.back().bNextOptional = false;
				aMatchPhrases[iPhrase].aWords.back().bNextOptional = false;

				aPhrases[iPhrase].chrPhraseEnd = sbCommands[iPos];
				aMatchPhrases[iPhrase].chrPhraseEnd = sbCommands[iPos];
			}
			iFirstPhrase = aPhrases.size();
			bParallel = bPreviousParenthesis = false;
			iWordsBeforeParallelInFirstPhrase = iParallelCount = 0;
			bEnd = false;
		}
	}

#if defined(DEBUG) || defined(_DEBUG)
	// Show command name.
	ChatMessage( "\t%s:", sCommandName.c_str() );

	for ( int iPhrase = 0; iPhrase < aPhrases.size(); ++iPhrase )
	{
		ChatMessage("\t\tTo generate: %s", PhraseToString(aPhrases[iPhrase]).c_str() );
		ChatMessage("\t\tTo match:    %s", PhraseToString(aMatchPhrases[iPhrase]).c_str() );
	}
#endif	

	m_aPhrases[iCommand] = aPhrases;
	m_aMatchPhrases[iCommand] = aMatchPhrases;

	return true;
}

	
//----------------------------------------------------------------------------------------------------------------
float CChat::ChatFromText( const good::string& sText, CBotChat& cCommand )
{
	good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);
	sbBuffer = sText;
	sbBuffer.trim();
	sbBuffer.lower_case();
	if ( !sbBuffer.ends_with('.') && !sbBuffer.ends_with('!') && !sbBuffer.ends_with('?') )
		sbBuffer.append('.');

	cCommand.iBotRequest = EBotChatUnknown;
	cCommand.iDirectedTo = cCommand.iReferringTo = -1;

	StringVector aPhrases[1]; // TODO: multiple commands in one sentence.
	int iCurrentPhrase = 0, iBegin = 0;
	char chrEnd;

	// Replace player's names and synonims.
	for ( int iPos = 0; iPos < sbBuffer.size(); ++iPos )
	{
		bool bWord = false, bEnd = false, bComma = false;
		switch ( sbBuffer[iPos] )
		{
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			bWord = true;
			break;

		case ',':  // Don't mind commas.
		case ';':
			bWord = bComma = true;
			break;

		case '.':
		case '?':
		case '!':
			bWord = bEnd = true;
			chrEnd = sbBuffer[iPos];
			break;
		}

		if ( bWord )
		{
			if ( iPos - iBegin > 0 )
			{
				// Check players names first.
				bool bPlayerName = false;
				if ( (cCommand.iDirectedTo == -1) || (cCommand.iReferringTo == -1) )
				{
					good::string sCurr( &szMainBuffer[iBegin], false, false, sbBuffer.size() - iBegin );

					for ( int i=0; i < CPlayers::GetMaxPlayers(); ++i )
					{
						CPlayer* pPlayer = CPlayers::Get(i);
						if ( pPlayer && sCurr.starts_with( pPlayer->GetLowerName() ) )
						{
							iBegin += pPlayer->GetLowerName().size();
							if (iBegin > iPos)
								iPos = iBegin;
							if ( cCommand.iDirectedTo == -1 )
							{
								aPhrases[iCurrentPhrase].push_back("$player1");
								cCommand.iDirectedTo = i;
							}
							else
							{
								aPhrases[iCurrentPhrase].push_back("$player2");
								cCommand.iReferringTo = i;
							}
							bPlayerName = true;
							break;
						}
					}
				}

				// Check synonims.
				if ( !bPlayerName )
				{
					szMainBuffer[iPos] = 0;
					good::string sCurr( &szMainBuffer[iBegin], false, false, iPos - iBegin );
					const good::string& sSynonim = GetSynonim(sCurr);

					aPhrases[iCurrentPhrase].push_back( good::string(sSynonim) ); // Don't copy string, we will just use it for matching.
					iBegin = iPos+1;
				}
			}
			else
				iBegin = iPos+1;
		}

		if ( bEnd )
			break;
	}

	StringVector& aWords = aPhrases[0];

	if ( aWords.size() == 0 )
		return 0.0f;

	// Search in all commands for match. TODO: detect 2+ commands in one sentence.
	int iBestFound, iBestRequired, iBestOrdered;
	int iBestPhrase, iBestTotalRequired;
	float fBestImportance = 0.0f; // Number from 0 to 10, represents how good matching is.

	for ( TBotChat iCommand = 0; iCommand < EBotChatTotal; ++iCommand )
	{
		for ( int iPhrase = 0; iPhrase < m_aMatchPhrases[iCommand].size(); ++iPhrase )
		{
			int iFound = 0, iRequired = 0, iOrdered = 0, iTotalRequired = 0, iPrevPos = 0;
			CPhrase& currPhrase = m_aMatchPhrases[iCommand][iPhrase];
			for ( int iWord = 0; iWord < currPhrase.aWords.size(); ++iWord )
			{
				const CPhraseWord& cPhraseWord = currPhrase.aWords[iWord];
				StringVector::const_iterator it = good::find( aWords.begin(), aWords.end(), cPhraseWord.sWord );

				if ( !cPhraseWord.bOptional )
					iTotalRequired++; // Count amount of required words in this phrase.

				if ( it != aWords.end() )
				{
					iFound++;
					
					if ( !currPhrase.aWords[iWord].bOptional )
						iRequired++;

					int iPos = it - aWords.begin();
					if ( iPrevPos <= iPos )
						iOrdered++;

					iPrevPos = iPos;
				}
			}

			int iOptionals = iFound-iRequired;
			int iTotalOptionals = currPhrase.aWords.size() - iTotalRequired;
			int iExtra = aWords.size() - iFound;

			// Give more importance to requiered words (not optionals), and less to optionals and words order.
			float fImportance = 0.0f;
			fImportance += 6*( (iTotalRequired) ? iRequired / (float)iTotalRequired : 0 );    // + Ratio of matched requered words.
			fImportance += 1*( (iTotalOptionals) ? iOptionals / (float)iTotalOptionals : 1 ); // + Ratio of matched optional words.
			fImportance += 2*( iOrdered / (float)currPhrase.aWords.size() );                  // + Ratio of matched ordered words.
			fImportance += (chrEnd == currPhrase.chrPhraseEnd) ? 1 : 0;                       // + 1 if ends with same symbol as matching phrase (. ? !).
			fImportance -= 4*( iExtra / (float)aWords.size() );                               // - Ratio of not matched words.

			if ( fBestImportance < fImportance )
			{
				fBestImportance = fImportance;
				iBestFound = iFound;
				iBestRequired = iRequired;
				iBestTotalRequired = iTotalRequired;
				iBestOrdered = iOrdered;
				cCommand.iBotRequest = iCommand;
				iBestPhrase = iPhrase;
			}
		}
	}

	if ( fBestImportance > 0.0f )
	{
		const CPhrase& cPhrase = m_aMatchPhrases[cCommand.iBotRequest][iBestPhrase];
		ChatMessage( "Chat match: %s", PhraseToString(cPhrase).c_str() );
		ChatMessage( "Matching (from 0 to 10): %f.", fBestImportance );
	}
	else
	{
		ChatMessage( "\tNo match found." );
	}
	return fBestImportance;
}


//----------------------------------------------------------------------------------------------------------------
const good::string& CChat::ChatToText( const CBotChat& cCommand )
{
	DebugAssert( (EBotChatUnknown < cCommand.iBotRequest) && (cCommand.iBotRequest < EBotChatTotal) );

	static good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false);
	sbBuffer.erase();

	if ( m_aPhrases[cCommand.iBotRequest].size() == 0 )
	{
		ChatError( "No phrases provided to generate chat message for '%s'.", CTypeToString::BotCommandToString(cCommand.iBotRequest).c_str() );
		return sbBuffer; // Empty string.
	}

	int iRand = rand() % m_aPhrases[cCommand.iBotRequest].size();
	const CPhrase& cPhrase = m_aPhrases[cCommand.iBotRequest][iRand];

	DebugAssert( cPhrase.aWords.size() );

	// Check if frase contains $player1 or $player2.
	int iOptionalPlayer1 = -1, iPlayer1 = -1, iOptionalPlayer2 = -2, iPlayer2 = -1;
	int iPrevOptional = 0;
	bool bSameOptional = false;
	if ( (cCommand.iDirectedTo != -1) || (cCommand.iReferringTo != -1))
	{
		for ( int i=0; i < cPhrase.aWords.size(); ++i )
		{
			const CPhraseWord& cWord = cPhrase.aWords[i];

			if ( cWord.bOptional && !bSameOptional )
				iPrevOptional = i;

			if ( (cWord.sWord == "$player1") && (cCommand.iDirectedTo != -1) )
			{
				iPlayer1 = i;
				if ( cWord.bOptional )
					iOptionalPlayer1 = iPrevOptional;
			}
			else if ( (cWord.sWord == "$player2") && (cCommand.iReferringTo != -1) )
			{
				iPlayer2 = i;
				if ( cWord.bOptional )
					iOptionalPlayer1 = iPrevOptional;
			}

			bSameOptional = cWord.bNextOptional;
		}
	}

 	bool bForceOptional = false;
	for ( int i=0; i < cPhrase.aWords.size(); ++i )
	{
		const CPhraseWord& cWord = cPhrase.aWords[i];
		bool bAdd = !cWord.bOptional;

		if ( (i == iOptionalPlayer1) || (i == iOptionalPlayer2) )
			bForceOptional = true;

		bool bUseOptional = bForceOptional || (rand()&1);
		if ( cWord.bOptional && bUseOptional )
		{
			bAdd = true;
			bForceOptional = cWord.bNextOptional;
		}

		if ( bAdd )
		{
			if ( cWord.bCommaBefore )
				sbBuffer.append(", ");

			if ( i == iPlayer1 )
				sbBuffer.append( CPlayers::Get(cCommand.iDirectedTo)->GetName() );
			else if ( i == iPlayer2 )
				sbBuffer.append( CPlayers::Get(cCommand.iReferringTo)->GetName() );
			else
				sbBuffer.append(cWord.sWord);

			if ( cWord.bCommaAfter )
				sbBuffer.append(',');
			sbBuffer.append(' ');
		}
	}

	sbBuffer[sbBuffer.size() - 1] = cPhrase.chrPhraseEnd;
	sbBuffer[0] = sbBuffer[0] - 'a' + 'A'; // Uppercase first letter.
	return sbBuffer;
}


//----------------------------------------------------------------------------------------------------------------
class CAnswers
{
public:
	CAnswers()
	{
		aTalkAnswers[EBotChatGreeting].push_back(EBotChatGreeting);
		
		aTalkAnswers[EBotChatBye].push_back(EBotChatBye);

		aTalkAnswers[EBotChatCall].push_back(EBotChatCallResponse);
		aTalkAnswers[EBotChatCall].push_back(EBotChatGreeting);

		aTalkAnswers[EBotChatHelp].push_back(EBotChatAffirmative);
		aTalkAnswers[EBotChatHelp].push_back(EBotChatNegative);
		aTalkAnswers[EBotChatHelp].push_back(EBotChatAffirm);
		aTalkAnswers[EBotChatHelp].push_back(EBotChatNegate);

		for ( TBotChat i = EBotChatStop; i <= EBotChatJump; ++i )
		{
			aTalkAnswers[i].push_back(EBotChatAffirm);
			aTalkAnswers[i].push_back(EBotChatNegate);
		}

		aTalkAnswers[EBotChatLeave].push_back(EBotChatBye);
	};

	good::vector<TBotChat> aTalkAnswers[EBotChatTotal];
};
/*
	// EBotChatError         { -1 }, 
	// EBotChatGreeting      { EBotChatGreeting, EBotChatBusy, -1 },
	// EBotChatBye           { -1 },
	// EBotChatBusy          { -1 },
	// EBotChatAffirmative   { -1 },
	// EBotChatNegative      { -1 },
	// EBotChatAffirm        { -1 },
	// EBotChatNegate        { -1 },
	// EBotChatCall          { EBotChatCallResponse, EBotChatBusy, -1 },
	// EBotChatCallResponse  { -1 },
	// EBotChatHelp          { EBotChatAffirmative, EBotChatNegative, EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
	// EBotChatStop          { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
	// EBotChatCome          { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
	// EBotChatFollow        { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
	// EBotChatAttack        { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
	// EBotChatNoKill        { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
	// EBotChatSitDown       { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
	// EBotChatStandUp       { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
	// EBotChatJump          { EBotChatAffirm, EBotChatNegate, EBotChatBusy, -1 },
	// EBotChatLeave         { EBotChatBye, -1 },
};
*/

const good::vector<TBotChat>& CChat::PossibleAnswers( TBotChat iTalk )
{
	static CAnswers cAnswers;
	DebugAssert( (0 <= iTalk) && (iTalk < EBotChatTotal) );
	return cAnswers.aTalkAnswers[iTalk];
}
