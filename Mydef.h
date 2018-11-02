/**
* @file Mydef.h
* @brief マクロ纏め
*/

/**
* @def TIMELIMIT
* @brief 時間制限[ms]
* @details 戦略がマニュアル以外の場合，時間がこの数値を超えないと打たない
*/
#define TIMELIMIT	1500	// 時間制限

#define MIDDLE_TURN	10	// 中盤に入るターン数
#define	FINISH_TURN	48	// 終盤に入るターン数

#define MAX_TURN 60		// ターン数の上限
#define PLAYER1	 0
#define PLAYER2  1

#define MANUAL	  0
#define RANDOM	  1
#define MINIMAX	  2
#define ALPHABETA 3
