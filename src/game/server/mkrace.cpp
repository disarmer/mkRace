//mkRace
#include "gamecontext.h"
#include <engine/shared/config.h>
#include <game/server/player.h>
#include <game/server/gamemodes/race.h>
#include <game/version.h>
#include <game/server/entities/character.h>

//bool CheckClientID(int ClientID);

void ToggleSpecPause(IConsole::IResult *pResult, void *pUserData, int PauseType)
{
	//if(!CheckClientID(pResult->GetInteger(0)))
	//	return;

	CGameContext *pSelf = (CGameContext *) pUserData;
	IServer* pServ = pSelf->Server();
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if(!pPlayer)
		return;

	int PauseState = pPlayer->IsPaused();
	if(PauseState > 0)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "You are force-paused for %d seconds.", (PauseState - pServ->Tick()) / pServ->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "spec", aBuf);
	}
	else if(pResult->NumArguments() > 0)
	{
		if(-PauseState == PauseType && pPlayer->m_SpectatorID != pResult->m_ClientID && pServ->ClientIngame(pPlayer->m_SpectatorID) && !str_comp(pServ->ClientName(pPlayer->m_SpectatorID), pResult->GetString(0)))
		{
			pPlayer->Pause(CPlayer::PAUSE_NONE, false);
		}
		else
		{
			pPlayer->Pause(PauseType, false);
			pPlayer->SpectatePlayerName(pResult->GetString(0));
		}
	}
	else if(-PauseState == PauseType)
	{
		pPlayer->Pause(CPlayer::PAUSE_NONE, false);
	}
	else if(-PauseState != PauseType)
	{
		pPlayer->Pause(PauseType, false);
	}
}

void CGameContext::ConToggleSpec(IConsole::IResult *pResult, void *pUserData)
{
	ToggleSpecPause(pResult, pUserData, g_Config.m_SvPauseable ? CPlayer::PAUSE_SPEC : CPlayer::PAUSE_PAUSED);
}

void CGameContext::ConTogglePause(IConsole::IResult *pResult, void *pUserData)
{
	ToggleSpecPause(pResult, pUserData, CPlayer::PAUSE_PAUSED);
}

void CGameContext::ConSetEyeEmote(IConsole::IResult *pResult,
		void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	//if (!CheckClientID(pResult->m_ClientID))
	//	return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	if(pResult->NumArguments() == 0) {
		pSelf->m_pChatConsole->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				(pPlayer->m_EyeEmote) ?
						"You can now use the preset eye emotes." :
						"You don't have any eye emotes, remember to bind some. (until you die)");
		return;
	}
	else if(str_comp_nocase(pResult->GetString(0), "on") == 0)
		pPlayer->m_EyeEmote = true;
	else if(str_comp_nocase(pResult->GetString(0), "off") == 0)
		pPlayer->m_EyeEmote = false;
	else if(str_comp_nocase(pResult->GetString(0), "toggle") == 0)
		pPlayer->m_EyeEmote = !pPlayer->m_EyeEmote;
	pSelf->m_pChatConsole->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"emote",
			(pPlayer->m_EyeEmote) ?
					"You can now use the preset eye emotes." :
					"You don't have any eye emotes, remember to bind some. (until you die)");
}

void CGameContext::ConEyeEmote(IConsole::IResult *pResult, void *pUserData, int Emote, int Duration)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (g_Config.m_SvEmotionalTees == -1)
	{
		pSelf->m_pChatConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "emote",
				"Emotes are disabled.");
		return;
	}

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if(pPlayer->m_LastEyeEmote + g_Config.m_SvEyeEmoteChangeDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;

	pPlayer->m_DefEmote = Emote;
	
	pPlayer->m_DefEmoteReset = pSelf->Server()->Tick() + Duration * pSelf->Server()->TickSpeed();
	pPlayer->m_LastEyeEmote = pSelf->Server()->Tick();
}

void CGameContext::ConEmote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	//if (!CheckClientID(pResult->m_ClientID))
	//	return;

	if (pResult->NumArguments() == 0)
	{
		pSelf->m_pChatConsole->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				"Emote commands are: /emote surprise /emote blink /emote close /emote angry /emote happy /emote pain");
		pSelf->m_pChatConsole->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				"Example: /emote surprise 10 for 10 seconds or /emote surprise (default 5 seconds)");
	}
	else
	{
		int Emote = -1;
		if (!str_comp(pResult->GetString(0), "angry"))
			Emote = EMOTE_ANGRY;
		else if (!str_comp(pResult->GetString(0), "blink"))
			Emote = EMOTE_BLINK;
		else if (!str_comp(pResult->GetString(0), "close"))
			Emote = EMOTE_BLINK;
		else if (!str_comp(pResult->GetString(0), "happy"))
			Emote = EMOTE_HAPPY;
		else if (!str_comp(pResult->GetString(0), "pain"))
			Emote = EMOTE_PAIN;
		else if (!str_comp(pResult->GetString(0), "surprise"))
			Emote = EMOTE_SURPRISE;
		else if (!str_comp(pResult->GetString(0), "normal"))
			Emote = EMOTE_NORMAL;
		else
			pSelf->m_pChatConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD,
					"emote", "Unknown emote... Say /emote");

		int Duration = 5;
		if (pResult->NumArguments() > 1)
			Duration = pResult->GetInteger(1);

		if(Emote != -1)
			ConEyeEmote(pResult, pUserData, Emote, Duration);
	}
}

void CGameContext::ConEmoteLove(IConsole::IResult *pResult, void *pUserData)
{
	ConEyeEmote(pResult, pUserData, EMOTE_HAPPY, 3600);	
}

void CGameContext::ConEmoteHate(IConsole::IResult *pResult, void *pUserData)
{
	ConEyeEmote(pResult, pUserData, EMOTE_ANGRY, 3600);	
}

void CGameContext::ConEmotePain(IConsole::IResult *pResult, void *pUserData)
{
	ConEyeEmote(pResult, pUserData, EMOTE_PAIN, 3600);	
}

void CGameContext::ConEmoteBlink(IConsole::IResult *pResult, void *pUserData)
{
	ConEyeEmote(pResult, pUserData, EMOTE_BLINK, 3600);	
}

void CGameContext::ConEmoteSurprise(IConsole::IResult *pResult, void *pUserData)
{
	ConEyeEmote(pResult, pUserData, EMOTE_SURPRISE, 3600);	
}

void CGameContext::ConEmoteReset(IConsole::IResult *pResult, void *pUserData)
{
	ConEyeEmote(pResult, pUserData, EMOTE_NORMAL, 0);	
}

void CGameContext::ConMe(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	//if (!CheckClientID(pResult->m_ClientID))
	//	return;

	char aBuf[256 + 24];

	str_format(aBuf, 256 + 24, "'%s' %s",
			pSelf->Server()->ClientName(pResult->m_ClientID),
			pResult->GetString(0));
	if (g_Config.m_SvSlashMe)
		pSelf->SendChat(-1, CHAT_ALL, -1, aBuf);
	else
		pSelf->m_pChatConsole->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"me",
				"/me is disabled on this server");
}

void CGameContext::ConSwap(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	const int ClientID = pResult->m_ClientID;

	//if(!CheckClientID(ClientID))
	//	return;

	if(!g_Config.m_SvSwap)
	{
		pSelf->m_pChatConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "swap", "Swap is not activated.");
		return;
	}

	int TargetID = -1;

	for(int Victim = 0; Victim < MAX_CLIENTS; Victim++)
		if (str_comp_nocase(pResult->GetString(0), pSelf->Server()->ClientName(Victim)) == 0)
			TargetID = Victim;

	if(TargetID == ClientID || TargetID == -1)
		return;

	CCharacter * pChar1 = pSelf->GetPlayerChar(ClientID);
	CCharacter * pChar2 = pSelf->GetPlayerChar(TargetID);
	
	if(!pChar1 || !pChar2)
	{
		// someone is not alive
		pSelf->m_pChatConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "swap", "Can\'t swap!");
		return;
	}
	
	if(pChar1->m_SwapRequest == TargetID)
		return;

	char aBuf[256];
	// check if TargetID agrees
	if(pChar2->m_SwapRequest != ClientID)
	{
		// send  notification to TargetID
		str_format(aBuf, sizeof(aBuf), "%s wants to swap places with you. Type \'/swap %s\' to accept.",
			   pSelf->Server()->ClientName(ClientID), pSelf->Server()->ClientName(ClientID));
		pSelf->SendChat(-1, CHAT_ALL, TargetID, aBuf);

		str_format(aBuf, sizeof(aBuf), "Requst sent to %s.",
			   pSelf->Server()->ClientName(TargetID));
		pSelf->SendChat(-1, CHAT_ALL, ClientID, aBuf);

		pChar1->m_SwapRequest = TargetID;
	}
	else
	{
		// TargetID agreed
		CPlayerRescueState State1 = GetPlayerState(pChar1, ClientID),
			State2 = GetPlayerState(pChar2, TargetID);

		// swap
		SetPlayerState(State2, pChar1, ClientID);
		SetPlayerState(State1, pChar2, TargetID);

		str_format(aBuf, sizeof(aBuf), "%s swapped with %s.",
			   pSelf->Server()->ClientName(TargetID),
			   pSelf->Server()->ClientName(ClientID));
		pSelf->SendChat(-1, CHAT_ALL, -1, aBuf);

		// reset swap requests
		pChar1->m_SwapRequest = -1;
		pChar2->m_SwapRequest = -1;
	}
}

void CGameContext::ConDisconnectRescue(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	int TargetID = pResult->m_ClientID;
	
	if(!g_Config.m_SvDisconnectRescue)
		return;

	CCharacter * pChar = pSelf->GetPlayerChar(TargetID);

	if(!pChar)
		return;

	std::map<const char, CPlayerRescueState>::iterator iterator = pSelf->m_SavedPlayers.find(*pSelf->Server()->ClientName(TargetID));
	if(iterator == pSelf->m_SavedPlayers.end())
		return;

	CPlayerRescueState& State = iterator->second;
	CCharacter * m_SoloEnts[MAX_CLIENTS];
	if (pSelf->m_World.FindEntities(State.m_Pos, g_Config.m_SvDisconnectRescueRadius, (CEntity**) m_SoloEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER))
	{
		pSelf->m_pChatConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "dr", "Someone is standing in your place, you have to wait until they move away.");
		return;
	}
	SetPlayerState(State, pChar, TargetID);
	pSelf->m_SavedPlayers.erase(iterator);
}

void CGameContext::ConFake(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int mode = pResult->GetInteger(0), sender = pResult->GetInteger(1), receiver = pResult->GetInteger(2);

	CNetMsg_Sv_Chat Msg;
	Msg.m_Mode = mode;
	Msg.m_TargetID = (mode == CHAT_WHISPER) ? receiver : -1;
	Msg.m_ClientID = sender;
	Msg.m_pMessage = pResult->GetString(3);
	pSelf->Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, receiver);
}

void CGameContext::ConUnicast(IConsole::IResult* pResult, void* pUserData)
{
        CGameContext *pSelf = (CGameContext *)pUserData;
        pSelf->SendBroadcast(pResult->GetString(1), pResult->GetInteger(0));
}

void CGameContext::ConJetpack(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter* pChr = pSelf->GetPlayerChar(pResult->m_ClientID);
	if (pChr && pSelf->Server()->IsAuthed(pResult->m_ClientID))
		pChr->m_Jetpack = true;
}

void CGameContext::ConUnJetpack(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CCharacter* pChr = pSelf->GetPlayerChar(pResult->m_ClientID);
	if (pChr && pSelf->Server()->IsAuthed(pResult->m_ClientID))
		pChr->m_Jetpack = false;
}