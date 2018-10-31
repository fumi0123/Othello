#include <iostream>
#include <string>
#include <random>
#include <iomanip>
//#include "Mydef.h"

using namespace std;

unsigned long long transfer(unsigned long long put, int k);
int bitCount(unsigned long long num);
unsigned long long makeLegalBoard(unsigned long long playerBoard, unsigned long long opponentBoard);

class Board {
public:
	unsigned long long playerBoard = 0;
	unsigned long long opponentBoard = 0;

	int nowTurn;
	int nowIndex;

	Board() {
		this->nowTurn = PLAYER1_TURN;
		this->nowIndex = 1;

		playerBoard = 0x0000000810000000;
		opponentBoard = 0x0000001008000000;
	}



	void initBoard() {
		this->nowTurn = PLAYER1_TURN;
		this->nowIndex = 1;

		playerBoard = 0x0000000810000000;
		opponentBoard = 0x0000001008000000;
	}
	/*
	void showBoard() {
		cout << endl;
		cout << " A B C D E F G H " << endl;

		for (int y = 0; y < 8; y++) {
			cout << y + 1;
			for (int x = 0; x < 8; x++) {
				if ((playerBoard >> 63 - (x + y * 8)) & 1 == 1) {
					if (nowTurn == PLAYER1_TURN) cout << "œ";
					else cout << "";
				}
				else if ((opponentBoard >> 63 - (x + y * 8)) & 1 == 1) {
					if (nowTurn == PLAYER1_TURN) cout << "";
					else cout << "œ";
				}
				else {
					cout << " ";
				}
			}
			cout << endl;
		}
	}*/
/*	void showBoard() {
		DrawGraph(0, 0, back, FALSE);
		DrawGraph(480, 0, sideBack, FALSE);
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < 8; x++) {
				int num = 63 - (x + y * 8);
				if (((playerBoard >> num) & 1) == 1) {
					if (nowTurn == PLAYER1_TURN) DrawGraph(x * 60, y * 60, pieces[0], TRUE);
					else DrawGraph(x * 60, y * 60, pieces[1], TRUE);
				}
				else if (((opponentBoard >> num) & 1) == 1) {
					if (nowTurn == PLAYER1_TURN) DrawGraph(x * 60, y * 60, pieces[1], TRUE);
					else DrawGraph(x * 60, y * 60, pieces[0], TRUE);
				}
			}
		}
		int mw = GetDrawStringWidth(turnMsg.c_str(), turnMsg.size());
//		DrawBox(192 - mw / 2 - 30, 172, 192 + mw / 2 + 30, 208, GetColor(200, 180, 150), TRUE);
		DrawString(480 + 80 - mw / 2, 80, turnMsg.c_str(), GetColor(0, 0, 0));
		ScreenFlip();
	}*/


	bool canPut(unsigned long long put) {
		unsigned long long legalBoard = makeLegalBoard(this->playerBoard, this->opponentBoard);
		if ((put & legalBoard) == put) {
			return true;
		}
		return false;
	}

	void reverse(unsigned long long put) {
		unsigned long long rev = 0;
		for (int k = 0; k < 8; k++) {
			unsigned long long rev_ = 0;
			unsigned long long mask = transfer(put, k);
			while ((mask != 0) && ((mask & opponentBoard) != 0)) {
				rev_ |= mask;
				mask = transfer(mask, k);
			}
			if ((mask & playerBoard) != 0) rev |= rev_;
		}
		playerBoard ^= put | rev;
		opponentBoard ^= rev;

		nowIndex = this->nowIndex + 1;
	}

	bool isPass() {
		unsigned long long playerLegalBoard = makeLegalBoard(playerBoard, opponentBoard);
		unsigned long long opponentLegalBoard = makeLegalBoard(opponentBoard, playerBoard);
		if ((bitCount(playerLegalBoard) == 0) && (bitCount(opponentLegalBoard) != 0)) {
			return true;
		}
		return false;
	}

	bool isGameFinished() {
		unsigned long long playerLegalBoard = makeLegalBoard(playerBoard, opponentBoard);
		unsigned long long opponentLegalBoard = makeLegalBoard(opponentBoard, playerBoard);
		if ((bitCount(playerLegalBoard) == 0) && (bitCount(opponentLegalBoard) == 0)) {
			return true;
		}
		return false;
	}

	void swapBoard() {
		unsigned long long tmp = playerBoard;
		playerBoard = opponentBoard;
		opponentBoard = tmp;

		nowTurn = nowTurn * -1;
	}

	void getResult(int *score) {
//		cout << "p " << hex << playerBoard << "o " << hex << opponentBoard << endl;
		int blackScore = bitCount(playerBoard);
		int whiteScore = bitCount(opponentBoard);
		if (this->nowTurn == PLAYER2_TURN) {
			int tmp = blackScore;
			blackScore = whiteScore;
			whiteScore = tmp;
		}
		score[0] = blackScore;
		score[1] = whiteScore;
	}

};

unsigned long long coordToBit(int x, int y) {
	unsigned long long mask = 0x8000000000000000;

	switch (x) {
	case 0:
		break;
	case 1:
		mask = mask >> 1;
		break;
	case 2:
		mask = mask >> 2;
		break;
	case 3:
		mask = mask >> 3;
		break;
	case 4:
		mask = mask >> 4;
		break;
	case 5:
		mask = mask >> 5;
		break;
	case 6:
		mask = mask >> 6;
		break;
	case 7:
		mask = mask >> 7;
		break;
	default:
		break;
	}

	mask = mask >> (y * 8);

	return mask;
}

int bitCount(unsigned long long num) {
	int boardSize = 64;
	unsigned long long mask = 0x8000000000000000;
	int count = 0;

	for (int i = 0; i < boardSize; i++) {
		if ((mask & num) != 0) {
			count += 1;
		}
		mask = mask >> 1;
	}
	return count;
}

unsigned long long transfer(unsigned long long put, int k) {
	switch (k) {
	case 0:
		return (put << 8) & 0xffffffffffffff00;
	case 1:
		return (put << 7) & 0x7f7f7f7f7f7f7f00;
	case 2:
		return (put >> 1) & 0x7f7f7f7f7f7f7f7f;
	case 3:
		return (put >> 9) & 0x007f7f7f7f7f7f7f;
	case 4:
		return (put >> 8) & 0x00ffffffffffffff;
	case 5:
		return (put >> 7) & 0x00fefefefefefefe;
	case 6:
		return (put << 1) & 0xfefefefefefefefe;
	case 7:
		return (put << 9) & 0xfefefefefefefe00;
	default:
		return 0;
	}
}

unsigned long long makeLegalBoard(unsigned long long playerBoard, unsigned long long opponentBoard) {
	unsigned long long horWatchBoard = opponentBoard & 0x7e7e7e7e7e7e7e7e;
	unsigned long long verWatchBoard = opponentBoard & 0x00FFFFFFFFFFFF00;
	unsigned long long allSideWatchBoard = opponentBoard & 0x007e7e7e7e7e7e00;
	unsigned long long blankBoard = ~(playerBoard | opponentBoard);
	unsigned long long tmp;
	unsigned long long legalBoard;

	tmp = horWatchBoard & (playerBoard << 1);
	for (int i = 0; i < 5; i++) tmp |= horWatchBoard & (tmp << 1);
	legalBoard = blankBoard & (tmp << 1);

	tmp = horWatchBoard & (playerBoard >> 1);
	for (int i = 0; i < 5; i++) tmp |= horWatchBoard & (tmp >> 1);
	legalBoard |= blankBoard & (tmp >> 1);

	tmp = verWatchBoard & (playerBoard << 8);
	for (int i = 0; i < 5; i++) tmp |= verWatchBoard & (tmp << 8);
	legalBoard |= blankBoard & (tmp << 8);

	tmp = verWatchBoard & (playerBoard >> 8);
	for (int i = 0; i < 5; i++) tmp |= verWatchBoard & (tmp >> 8);
	legalBoard |= blankBoard & (tmp >> 8);

	tmp = allSideWatchBoard & (playerBoard << 7);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp << 7);
	legalBoard |= blankBoard & (tmp << 7);

	tmp = allSideWatchBoard & (playerBoard << 9);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp << 9);
	legalBoard |= blankBoard & (tmp << 9);

	tmp = allSideWatchBoard & (playerBoard >> 9);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp >> 9);
	legalBoard |= blankBoard & (tmp >> 9);

	tmp = allSideWatchBoard & (playerBoard >> 7);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp >> 7);
	legalBoard |= blankBoard & (tmp >> 7);

	return legalBoard;
}