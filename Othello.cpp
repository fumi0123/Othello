/**
 * @file Othello.cpp
 * @brief メインファイル
 */

#include <iostream>
#include "Mydef.h"
#include "Board.h"
#include "Search.h"
#include "Gui.h"

using namespace std;

/**
 * @brief main関数
 * @return 0
 */
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	Gui	gui; // GUIクラス

	// menuで使う変数配列を宣言
	// 0:スタート　1:Player1の戦略　2:Player2の戦略 
	// 3:Player1の探索深さ　4:Player2の探索深さ
	int menuItem[5] = { 0, -1, -1, -1, -1 };
	gui.menu(menuItem);	// メニューの表示

	Board board;	// ボードクラス
	Search player1Search(PLAYER1, menuItem[3], menuItem[1]); // Player1の探索クラス
	Search player2Search(PLAYER2, menuItem[4], menuItem[2]); // Player2の探索クラス

	// GUIの表示
	gui.setPlayerMsg(player1Search.method, player2Search.method);
	gui.setTurnMsg(board.nowTurn, 0);
	gui.setScoreMsg(board);
	gui.showBoard(board);

	while (1) {
		unsigned long long put;	// 指し手

		// ゲーム終了判定が真（＝両プレイヤー共に指すことのできる手がない）になるまでループ
		if (!board.isGameFinished()) {

			// パスが起こらない（＝まだ指すことができる手がある）なら指し手を決める
			if (!board.isPass()) {
				// 現在ターンのプレイヤーの指し手を決定
				if(board.nowTurn == PLAYER1) put = player1Search.think(board);
				else put = player2Search.think(board);

				board.reverse(put);		// 指し手を盤面に反映する
				gui.setScoreMsg(board);	// スコアを更新
				board.swapBoard();		// 手番を入れ替える
				gui.setTurnMsg(board.nowTurn, 0);
			}else{
				// パスが起きたら手番を入れ替える
				gui.setTurnMsg(board.nowTurn, 1);
				board.swapBoard();
			}
		}else{
			// ゲームが終了のためスコアを取得．結果を表示．
			int score[2] = {};
			board.getResult(score);
			if (score[0] > score[1]) gui.setTurnMsg(0, 2);
			else if (score[0] == score[1]) gui.setTurnMsg(2, 0);
			else gui.setTurnMsg(1, 2);
			gui.showBoard(board);
			break;
		}
		// 盤面を表示
		gui.showBoard(board);
	}
	while (!CheckHitKey(KEY_INPUT_RETURN)) {}
	DxLib_End();

	return 0;
}