/**
 * @file Othello.cpp
 * @brief ���C���t�@�C��
 */

#include <iostream>
#include "Mydef.h"
#include "Board.h"
#include "Search.h"
#include "Gui.h"

using namespace std;

/**
 * @brief main�֐�
 * @return 0
 */
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	Gui	gui; // GUI�N���X

	// menu�Ŏg���ϐ��z���錾
	// 0:�X�^�[�g�@1:Player1�̐헪�@2:Player2�̐헪 
	// 3:Player1�̒T���[���@4:Player2�̒T���[��
	int menuItem[5] = { 0, -1, -1, -1, -1 };
	gui.menu(menuItem);	// ���j���[�̕\��

	Board board;	// �{�[�h�N���X
	Search player1Search(PLAYER1, menuItem[3], menuItem[1]); // Player1�̒T���N���X
	Search player2Search(PLAYER2, menuItem[4], menuItem[2]); // Player2�̒T���N���X

	// GUI�̕\��
	gui.setPlayerMsg(player1Search.method, player2Search.method);
	gui.setTurnMsg(board.nowTurn, 0);
	gui.setScoreMsg(board);
	gui.showBoard(board);

	while (1) {
		unsigned long long put;	// �w����

		// �Q�[���I�����肪�^�i�����v���C���[���Ɏw�����Ƃ̂ł���肪�Ȃ��j�ɂȂ�܂Ń��[�v
		if (!board.isGameFinished()) {

			// �p�X���N����Ȃ��i���܂��w�����Ƃ��ł���肪����j�Ȃ�w��������߂�
			if (!board.isPass()) {
				// ���݃^�[���̃v���C���[�̎w���������
				if(board.nowTurn == PLAYER1) put = player1Search.think(board);
				else put = player2Search.think(board);

				board.reverse(put);		// �w�����Ֆʂɔ��f����
				gui.setScoreMsg(board);	// �X�R�A���X�V
				board.swapBoard();		// ��Ԃ����ւ���
				gui.setTurnMsg(board.nowTurn, 0);
			}else{
				// �p�X���N�������Ԃ����ւ���
				gui.setTurnMsg(board.nowTurn, 1);
				board.swapBoard();
			}
		}else{
			// �Q�[�����I���̂��߃X�R�A���擾�D���ʂ�\���D
			int score[2] = {};
			board.getResult(score);
			if (score[0] > score[1]) gui.setTurnMsg(0, 2);
			else if (score[0] == score[1]) gui.setTurnMsg(2, 0);
			else gui.setTurnMsg(1, 2);
			gui.showBoard(board);
			break;
		}
		// �Ֆʂ�\��
		gui.showBoard(board);
	}
	while (!CheckHitKey(KEY_INPUT_RETURN)) {}
	DxLib_End();

	return 0;
}