#include "Client_PCH.h"

#include "Gpackets/GCMove.h"
#include "Gpackets/GCMoveError.h"
#include "Gpackets/GCMoveOK.h"
#include "Gpackets/GCAddVampireCorpse.h"
#include "Gpackets/GCGlobalChat.h"
#include "Gpackets/GCGuildChat.h"
#include "Gpackets/GCPartySay.h"
#include "Gpackets/GCPartyJoined.h"
#include "Gpackets/GCPartyLeave.h"
#include "Gpackets/GCSay.h"
#include "Gpackets/GCSystemMessage.h"
#include "Gpackets/GCWhisper.h"
#include "Gpackets/GCWhisperFailed.h"

void GCMoveHandler::execute(GCMove*, Player*)
{
}

void GCMoveOKHandler::execute(GCMoveOK*, Player*)
{
}

void GCMoveErrorHandler::execute(GCMoveError*, Player*)
{
}

void GCAddVampireCorpseHandler::execute(GCAddVampireCorpse*, Player*)
{
}

void GCSayHandler::execute(GCSay*, Player*)
{
}

void GCGlobalChatHandler::execute(GCGlobalChat*, Player*)
{
}

void GCWhisperHandler::execute(GCWhisper*, Player*)
{
}

void GCWhisperFailedHandler::execute(GCWhisperFailed*, Player*)
{
}

void GCPartySayHandler::execute(GCPartySay*, Player*)
{
}

void GCGuildChatHandler::execute(GCGuildChat*, Player*)
{
}

void GCPartyJoinedHandler::execute(GCPartyJoined*, Player*)
{
}

void GCPartyLeaveHandler::execute(GCPartyLeave*, Player*)
{
}

void GCSystemMessageHandler::execute(GCSystemMessage*, Player*)
{
}
