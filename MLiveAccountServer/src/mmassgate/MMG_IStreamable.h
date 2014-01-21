#pragma once

class MMG_IStreamable
{
public:

private:

public:
	virtual void ToStream	(MN_WriteMessage *aMessage)	= 0;
	virtual bool FromStream	(MN_ReadMessage *aMessage)	= 0;
};