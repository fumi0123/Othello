#include <iostream>
//#include "DxLib.h"
#include "Mydef.h"
#include "Board.h"
#include "Search.h"
#include "Gui.h"

using namespace std;

// WinMain
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	unsigned long long put;
	Gui	gui;
	Board board;
	Search player1Search(1, 6, MINIMAX);
	Search player2Search(-1, 6, MANUAL);
	gui.setPlayerMsg(player1Search.method, player2Search.method);

	gui.setTurnMsg(board.nowTurn, 0);
	gui.showBoard(board);
	while (1) {
		unsigned long long put;
		if (!board.isGameFinished()) {
			if (!board.isPass()) {
				if(board.nowTurn == PLAYER1_TURN) put = player1Search.think(board);
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
			if (score[0] > score[1]) gui.setTurnMsg(1, 2);
			else if (score[0] == score[1]) gui.setTurnMsg(3, 0);
			else gui.setTurnMsg(-1, 2);
			gui.showBoard(board);
			break;
		}
		gui.showBoard(board);
	}
//	system("pause");
	while (!CheckHitKey(KEY_INPUT_RETURN)) {}
	DxLib_End();
//	cout << hex << makeLegalBoard(0x000109250D013F3F, 0x7DFEF6DAF2FEC0C0) << endl;
	
	return 0;
}