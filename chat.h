#ifndef __BOTRIX_CHAT_H__
#define __BOTRIX_CHAT_H__


#include "types.h"


/// Enum to represent invalid chat variable.
enum TChatVariables
{
	EChatVariableInvalid = -1,             ///< Invalid chat variable.
};
typedef int TChatVariable;                 ///< Number that represents chat variable (the one that starts with $ symbol).


/// Enum to represent invalid chat variable value.
enum TChatVariableValues
{
	EChatVariableValueInvalid = -1,        ///< Invalid chat variable value.
};
typedef int TChatVariableValue;            ///< Number that represents chat variable value.


/// Class that hold information about player's request.
class CBotChat
{
public:
	CBotChat( TBotChat iBotRequest = EBotChatUnknown, int iSpeaker = -1, int iDirectedTo = -1, int iReferringTo = -1 ):
		iBotRequest(iBotRequest), iSpeaker(iSpeaker), iDirectedTo(iDirectedTo), iReferringTo(iReferringTo) {}

	TBotChat iBotRequest;                  ///< Request type.
	int iSpeaker;                          ///< Index of player, that talked this request.
	int iDirectedTo;                       ///< Index of player, request is directed to.
	int iReferringTo;                      ///< Index of player, request is referring to.
};



/// Word from a phrase.
class CPhraseWord
{
public:
	CPhraseWord( good::string sWord, bool bOptional = false, bool bNextOptional = false, bool bCommaBefore = false, bool bCommaAfter = false):
		sWord(sWord), bOptional(bOptional), bNextOptional(bNextOptional), bCommaBefore(bCommaBefore), bCommaAfter(bCommaAfter) {}

	good::string sWord;                    ///< Word itself.
	//bool bVerb:1;                          ///< True, if word is verb (important).
	bool bOptional:1;                      ///< True, if word is optional in the phrase.
	bool bNextOptional:1;                  ///< True, if next word belongs to same optional subphrase in the phrase.
	bool bCommaBefore:1;                   ///< True, if there is a comma before this word. Case when comma is first in optional. Not used for matching.
	bool bCommaAfter:1;                    ///< True, if there is a comma after this word. Not used for matching.
};



/// Sentence of words.
class CPhrase
{
public:
	CPhrase(): aWords(16), chrPhraseEnd('.') {}

	good::vector<CPhraseWord> aWords;      ///< Array of words.
	char chrPhraseEnd;                     ///< One of '.', '?' or '!'.
};



/// Class that process what player said and transforms it to commands and viceversa.
class CChat
{

public:
	/// Add synonims for a word.
	static void AddSynonims( const good::string& sKey, const good::string& sValue );

	/// Add phrase with command meaning.
	static bool AddChat( const good::string& sKey, const good::string& sValue );

	/// Get command from text, returning number from 0 to 10 which represents matching.
	static float ChatFromText( const good::string& sText, CBotChat& cCommand );

	/// Get text from chat.
	static const good::string& ChatToText( const CBotChat& cCommand );


	/// Remove all chat variable values.
	static void CleanVariableValues()
	{
		for ( int i=0; i < m_aVariableValues.size(); ++i )
			m_aVariableValues[i].clear();
	}

	/// Add possible variables value.
	static TChatVariable AddVariable( const good::string& sVar, int iValuesSize = 0 )
	{
		m_aVariables.push_back(sVar);
		m_aVariableValues.push_back( StringVector(iValuesSize) );
		return m_aVariables.size() - 1;
	}

	/// Get chat variable from string.
	static TChatVariable GetVariable( const good::string& sVar )
	{
		StringVector::const_iterator it = good::find(m_aVariables.begin(), m_aVariables.end(), sVar);
		return ( it == m_aVariables.end() )  ?  EChatVariableInvalid  :  ( it - m_aVariables.begin() );
	}

	/// Add possible variables value.
	static TChatVariableValue AddVariableValue( TChatVariable iVar, const good::string& sValue )
	{
		StringVector& cValues = m_aVariableValues[iVar];
		cValues.push_back( sValue );
		return cValues.size() - 1;
	}

	/// Remove variables value.
	static const good::string& RemoveVariableValue( TChatVariable iVar, TChatVariableValue iValue )
	{
		m_aVariableValues[iVar][iValue] = "";
	}

	/// Add possible variables value.
	static const good::string& GetVariableValue( TChatVariable iVar, TChatVariableValue iValue )
	{
		return m_aVariableValues[iVar][iValue];
	}


	/// Get possible answers to a chat request.
	static const good::vector<TBotChat>& PossibleAnswers( TBotChat iTalk );

protected:
	static const good::string& GetSynonim( const good::string& sWord );


	static good::vector<CPhrase> m_aMatchPhrases[EBotChatTotal]; // Phrases for commands used for matching.
	static good::vector<CPhrase> m_aPhrases[EBotChatTotal];      // Phrases for commands used for generation of commands.
	
	static good::vector<StringVector> m_aSynonims;               // Available synonims.

	static StringVector m_aVariables;                            // Available variable names ($player, $door, $button, etc).
	static good::vector<StringVector> m_aVariableValues;         // Available variable values (1, 2, opened, closed, weapon_...).

};


#endif // __BOTRIX_CHAT_H__
