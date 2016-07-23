#include "../stdafx.h"

MMG_LadderProtocol::LadderRsp::LadderRsp() : ladderItems()
{
	this->startPos				= 0;
	this->requestId				= 0;
	this->ladderSize			= 0;
	this->responseIsFull		= false;
}

/*void MMG_LadderProtocol::LadderRsp::ToStream(MN_WriteMessage *aMessage)
{	
}

bool MMG_LadderProtocol::LadderRsp::FromStream(MN_ReadMessage *aMessage)
{
	return true;
}
*/

MMG_LadderProtocol::MMG_LadderProtocol()
{
}