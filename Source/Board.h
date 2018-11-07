/**
* @file Board.h
* @brief �{�[�h�N���X�Ɗ֘A�����֐��̃w�b�_
*/

#include <iostream>
#include <string>
#include <random>
#include <iomanip>

using namespace std;

unsigned long long coordToBit(int x, int y);
unsigned long long transfer(unsigned long long put, int i);
int bitCount(unsigned long long num);
unsigned long long makeLegalBoard(unsigned long long playerBoard, unsigned long long opponentBoard);

/**
* @brief �{�[�h��\������N���X
* @details �r�b�g�{�[�h��p�����Ֆʂ�\������N���X�D
*/
class Board {
public:
	// ���v���C���[�̃r�b�g�{�[�h
	// 64�r�b�g�ϐ�
	unsigned long long playerBoard = 0;
	unsigned long long opponentBoard = 0;

	int nowTurn;	// ���݂̎�ԁ@0�����ԁC1������
	int nowIndex;	// ���݂̃^�[����

	/**
	* @brief �{�[�h�N���X�̃R���X�g���N�^
	*/
	Board() {
		this->nowTurn = PLAYER1;
		this->nowIndex = 1;

		// �r�b�h�{�[�h�ɏ����z�u���i�[
		// ������Player�ɍ��ՖʁCopponent�ɔ��Ֆ�
		playerBoard = 0x0000000810000000;
		opponentBoard = 0x0000001008000000;
	}

	/**
	* @brief �{�[�h��������Ԃɖ߂��֐�
	*/
	void initBoard() {
		this->nowTurn = PLAYER1;
		this->nowIndex = 1;

		playerBoard = 0x0000000810000000;
		opponentBoard = 0x0000001008000000;
	}

	/**
	* @brief �w�����Ƃ��\���̔���
	* @param put �w����
	* @return bool �w�����Ƃ��\�Ȃ�True�C�s�\�Ȃ�false
	*/
	bool canPut(unsigned long long put) {
		// �{�[�h�������Ƃ��C���@��{�[�h���擾
		unsigned long long legalBoard = makeLegalBoard(this->playerBoard, this->opponentBoard);
		// �w�����Ƃ��\������
		if ((put & legalBoard) == put && put != 0) return true;
		return false;
	}

	/**
	* @brief �w����𔽉f
	* @param put �w����
	* @details �w�����Ֆʂɔ��f������D
	*
	* ���񂾐΂��Ђ�����Ԃ��D
	*/
	void reverse(unsigned long long put) {
		unsigned long long rev = 0;	// ���]����ʒu
		for (int i = 0; i < 8; i++) {
			unsigned long long rev_ = 0;	// 1�����ɂ����锽�]����ʒu
			unsigned long long mask = transfer(put, i);	// �w��������炵���ʒu

			// �w�����1�����ɂ��炵�����ʏ�O�ɂ������C���炵���Ƃ���ɑ���̐΂��Ȃ���ΏI��
			while ((mask != 0) && ((mask & opponentBoard) != 0)) {
				rev_ |= mask;	// ���炵���ʒu�ɍ���������̐΂�������
				mask = transfer(mask, i);	// �X�ɂ��炷
			}
			// 1�����Ŕ��]�����ʒu���C�S�̂ł̔��]����ʒu�ɉ�����
			if ((mask & playerBoard) != 0) rev |= rev_;
		}
		// ���]����
		playerBoard ^= put | rev;
		opponentBoard ^= rev;

		nowIndex = this->nowIndex + 1;	// �^�[������i�߂�
	}

	/**
	 * @brief �p�X�̔���D��ԃv���C���[�݂̂��u���Ȃ��ꍇ�D
	 * @return �p�X�Ȃ�true
	 */
	bool isPass() {
		// ���v���C���[�̍��@��{�[�h���擾
		unsigned long long playerLegalBoard = makeLegalBoard(playerBoard, opponentBoard);
		unsigned long long opponentLegalBoard = makeLegalBoard(opponentBoard, playerBoard);

		// ��ԃv���C���[�݂̂����@��{�[�h�ɉ����Ȃ��ꍇtrue��Ԃ�
		if ((bitCount(playerLegalBoard) == 0) && (bitCount(opponentLegalBoard) != 0))
			return true;
		return false;
	}

	/**
	 * @brief �Q�[���I������D���v���C���[���u���Ȃ��ꍇ�D
	 * @return �I���Ȃ�true
	 */
	bool isGameFinished() {
		// ���v���C���[�̍��@��{�[�h���擾
		unsigned long long playerLegalBoard = makeLegalBoard(playerBoard, opponentBoard);
		unsigned long long opponentLegalBoard = makeLegalBoard(opponentBoard, playerBoard);

		// ���v���C���[���C���@��{�[�h�ɉ����Ȃ��ꍇtrue��Ԃ�
		if ((bitCount(playerLegalBoard) == 0) && (bitCount(opponentLegalBoard) == 0))
			return true;
		return false;
	}

	/**
	 * @brief ��Ԃ����ւ���
	 */
	void swapBoard() {
		// ���{�[�h�����ւ���
		unsigned long long tmp = playerBoard;
		playerBoard = opponentBoard;
		opponentBoard = tmp;

		nowTurn = 1 - nowTurn;	// ��ԃv���C���[�̓���ւ�
	}

	/**
	 * @brief ���ʂ̎擾
	 * @param score �X�R�A
	 */
	void getResult(int *score) {

		int playerScore, opponentScore;	// ���v���C���[�̃X�R�A

		if (this->nowTurn == PLAYER1) {
			playerScore = bitCount(playerBoard);
			opponentScore = bitCount(opponentBoard);
		}else{
			playerScore = bitCount(opponentBoard);
			opponentScore = bitCount(playerBoard);
		}
		score[0] = playerScore;
		score[1] = opponentScore;
	}
};

/**
 * @brief ���W���r�b�g�ɕϊ�
 * @param x x���W�i���j
 * @param y y���W�i�c�j
 * @return �w����
 */
unsigned long long coordToBit(int x, int y) {
	unsigned long long mask = 0x8000000000000000; // �{�[�h�̍���ix=0,y=0�j
	
	// �������ɃV�t�g����
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
	
	mask = mask >> (y * 8); // �c�����ɃV�t�g����

	return mask;
}

/**
 * @brief �����Ă���r�b�g�̐��𐔂���
 * @param num ���������r�b�g
 * @return �����Ă���r�b�g
 */
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

/**
 * @brief ���]�ʒu�����߂�
 * @param put �w����
 * @param i ����
 * @return ���]�ʒu
 */
unsigned long long transfer(unsigned long long put, int i) {
	switch (i) {
	case 0: // �����
		return (put << 8) & 0xffffffffffffff00;
	case 1: // �E������@put���E�[�ɂ������ꍇand�ŏ�����
		return (put << 7) & 0x7f7f7f7f7f7f7f00;
	case 2: // �E�����@put���E�[�ɂ������ꍇand�ŏ�����
		return (put >> 1) & 0x7f7f7f7f7f7f7f7f;
	case 3: // �E�������@put���E�[�ɂ������ꍇand�ŏ�����
		return (put >> 9) & 0x007f7f7f7f7f7f7f;
	case 4: // ������
		return (put >> 8) & 0x00ffffffffffffff;
	case 5: // ���������@put�����[�ɂ������ꍇand�ŏ�����
		return (put >> 7) & 0x00fefefefefefefe;
	case 6: // �������@put�����[�ɂ������ꍇand�ŏ�����
		return (put << 1) & 0xfefefefefefefefe;
	case 7: // ��������@put�����[�ɂ������ꍇand�ŏ�����
		return (put << 9) & 0xfefefefefefefe00;
	default:
		return 0;
	}
}

/**
 * @brief ���@��{�[�h�̍쐬
 * @param playerBoard ��ԃv���C���[�̃{�[�h
 * @param opponentBoard �ΐ푊��v���C���[�̃{�[�h
 * @return ���@��{�[�h
 */
unsigned long long makeLegalBoard(unsigned long long playerBoard, unsigned long long opponentBoard) {
	// ���E�[�C�㉺�[�C�S�ӂƑΐ푊��{�[�h�̐ς����
	unsigned long long horWatchBoard = opponentBoard & 0x7e7e7e7e7e7e7e7e;
	unsigned long long verWatchBoard = opponentBoard & 0x00FFFFFFFFFFFF00;
	unsigned long long allSideWatchBoard = opponentBoard & 0x007e7e7e7e7e7e00;

	unsigned long long blankBoard = ~(playerBoard | opponentBoard); // �󂢂Ă���}�X
	unsigned long long tmp;			// �ׂɑ���̐΂����邩�ۑ�����
	unsigned long long legalBoard;	// ���@��{�[�h


	// �����̐΂�������ɂ��炵�C����̐΂�����Εۑ�
	// �ۑ��������̂��X��5�񂸂炵�āC�A�����ėאڂ����΂�����Εۑ����Ă���
	// �����̐΂���A�����Ēu���ꂽ����΂̍X��1�ׂ��󂢂Ă���Βu����ꏊ�Ƃ���
	// �Ȃ��C���E���炵���ƍ��E�[�C�㉺���炵���Ə㉺�[�C�΂߂��炵���ƑS�ӂƁC
	// ��قǍ�����[�{�[�h���ז������邱�Ƃŏ�O�u����h�~���Ă���

	// ��
	tmp = horWatchBoard & (playerBoard << 1);
	for (int i = 0; i < 5; i++) tmp |= horWatchBoard & (tmp << 1); 
	legalBoard = blankBoard & (tmp << 1); 
	// �E
	tmp = horWatchBoard & (playerBoard >> 1);
	for (int i = 0; i < 5; i++) tmp |= horWatchBoard & (tmp >> 1);
	legalBoard |= blankBoard & (tmp >> 1);
	// ��
	tmp = verWatchBoard & (playerBoard << 8);
	for (int i = 0; i < 5; i++) tmp |= verWatchBoard & (tmp << 8);
	legalBoard |= blankBoard & (tmp << 8);
	// ��
	tmp = verWatchBoard & (playerBoard >> 8);
	for (int i = 0; i < 5; i++) tmp |= verWatchBoard & (tmp >> 8);
	legalBoard |= blankBoard & (tmp >> 8);
	// �E��
	tmp = allSideWatchBoard & (playerBoard << 7);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp << 7);
	legalBoard |= blankBoard & (tmp << 7);
	// ����
	tmp = allSideWatchBoard & (playerBoard << 9);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp << 9);
	legalBoard |= blankBoard & (tmp << 9);
	// ����
	tmp = allSideWatchBoard & (playerBoard >> 9);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp >> 9);
	legalBoard |= blankBoard & (tmp >> 9);
	// �E��
	tmp = allSideWatchBoard & (playerBoard >> 7);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp >> 7);
	legalBoard |= blankBoard & (tmp >> 7);

	return legalBoard;
}