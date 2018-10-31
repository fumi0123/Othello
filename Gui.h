
#include <iostream>
#include <string>
//#include "Mydef.h"
#include "DxLib.h"
//#include "Board.h"

using namespace std;
string turnMsg;
string player1Msg;
string player2Msg;
string player1Score;
string player2Score;

class Gui {
	int pieces[2];
	int back;
	int sideBack;
public:
	Gui() {
		SetGraphMode(640, 480, 32);
		ChangeWindowMode(TRUE);
		DxLib_Init();
		SetDrawScreen(DX_SCREEN_BACK);
		pieces[0] = LoadGraph("stoneB.png");
		pieces[1] = LoadGraph("stoneW.png");
		back = LoadGraph("board.png");
		sideBack = LoadGraph("sideBack.png");

		SetFontSize(18);                             //サイズを20に変更
		SetFontThickness(2);                         //太さを1に変更
		ChangeFont("ＭＳ 明朝");
//		ChangeFontType(DX_FONTTYPE_ANTIALIASING_EDGE);//アンチエイリアス＆エッジ付きフォントに変更
	}
	void showBoard(Board board) {
		DrawGraph(0, 0, back, FALSE);
		DrawGraph(480, 0, sideBack, FALSE);
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < 8; x++) {
				int num = 63 - (x + y * 8);
				if (((board.playerBoard >> num) & 1) == 1) {
					if (board.nowTurn == PLAYER1_TURN) DrawGraph(x * 60, y * 60, pieces[0], TRUE);
					else DrawGraph(x * 60, y * 60, pieces[1], TRUE);
				}
				else if (((board.opponentBoard >> num) & 1) == 1) {
					if (board.nowTurn == PLAYER1_TURN) DrawGraph(x * 60, y * 60, pieces[1], TRUE);
					else DrawGraph(x * 60, y * 60, pieces[0], TRUE);
				}
			}
		}
		//		DrawBox(192 - mw / 2 - 30, 172, 192 + mw / 2 + 30, 208, GetColor(200, 180, 150), TRUE);
		int mw1 = GetDrawStringWidth(turnMsg.c_str(), turnMsg.size());
		DrawString(480 + 80 - mw1 / 2, 60, turnMsg.c_str(), GetColor(0, 0, 0));

		int mw2 = GetDrawStringWidth(player1Msg.c_str(), player1Msg.size());
		DrawString(480 + 80 - mw2 / 2, 160, player1Msg.c_str(), GetColor(0, 0, 0));

		int mw3 = GetDrawStringWidth(player1Score.c_str(), player1Score.size());
		DrawString(480 + 80 - mw3 / 2, 240, player1Score.c_str(), GetColor(0, 0, 0));

		int mw4 = GetDrawStringWidth(player2Msg.c_str(), player2Msg.size());
		DrawString(480 + 80 - mw4 / 2, 320, player2Msg.c_str(), GetColor(0, 0, 0));

		int mw5 = GetDrawStringWidth(player2Score.c_str(), player2Score.size());
		DrawString(480 + 80 - mw5 / 2, 400, player2Score.c_str(), GetColor(0, 0, 0));

		ScreenFlip();
	}

	void setPlayerMsg(int player1, int player2) {
		string player_str[] = { "MANUAL", "RANDOM", "MINIMAX" };
		player1Msg = "Player1 : " + player_str[player1];
		player2Msg = "Player2 : " + player_str[player2];
	}
	// メッセージセット
	// turn ... 1:BLACK 2:WHITE 3:DRAW
	// type ... 0:TURN 1:PASS 2:WIN!
	void setTurnMsg(int turn, int type) {
		string turn_str[] = { "BLACK", "WHITE", "DRAW" };
		string type_str[] = { "TURN", "PASS", "WIN!" };
		if (turn == 1 || turn == 3) turnMsg = turn_str[turn - 1];
		else if (turn == -1) turnMsg = turn_str[turn + 2];
		if (turn != 3) turnMsg += " " + type_str[type];
	}

	void setScoreMsg(Board board) {
		if (board.nowTurn == PLAYER1_TURN) {
			player1Score = to_string(bitCount(board.playerBoard));
			player2Score = to_string(bitCount(board.opponentBoard));
		}else{
			player1Score = to_string(bitCount(board.opponentBoard));
			player2Score = to_string(bitCount(board.playerBoard));
		}
	}
};
