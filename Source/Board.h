/**
* @file Board.h
* @brief ボードクラスと関連した関数のヘッダ
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
* @brief ボードを表現するクラス
* @details ビットボードを用いた盤面を表現するクラス．
*/
class Board {
public:
	// 両プレイヤーのビットボード
	// 64ビット変数
	unsigned long long playerBoard = 0;
	unsigned long long opponentBoard = 0;

	int nowTurn;	// 現在の手番　0が黒番，1が白番
	int nowIndex;	// 現在のターン数

	/**
	* @brief ボードクラスのコンストラクタ
	*/
	Board() {
		this->nowTurn = PLAYER1;
		this->nowIndex = 1;

		// ビッドボードに初期配置を格納
		// 初期はPlayerに黒盤面，opponentに白盤面
		playerBoard = 0x0000000810000000;
		opponentBoard = 0x0000001008000000;
	}

	/**
	* @brief ボードを初期状態に戻す関数
	*/
	void initBoard() {
		this->nowTurn = PLAYER1;
		this->nowIndex = 1;

		playerBoard = 0x0000000810000000;
		opponentBoard = 0x0000001008000000;
	}

	/**
	* @brief 指すことが可能かの判定
	* @param put 指し手
	* @return bool 指すことが可能ならTrue，不可能ならfalse
	*/
	bool canPut(unsigned long long put) {
		// ボードを引数とし，合法手ボードを取得
		unsigned long long legalBoard = makeLegalBoard(this->playerBoard, this->opponentBoard);
		// 指すことが可能か判定
		if ((put & legalBoard) == put && put != 0) return true;
		return false;
	}

	/**
	* @brief 指し手を反映
	* @param put 指し手
	* @details 指し手を盤面に反映させる．
	*
	* 挟んだ石をひっくり返す．
	*/
	void reverse(unsigned long long put) {
		unsigned long long rev = 0;	// 反転する位置
		for (int i = 0; i < 8; i++) {
			unsigned long long rev_ = 0;	// 1方向における反転する位置
			unsigned long long mask = transfer(put, i);	// 指し手をずらした位置

			// 指し手を1方向にずらした結果場外にいくか，ずらしたところに相手の石がなければ終了
			while ((mask != 0) && ((mask & opponentBoard) != 0)) {
				rev_ |= mask;	// ずらした位置に合った相手の石を加える
				mask = transfer(mask, i);	// 更にずらす
			}
			// 1方向で反転した位置を，全体での反転する位置に加える
			if ((mask & playerBoard) != 0) rev |= rev_;
		}
		// 反転する
		playerBoard ^= put | rev;
		opponentBoard ^= rev;

		nowIndex = this->nowIndex + 1;	// ターン数を進める
	}

	/**
	 * @brief パスの判定．手番プレイヤーのみが置けない場合．
	 * @return パスならtrue
	 */
	bool isPass() {
		// 両プレイヤーの合法手ボードを取得
		unsigned long long playerLegalBoard = makeLegalBoard(playerBoard, opponentBoard);
		unsigned long long opponentLegalBoard = makeLegalBoard(opponentBoard, playerBoard);

		// 手番プレイヤーのみが合法手ボードに何もない場合trueを返す
		if ((bitCount(playerLegalBoard) == 0) && (bitCount(opponentLegalBoard) != 0))
			return true;
		return false;
	}

	/**
	 * @brief ゲーム終了判定．両プレイヤー共置けない場合．
	 * @return 終了ならtrue
	 */
	bool isGameFinished() {
		// 両プレイヤーの合法手ボードを取得
		unsigned long long playerLegalBoard = makeLegalBoard(playerBoard, opponentBoard);
		unsigned long long opponentLegalBoard = makeLegalBoard(opponentBoard, playerBoard);

		// 両プレイヤー共，合法手ボードに何もない場合trueを返す
		if ((bitCount(playerLegalBoard) == 0) && (bitCount(opponentLegalBoard) == 0))
			return true;
		return false;
	}

	/**
	 * @brief 手番を入れ替える
	 */
	void swapBoard() {
		// 両ボードを入れ替える
		unsigned long long tmp = playerBoard;
		playerBoard = opponentBoard;
		opponentBoard = tmp;

		nowTurn = 1 - nowTurn;	// 手番プレイヤーの入れ替え
	}

	/**
	 * @brief 結果の取得
	 * @param score スコア
	 */
	void getResult(int *score) {

		int playerScore, opponentScore;	// 両プレイヤーのスコア

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
 * @brief 座標をビットに変換
 * @param x x座標（横）
 * @param y y座標（縦）
 * @return 指し手
 */
unsigned long long coordToBit(int x, int y) {
	unsigned long long mask = 0x8000000000000000; // ボードの左上（x=0,y=0）
	
	// 横方向にシフトする
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
	
	mask = mask >> (y * 8); // 縦方向にシフトする

	return mask;
}

/**
 * @brief 立っているビットの数を数える
 * @param num 数えたいビット
 * @return 立っているビット
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
 * @brief 反転位置を求める
 * @param put 指し手
 * @param i 方向
 * @return 反転位置
 */
unsigned long long transfer(unsigned long long put, int i) {
	switch (i) {
	case 0: // 上方向
		return (put << 8) & 0xffffffffffffff00;
	case 1: // 右上方向　putが右端にあった場合andで消える
		return (put << 7) & 0x7f7f7f7f7f7f7f00;
	case 2: // 右方向　putが右端にあった場合andで消える
		return (put >> 1) & 0x7f7f7f7f7f7f7f7f;
	case 3: // 右下方向　putが右端にあった場合andで消える
		return (put >> 9) & 0x007f7f7f7f7f7f7f;
	case 4: // 下方向
		return (put >> 8) & 0x00ffffffffffffff;
	case 5: // 左下方向　putが左端にあった場合andで消える
		return (put >> 7) & 0x00fefefefefefefe;
	case 6: // 左方向　putが左端にあった場合andで消える
		return (put << 1) & 0xfefefefefefefefe;
	case 7: // 左上方向　putが左端にあった場合andで消える
		return (put << 9) & 0xfefefefefefefe00;
	default:
		return 0;
	}
}

/**
 * @brief 合法手ボードの作成
 * @param playerBoard 手番プレイヤーのボード
 * @param opponentBoard 対戦相手プレイヤーのボード
 * @return 合法手ボード
 */
unsigned long long makeLegalBoard(unsigned long long playerBoard, unsigned long long opponentBoard) {
	// 左右端，上下端，全辺と対戦相手ボードの積を取る
	unsigned long long horWatchBoard = opponentBoard & 0x7e7e7e7e7e7e7e7e;
	unsigned long long verWatchBoard = opponentBoard & 0x00FFFFFFFFFFFF00;
	unsigned long long allSideWatchBoard = opponentBoard & 0x007e7e7e7e7e7e00;

	unsigned long long blankBoard = ~(playerBoard | opponentBoard); // 空いているマス
	unsigned long long tmp;			// 隣に相手の石があるか保存する
	unsigned long long legalBoard;	// 合法手ボード


	// 自分の石を一方向にずらし，相手の石があれば保存
	// 保存したものを更に5回ずらして，連続して隣接した石があれば保存していく
	// 自分の石から連続して置かれた相手石の更に1つ隣が空いていれば置ける場所とする
	// なお，左右ずらしだと左右端，上下ずらしだと上下端，斜めずらしだと全辺と，
	// 先ほど作った端ボードが邪魔をすることで場外置きを防止している

	// 左
	tmp = horWatchBoard & (playerBoard << 1);
	for (int i = 0; i < 5; i++) tmp |= horWatchBoard & (tmp << 1); 
	legalBoard = blankBoard & (tmp << 1); 
	// 右
	tmp = horWatchBoard & (playerBoard >> 1);
	for (int i = 0; i < 5; i++) tmp |= horWatchBoard & (tmp >> 1);
	legalBoard |= blankBoard & (tmp >> 1);
	// 上
	tmp = verWatchBoard & (playerBoard << 8);
	for (int i = 0; i < 5; i++) tmp |= verWatchBoard & (tmp << 8);
	legalBoard |= blankBoard & (tmp << 8);
	// 下
	tmp = verWatchBoard & (playerBoard >> 8);
	for (int i = 0; i < 5; i++) tmp |= verWatchBoard & (tmp >> 8);
	legalBoard |= blankBoard & (tmp >> 8);
	// 右上
	tmp = allSideWatchBoard & (playerBoard << 7);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp << 7);
	legalBoard |= blankBoard & (tmp << 7);
	// 左上
	tmp = allSideWatchBoard & (playerBoard << 9);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp << 9);
	legalBoard |= blankBoard & (tmp << 9);
	// 左下
	tmp = allSideWatchBoard & (playerBoard >> 9);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp >> 9);
	legalBoard |= blankBoard & (tmp >> 9);
	// 右下
	tmp = allSideWatchBoard & (playerBoard >> 7);
	for (int i = 0; i < 5; i++) tmp |= allSideWatchBoard & (tmp >> 7);
	legalBoard |= blankBoard & (tmp >> 7);

	return legalBoard;
}