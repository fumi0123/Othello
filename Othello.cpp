#include <iostream>
//#include "DxLib.h"
#include "Mydef.h"
#include "Board.h"
#include "Search.h"
#include "Gui.h"

using namespace std;

// WinMain
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	Gui	gui;

	int start = 0;
	int menuItem[5] = { 0, -1, -1, -1, -1 };

	gui.menu(menuItem);

	Board board;
	Search player1Search(PLAYER1, menuItem[3], menuItem[1]);
	Search player2Search(PLAYER2, menuItem[4], menuItem[2]);

	gui.setPlayerMsg(player1Search.method, player2Search.method);
	gui.setTurnMsg(board.nowTurn, 0);
	gui.setScoreMsg(board);
	gui.showBoard(board);

	while (1) {
		unsigned long long put;
		if (!board.isGameFinished()) {
			if (!board.isPass()) {
				if(board.nowTurn == PLAYER1) put = player1Search.think(board);
				else put = player2Search.think(board);
				board.reverse(put);
				gui.setScoreMsg(board);
				board.swapBoard();
				gui.setTurnMsg(board.nowTurn, 0);
			}else {
				gui.setTurnMsg(board.nowTurn, 1);
				board.swapBoard();
			}
		}else{
			int score[2] = {};
			board.getResult(score);
			if (score[0] > score[1]) gui.setTurnMsg(0, 2);
			else if (score[0] == score[1]) gui.setTurnMsg(2, 0);
			else gui.setTurnMsg(1, 2);
			gui.showBoard(board);
			break;
		}
		gui.showBoard(board);
	}

	while (!CheckHitKey(KEY_INPUT_RETURN)) {}
	DxLib_End();	
	return 0;
}