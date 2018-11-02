/**
* @file Gui.h
* @brief GUI�N���X�w�b�_
*/

#include <iostream>
#include <string>
#include "DxLib.h"

using namespace std;

/**
* @brief GUI���Ǘ�����N���X
* @details ��ʕ\���S�ʂ��Ǘ��D�G�╶����\������D
*/
class Gui {
	int pieces[2];	// ���΁C���΂̉摜
	int back;		// �w�i�i�{�[�h�j�摜
	int sideBack;	// �w�i�i�����\���j�摜

	// ���b�Z�[�W������
	string turnMsg;
	string player1Msg;
	string player2Msg;
	string player1Score;
	string player2Score;

public:
	/**
	* @brief GUI�N���X�̃R���X�g���N�^
	*/
	Gui() {
		SetGraphMode(640, 480, 32);	// �E�B���h�E�̑傫����640�~480
		ChangeWindowMode(TRUE);
		DxLib_Init();
		SetDrawScreen(DX_SCREEN_BACK);
		// �e�摜�̓ǂݍ���
		pieces[0] = LoadGraph("stoneB.png");
		pieces[1] = LoadGraph("stoneW.png");
		back = LoadGraph("board.png");
		sideBack = LoadGraph("sideBack.png");

		SetFontSize(20);		// �����T�C�Y��20�ɕύX
		SetFontThickness(2);	// ����������2�ɕύX
	}

	/**
	* @brief ���j���[�\��
	* @param item �e���ڂ̑I����
	* @details �e���ڂ̑I���󋵂ɍ��킹�ĕ����̕\����������ԂɕύX
	*/
	void showMenu(int *item) {
		string msg;

		// ���v���C���[�̖��O��\��
		msg = "BLACK";
		int mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());			// �����̉������擾
		DrawString(150 - mw / 2, 40, msg.c_str(), GetColor(255, 255, 255));	// ������\��
		msg = "WHITE";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		DrawString(490 - mw / 2, 40, msg.c_str(), GetColor(255, 255, 255));

		// �헪�̑I���{�b�N�X��\��
		// �N���b�N�����ƕ����F��������Ԃɕω�
		DrawBox(60, 80, 240, 120, GetColor(204, 204, 204), TRUE);	// ����\��
		DrawBox(400, 80, 580, 120, GetColor(204, 204, 204), TRUE);
		msg = "MANUAL";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		// �I������Ă��Ȃ��ƍ��C����Ă���Ɛԕ����ŕ\��
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

		// �����@���I�΂�Ă���ƒT���[����\��
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

		// �X�^�[�g��\��
		DrawBox(240, 420, 400, 460, GetColor(204, 204, 204), TRUE);
		msg = "START";
		mw = GetDrawStringWidth(msg.c_str(), (int)msg.size());
		DrawString(320 - mw / 2, 430, msg.c_str(), GetColor(0, 0, 0));

		ScreenFlip();
	}

	/**
	* @brief ���j���[���ł̏���
	* @param item �e���ڂ̑I����
	* @details ���͈�(�{�b�N�X�͈͓̔�)���N���b�N�����ƑI���������ƂɂȂ�D
	*/
	void menu(int *item) {
		showMenu(item);	// ���j���[��\��
		static bool mouse_flag = false;
		while (1) {
			// �}�E�X�̍��N���b�N�����m
			if (GetMouseInput() & MOUSE_INPUT_LEFT) {
				// �}�E�X����x�����Ȃ���2��ڈȍ~�̔�������Ȃ�
				if (!mouse_flag) {
					mouse_flag = true;

					int mx, my;					// �}�E�X�̃N���b�N�ӏ�
					GetMousePoint(&mx, &my);	// �}�E�X�̃N���b�N�ӏ����擾

					// �N���b�N�ӏ������͈͓��Ȃ�΁C�Ή������t���O���I���ɂ���
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

					// �X�^�[�g�������ꂽ�Ƃ����C���̑I���ɕs�����Ȃ��ꍇ���j���[�𔲂���
					else if (240 <= mx && mx <= 400 && 420 <= my && my <= 460 
						&& item[1] != -1 && item[2] != -1 && !(item[1] == 3 && item[3] == -1) && !(item[2] == 3 && item[4] == -1)) 
						break;

					showMenu(item);
				}
			}else mouse_flag = false;
		}
	}
	
	/**
	* @brief �{�[�h�\��
	* @param board Board�C���X�^���X
	*/
	void showBoard(Board board) {
		// �w�i�̕\��
		DrawGraph(0, 0, back, FALSE);
		DrawGraph(480, 0, sideBack, FALSE);

		// �΂̕\��
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < 8; x++) {
				int num = 63 - (x + y * 8);
				if (((board.playerBoard >> num) & 1) == 1) DrawGraph(x * 60, y * 60, pieces[board.nowTurn], TRUE);
				else if (((board.opponentBoard >> num) & 1) == 1) DrawGraph(x * 60, y * 60, pieces[1 - board.nowTurn], TRUE);
			}
		}
		// �����̕\��
		// 1. ���݂̃^�[���\��
		// 2,3. �v���C���[�̖��O�\��
		// 4,5. �v���C���[�̃X�R�A�\��
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
	* @brief �v���C���[�̖��O�Ǝg�p�헪�̃��b�Z�[�W�̃Z�b�g
	* @param method1 ���Ԃ̐헪
	* @param method2 ���Ԃ̐헪
	*/
	void setPlayerMsg(int method1, int method2) {
		string player_str[] = { "MANUAL", "RANDOM", "MINIMAX", "����" };
		player1Msg = "BLACK : " + player_str[method1];
		player2Msg = "WHITE : " + player_str[method2];
	}

	/**
	* @brief �^�[�����b�Z�[�W�̃Z�b�g
	* @param turn �v���C���[
	* @param type �v���C���[�̏�
	*/
	void setTurnMsg(int turn, int type) {
		string turn_str[] = { "BLACK", "WHITE", "DRAW" };
		string type_str[] = { "TURN", "PASS", "WIN!" };
		turnMsg = turn_str[turn];
		if (turn != 2) turnMsg += " " + type_str[type];
	}

	/**
	* @brief �X�R�A�l�̃Z�b�g
	* @param board Board�C���X�^���X
	*/
	void setScoreMsg(Board board) {
		if (board.nowTurn == PLAYER1) {
			// ���݂̔Ֆʂ������Ƃ��C�΂̐����擾
			player1Score = to_string(bitCount(board.playerBoard));
			player2Score = to_string(bitCount(board.opponentBoard));
		}else{
			player1Score = to_string(bitCount(board.opponentBoard));
			player2Score = to_string(bitCount(board.playerBoard));
		}
	}
};
