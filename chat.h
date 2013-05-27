#ifndef __BOTRIX_CHAT_H__
#define __BOTRIX_CHAT_H__


#include "types.h"
#include "good/graph.h"


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

	/// Get text from command.
	static const good::string& ChatToText( const CBotChat& cCommand );

	/// Get possible answers to a chat request. Arry ends with -1.
	static const good::vector<TBotChat>& PossibleAnswers( TBotChat iTalk );

protected:
	static const good::string& GetSynonim( const good::string& sWord );


	static good::vector<CPhrase> m_aMatchPhrases[EBotChatTotal]; // Phrases for commands used for matching.
	static good::vector<CPhrase> m_aPhrases[EBotChatTotal];      // Phrases for commands used for generation of commands.
	
	static good::vector<StringVector> m_aSynonims;                  // Available synonims.

};

#endif // __BOTRIX_CHAT_H__
