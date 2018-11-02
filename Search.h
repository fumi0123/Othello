/**
* @file Search.h
* @brief 探索クラスと関連した関数のヘッダ
*/

#include "DxLib.h"
using namespace std;

// 各マスの評価値テーブル
// 数値が大きいマスほど取れると有利
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
* @brief 探索を行うクラス
* @details 使用する戦略に合わせて探索を行い指し手を決める．プレイヤー毎に宣言する．
*/
class Search {
	int maxDepth;		// 探索深さ
	int player;			// プレイヤー番号
	int timeCount = 0;	// 時間
	unsigned long long bestPut;	// 最善手

public:	
	int method;	// プレイヤーの戦略

	/**
	* @brief 探索クラスのコンストラクタ
	* @param player プレイヤー番号．黒なら0，白なら1．
	* @param maxDepth 探索深さ
	* @param method 使用戦略
	*/
	Search(int player, int maxDepth, int method) {
		this->player = player;
		this->maxDepth = maxDepth;
		this->method = method;
	}
	/**
	* @brief 指し手を決める関数
	* @param board Boardインスタンス
	* @return unsigned long long 指し手
	* @details 指し手を決める関数．使用戦略によって決め方が変わる．
	* 
	* 戦略が操作以外の場合，一定時間が経過するまで指し手を返さない．
	*/
	unsigned long long think(Board board) {
		unsigned long long put;
		while (1) {
			if (method == MANUAL) {	// 戦略がマニュアル（操作）の場合
				put = inputPut();	// 指し手をマウスから取得
				if (board.canPut(put)) break;	// 指すことが可能ならループ脱出

			}else if (method == RANDOM) {	// 戦略がランダムの場合
				timeCount = GetNowCount();	// 時間計測開始
				put = randomPut();			// 指し手をランダムに取得
				if (board.canPut(put)) break;

			}else if (method == ALPHABETA) { // 戦略がαβ法の場合
				timeCount = GetNowCount();
				int eval;	// 評価値
				if(board.nowTurn <= FINISH_TURN)
					eval = alphaBeta(board, maxDepth, -99999, 99999);	// αβ法で最高評価値を取得
				else eval = alphaBeta(board, MAX_TURN - FINISH_TURN, -99999, 99999);	// αβ法で最高評価値を取得
				put = bestPut;	// 指し手を最善手から取得
				if (!board.canPut(put)) {
					put = randomPut();
					if (board.canPut(put)) break;
				}else break;
			}
		}
		while (GetNowCount() - timeCount <= TIMELIMIT) {}	// 一定時間が経過するまで待機
		return put;
	}

	/**
	* @brief 盤面の評価値を取得する関数
	* @param board Boardインスタンス
	* @return int 評価値
	* @details 序盤，中盤は自分のマスの評価値 - 相手のマスの評価値±着手可能な手の数．
	* 
	* 終盤は自分の石の数 - 相手の石の数
	*/
	int eval(Board board) {
		int eval = 0;
		// 終盤に入るまで
		if (board.nowIndex <= FINISH_TURN) {
			int playerBoardScore = 0;	// マスの評価値
			int opponentBoardScore = 0;
			int CanPutCount;			// 着手可能な手の数

			// スコアテーブルからマスの評価値を取得
			// 両プレイヤーの評価値に加算する
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
			// 着手可能な手の数を取得
			CanPutCount = bitCount(makeLegalBoard(board.playerBoard, board.opponentBoard));

			// 評価値計算し返す
			if (board.nowTurn == player) eval = playerBoardScore - opponentBoardScore + CanPutCount;
			else eval = opponentBoardScore - playerBoardScore - CanPutCount;
			return eval;
		}else{
			// 終盤
			int playerStoneCount;	// プレイヤーの石の数
			int opponentStoneCount;

			// 両プレイヤーの石の数を取得
			// 評価値を計算し返す
			playerStoneCount = bitCount(board.playerBoard);
			opponentStoneCount = bitCount(board.opponentBoard);
			if (board.nowTurn == player) return playerStoneCount - opponentStoneCount;
			else return opponentStoneCount - playerStoneCount;
		}
		return 0;
	}
	/**
	* @brief αβ法
	* @param board Boardインスタンス
	* @param depth 残りの探索深さ
	* @param alpha α値
	* @param beta β値
	* @return int 最善手の評価値
	* @details 着手可能な手から盤面を作る．
	* 
	* 再帰を用い，最奥まで探索した後に評価値を押し上げていく．
	* 
	* α値とβ値を元に枝狩りをすることで計算回数を減らしている
	*/
	int alphaBeta(Board board, int depth, int alpha, int beta) {
		// ボードのコピーボードの作成
		Board expBoard;
		expBoard.playerBoard = board.playerBoard;
		expBoard.opponentBoard = board.opponentBoard;
		expBoard.nowTurn = board.nowTurn;
		// 後に手を打った後，1手進んでしまうが，初期ターンを保持しておきたいため－1する
		expBoard.nowIndex = board.nowIndex - 1;

		// 最奥にたどり着くか，指し手がなくなるとその時点での評価値を計算し返す
		if (depth == 0 || board.isPass() || board.isGameFinished())return eval(board);
		
		// 指すことが可能な手を全て取得
		unsigned long long expandPut[30];
		expandNode(board, expandPut);

		// 指すことが可能な手の数を取得
		int nodeNum = bitCount(makeLegalBoard(board.playerBoard, board.opponentBoard));

		int val;
		// 全て探索するか，
		// 枝分かれしたノードより優先されるべきものが見つからないことが決まったとき探索を打ち切る
		for (int i = 0; (alpha < beta) && (i < nodeNum); i++) {
			expBoard.reverse(expandPut[i]);	// 着手可能な手を反映
			expBoard.swapBoard();			// 手番を入れ替える
			val = alphaBeta(expBoard, depth - 1, alpha, beta);	// 再帰呼び出し．残り深さを1下げる

			// 下の層を探索し終えると元に戻す
			expBoard.playerBoard = board.playerBoard;
			expBoard.opponentBoard = board.opponentBoard;
			expBoard.nowTurn = board.nowTurn;
			expBoard.nowIndex = board.nowIndex - 1;

			// 評価値を更新
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
	* @brief ノードを広げるための関数
	* @param board Boardインスタンス
	* @param expandBoard 指すことが可能な手を保存する配列
	*/
	void expandNode(Board board, unsigned long long *expandBoard) {
		// ボードを引数とし，合法手ボードを取得
		unsigned long long legalBoard = makeLegalBoard(board.playerBoard, board.opponentBoard);
		int nodeNum = bitCount(legalBoard);
		
		// 合法手ボードを分解し，expandBoardに保存
		for (int i = 0; i < nodeNum; i++) {
			for (int j = 0; j < 64; j++) {
				// 1マスずつずらしていき，1が見つかれば保存
				// 合法手ボードからそのマスを除外
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
* @brief 指し手をマウスから取得する関数
* @return unsigned long long 指し手
*/
unsigned long long inputPut() {
	static bool mouse_flag = false;
	while (1) {
		// マウスの左クリックを感知
		if (GetMouseInput() & MOUSE_INPUT_LEFT) {
			if (!mouse_flag) {
				mouse_flag = true;
				int mx, my;
				GetMousePoint(&mx, &my);

				// 座標をビットに変換
				// 分母は1マスのピクセル数
				// 画面左上の端が盤面の端としてない場合は適宜直すこと
				unsigned long long put = coordToBit(mx / 60, my / 60);

				return put;
			}
		}
		else mouse_flag = false;
	}
	return 0;
}

/**
* @brief 指し手をランダムに取得する関数
* @return unsigned long long 指し手
*/
unsigned long long randomPut() {
	random_device rnd;
	// x,y座標は0～7
	int x = rnd() % 8;
	int y = rnd() % 8;

	// 座標をビットに変換
	unsigned long long put = coordToBit(x, y);
	return put;
}