
#include <iostream>
#include <string>
//#include "Mydef.h"
#include "DxLib.h"
//#include "Board.h"

using namespace std;

class Gui {
	int pieces[2];
	int back;
	int sideBack;

	string turnMsg;
	string player1Msg;
	string player2Msg;
	string player1Score;
	string player2Score;



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

		SetFontSize(20);                             //�T�C�Y��20�ɕύX
		SetFontThickness(2);                         //������1�ɕύX
//		ChangeFont("�l�r ����");
//		ChangeFontType(DX_FONTTYPE_ANTIALIASING_EDGE);//�A���`�G�C���A�X���G�b�W�t���t�H���g�ɕύX
	}

	void showMenu(int *item) {
//		DrawGraph(0, 0, back, FALSE);
		string msg;

		msg = "BLACK";
		int mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		DrawString(150 - mw / 2, 40, msg.c_str(), GetColor(255, 255, 255));

		msg = "WHITE";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		DrawString(490 - mw / 2, 40, msg.c_str(), GetColor(255, 255, 255));

		DrawBox(60, 80, 240, 120, GetColor(204, 204, 204), TRUE);
		DrawBox(400, 80, 580, 120, GetColor(204, 204, 204), TRUE);
		msg = "MANUAL";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		if(item[1] != 0) DrawString(150 - mw / 2, 90, msg.c_str(), GetColor(0, 0, 0));
		else DrawString(150 - mw / 2, 90, msg.c_str(), GetColor(255, 0, 0));

		if (item[2] != 0) DrawString(490 - mw / 2, 90, msg.c_str(), GetColor(0, 0, 0));
		else DrawString(490 - mw / 2, 90, msg.c_str(), GetColor(255, 0, 0));

		DrawBox(60, 160, 240, 200, GetColor(204, 204, 204), TRUE);
		DrawBox(400,160, 580, 200, GetColor(204, 204, 204), TRUE);
		msg = "RANDOM";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		if (item[1] != 1) DrawString(150 - mw / 2, 170, msg.c_str(), GetColor(0, 0, 0));
		else DrawString(150 - mw / 2, 170, msg.c_str(), GetColor(255, 0, 0));

		if (item[2] != 1) DrawString(490 - mw / 2, 170, msg.c_str(), GetColor(0, 0, 0));
		else DrawString(490 - mw / 2, 170, msg.c_str(), GetColor(255, 0, 0));

		DrawBox(60, 240, 240, 280, GetColor(204, 204, 204), TRUE);
		DrawBox(400, 240, 580, 280, GetColor(204, 204, 204), TRUE);
		msg = "����";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		if (item[1] != 3) DrawString(150 - mw / 2, 250, msg.c_str(), GetColor(0, 0, 0));
		else DrawString(150 - mw / 2, 250, msg.c_str(), GetColor(255, 0, 0));
		if (item[2] != 3)DrawString(490 - mw / 2, 250, msg.c_str(), GetColor(0, 0, 0));
		else DrawString(490 - mw / 2, 250, msg.c_str(), GetColor(255, 0, 0));

		string nMsg[5] = { "3", "4", "5", "6", "7" };
		for (int i = 0; i < 5; i++) {
			mw = GetDrawStringWidth(nMsg[i].c_str(), (int)nMsg[i].size());
			if (item[1] == 3 && item[3] == i + 3) DrawString(70 + 40 * i - mw / 2, 290, nMsg[i].c_str(), GetColor(255, 0, 0));
			else if (item[1] == 3) DrawString(70 + 40 * i - mw / 2, 290, nMsg[i].c_str(), GetColor(255, 255, 255));
			else DrawString(70 + 40 * i - mw / 2, 290, nMsg[i].c_str(), GetColor(0, 0, 0));

			if (item[2] == 3 && item[4] == i + 3) DrawString(410 + 40 * i - mw / 2, 290, nMsg[i].c_str(), GetColor(255, 0, 0));
			else if (item[2] == 3) DrawString(410 + 40 * i - mw / 2, 290, nMsg[i].c_str(), GetColor(255, 255, 255));
			else DrawString(410 + 40 * i - mw / 2, 290, nMsg[i].c_str(), GetColor(0, 0, 0));
		}

		DrawBox(240, 420, 400, 460, GetColor(204, 204, 204), TRUE);
		msg = "START";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		DrawString(320 - mw / 2, 430, msg.c_str(), GetColor(0, 0, 0));

		ScreenFlip();
	}

	void menu(int *item) {
		showMenu(item);
		static bool mouse_flag = false;
		while (1) {
			if (GetMouseInput() & MOUSE_INPUT_LEFT) {
				if (!mouse_flag) {
					mouse_flag = true;
					int mx, my;
					GetMousePoint(&mx, &my);
					if (60 <= mx && mx <= 240 && 80 <= my && my <= 120) item[1] = 0;
					else if (400 <= mx && mx <= 580 && 80 <= my && my <= 120) item[2] = 0;

					else if (60 <= mx && mx <= 240 && 160 <= my && my <= 200) item[1] = 1;
					else if (400 <= mx && mx <= 580 && 160 <= my && my <= 200) item[2] = 1;

					else if (60 <= mx && mx <= 240 && 240 <= my && my <= 280) item[1] = 3;
					else if (400 <= mx && mx <= 580 && 240 <= my && my <= 280) item[2] = 3;
					
					else if (55 <= mx && mx <= 85 && 290 <= my && my <= 330) item[3] = 3;
					else if (95 <= mx && mx <= 125 && 290 <= my && my <= 330) item[3] = 4;
					else if (135 <= mx && mx <= 165 && 290 <= my && my <= 330) item[3] = 5;
					else if (175 <= mx && mx <= 205 && 290 <= my && my <= 330) item[3] = 6;
					else if (215 <= mx && mx <= 245 && 290 <= my && my <= 330) item[3] = 7;
		
					else if (395 <= mx && mx <= 425 && 290 <= my && my <= 330) item[4] = 3;
					else if (435 <= mx && mx <= 465 && 290 <= my && my <= 330) item[4] = 4;
					else if (475 <= mx && mx <= 505 && 290 <= my && my <= 330) item[4] = 5;
					else if (515 <= mx && mx <= 545 && 290 <= my && my <= 330) item[4] = 6;
					else if (555 <= mx && mx <= 585 && 290 <= my && my <= 330) item[4] = 7;

					else if (240 <= mx && mx <= 400 && 420 <= my && my <= 460 
						&& item[1] != -1 && item[2] != -1 && !(item[1] == 3 && item[3] == -1) && !(item[2] == 3 && item[4] == -1)) 
						break;

					showMenu(item);
				}
			}else mouse_flag = false;
		}
	}

	void showBoard(Board board) {
		DrawGraph(0, 0, back, FALSE);
		DrawGraph(480, 0, sideBack, FALSE);
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < 8; x++) {
				int num = 63 - (x + y * 8);
				if (((board.playerBoard >> num) & 1) == 1) DrawGraph(x * 60, y * 60, pieces[board.nowTurn], TRUE);
				else if (((board.opponentBoard >> num) & 1) == 1) DrawGraph(x * 60, y * 60, pieces[1 - board.nowTurn], TRUE);
			}
		}

//		DrawBox(485, 140, 635, 230, GetColor(255, 255, 255), TRUE);
//		DrawBox(485, 300, 635, 390, GetColor(255, 255, 255), TRUE);

		int mw1 = GetDrawStringWidth(turnMsg.c_str(), (int)turnMsg.size());
		DrawString(480 + 80 - mw1 / 2, 60, turnMsg.c_str(), GetColor(0, 0, 0));

		int mw2 = GetDrawStringWidth(player1Msg.c_str(), (int)player1Msg.size());
		DrawString(480 + 80 - mw2 / 2, 160, player1Msg.c_str(), GetColor(0, 0, 0));

		int mw3 = GetDrawStringWidth(player1Score.c_str(), (int)player1Score.size());
		DrawString(480 + 80 - mw3 / 2, 200, player1Score.c_str(), GetColor(0, 0, 0));

		int mw4 = GetDrawStringWidth(player2Msg.c_str(), (int)player2Msg.size());
		DrawString(480 + 80 - mw4 / 2, 320, player2Msg.c_str(), GetColor(0, 0, 0));

		int mw5 = GetDrawStringWidth(player2Score.c_str(), (int)player2Score.size());
		DrawString(480 + 80 - mw5 / 2, 360, player2Score.c_str(), GetColor(0, 0, 0));

		ScreenFlip();
	}

	void setPlayerMsg(int player1, int player2) {
		string player_str[] = { "MANUAL", "RANDOM", "MINIMAX", "����" };
		player1Msg = "BLACK : " + player_str[player1];
		player2Msg = "WHITE : " + player_str[player2];
	}
	// ���b�Z�[�W�Z�b�g
	// turn ... 0:BLACK 1:WHITE 2:DRAW
	// type ... 0:TURN 1:PASS 2:WIN!
	void setTurnMsg(int turn, int type) {
		string turn_str[] = { "BLACK", "WHITE", "DRAW" };
		string type_str[] = { "TURN", "PASS", "WIN!" };
		turnMsg = turn_str[turn];
		if (turn != 2) turnMsg += " " + type_str[type];
	}

	void setScoreMsg(Board board) {
		if (board.nowTurn == PLAYER1) {
			player1Score = to_string(bitCount(board.playerBoard));
			player2Score = to_string(bitCount(board.opponentBoard));
		}else{
			player1Score = to_string(bitCount(board.opponentBoard));
			player2Score = to_string(bitCount(board.playerBoard));
		}
	}
};
