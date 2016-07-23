#pragma once

CLASS_SINGLE(MMG_LadderProtocol)
{
public:
	class LadderRsp
	{
	public:
		class LadderItem
		{
		public:
			MMG_Profile profile;
			uint score;

			LadderItem(): profile()
			{
				score = 0;
			}
		};

		LadderItem ladderItems[100];
		uint startPos;
		uint requestId;
		uint ladderSize;
		bool responseIsFull;

	private:

	public:
		LadderRsp();

		//void ToStream	(MN_WriteMessage *aMessage);
		//bool FromStream	(MN_ReadMessage *aMessage);
	};

private:

public:
	MMG_LadderProtocol();
};