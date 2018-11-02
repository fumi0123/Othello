/**
* @file Gui.h
* @brief GUIクラスヘッダ
*/

#include <iostream>
#include <string>
#include "DxLib.h"

using namespace std;

/**
* @brief GUIを管理するクラス
* @details 画面表示全般を管理．絵や文字を表示する．
*/
class Gui {
	int pieces[2];	// 黒石，白石の画像
	int back;		// 背景（ボード）画像
	int sideBack;	// 背景（文字表示）画像

	// メッセージ文字列
	string turnMsg;
	string player1Msg;
	string player2Msg;
	string player1Score;
	string player2Score;

public:
	/**
	* @brief GUIクラスのコンストラクタ
	*/
	Gui() {
		SetGraphMode(640, 480, 32);	// ウィンドウの大きさを640×480
		ChangeWindowMode(TRUE);
		DxLib_Init();
		SetDrawScreen(DX_SCREEN_BACK);
		// 各画像の読み込み
		pieces[0] = LoadGraph("stoneB.png");
		pieces[1] = LoadGraph("stoneW.png");
		back = LoadGraph("board.png");
		sideBack = LoadGraph("sideBack.png");

		SetFontSize(20);		// 文字サイズを20に変更
		SetFontThickness(2);	// 文字太さを2に変更
	}

	/**
	* @brief メニュー表示
	* @param item 各項目の選択状況
	* @details 各項目の選択状況に合わせて文字の表示を黒から赤に変更
	*/
	void showMenu(int *item) {
		string msg;

		// 両プレイヤーの名前を表示
		msg = "BLACK";
		int mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());			// 文字の横幅を取得
		DrawString(150 - mw / 2, 40, msg.c_str(), GetColor(255, 255, 255));	// 文字を表示
		msg = "WHITE";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		DrawString(490 - mw / 2, 40, msg.c_str(), GetColor(255, 255, 255));

		// 戦略の選択ボックスを表示
		// クリックされると文字色が黒から赤に変化
		DrawBox(60, 80, 240, 120, GetColor(204, 204, 204), TRUE);	// 箱を表示
		DrawBox(400, 80, 580, 120, GetColor(204, 204, 204), TRUE);
		msg = "MANUAL";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		// 選択されていないと黒，されていると赤文字で表示
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
		msg = "αβ";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		if (item[1] != 3) DrawString(150 - mw / 2, 250, msg.c_str(), GetColor(0, 0, 0));
		else DrawString(150 - mw / 2, 250, msg.c_str(), GetColor(255, 0, 0));
		if (item[2] != 3)DrawString(490 - mw / 2, 250, msg.c_str(), GetColor(0, 0, 0));
		else DrawString(490 - mw / 2, 250, msg.c_str(), GetColor(255, 0, 0));

		// αβ法が選ばれていると探索深さを表示
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

		// スタートを表示
		DrawBox(240, 420, 400, 460, GetColor(204, 204, 204), TRUE);
		msg = "START";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		DrawString(320 - mw / 2, 430, msg.c_str(), GetColor(0, 0, 0));

		ScreenFlip();
	}

	/**
	* @brief メニュー内での処理
	* @param item 各項目の選択状況
	* @details 一定範囲(ボックスの範囲内)がクリックされると選択したことになる．
	*/
	void menu(int *item) {
		showMenu(item);	// メニューを表示
		static bool mouse_flag = false;
		while (1) {
			// マウスの左クリックを感知
			if (GetMouseInput() & MOUSE_INPUT_LEFT) {
				// マウスを一度離さないと2回目以降の判定をしない
				if (!mouse_flag) {
					mouse_flag = true;

					int mx, my;					// マウスのクリック箇所
					GetMousePoint(&mx, &my);	// マウスのクリック箇所を取得

					// クリック箇所が一定範囲内ならば，対応したフラグをオンにする
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

					// スタートが押されたときかつ，他の選択に不足がない場合メニューを抜ける
					else if (240 <= mx && mx <= 400 && 420 <= my && my <= 460 
						&& item[1] != -1 && item[2] != -1 && !(item[1] == 3 && item[3] == -1) && !(item[2] == 3 && item[4] == -1)) 
						break;

					showMenu(item);
				}
			}else mouse_flag = false;
		}
	}
	
	/**
	* @brief ボード表示
	* @param board Boardインスタンス
	*/
	void showBoard(Board board) {
		// 背景の表示
		DrawGraph(0, 0, back, FALSE);
		DrawGraph(480, 0, sideBack, FALSE);

		// 石の表示
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < 8; x++) {
				int num = 63 - (x + y * 8);
				if (((board.playerBoard >> num) & 1) == 1) DrawGraph(x * 60, y * 60, pieces[board.nowTurn], TRUE);
				else if (((board.opponentBoard >> num) & 1) == 1) DrawGraph(x * 60, y * 60, pieces[1 - board.nowTurn], TRUE);
			}
		}
		// 文字の表示
		// 1. 現在のターン表示
		// 2,3. プレイヤーの名前表示
		// 4,5. プレイヤーのスコア表示
		int mw = GetDrawStringWidth(turnMsg.c_str(), (int)turnMsg.size());
		DrawString(480 + 80 - mw / 2, 60, turnMsg.c_str(), GetColor(0, 0, 0));

		mw = GetDrawStringWidth(player1Msg.c_str(), (int)player1Msg.size());
		DrawString(480 + 80 - mw / 2, 160, player1Msg.c_str(), GetColor(0, 0, 0));

		mw = GetDrawStringWidth(player1Score.c_str(), (int)player1Score.size());
		DrawString(480 + 80 - mw / 2, 200, player1Score.c_str(), GetColor(0, 0, 0));

		mw = GetDrawStringWidth(player2Msg.c_str(), (int)player2Msg.size());
		DrawString(480 + 80 - mw / 2, 320, player2Msg.c_str(), GetColor(0, 0, 0));

		mw = GetDrawStringWidth(player2Score.c_str(), (int)player2Score.size());
		DrawString(480 + 80 - mw / 2, 360, player2Score.c_str(), GetColor(0, 0, 0));

		ScreenFlip();
	}

	/**
	* @brief プレイヤーの名前と使用戦略のメッセージのセット
	* @param method1 黒番の戦略
	* @param method2 白番の戦略
	*/
	void setPlayerMsg(int method1, int method2) {
		string player_str[] = { "MANUAL", "RANDOM", "MINIMAX", "αβ" };
		player1Msg = "BLACK : " + player_str[method1];
		player2Msg = "WHITE : " + player_str[method2];
	}

	/**
	* @brief ターンメッセージのセット
	* @param turn プレイヤー
	* @param type プレイヤーの状況
	*/
	void setTurnMsg(int turn, int type) {
		string turn_str[] = { "BLACK", "WHITE", "DRAW" };
		string type_str[] = { "TURN", "PASS", "WIN!" };
		turnMsg = turn_str[turn];
		if (turn != 2) turnMsg += " " + type_str[type];
	}

	/**
	* @brief スコア値のセット
	* @param board Boardインスタンス
	*/
	void setScoreMsg(Board board) {
		if (board.nowTurn == PLAYER1) {
			// 現在の盤面を引数とし，石の数を取得
			player1Score = to_string(bitCount(board.playerBoard));
			player2Score = to_string(bitCount(board.opponentBoard));
		}else{
			player1Score = to_string(bitCount(board.opponentBoard));
			player2Score = to_string(bitCount(board.playerBoard));
		}
	}
};
