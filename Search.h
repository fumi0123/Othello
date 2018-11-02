/**
* @file Search.h
* @brief �T���N���X�Ɗ֘A�����֐��̃w�b�_
*/

#include "DxLib.h"
using namespace std;

// �e�}�X�̕]���l�e�[�u��
// ���l���傫���}�X�قǎ���ƗL��
const int scoreTable[8][8] = {
{ 45,-11,  4, -1, -1,  4,-11, 45, },
{-11,-16, -1, -3, -3, -1,-16,-11, },
{  4, -1,  2, -1, -1,  2, -1,  4, },
{ -1, -3, -1,  0,  0, -1, -3, -1, },
{ -1, -3, -1,  0,  0, -1, -3, -1, },
{  4, -1,  2, -1, -1,  2, -1,  4, },
{-11,-16, -1, -3, -3, -1,-16,-11, },
{ 45,-11,  4, -1, -1,  4,-11, 45, } };

unsigned long long inputPut();
unsigned long long randomPut();

/**
* @brief �T�����s���N���X
* @details �g�p����헪�ɍ��킹�ĒT�����s���w��������߂�D�v���C���[���ɐ錾����D
*/
class Search {
	int maxDepth;		// �T���[��
	int player;			// �v���C���[�ԍ�
	int timeCount = 0;	// ����
	unsigned long long bestPut;	// �őP��

public:	
	int method;	// �v���C���[�̐헪

	/**
	* @brief �T���N���X�̃R���X�g���N�^
	* @param player �v���C���[�ԍ��D���Ȃ�0�C���Ȃ�1�D
	* @param maxDepth �T���[��
	* @param method �g�p�헪
	*/
	Search(int player, int maxDepth, int method) {
		this->player = player;
		this->maxDepth = maxDepth;
		this->method = method;
	}
	/**
	* @brief �w��������߂�֐�
	* @param board Board�C���X�^���X
	* @return unsigned long long �w����
	* @details �w��������߂�֐��D�g�p�헪�ɂ���Č��ߕ����ς��D
	* 
	* �헪������ȊO�̏ꍇ�C��莞�Ԃ��o�߂���܂Ŏw�����Ԃ��Ȃ��D
	*/
	unsigned long long think(Board board) {
		unsigned long long put;
		while (1) {
			if (method == MANUAL) {	// �헪���}�j���A���i����j�̏ꍇ
				put = inputPut();	// �w������}�E�X����擾
				if (board.canPut(put)) break;	// �w�����Ƃ��\�Ȃ烋�[�v�E�o

			}else if (method == RANDOM) {	// �헪�������_���̏ꍇ
				timeCount = GetNowCount();	// ���Ԍv���J�n
				put = randomPut();			// �w����������_���Ɏ擾
				if (board.canPut(put)) break;

			}else if (method == ALPHABETA) { // �헪�������@�̏ꍇ
				timeCount = GetNowCount();
				int eval;	// �]���l
				if(board.nowTurn <= FINISH_TURN)
					eval = alphaBeta(board, maxDepth, -99999, 99999);	// �����@�ōō��]���l���擾
				else eval = alphaBeta(board, MAX_TURN - FINISH_TURN, -99999, 99999);	// �����@�ōō��]���l���擾
				put = bestPut;	// �w������őP�肩��擾
				if (!board.canPut(put)) {
					put = randomPut();
					if (board.canPut(put)) break;
				}else break;
			}
		}
		while (GetNowCount() - timeCount <= TIMELIMIT) {}	// ��莞�Ԃ��o�߂���܂őҋ@
		return put;
	}

	/**
	* @brief �Ֆʂ̕]���l���擾����֐�
	* @param board Board�C���X�^���X
	* @return int �]���l
	* @details ���ՁC���Ղ͎����̃}�X�̕]���l - ����̃}�X�̕]���l�}����\�Ȏ�̐��D
	* 
	* �I�Ղ͎����̐΂̐� - ����̐΂̐�
	*/
	int eval(Board board) {
		int eval = 0;
		// �I�Ղɓ���܂�
		if (board.nowIndex <= FINISH_TURN) {
			int playerBoardScore = 0;	// �}�X�̕]���l
			int opponentBoardScore = 0;
			int CanPutCount;			// ����\�Ȏ�̐�

			// �X�R�A�e�[�u������}�X�̕]���l���擾
			// ���v���C���[�̕]���l�ɉ��Z����
			for (int y = 0; y < 8; y++) {
				for (int x = 0; x < 8; x++) {
					int num = 63 - (x + y * 8);
					if (((board.playerBoard >> num) & 1) == 1){
						playerBoardScore += scoreTable[x][y];
					}else if (((board.opponentBoard >> num) & 1) == 1) {
						opponentBoardScore += scoreTable[x][y];
					}
				}
			}
			// ����\�Ȏ�̐����擾
			CanPutCount = bitCount(makeLegalBoard(board.playerBoard, board.opponentBoard));

			// �]���l�v�Z���Ԃ�
			if (board.nowTurn == player) eval = playerBoardScore - opponentBoardScore + CanPutCount;
			else eval = opponentBoardScore - playerBoardScore - CanPutCount;
			return eval;
		}else{
			// �I��
			int playerStoneCount;	// �v���C���[�̐΂̐�
			int opponentStoneCount;

			// ���v���C���[�̐΂̐����擾
			// �]���l���v�Z���Ԃ�
			playerStoneCount = bitCount(board.playerBoard);
			opponentStoneCount = bitCount(board.opponentBoard);
			if (board.nowTurn == player) return playerStoneCount - opponentStoneCount;
			else return opponentStoneCount - playerStoneCount;
		}
		return 0;
	}
	/**
	* @brief �����@
	* @param board Board�C���X�^���X
	* @param depth �c��̒T���[��
	* @param alpha ���l
	* @param beta ���l
	* @return int �őP��̕]���l
	* @details ����\�Ȏ肩��Ֆʂ����D
	* 
	* �ċA��p���C�ŉ��܂ŒT��������ɕ]���l�������グ�Ă����D
	* 
	* ���l�ƃ��l�����Ɏ}�������邱�ƂŌv�Z�񐔂����炵�Ă���
	*/
	int alphaBeta(Board board, int depth, int alpha, int beta) {
		// �{�[�h�̃R�s�[�{�[�h�̍쐬
		Board expBoard;
		expBoard.playerBoard = board.playerBoard;
		expBoard.opponentBoard = board.opponentBoard;
		expBoard.nowTurn = board.nowTurn;
		// ��Ɏ��ł�����C1��i��ł��܂����C�����^�[����ێ����Ă����������߁|1����
		expBoard.nowIndex = board.nowIndex - 1;

		// �ŉ��ɂ��ǂ蒅�����C�w���肪�Ȃ��Ȃ�Ƃ��̎��_�ł̕]���l���v�Z���Ԃ�
		if (depth == 0 || board.isPass() || board.isGameFinished())return eval(board);
		
		// �w�����Ƃ��\�Ȏ��S�Ď擾
		unsigned long long expandPut[30];
		expandNode(board, expandPut);

		// �w�����Ƃ��\�Ȏ�̐����擾
		int nodeNum = bitCount(makeLegalBoard(board.playerBoard, board.opponentBoard));

		int val;
		// �S�ĒT�����邩�C
		// �}�����ꂵ���m�[�h���D�悳���ׂ����̂�������Ȃ����Ƃ����܂����Ƃ��T����ł��؂�
		for (int i = 0; (alpha < beta) && (i < nodeNum); i++) {
			expBoard.reverse(expandPut[i]);	// ����\�Ȏ�𔽉f
			expBoard.swapBoard();			// ��Ԃ����ւ���
			val = alphaBeta(expBoard, depth - 1, alpha, beta);	// �ċA�Ăяo���D�c��[����1������

			// ���̑w��T�����I����ƌ��ɖ߂�
			expBoard.playerBoard = board.playerBoard;
			expBoard.opponentBoard = board.opponentBoard;
			expBoard.nowTurn = board.nowTurn;
			expBoard.nowIndex = board.nowIndex - 1;

			// �]���l���X�V
			if (board.nowTurn == player && alpha < val) {
				alpha = val;
				if (depth == maxDepth)bestPut = expandPut[i];
			}else if (board.nowTurn != player && beta > val) {
				beta = val;
				if (depth == maxDepth)bestPut = expandPut[i];
			}
		}
		if (board.nowTurn == player) return alpha;
		else return beta;
	}

	/**
	* @brief �m�[�h���L���邽�߂̊֐�
	* @param board Board�C���X�^���X
	* @param expandBoard �w�����Ƃ��\�Ȏ��ۑ�����z��
	*/
	void expandNode(Board board, unsigned long long *expandBoard) {
		// �{�[�h�������Ƃ��C���@��{�[�h���擾
		unsigned long long legalBoard = makeLegalBoard(board.playerBoard, board.opponentBoard);
		int nodeNum = bitCount(legalBoard);
		
		// ���@��{�[�h�𕪉����CexpandBoard�ɕۑ�
		for (int i = 0; i < nodeNum; i++) {
			for (int j = 0; j < 64; j++) {
				// 1�}�X�����炵�Ă����C1��������Εۑ�
				// ���@��{�[�h���炻�̃}�X�����O
				if (((legalBoard >> j) & 1) == 1) {
					expandBoard[i] = ((legalBoard >> j) & 1) << j;
					legalBoard = legalBoard & (~expandBoard[i]);
					break;
				}
			}
		}
	}
};

/**
* @brief �w������}�E�X����擾����֐�
* @return unsigned long long �w����
*/
unsigned long long inputPut() {
	static bool mouse_flag = false;
	while (1) {
		// �}�E�X�̍��N���b�N�����m
		if (GetMouseInput() & MOUSE_INPUT_LEFT) {
			if (!mouse_flag) {
				mouse_flag = true;
				int mx, my;
				GetMousePoint(&mx, &my);

				// ���W���r�b�g�ɕϊ�
				// �����1�}�X�̃s�N�Z����
				// ��ʍ���̒[���Ֆʂ̒[�Ƃ��ĂȂ��ꍇ�͓K�X��������
				unsigned long long put = coordToBit(mx / 60, my / 60);

				return put;
			}
		}
		else mouse_flag = false;
	}
	return 0;
}

/**
* @brief �w����������_���Ɏ擾����֐�
* @return unsigned long long �w����
*/
unsigned long long randomPut() {
	random_device rnd;
	// x,y���W��0�`7
	int x = rnd() % 8;
	int y = rnd() % 8;

	// ���W���r�b�g�ɕϊ�
	unsigned long long put = coordToBit(x, y);
	return put;
}