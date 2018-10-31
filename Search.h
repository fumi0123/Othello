//#include "Mydef.h"
#include "DxLib.h"

//#include "Board.h"

const int scoreTable[8][8] = {
{ 45,-11,  4, -1, -1,  4,-11, 45, },
{-11,-16, -1, -3, -3, -1,-16,-11, },
{  4, -1,  2, -1, -1,  2, -1,  4, },
{ -1, -3, -1,  0,  0, -1, -3, -1, },
{ -1, -3, -1,  0,  0, -1, -3, -1, },
{  4, -1,  2, -1, -1,  2, -1,  4, },
{-11,-16, -1, -3, -3, -1,-16,-11, },
{ 45,-11,  4, -1, -1,  4,-11, 45, } };

using namespace std;

unsigned long long inputPut();
unsigned long long randomPut();

class Search {
	int maxDepth;
	int player;

	unsigned long long bestPut;


public:	
	int method;
	Search(int player, int maxDepth, int method) {
		this->player = player;
		this->maxDepth = maxDepth;
		this->method = method;
	}

	unsigned long long think(Board board) {
		unsigned long long put;
		while (1) {
			if (method == MANUAL) {
				put = inputPut();
				if (board.canPut(put)) break;
//				else cout << "Input Error" << endl;
			}
			else if (method == RANDOM) {
				put = randomPut();
				if (board.canPut(put)) break;
			}
			else if (method == MINIMAX) {
				int eval;
				eval = miniMax(board, 1, maxDepth);
				put = bestPut;
				if (!board.canPut(put)) {
					put = randomPut();
					if (board.canPut(put)) break;
				}else break;
			}
		}
		return put;
	}

	int eval(Board board) {
		int playerBoardScore = 0;
		int opponentBoardScore = 0;
		int CanPutCount = 0;
		int playerStoneCount = 0;
		int opponentStoneCount = 0;
		int eval = 0;
		if (board.nowIndex <= FINISH_TURN) {
			for (int y = 0; y < 8; y++) {
				for (int x = 0; x < 8; x++) {
					int num = 63 - (x + y * 8);
					if (((board.playerBoard >> num) & 1) == 1){
						playerBoardScore += scoreTable[x][y];
//						cout << scoreTable[x][y] << endl;
					}
					else if (((board.opponentBoard >> num) & 1) == 1) {
						opponentBoardScore += scoreTable[x][y];
//						cout << scoreTable[x][y] << endl;
					}
				}
			}
			CanPutCount = bitCount(makeLegalBoard(board.playerBoard, board.opponentBoard));
			if (board.nowTurn == player) eval = playerBoardScore - opponentBoardScore + CanPutCount;
			else eval = opponentBoardScore - playerBoardScore - CanPutCount;
//			board.showBoard();
//			cout << "eval" << eval << "player" << playerBoardScore << "opponent" << opponentBoardScore << "can" << CanPutCount << "Turn" << board.nowTurn << endl;
			return eval;
		}else {
			playerStoneCount = bitCount(board.playerBoard);
			opponentStoneCount = bitCount(board.opponentBoard);
			if (board.nowTurn == player) return playerStoneCount - opponentStoneCount;
			else return opponentStoneCount - playerStoneCount;
		}
		return 0;
	}

	int miniMax(Board board, int turn, int depth) {
		int i = 0;
		if (turn == -1) board.swapBoard();
		Board tmpBoard;
		tmpBoard.nowIndex = board.nowIndex;
		tmpBoard.nowTurn = board.nowTurn;
		tmpBoard.opponentBoard = board.opponentBoard;
		tmpBoard.playerBoard = board.playerBoard;

		if (depth == 0 || board.isGameFinished())return eval(board);

		unsigned long long expandBoard[30];
		int nodeNum = bitCount(makeLegalBoard(board.playerBoard, board.opponentBoard));

		expandNode(board,expandBoard);
		int bestEval = -100000;
		int val;
		for (i = 0; i < nodeNum; i++) {
			tmpBoard.reverse(expandBoard[i]);
//			tmpBoard.reverse(coordToBit(5,5));
//			cout << hex << expandBoard[0] << endl;
//			cout << hex << coordToBit(5, 5) << endl;
//			tmpBoard.showBoard();
			val = miniMax(tmpBoard, -1, depth - 1);
//			cout << "-----------" << endl;
			tmpBoard.opponentBoard = board.opponentBoard;
			tmpBoard.playerBoard = board.playerBoard;
			if (board.nowTurn == player && bestEval < val) {
				bestEval = val;
				if (depth == maxDepth)bestPut = expandBoard[i];
			}
			if (board.nowTurn != player && bestEval < -val) {
				bestEval = -val;
				if (depth == maxDepth)bestPut = expandBoard[i];
			}
		}
		return bestEval;
	}

	void expandNode(Board board, unsigned long long *expandBoard) {
		unsigned long long legalBoard = makeLegalBoard(board.playerBoard, board.opponentBoard);
		int nodeNum = bitCount(legalBoard);
		for (int i = 0; i < nodeNum; i++) {
			for (int j = 0; j < 64; j++) {
				if (((legalBoard >> j) & 1) == 1) {
					expandBoard[i] = ((legalBoard >> j) & 1) << j;
					legalBoard = legalBoard & (~expandBoard[i]);
					break;
				}
			}
//			cout << hex << expandBoard[i] << endl;
		}
	}
};
/*
unsigned long long inputPut() {
	string s;
	cin >> s;
	int x = s[0] - 'A';
	int y = s[1] - '0' - 1;
	unsigned long long put = coordToBit(x, y);
	return put;
}*/

unsigned long long inputPut() {
	unsigned long long put;
	static bool mouse_flag = false;
	while (1) {
		if (GetMouseInput() & MOUSE_INPUT_LEFT) {
			if (!mouse_flag) {
				mouse_flag = true;
				int mx, my;
				GetMousePoint(&mx, &my);
				put = coordToBit(mx / 60, my / 60);
				return put;
			}
		}
		else mouse_flag = false;
	}
	return 0;
}

unsigned long long randomPut() {
	random_device rnd;
	int x = rnd() % 8;
	int y = rnd() % 8;
	unsigned long long put = coordToBit(x, y);
	return put;
}