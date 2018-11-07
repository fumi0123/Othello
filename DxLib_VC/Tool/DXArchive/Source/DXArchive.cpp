// -------------------------------------------------------------------------------
// 
// 		ＤＸライブラリアーカイバ
// 
//	Creator			: 山田 巧
//	Creation Date	: 2003/09/11
//	Version			: 1.02
// 
// -------------------------------------------------------------------------------

#define INLINE_ASM

// include ----------------------------
#include "DXArchive.h"
#include "CharCode.h"
#include <stdio.h>
#include <windows.h>
#include <string.h>

// define -----------------------------

#define MIN_COMPRESS		(4)						// 最低圧縮バイト数
#define MAX_SEARCHLISTNUM	(64)					// 最大一致長を探す為のリストを辿る最大数
#define MAX_SUBLISTNUM		(65536)					// 圧縮時間短縮のためのサブリストの最大数
#define MAX_COPYSIZE 		(0x1fff + MIN_COMPRESS)	// 参照アドレスからコピー出切る最大サイズ( 圧縮コードが表現できるコピーサイズの最大値 + 最低圧縮バイト数 )
#define MAX_ADDRESSLISTNUM	(1024 * 1024 * 1)		// スライド辞書の最大サイズ
#define MAX_POSITION		(1 << 24)				// 参照可能な最大相対アドレス( 16MB )

// struct -----------------------------

// 圧縮時間短縮用リスト
typedef struct LZ_LIST
{
	LZ_LIST *next, *prev ;
	u32 address ;
} LZ_LIST ;

// class code -------------------------

// ファイル名も一緒になっていると分かっているパス中からファイルパスとディレクトリパスを分割する
// フルパスである必要は無い
int DXArchive::GetFilePathAndDirPath( char *Src, char *FilePath, char *DirPath )
{
	int i, Last ;
	
	// ファイル名を抜き出す
	i = 0 ;
	Last = -1 ;
	while( Src[i] != '\0' )
	{
		if( CheckMultiByteChar( &Src[i] ) == FALSE )
		{
			if( Src[i] == '\\' || Src[i] == '/' || Src[i] == '\0' || Src[i] == ':' ) Last = i ;
			i ++ ;
		}
		else
		{
			i += 2 ;
		}
	}
	if( FilePath != NULL )
	{
		if( Last != -1 ) strcpy( FilePath, &Src[Last+1] ) ;
		else strcpy( FilePath, Src ) ;
	}
	
	// ディレクトリパスを抜き出す
	if( DirPath != NULL )
	{
		if( Last != -1 )
		{
			strncpy( DirPath, Src, Last ) ;
			DirPath[Last] = '\0' ;
		}
		else
		{
			DirPath[0] = '\0' ;
		}
	}
	
	// 終了
	return 0 ;
}

// ファイルの情報を得る
DARC_FILEHEAD *DXArchive::GetFileInfo( const char *FilePath, DARC_DIRECTORY **DirectoryP )
{
	DARC_DIRECTORY *OldDir ;
	DARC_FILEHEAD *FileH ;
	u8 *NameData ;
	int i, j, k, Num, FileHeadSize ;
	SEARCHDATA SearchData ;

	// 元のディレクトリを保存しておく
	OldDir = this->CurrentDirectory ;

	// ファイルパスに \ が含まれている場合、ディレクトリ変更を行う
	if( strchr( FilePath, '\\' ) != NULL )
	{
		// カレントディレクトリを目的のファイルがあるディレクトリに変更する
		if( this->ChangeCurrentDirectoryBase( FilePath, false, &SearchData ) >= 0 )
		{
			// エラーが起きなかった場合はファイル名もディレクトリだったことになるのでエラー
			goto ERR ;
		}
	}
	else
	{
		// ファイル名を検索用データに変換する
		ConvSearchData( &SearchData, FilePath, NULL ) ;
	}

	// 同名のファイルを探す
	FileHeadSize = sizeof( DARC_FILEHEAD ) ;
	FileH = ( DARC_FILEHEAD * )( this->FileP + this->CurrentDirectory->FileHeadAddress ) ;
	Num = ( int )this->CurrentDirectory->FileHeadNum ;
	for( i = 0 ; i < Num ; i ++, FileH = (DARC_FILEHEAD *)( (u8 *)FileH + FileHeadSize ) )
	{
		// ディレクトリチェック
		if( ( FileH->Attributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 ) continue ;

		// 文字列数とパリティチェック
		NameData = this->NameP + FileH->NameAddress ;
		if( SearchData.PackNum != ((u16 *)NameData)[0] || SearchData.Parity != ((u16 *)NameData)[1] ) continue ;

		// 文字列チェック
		NameData += 4 ;
		for( j = 0, k = 0 ; j < SearchData.PackNum ; j ++, k += 4 )
			if( *((u32 *)&SearchData.FileName[k]) != *((u32 *)&NameData[k]) ) break ;

		// 適合したファイルがあったらここで終了
		if( SearchData.PackNum == j ) break ;
	}

	// 無かったらエラー
	if( i == Num ) goto ERR ;

	// ディレクトリのアドレスを保存する指定があった場合は保存
	if( DirectoryP != NULL )
	{
		*DirectoryP = this->CurrentDirectory ;
	}
	
	// ディレクトリを元に戻す
	this->CurrentDirectory = OldDir ;
	
	// 目的のファイルのアドレスを返す
	return FileH ;
	
ERR :
	// ディレクトリを元に戻す
	this->CurrentDirectory = OldDir ;
	
	// エラー終了
	return NULL ;
}

// アーカイブ内のカレントディレクトリの情報を取得する
DARC_DIRECTORY *DXArchive::GetCurrentDirectoryInfo( void )
{
	return CurrentDirectory ;
}

// どちらが新しいかを比較する
DXArchive::DATE_RESULT DXArchive::DateCmp( DARC_FILETIME *date1, DARC_FILETIME *date2 )
{
	if( date1->LastWrite == date2->LastWrite ) return DATE_RESULT_DRAW ;
	else if( date1->LastWrite > date2->LastWrite ) return DATE_RESULT_LEFTNEW ;
	else return DATE_RESULT_RIGHTNEW ;
}

// 比較対照の文字列中の大文字を小文字として扱い比較する( 0:等しい  1:違う )
int DXArchive::StrICmp( const char *Str1, const char *Str2 )
{
	int c1, c2 ;
	
	while( *Str1 != '\0' && *Str2 != '\0' )
	{
		if( CheckMultiByteChar( Str1 ) == FALSE )
		{
			c1 = ( *Str1 >= 'A' && *Str1 <= 'Z' ) ? *Str1 - 'A' + 'a' : *Str1 ;
			c2 = ( *Str2 >= 'A' && *Str2 <= 'Z' ) ? *Str2 - 'A' + 'a' : *Str2 ;
			if( c1 != c2 ) return 1 ;
			Str1 ++ ;
			Str2 ++ ;
		}
		else
		{
			if( *( (unsigned short *)Str1 ) != *( (unsigned short *)Str2 ) ) return 1 ;
			Str1 += 2 ;
			Str2 += 2 ;
		}
	}
	if( *Str1 != '\0' || *Str2 != '\0' ) return 1 ;

	// 此処まで来て初めて等しい
	return 0 ;
}

// 文字列中の英字の小文字を大文字に変換
int DXArchive::ConvSearchData( SEARCHDATA *SearchData, const char *Src, int *Length )
{
	int i, StringLength ;
	u16 ParityData ;

	ParityData = 0 ;
	for( i = 0 ; Src[i] != '\0' && Src[i] != '\\' ; )
	{
		if( CheckMultiByteChar( &Src[i] ) == TRUE )
		{
			// ２バイト文字の場合はそのままコピー
			*((u16 *)&SearchData->FileName[i]) = *((u16 *)&Src[i]) ;
			ParityData += (u8)Src[i] + (u8)Src[i+1] ;
			i += 2 ;
		}
		else
		{
			// 小文字の場合は大文字に変換
			if( Src[i] >= 'a' && Src[i] <= 'z' )	SearchData->FileName[i] = (u8)Src[i] - 'a' + 'A' ;
			else									SearchData->FileName[i] = Src[i] ;
			ParityData += (u8)SearchData->FileName[i] ;
			i ++ ;
		}
	}

	// 文字列の長さを保存
	if( Length != NULL ) *Length = i ;

	// ４の倍数の位置まで０を代入
	StringLength = ( ( i + 1 ) + 3 ) / 4 * 4 ;
	memset( &SearchData->FileName[i], 0, StringLength - i ) ;

	// パリティデータの保存
	SearchData->Parity = ParityData ;

	// パックデータ数の保存
	SearchData->PackNum = StringLength / 4 ;

	// 正常終了
	return 0 ;
}

// ファイル名データを追加する( 戻り値は使用したデータバイト数 )
int DXArchive::AddFileNameData( int CharCodeFormat, const char *FileName, u8 *FileNameTable )
{
	int PackNum, Length, i, j ;
	u32 Parity ;

	// サイズをセット
	Length = ( int )CL_strlen( CharCodeFormat, FileName ) * GetCharCodeFormatUnitSize( CharCodeFormat ) ;

	// 一文字も無かった場合の処理
	if( Length == 0 )
	{
		// パック数とパリティ情報のみ保存
		*((u32 *)&FileNameTable[0]) = 0 ;

		// 使用サイズを返す
		return 4 ;
	}
	Length ++ ;

	PackNum = ( Length + 3 ) / 4 ;

	// パック数を保存
	*((u16 *)&FileNameTable[0]) = PackNum ;

	// バッファの初期化
	memset( &FileNameTable[4], 0, PackNum * 4 * 2 ) ;

	// 文字列をコピー
	CL_strcpy( CharCodeFormat, (char *)&FileNameTable[4 + PackNum * 4], FileName ) ;

	// 英字の小文字を全て大文字に変換したファイル名を保存
	Parity = 0 ;
	for( i = 0 ; FileName[i] != '\0' ; )
	{
		int Bytes = GetCharBytes( &FileName[i], CharCodeFormat ) ;

		// 1バイト文字かどうかで処理を分岐
		if( Bytes == 1 )
		{
			// １バイト文字
			if( FileName[i] >= 'a' && FileName[i] <= 'z' )
			{
				// 小文字の場合は大文字に変換
				FileNameTable[4 + i] = (u8)FileName[i] - 'a' + 'A' ;
			}
			else
			{
				// そうではない場合は普通にコピー
				FileNameTable[4 + i] = (u8)FileName[i] ;
			}
			Parity += FileNameTable[4 + i] ;
			i ++ ;
		}
		else
		{
			// マルチバイト文字
			for( j = 0 ; j < Bytes ; j ++ )
			{
				FileNameTable[4 + i + j] = (u8)FileName[i + j] ;
				Parity += (u8)FileName[i + j] ;
			}
			i += Bytes ;
		}
	}

	// パリティ情報を保存
	*((u16 *)&FileNameTable[2]) = (u16)Parity ;

	// 使用したサイズを返す
	return PackNum * 4 * 2 + 4 ;
}

// ファイル名データから元のファイル名の文字列を取得する
const char *DXArchive::GetOriginalFileName( u8 *FileNameTable )
{
	return (char *)FileNameTable + *((u16 *)&FileNameTable[0]) * 4 + 4 ;
}

// 標準ストリームにデータを書き込む( 64bit版 )
void DXArchive::fwrite64( void *Data, s64 Size, FILE *fp )
{
	int WriteSize ;
	s64 TotalWriteSize ;

	TotalWriteSize = 0 ;
	while( TotalWriteSize < Size )
	{
		if( Size > 0x7fffffff )
		{
			WriteSize = 0x7fffffff ;
		}
		else
		{
			WriteSize = ( int )Size ;
		}

		fwrite( ( u8 * )Data + TotalWriteSize, 1, WriteSize, fp ) ;

		TotalWriteSize += WriteSize ;
	}
}

// 標準ストリームからデータを読み込む( 64bit版 )
void DXArchive::fread64( void *Buffer, s64 Size, FILE *fp )
{
	int ReadSize ;
	s64 TotalReadSize ;

	TotalReadSize = 0 ;
	while( TotalReadSize < Size )
	{
		if( Size > 0x7fffffff )
		{
			ReadSize = 0x7fffffff ;
		}
		else
		{
			ReadSize = ( int )Size ;
		}

		fread( ( u8 * )Buffer + TotalReadSize, 1, ReadSize, fp ) ;

		TotalReadSize += ReadSize ;
	}
}

// データを反転させる関数
void DXArchive::NotConv( void *Data , s64 Size )
{
	s64 DwordNumQ ;
	s64 ByteNum ;
	u32 *dd ;

	dd = ( u32 * )Data ;

	DwordNumQ = Size / 4 ;
	ByteNum = Size - DwordNumQ * 4 ;

	if( DwordNumQ != 0 )
	{
		if( DwordNumQ < 0x100000000 )
		{
			u32 DwordNum ;

			DwordNum = ( u32 )DwordNumQ ;
			do
			{
				*dd++ = ~*dd ;
			}while( --DwordNum ) ;
		}
		else
		{
			do
			{
				*dd++ = ~*dd ;
			}while( --DwordNumQ ) ;
		}
	}
	if( ByteNum != 0 )
	{
		do
		{
			*((BYTE *)dd) = ~*((u8 *)dd) ;
			dd = (u32 *)(((u8 *)dd) + 1) ;
		}while( --ByteNum ) ;
	}
}


// データを反転させてファイルに書き出す関数
void DXArchive::NotConvFileWrite( void *Data, s64 Size, FILE *fp )
{
	// データを反転する
	NotConv( Data, Size ) ;

	// 書き出す
	fwrite64( Data, Size, fp ) ;

	// 再び反転
	NotConv( Data, Size ) ;
}

// データを反転させてファイルから読み込む関数
void DXArchive::NotConvFileRead( void *Data, s64 Size, FILE *fp )
{
	// 読み込む
	fread64( Data, Size, fp ) ;

	// データを反転
	NotConv( Data, Size ) ;
}

// カレントディレクトリにある指定のファイルの鍵バージョン２用の文字列を作成する、戻り値は文字列の長さ( 単位：Byte )( FileString は DXA_KEYV2_STRING_MAXLENGTH の長さが必要 )
size_t DXArchive::CreateKeyV2FileString( int CharCodeFormat, const char *KeyV2String, size_t KeyV2StringBytes, DARC_DIRECTORY *Directory, DARC_FILEHEAD *FileHead, u8 *FileTable, u8 *DirectoryTable, u8 *NameTable, u8 *FileString )
{
	size_t StartAddr ;

	// 最初にパスワードの文字列をセット
	if( KeyV2String != NULL && KeyV2StringBytes != 0 )
	{
		memcpy( FileString, KeyV2String, KeyV2StringBytes ) ;
		FileString[ KeyV2StringBytes ] = '\0' ;
		StartAddr = KeyV2StringBytes ;
	}
	else
	{
		FileString[ 0 ] = '\0' ;
		StartAddr = 0 ;
	}

	// 次にファイル名の文字列をセット
	CL_strcat_s( CharCodeFormat, ( char * )&FileString[ StartAddr ], DXA_KEYV2_STRING_MAXLENGTH - StartAddr, ( char * )( NameTable + FileHead->NameAddress + 4 ) ) ;

	// その後にディレクトリの文字列をセット
	if( Directory->ParentDirectoryAddress != 0xffffffffffffffff )
	{
		do
		{
			CL_strcat_s( CharCodeFormat, ( char * )&FileString[ StartAddr ], DXA_KEYV2_STRING_MAXLENGTH - StartAddr, ( char * )( NameTable + ( ( DARC_FILEHEAD * )( FileTable + Directory->DirectoryAddress ) )->NameAddress + 4 ) ) ;
			Directory = ( DARC_DIRECTORY * )( DirectoryTable + Directory->ParentDirectoryAddress ) ;
		}while( Directory->ParentDirectoryAddress != 0xffffffffffffffff ) ;
	}

	return StartAddr + CL_strlen( CharCodeFormat, ( char * )&FileString[ StartAddr ] ) * GetCharCodeFormatUnitSize( CharCodeFormat ) ;
}

// 鍵文字列を作成
void DXArchive::KeyCreate( const char *Source, unsigned char *Key )
{
	int Len ;

	if( Source == NULL )
	{
		memset( Key, 0xaaaaaaaa, DXA_KEYSTR_LENGTH ) ;
	}
	else
	{
		Len = ( int )strlen( Source ) ;
		if( Len > DXA_KEYSTR_LENGTH )
		{
			memcpy( Key, Source, DXA_KEYSTR_LENGTH ) ;
		}
		else
		{
			// 鍵文字列が DXA_KEYSTR_LENGTH より短かったらループする
			int i ;

			for( i = 0 ; i + Len <= DXA_KEYSTR_LENGTH ; i += Len )
				memcpy( Key + i, Source, Len ) ;
			if( i < DXA_KEYSTR_LENGTH )
				memcpy( Key + i, Source, DXA_KEYSTR_LENGTH - i ) ;
		}
	}

	Key[0] = ~Key[0] ;
	Key[1] = ( Key[1] >> 4 ) | ( Key[1] << 4 ) ;
	Key[2] = Key[2] ^ 0x8a ;
	Key[3] = ~( ( Key[3] >> 4 ) | ( Key[3] << 4 ) ) ;
	Key[4] = ~Key[4] ;
	Key[5] = Key[5] ^ 0xac ;
	Key[6] = ~Key[6] ;
	Key[7] = ~( ( Key[7] >> 3 ) | ( Key[7] << 5 ) ) ;
	Key[8] = ( Key[8] >> 5 ) | ( Key[8] << 3 ) ;
	Key[9] = Key[9] ^ 0x7f ;
	Key[10] = ( ( Key[10] >> 4 ) | ( Key[10] << 4 ) ) ^ 0xd6 ;
	Key[11] = Key[11] ^ 0xcc ;
}

// 鍵バージョン２文字列を作成
void DXArchive::KeyV2Create( const char *Source, unsigned char *Key, size_t KeyBytes )
{
	if( Source == NULL )
	{
		static BYTE DefaultKeyString[ 5 ] = { 0x44, 0x58, 0x41, 0x52, 0x43 }; // "DXARC"
		HashSha256( DefaultKeyString, sizeof( DefaultKeyString ), Key ) ;
	}
	else
	{
		HashSha256( Source, KeyBytes == 0 ? CL_strlen( CHARCODEFORMAT_ASCII, Source ) : KeyBytes, Key ) ;
	}
}

// 鍵文字列を使用して Xor 演算( Key は必ず DXA_KEYSTR_LENGTH の長さがなければならない )
void DXArchive::KeyConv( void *Data, s64 Size, s64 Position, unsigned char *Key )
{
	Position %= DXA_KEYSTR_LENGTH ;

	if( Size < 0x100000000 )
	{
		u32 i, j ;

		j = ( u32 )Position ;
		for( i = 0 ; i < Size ; i ++ )
		{
			((u8 *)Data)[i] ^= Key[j] ;

			j ++ ;
			if( j == DXA_KEYSTR_LENGTH ) j = 0 ;
		}
	}
	else
	{
		s64 i, j ;

		j = Position ;
		for( i = 0 ; i < Size ; i ++ )
		{
			((u8 *)Data)[i] ^= Key[j] ;

			j ++ ;
			if( j == DXA_KEYSTR_LENGTH ) j = 0 ;
		}
	}
}

// 鍵バージョン２文字列を使用して Xor 演算( Key は必ず DXA_KEYV2_LENGTH の長さがなければならない )
void DXArchive::KeyV2Conv( void *Data, s64 Size, s64 Position, unsigned char *Key )
{
	Position %= DXA_KEYV2_LENGTH ;

	if( Size < 0x100000000 )
	{
		u32 i, j ;

		j = ( u32 )Position ;
		for( i = 0 ; i < Size ; i ++ )
		{
			((u8 *)Data)[i] ^= Key[j] ;

			j ++ ;
			if( j == DXA_KEYV2_LENGTH ) j = 0 ;
		}
	}
	else
	{
		s64 i, j ;

		j = Position ;
		for( i = 0 ; i < Size ; i ++ )
		{
			((u8 *)Data)[i] ^= Key[j] ;

			j ++ ;
			if( j == DXA_KEYV2_LENGTH ) j = 0 ;
		}
	}
}

// データを鍵文字列を使用して Xor 演算した後ファイルに書き出す関数( Key は必ず DXA_KEYSTR_LENGTH の長さがなければならない )
void DXArchive::KeyConvFileWrite( void *Data, s64 Size, FILE *fp, unsigned char *Key, s64 Position )
{
	s64 pos ;

	// ファイルの位置を取得しておく
	if( Position == -1 ) pos = _ftelli64( fp ) ;
	else                 pos = Position ;

	// データを鍵文字列を使って Xor 演算する
	KeyConv( Data, Size, pos, Key ) ;

	// 書き出す
	fwrite64( Data, Size, fp ) ;

	// 再び Xor 演算
	KeyConv( Data, Size, pos, Key ) ;
}

// データを鍵バージョン２文字列を使用して Xor 演算した後ファイルに書き出す関数( Key は必ず DXA_KEYV2_LENGTH の長さがなければならない )
void DXArchive::KeyV2ConvFileWrite( void *Data, s64 Size, FILE *fp, unsigned char *Key, s64 Position )
{
	// データを鍵文字列を使って Xor 演算する
	KeyV2Conv( Data, Size, Position, Key ) ;

	// 書き出す
	fwrite64( Data, Size, fp ) ;

	// 再び Xor 演算
	KeyV2Conv( Data, Size, Position, Key ) ;
}

// ファイルから読み込んだデータを鍵文字列を使用して Xor 演算する関数( Key は必ず DXA_KEYSTR_LENGTH の長さがなければならない )
void DXArchive::KeyConvFileRead( void *Data, s64 Size, FILE *fp, unsigned char *Key, s64 Position )
{
	s64 pos ;

	// ファイルの位置を取得しておく
	if( Position == -1 ) pos = _ftelli64( fp ) ;
	else                 pos = Position ;

	// 読み込む
	fread64( Data, Size, fp ) ;

	// データを鍵文字列を使って Xor 演算
	KeyConv( Data, Size, pos, Key ) ;
}

// ファイルから読み込んだデータを鍵バージョン２文字列を使用して Xor 演算する関数( Key は必ず DXA_KEYV2_LENGTH の長さがなければならない )
void DXArchive::KeyV2ConvFileRead( void *Data, s64 Size, FILE *fp, unsigned char *Key, s64 Position )
{
	// 読み込む
	fread64( Data, Size, fp ) ;

	// データを鍵文字列を使って Xor 演算
	KeyV2Conv( Data, Size, Position, Key ) ;
}

/*
// ２バイト文字か調べる( TRUE:２バイト文字 FALSE:１バイト文字 )
int DXArchive::CheckMultiByteChar( const char *Buf )
{
	return  ( (unsigned char)*Buf >= 0x81 && (unsigned char)*Buf <= 0x9F ) || ( (unsigned char)*Buf >= 0xE0 && (unsigned char)*Buf <= 0xFC ) ;
}
*/

// 指定のディレクトリにあるファイルをアーカイブデータに吐き出す
int DXArchive::DirectoryEncode( int CharCodeFormat, char *DirectoryName, u8 *NameP, u8 *DirP, u8 *FileP, DARC_DIRECTORY *ParentDir, SIZESAVE *Size, int DataNumber, FILE *DestP, void *TempBuffer, bool Press, const char *KeyV2String, size_t KeyV2StringBytes, char *KeyV2StringBuffer )
{
	char DirPath[MAX_PATH] ;
	WIN32_FIND_DATAA FindData ;
	HANDLE FindHandle ;
	DARC_DIRECTORY Dir ;
	DARC_DIRECTORY *DirectoryP ;
	DARC_FILEHEAD File ;
	u8 lKeyV2[DXA_KEYV2_LENGTH] ;
	size_t KeyV2StringBufferBytes ;

	// ディレクトリの情報を得る
	FindHandle = FindFirstFileA( DirectoryName, &FindData ) ;
	if( FindHandle == INVALID_HANDLE_VALUE ) return 0 ;
	
	// ディレクトリ情報を格納するファイルヘッダをセットする
	{
		File.NameAddress     = Size->NameSize ;
		File.Time.Create     = ( ( ( LONGLONG )FindData.ftCreationTime.dwHighDateTime ) << 32 ) + FindData.ftCreationTime.dwLowDateTime ;
		File.Time.LastAccess = ( ( ( LONGLONG )FindData.ftLastAccessTime.dwHighDateTime ) << 32 ) + FindData.ftLastAccessTime.dwLowDateTime ;
		File.Time.LastWrite  = ( ( ( LONGLONG )FindData.ftLastWriteTime.dwHighDateTime ) << 32 ) + FindData.ftLastWriteTime.dwLowDateTime ;
		File.Attributes      = FindData.dwFileAttributes ;
		File.DataAddress     = Size->DirectorySize ;
		File.DataSize        = 0 ;
		File.PressDataSize	 = 0xffffffffffffffff ;
	}

	// ディレクトリ名を書き出す
	Size->NameSize += AddFileNameData( CharCodeFormat, FindData.cFileName, NameP + Size->NameSize ) ;

	// ディレクトリ情報が入ったファイルヘッダを書き出す
	memcpy( FileP + ParentDir->FileHeadAddress + DataNumber * sizeof( DARC_FILEHEAD ),
			&File, sizeof( DARC_FILEHEAD ) ) ;

	// Find ハンドルを閉じる
	FindClose( FindHandle ) ;

	// 指定のディレクトリにカレントディレクトリを移す
	GetCurrentDirectoryA( MAX_PATH, DirPath ) ;
	SetCurrentDirectoryA( DirectoryName ) ;

	// ディレクトリ情報のセット
	{
		Dir.DirectoryAddress = ParentDir->FileHeadAddress + DataNumber * sizeof( DARC_FILEHEAD ) ;
		Dir.FileHeadAddress  = Size->FileSize ;

		// 親ディレクトリの情報位置をセット
		if( ParentDir->DirectoryAddress != 0xffffffffffffffff && ParentDir->DirectoryAddress != 0 )
		{
			Dir.ParentDirectoryAddress = ((DARC_FILEHEAD *)( FileP + ParentDir->DirectoryAddress ))->DataAddress ;
		}
		else
		{
			Dir.ParentDirectoryAddress = 0 ;
		}

		// ディレクトリ中のファイルの数を取得する
		Dir.FileHeadNum = GetDirectoryFilePath( "", NULL ) ;
	}

	// ディレクトリの情報を出力する
	memcpy( DirP + Size->DirectorySize, &Dir, sizeof( DARC_DIRECTORY ) ) ;	
	DirectoryP = ( DARC_DIRECTORY * )( DirP + Size->DirectorySize ) ;

	// アドレスを推移させる
	Size->DirectorySize += sizeof( DARC_DIRECTORY ) ;
	Size->FileSize      += sizeof( DARC_FILEHEAD ) * Dir.FileHeadNum ;
	
	// ファイルが何も無い場合はここで終了
	if( Dir.FileHeadNum == 0 )
	{
		// もとのディレクトリをカレントディレクトリにセット
		SetCurrentDirectoryA( DirPath ) ;
		return 0 ;
	}

	// ファイル情報を出力する
	{
		int i ;
		
		i = 0 ;
		
		// 列挙開始
		FindHandle = FindFirstFileA( "*", &FindData ) ;
		do
		{
			// 上のディレクトリに戻ったりするためのパスは無視する
			if( strcmp( FindData.cFileName, "." ) == 0 || strcmp( FindData.cFileName, ".." ) == 0 ) continue ;

			// ファイルではなく、ディレクトリだった場合は再帰する
			if( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// ディレクトリだった場合の処理
				if( DirectoryEncode( CharCodeFormat, FindData.cFileName, NameP, DirP, FileP, &Dir, Size, i, DestP, TempBuffer, Press, KeyV2String, KeyV2StringBytes, KeyV2StringBuffer ) < 0 ) return -1 ;
			}
			else
			{
				// ファイルだった場合の処理

				// ファイルのデータをセット
				File.NameAddress     = Size->NameSize ;
				File.Time.Create     = ( ( ( LONGLONG )FindData.ftCreationTime.dwHighDateTime   ) << 32 ) + FindData.ftCreationTime.dwLowDateTime   ;
				File.Time.LastAccess = ( ( ( LONGLONG )FindData.ftLastAccessTime.dwHighDateTime ) << 32 ) + FindData.ftLastAccessTime.dwLowDateTime ;
				File.Time.LastWrite  = ( ( ( LONGLONG )FindData.ftLastWriteTime.dwHighDateTime  ) << 32 ) + FindData.ftLastWriteTime.dwLowDateTime  ;
				File.Attributes      = FindData.dwFileAttributes ;
				File.DataAddress     = Size->DataSize ;
				File.DataSize        = ( ( ( LONGLONG )FindData.nFileSizeHigh ) << 32 ) + FindData.nFileSizeLow ;
				File.PressDataSize   = 0xffffffffffffffff ;

				// ファイル名を書き出す
				Size->NameSize += AddFileNameData( CharCodeFormat, FindData.cFileName, NameP + Size->NameSize ) ;

				// ファイル個別の鍵を作成
				KeyV2StringBufferBytes = CreateKeyV2FileString( CharCodeFormat, KeyV2String, KeyV2StringBytes, DirectoryP, &File, FileP, DirP, NameP, ( BYTE * )KeyV2StringBuffer ) ;
				KeyV2Create( KeyV2StringBuffer, lKeyV2 , KeyV2StringBufferBytes ) ;
				
				// ファイルデータを書き出す
				if( File.DataSize != 0 )
				{
					FILE *SrcP ;
					u64 FileSize, WriteSize, MoveSize ;

					// ファイルを開く
					SrcP = fopen( FindData.cFileName, "rb" ) ;
					
					// サイズを得る
					_fseeki64( SrcP, 0, SEEK_END ) ;
					FileSize = _ftelli64( SrcP ) ;
					_fseeki64( SrcP, 0, SEEK_SET ) ;
					
					// ファイルサイズが 10MB 以下の場合で、圧縮の指定がある場合は圧縮を試みる
					if( Press == true && File.DataSize < 10 * 1024 * 1024 )
					{
						void *SrcBuf, *DestBuf ;
						u32 DestSize, Len ;
						
						// 一部のファイル形式の場合は予め弾く
						if( ( Len = ( int )strlen( FindData.cFileName ) ) > 4 )
						{
							char *sp ;
							
							sp = &FindData.cFileName[Len-3] ;
							if( StrICmp( sp, "wav" ) == 0 ||
								StrICmp( sp, "jpg" ) == 0 ||
								StrICmp( sp, "png" ) == 0 ||
								StrICmp( sp, "mpg" ) == 0 ||
								StrICmp( sp, "mp3" ) == 0 ||
								StrICmp( sp, "ogg" ) == 0 ||
								StrICmp( sp, "wmv" ) == 0 ||
								StrICmp( sp - 1, "jpeg" ) == 0 ) goto NOPRESS ;
						}
						
						// データが丸ごと入るメモリ領域の確保
						SrcBuf  = malloc( ( size_t )( FileSize + FileSize * 2 + 64 ) ) ;
						DestBuf = (u8 *)SrcBuf + FileSize ;
						
						// ファイルを丸ごと読み込む
						fread64( SrcBuf, FileSize, SrcP ) ;
						
						// 圧縮
						DestSize = Encode( SrcBuf, ( u32 )FileSize, DestBuf ) ;
						
						// 殆ど圧縮出来なかった場合は圧縮無しでアーカイブする
						if( (f64)DestSize / (f64)FileSize > 0.90 )
						{
							_fseeki64( SrcP, 0L, SEEK_SET ) ;
							free( SrcBuf ) ;
							goto NOPRESS ;
						}
						
						// 圧縮データを反転して書き出す
						WriteSize = ( DestSize + 3 ) / 4 * 4 ;
						KeyV2ConvFileWrite( DestBuf, WriteSize, DestP, lKeyV2, File.DataSize ) ;
						
						// メモリの解放
						free( SrcBuf ) ;
						
						// 圧縮データのサイズを保存する
						File.PressDataSize = DestSize ;
					}
					else
					{
NOPRESS:					
						// 転送開始
						WriteSize = 0 ;
						while( WriteSize < FileSize )
						{
							// 転送サイズ決定
							MoveSize = DXA_BUFFERSIZE < FileSize - WriteSize ? DXA_BUFFERSIZE : FileSize - WriteSize ;
							MoveSize = ( MoveSize + 3 ) / 4 * 4 ;	// サイズは４の倍数に合わせる
							
							// ファイルの反転読み込み
							KeyV2ConvFileRead( TempBuffer, MoveSize, SrcP, lKeyV2, File.DataSize + WriteSize ) ;

							// 書き出し
							fwrite64( TempBuffer, MoveSize, DestP ) ;
							
							// 書き出しサイズの加算
							WriteSize += MoveSize ;
						}
					}
					
					// 書き出したファイルを閉じる
					fclose( SrcP ) ;
				
					// データサイズの加算
					Size->DataSize += WriteSize ;
				}
				
				// ファイルヘッダを書き出す
				memcpy( FileP + Dir.FileHeadAddress + sizeof( DARC_FILEHEAD ) * i, &File, sizeof( DARC_FILEHEAD ) ) ;
			}
			
			i ++ ;
		}
		while( FindNextFileA( FindHandle, &FindData ) != 0 ) ;
		
		// Find ハンドルを閉じる
		FindClose( FindHandle ) ;
	}
						
	// もとのディレクトリをカレントディレクトリにセット
	SetCurrentDirectoryA( DirPath ) ;

	// 終了
	return 0 ;
}

// 指定のディレクトリデータにあるファイルを展開する
int DXArchive::DirectoryDecode( u8 *NameP, u8 *DirP, u8 *FileP, DARC_HEAD *Head, DARC_DIRECTORY *Dir, FILE *ArcP, unsigned char *Key, const char *KeyV2String, size_t KeyV2StringBytes, char *KeyV2StringBuffer )
{
	char DirPath[MAX_PATH] ;
	
	// 現在のカレントディレクトリを保存
	GetCurrentDirectoryA( MAX_PATH, DirPath ) ;

	// ディレクトリ情報がある場合は、まず展開用のディレクトリを作成する
	if( Dir->DirectoryAddress != 0xffffffffffffffff && Dir->ParentDirectoryAddress != 0xffffffffffffffff )
	{
		DARC_FILEHEAD *DirFile ;
		
		// DARC_FILEHEAD のアドレスを取得
		DirFile = ( DARC_FILEHEAD * )( FileP + Dir->DirectoryAddress ) ;
		
		// ディレクトリの作成
		CreateDirectoryA( GetOriginalFileName( NameP + DirFile->NameAddress ), NULL ) ;
		
		// そのディレクトリにカレントディレクトリを移す
		SetCurrentDirectoryA( GetOriginalFileName( NameP + DirFile->NameAddress ) ) ;
	}

	// 展開処理開始
	{
		u32 i, FileHeadSize ;
		DARC_FILEHEAD *File ;
		size_t KeyV2StringBufferBytes ;
		unsigned char lKeyV2[ DXA_KEYV2_LENGTH ] ;

		// 格納されているファイルの数だけ繰り返す
		FileHeadSize = sizeof( DARC_FILEHEAD ) ;
		File = ( DARC_FILEHEAD * )( FileP + Dir->FileHeadAddress ) ;
		for( i = 0 ; i < Dir->FileHeadNum ; i ++, File = (DARC_FILEHEAD *)( (u8 *)File + FileHeadSize ) )
		{
			// ディレクトリかどうかで処理を分岐
			if( File->Attributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// ディレクトリの場合は再帰をかける
				DirectoryDecode( NameP, DirP, FileP, Head, ( DARC_DIRECTORY * )( DirP + File->DataAddress ), ArcP, Key, KeyV2String, KeyV2StringBytes, KeyV2StringBuffer ) ;
			}
			else
			{
				FILE *DestP ;
				void *Buffer ;
			
				// ファイルの場合は展開する
				
				// バッファを確保する
				Buffer = malloc( DXA_BUFFERSIZE ) ;
				if( Buffer == NULL ) return -1 ;

				// ファイルを開く
				DestP = fopen( GetOriginalFileName( NameP + File->NameAddress ), "wb" ) ;

				// ファイル個別の鍵を作成
				KeyV2StringBufferBytes = CreateKeyV2FileString( ( int )Head->CharCodeFormat, KeyV2String, KeyV2StringBytes, Dir, File, FileP, DirP, NameP, ( BYTE * )KeyV2StringBuffer ) ;
				KeyV2Create( KeyV2StringBuffer, lKeyV2 , KeyV2StringBufferBytes ) ;
				
				// データがある場合のみ転送
				if( File->DataSize != 0 )
				{
					// 初期位置をセットする
					if( _ftelli64( ArcP ) != ( s32 )( Head->DataStartAddress + File->DataAddress ) )
						_fseeki64( ArcP, Head->DataStartAddress + File->DataAddress, SEEK_SET ) ;
						
					// データが圧縮されているかどうかで処理を分岐
					if( File->PressDataSize != 0xffffffffffffffff )
					{
						void *temp ;
						
						// 圧縮されている場合

						// 圧縮データが収まるメモリ領域の確保
						temp = malloc( ( size_t )( File->PressDataSize + File->DataSize ) ) ;

						// 圧縮データの読み込み
						if( Head->Version >= DXA_KEYV2_VER )
						{
							KeyV2ConvFileRead( temp, File->PressDataSize, ArcP, lKeyV2, File->DataSize ) ;
						}
						else
						{
							KeyConvFileRead( temp, File->PressDataSize, ArcP, Key, File->DataSize ) ;
						}
						
						// 解凍
						Decode( temp, (u8 *)temp + File->PressDataSize ) ;
						
						// 書き出し
						fwrite64( (u8 *)temp + File->PressDataSize, File->DataSize, DestP ) ;
						
						// メモリの解放
						free( temp ) ;
					}
					else
					{
						// 圧縮されていない場合
					
						// 転送処理開始
						{
							u64 MoveSize, WriteSize ;
							
							WriteSize = 0 ;
							while( WriteSize < File->DataSize )
							{
								MoveSize = File->DataSize - WriteSize > DXA_BUFFERSIZE ? DXA_BUFFERSIZE : File->DataSize - WriteSize ;

								// ファイルの反転読み込み
								if( Head->Version >= DXA_KEYV2_VER )
								{
									KeyV2ConvFileRead( Buffer, MoveSize, ArcP, lKeyV2, File->DataSize + WriteSize ) ;
								}
								else
								{
									KeyConvFileRead( Buffer, MoveSize, ArcP, Key, File->DataSize + WriteSize ) ;
								}

								// 書き出し
								fwrite64( Buffer, MoveSize, DestP ) ;
								
								WriteSize += MoveSize ;
							}
						}
					}
				}
				
				// ファイルを閉じる
				fclose( DestP ) ;

				// バッファを開放する
				free( Buffer ) ;

				// ファイルのタイムスタンプを設定する
				{
					HANDLE HFile ;
					FILETIME CreateTime, LastAccessTime, LastWriteTime ;

					HFile = CreateFileA( GetOriginalFileName( NameP + File->NameAddress ),
										GENERIC_WRITE, 0, NULL,
										OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) ;
					if( HFile == INVALID_HANDLE_VALUE )
					{
						HFile = HFile ;
					}

					CreateTime.dwHighDateTime     = ( u32 )( File->Time.Create     >> 32        ) ;
					CreateTime.dwLowDateTime      = ( u32 )( File->Time.Create     & 0xffffffffffffffff ) ;
					LastAccessTime.dwHighDateTime = ( u32 )( File->Time.LastAccess >> 32        ) ;
					LastAccessTime.dwLowDateTime  = ( u32 )( File->Time.LastAccess & 0xffffffffffffffff ) ;
					LastWriteTime.dwHighDateTime  = ( u32 )( File->Time.LastWrite  >> 32        ) ;
					LastWriteTime.dwLowDateTime   = ( u32 )( File->Time.LastWrite  & 0xffffffffffffffff ) ;
					SetFileTime( HFile, &CreateTime, &LastAccessTime, &LastWriteTime ) ;
					CloseHandle( HFile ) ;
				}

				// ファイル属性を付ける
				SetFileAttributesA( GetOriginalFileName( NameP + File->NameAddress ), ( u32 )File->Attributes ) ;
			}
		}
	}
	
	// カレントディレクトリを元に戻す
	SetCurrentDirectoryA( DirPath ) ;

	// 終了
	return 0 ;
}

// ディレクトリ内のファイルパスを取得する
int DXArchive::GetDirectoryFilePath( const char *DirectoryPath, char *FileNameBuffer )
{
	WIN32_FIND_DATAA FindData ;
	HANDLE FindHandle ;
	int FileNum ;
	char DirPath[256], String[256] ;

	// ディレクトリかどうかをチェックする
	if( DirectoryPath[0] != '\0' )
	{
		FindHandle = FindFirstFileA( DirectoryPath, &FindData ) ;
		if( FindHandle == INVALID_HANDLE_VALUE || ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 ) return -1 ;
		FindClose( FindHandle ) ;
	}

	// 指定のフォルダのファイルの名前を取得する
	FileNum = 0 ;
	if( DirectoryPath[0] != '\0' )
	{
		strcpy( DirPath, DirectoryPath ) ;
		if( DirPath[ strlen( DirPath ) - 1 ] != '\\' ) strcat( DirPath, "\\" ) ;
		strcpy( String, DirPath ) ;
		strcat( String, "*" ) ;
	}
	else
	{
		strcpy( DirPath, "" ) ;
		strcpy( String, "*" ) ;
	}
	FindHandle = FindFirstFileA( String, &FindData ) ;
	if( FindHandle != INVALID_HANDLE_VALUE )
	{
		do
		{
			// 上のディレクトリに戻ったりするためのパスは無視する
			if( strcmp( FindData.cFileName, "." ) == 0 || strcmp( FindData.cFileName, ".." ) == 0 ) continue ;

			// ファイルパスを保存する
			if( FileNameBuffer != NULL )
			{
				strcpy( FileNameBuffer, DirPath ) ;
				strcat( FileNameBuffer, FindData.cFileName ) ;
				FileNameBuffer += 256 ;
			}

			// ファイルの数を増やす
			FileNum ++ ;
		}
		while( FindNextFileA( FindHandle, &FindData ) != 0 ) ;
		FindClose( FindHandle ) ;
	}

	// 数を返す
	return FileNum ;
}

// エンコード( 戻り値:圧縮後のサイズ  -1 はエラー  Dest に NULL を入れることも可能 )
int DXArchive::Encode( void *Src, u32 SrcSize, void *Dest )
{
	s32 dstsize ;
	s32    bonus,    conbo,    conbosize,    address,    addresssize ;
	s32 maxbonus, maxconbo, maxconbosize, maxaddress, maxaddresssize ;
	u8 keycode, *srcp, *destp, *dp, *sp, *sp2, *sp1 ;
	u32 srcaddress, nextprintaddress, code ;
	s32 j ;
	u32 i, m ;
	u32 maxlistnum, maxlistnummask, listaddp ;
	u32 sublistnum, sublistmaxnum ;
	LZ_LIST *listbuf, *listtemp, *list, *newlist ;
	u8 *listfirsttable, *usesublistflagtable, *sublistbuf ;
	
	// サブリストのサイズを決める
	{
			 if( SrcSize < 100 * 1024 )			sublistmaxnum = 1 ;
		else if( SrcSize < 3 * 1024 * 1024 )	sublistmaxnum = MAX_SUBLISTNUM / 3 ;
		else									sublistmaxnum = MAX_SUBLISTNUM ;
	}

	// リストのサイズを決める
	{
		maxlistnum = MAX_ADDRESSLISTNUM ;
		if( maxlistnum > SrcSize )
		{
			while( ( maxlistnum >> 1 ) > 0x100 && ( maxlistnum >> 1 ) > SrcSize )
				maxlistnum >>= 1 ;
		}
		maxlistnummask = maxlistnum - 1 ;
	}

	// メモリの確保
	usesublistflagtable   = (u8 *)malloc(
		sizeof( void * )  * 65536 +					// メインリストの先頭オブジェクト用領域
		sizeof( LZ_LIST ) * maxlistnum +			// メインリスト用領域
		sizeof( u8 )      * 65536 +					// サブリストを使用しているかフラグ用領域
		sizeof( void * )  * 256 * sublistmaxnum ) ;	// サブリスト用領域
		
	// アドレスのセット
	listfirsttable =     usesublistflagtable + sizeof(     u8 ) * 65536 ;
	sublistbuf     =          listfirsttable + sizeof( void * ) * 65536 ;
	listbuf        = (LZ_LIST *)( sublistbuf + sizeof( void * ) * 256 * sublistmaxnum ) ;
	
	// 初期化
	memset( usesublistflagtable, 0, sizeof(     u8 ) * 65536               ) ;
	memset(          sublistbuf, 0, sizeof( void * ) * 256 * sublistmaxnum ) ;
	memset(      listfirsttable, 0, sizeof( void * ) * 65536               ) ;
	list = listbuf ;
	for( i = maxlistnum / 8 ; i ; i --, list += 8 )
	{
		list[0].address =
		list[1].address =
		list[2].address =
		list[3].address =
		list[4].address =
		list[5].address =
		list[6].address =
		list[7].address = 0xffffffff ;
	}

	srcp  = (u8 *)Src ;
	destp = (u8 *)Dest ;

	// 圧縮元データの中で一番出現頻度が低いバイトコードを検索する
	{
		u32 qnum, table[256], mincode ;

		for( i = 0 ; i < 256 ; i ++ )
			table[i] = 0 ;
		
		sp   = srcp ;
		qnum = SrcSize / 8 ;
		i    = qnum * 8 ;
		for( ; qnum ; qnum --, sp += 8 )
		{
			table[sp[0]] ++ ;
			table[sp[1]] ++ ;
			table[sp[2]] ++ ;
			table[sp[3]] ++ ;
			table[sp[4]] ++ ;
			table[sp[5]] ++ ;
			table[sp[6]] ++ ;
			table[sp[7]] ++ ;
		}
		for( ; i < SrcSize ; i ++, sp ++ )
			table[*sp] ++ ;
			
		keycode = 0 ;
		mincode = table[0] ;
		for( i = 1 ; i < 256 ; i ++ )
		{
			if( mincode < table[i] ) continue ;
			mincode = table[i] ;
			keycode = (u8)i ;
		}
	}

	// 圧縮元のサイズをセット
	((u32 *)destp)[0] = SrcSize ;

	// キーコードをセット
	destp[8] = keycode ;

	// 圧縮処理
	dp               = destp + 9 ;
	sp               = srcp ;
	srcaddress       = 0 ;
	dstsize          = 0 ;
	listaddp         = 0 ;
	sublistnum       = 0 ;
	nextprintaddress = 1024 * 100 ;
	while( srcaddress < SrcSize )
	{
		// 残りサイズが最低圧縮サイズ以下の場合は圧縮処理をしない
		if( srcaddress + MIN_COMPRESS >= SrcSize ) goto NOENCODE ;

		// リストを取得
		code = *((u16 *)sp) ;
		list = (LZ_LIST *)( listfirsttable + code * sizeof( void * ) ) ;
		if( usesublistflagtable[code] == 1 )
		{
			list = (LZ_LIST *)( (void **)list->next + sp[2] ) ;
		}
		else
		{
			if( sublistnum < sublistmaxnum )
			{
				list->next = (LZ_LIST *)( sublistbuf + sizeof( void * ) * 256 * sublistnum ) ;
				list       = (LZ_LIST *)( (void **)list->next + sp[2] ) ;
			
				usesublistflagtable[code] = 1 ;
				sublistnum ++ ;
			}
		}

		// 一番一致長の長いコードを探す
		maxconbo   = -1 ;
		maxaddress = -1 ;
		maxbonus   = -1 ;
		for( m = 0, listtemp = list->next ; m < MAX_SEARCHLISTNUM && listtemp != NULL ; listtemp = listtemp->next, m ++ )
		{
			address = srcaddress - listtemp->address ;
			if( address >= MAX_POSITION )
			{
				if( listtemp->prev ) listtemp->prev->next = listtemp->next ;
				if( listtemp->next ) listtemp->next->prev = listtemp->prev ;
				listtemp->address = 0xffffffff ;
				continue ;
			}
			
			sp2 = &sp[-address] ;
			sp1 = sp ;
			if( srcaddress + MAX_COPYSIZE < SrcSize )
			{
				conbo = MAX_COPYSIZE / 4 ;
				while( conbo && *((u32 *)sp2) == *((u32 *)sp1) )
				{
					sp2 += 4 ;
					sp1 += 4 ;
					conbo -- ;
				}
				conbo = MAX_COPYSIZE - ( MAX_COPYSIZE / 4 - conbo ) * 4 ;

				while( conbo && *sp2 == *sp1 )
				{
					sp2 ++ ;
					sp1 ++ ;
					conbo -- ;
				}
				conbo = MAX_COPYSIZE - conbo ;
			}
			else
			{
				for( conbo = 0 ;
						conbo < MAX_COPYSIZE &&
						conbo + srcaddress < SrcSize &&
						sp[conbo - address] == sp[conbo] ;
							conbo ++ ){}
			}

			if( conbo >= 4 )
			{
				conbosize   = ( conbo - MIN_COMPRESS ) < 0x20 ? 0 : 1 ;
				addresssize = address < 0x100 ? 0 : ( address < 0x10000 ? 1 : 2 ) ;
				bonus       = conbo - ( 3 + conbosize + addresssize ) ;

				if( bonus > maxbonus )
				{
					maxconbo       = conbo ;
					maxaddress     = address ;
					maxaddresssize = addresssize ;
					maxconbosize   = conbosize ;
					maxbonus       = bonus ;
				}
			}
		}

		// リストに登録
		newlist = &listbuf[listaddp] ;
		if( newlist->address != 0xffffffff )
		{
			if( newlist->prev ) newlist->prev->next = newlist->next ;
			if( newlist->next ) newlist->next->prev = newlist->prev ;
			newlist->address = 0xffffffff ;
		}
		newlist->address = srcaddress ;
		newlist->prev    = list ;
		newlist->next    = list->next ;
		if( list->next != NULL ) list->next->prev = newlist ;
		list->next       = newlist ;
		listaddp         = ( listaddp + 1 ) & maxlistnummask ;

		// 一致コードが見つからなかったら非圧縮コードとして出力
		if( maxconbo == -1 )
		{
NOENCODE:
			// キーコードだった場合は２回連続で出力する
			if( *sp == keycode )
			{
				if( destp != NULL )
				{
					dp[0]  =
					dp[1]  = keycode ;
					dp += 2 ;
				}
				dstsize += 2 ;
			}
			else
			{
				if( destp != NULL )
				{
					*dp = *sp ;
					dp ++ ;
				}
				dstsize ++ ;
			}
			sp ++ ;
			srcaddress ++ ;
		}
		else
		{
			// 見つかった場合は見つけた位置と長さを出力する
			
			// キーコードと見つけた位置と長さを出力
			if( destp != NULL )
			{
				// キーコードの出力
				*dp++ = keycode ;

				// 出力する連続長は最低 MIN_COMPRESS あることが前提なので - MIN_COMPRESS したものを出力する
				maxconbo -= MIN_COMPRESS ;

				// 連続長０～４ビットと連続長、相対アドレスのビット長を出力
				*dp = (u8)( ( ( maxconbo & 0x1f ) << 3 ) | ( maxconbosize << 2 ) | maxaddresssize ) ;

				// キーコードの連続はキーコードと値の等しい非圧縮コードと
				// 判断するため、キーコードの値以上の場合は値を＋１する
				if( *dp >= keycode ) dp[0] += 1 ;
				dp ++ ;

				// 連続長５～１２ビットを出力
				if( maxconbosize == 1 )
					*dp++ = (u8)( ( maxconbo >> 5 ) & 0xff ) ;

				// maxconbo はまだ使うため - MIN_COMPRESS した分を戻す
				maxconbo += MIN_COMPRESS ;

				// 出力する相対アドレスは０が( 現在のアドレス－１ )を挿すので、－１したものを出力する
				maxaddress -- ;

				// 相対アドレスを出力
				*dp++ = (u8)( maxaddress ) ;
				if( maxaddresssize > 0 )
				{
					*dp++ = (u8)( maxaddress >> 8 ) ;
					if( maxaddresssize == 2 )
						*dp++ = (u8)( maxaddress >> 16 ) ;
				}
			}
			
			// 出力サイズを加算
			dstsize += 3 + maxaddresssize + maxconbosize ;
			
			// リストに情報を追加
			if( srcaddress + maxconbo < SrcSize )
			{
				sp2 = &sp[1] ;
				for( j = 1 ; j < maxconbo && (u64)&sp2[2] - (u64)srcp < SrcSize ; j ++, sp2 ++ )
				{
					code = *((u16 *)sp2) ;
					list = (LZ_LIST *)( listfirsttable + code * sizeof( void * ) ) ;
					if( usesublistflagtable[code] == 1 )
					{
						list = (LZ_LIST *)( (void **)list->next + sp2[2] ) ;
					}
					else
					{
						if( sublistnum < sublistmaxnum )
						{
							list->next = (LZ_LIST *)( sublistbuf + sizeof( void * ) * 256 * sublistnum ) ;
							list       = (LZ_LIST *)( (void **)list->next + sp2[2] ) ;
						
							usesublistflagtable[code] = 1 ;
							sublistnum ++ ;
						}
					}

					newlist = &listbuf[listaddp] ;
					if( newlist->address != 0xffffffff )
					{
						if( newlist->prev ) newlist->prev->next = newlist->next ;
						if( newlist->next ) newlist->next->prev = newlist->prev ;
						newlist->address = 0xffffffff ;
					}
					newlist->address = srcaddress + j ;
					newlist->prev = list ;
					newlist->next = list->next ;
					if( list->next != NULL ) list->next->prev = newlist ;
					list->next = newlist ;
					listaddp = ( listaddp + 1 ) & maxlistnummask ;
				}
			}
			
			sp         += maxconbo ;
			srcaddress += maxconbo ;
		}

		// 圧縮率の表示
		if( nextprintaddress < srcaddress )
		{
			nextprintaddress = srcaddress + 100 * 1024 ;
		}
	}

	// 圧縮後のデータサイズを保存する
	*((u32 *)&destp[4]) = dstsize + 9 ;

	// 確保したメモリの解放
	free( usesublistflagtable ) ;

	// データのサイズを返す
	return dstsize + 9 ;
}

// デコード( 戻り値:解凍後のサイズ  -1 はエラー  Dest に NULL を入れることも可能 )
int DXArchive::Decode( void *Src, void *Dest )
{
	u32 srcsize, destsize, code, indexsize, keycode, conbo, index ;
	u8 *srcp, *destp, *dp, *sp ;

	destp = (u8 *)Dest ;
	srcp  = (u8 *)Src ;
	
	// 解凍後のデータサイズを得る
	destsize = *((u32 *)&srcp[0]) ;

	// 圧縮データのサイズを得る
	srcsize = *((u32 *)&srcp[4]) - 9 ;

	// キーコード
	keycode = srcp[8] ;
	
	// 出力先がない場合はサイズだけ返す
	if( Dest == NULL )
		return destsize ;
	
	// 展開開始
	sp  = srcp + 9 ;
	dp  = destp ;
	while( srcsize )
	{
		// キーコードか同かで処理を分岐
		if( sp[0] != keycode )
		{
			// 非圧縮コードの場合はそのまま出力
			*dp = *sp ;
			dp      ++ ;
			sp      ++ ;
			srcsize -- ;
			continue ;
		}
	
		// キーコードが連続していた場合はキーコード自体を出力
		if( sp[1] == keycode )
		{
			*dp = (u8)keycode ;
			dp      ++ ;
			sp      += 2 ;
			srcsize -= 2 ;
			
			continue ;
		}

		// 第一バイトを得る
		code = sp[1] ;

		// もしキーコードよりも大きな値だった場合はキーコード
		// とのバッティング防止の為に＋１しているので－１する
		if( code > keycode ) code -- ;

		sp      += 2 ;
		srcsize -= 2 ;

		// 連続長を取得する
		conbo = code >> 3 ;
		if( code & ( 0x1 << 2 ) )
		{
			conbo |= *sp << 5 ;
			sp      ++ ;
			srcsize -- ;
		}
		conbo += MIN_COMPRESS ;	// 保存時に減算した最小圧縮バイト数を足す

		// 参照相対アドレスを取得する
		indexsize = code & 0x3 ;
		switch( indexsize )
		{
		case 0 :
			index = *sp ;
			sp      ++ ;
			srcsize -- ;
			break ;
			
		case 1 :
			index = *((u16 *)sp) ;
			sp      += 2 ;
			srcsize -= 2 ;
			break ;
			
		case 2 :
			index = *((u16 *)sp) | ( sp[2] << 16 ) ;
			sp      += 3 ;
			srcsize -= 3 ;
			break ;
		}
		index ++ ;		// 保存時に－１しているので＋１する

		// 展開
		if( index < conbo )
		{
			u32 num ;

			num  = index ;
			while( conbo > num )
			{
				memcpy( dp, dp - num, num ) ;
				dp    += num ;
				conbo -= num ;
				num   += num ;
			}
			if( conbo != 0 )
			{
				memcpy( dp, dp - num, conbo ) ;
				dp += conbo ;
			}
		}
		else
		{
			memcpy( dp, dp - index, conbo ) ;
			dp += conbo ;
		}
	}

	// 解凍後のサイズを返す
	return (int)destsize ;
}

// バイナリデータを元に SHA-256 のハッシュ値を計算する( DestBuffer の示すアドレスを先頭に 32byte ハッシュ値が書き込まれます )
#define RROT( a, n )		( u32 )( ( ( a ) >> ( n ) ) | ( u32 )( ( a ) << ( 32 - ( n ) ) ) )
#define S0( x )				( RROT( ( x ),  2 ) ^ RROT( ( x ), 13 ) ^ RROT( ( x ),22 ) )
#define S1( x )				( RROT( ( x ),  6 ) ^ RROT( ( x ), 11 ) ^ RROT( ( x ),25 ) )
#define s0( x )				( RROT( ( x ),  7 ) ^ RROT( ( x ), 18 ) ^ ( ( x ) >>  3 ) )
#define s1( x )				( RROT( ( x ), 17 ) ^ RROT( ( x ), 19 ) ^ ( ( x ) >> 10 ) )
#define CH( x, y, z )		( ( ( x ) & ( y ) ) ^ ( ( ~( x ) ) & ( z ) ) )
#define MAJ( x, y, z )		( ( ( x ) & ( y ) ) ^ ( ( x ) & ( z ) ) ^ ( ( y ) & ( z ) ) )

#define LIT_TO_BIG( sp, b )	( ( sp[ 0 + b ] << 24 ) | ( sp[ 1 + b ] << 16 ) | ( sp[ 2 + b ] << 8 ) | sp[ 3 + b ] )
#define W_CALC( i )			( W[ i - 16 ] + s0( W[ i - 15 ] ) + W[ i - 7 ] + s1( W[ i - 2 ] ) )

static u32 K[ 64 ] =
{
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
} ;

__inline void HashSha256_Calc( const u8 *Src, u32 *H )
{
	u32 i ;
	u32 X[ 8 ] ;
	u32 W[ 64 ] ;

	W[  0 ] = LIT_TO_BIG( Src, 4 *  0 ) ;		W[  1 ] = LIT_TO_BIG( Src, 4 *  1 ) ;
	W[  2 ] = LIT_TO_BIG( Src, 4 *  2 ) ;		W[  3 ] = LIT_TO_BIG( Src, 4 *  3 ) ;
	W[  4 ] = LIT_TO_BIG( Src, 4 *  4 ) ;		W[  5 ] = LIT_TO_BIG( Src, 4 *  5 ) ;
	W[  6 ] = LIT_TO_BIG( Src, 4 *  6 ) ;		W[  7 ] = LIT_TO_BIG( Src, 4 *  7 ) ;
	W[  8 ] = LIT_TO_BIG( Src, 4 *  8 ) ;		W[  9 ] = LIT_TO_BIG( Src, 4 *  9 ) ;
	W[ 10 ] = LIT_TO_BIG( Src, 4 * 10 ) ;		W[ 11 ] = LIT_TO_BIG( Src, 4 * 11 ) ;
	W[ 12 ] = LIT_TO_BIG( Src, 4 * 12 ) ;		W[ 13 ] = LIT_TO_BIG( Src, 4 * 13 ) ;
	W[ 14 ] = LIT_TO_BIG( Src, 4 * 14 ) ;		W[ 15 ] = LIT_TO_BIG( Src, 4 * 15 ) ;

	W[ 16 ] = W_CALC( 16 ) ;	W[ 17 ] = W_CALC( 17 ) ;	W[ 18 ] = W_CALC( 18 ) ;	W[ 19 ] = W_CALC( 19 ) ;
	W[ 20 ] = W_CALC( 20 ) ;	W[ 21 ] = W_CALC( 21 ) ;	W[ 22 ] = W_CALC( 22 ) ;	W[ 23 ] = W_CALC( 23 ) ;
	W[ 24 ] = W_CALC( 24 ) ;	W[ 25 ] = W_CALC( 25 ) ;	W[ 26 ] = W_CALC( 26 ) ;	W[ 27 ] = W_CALC( 27 ) ;
	W[ 28 ] = W_CALC( 28 ) ;	W[ 29 ] = W_CALC( 29 ) ;	W[ 30 ] = W_CALC( 30 ) ;	W[ 31 ] = W_CALC( 31 ) ;
	W[ 32 ] = W_CALC( 32 ) ;	W[ 33 ] = W_CALC( 33 ) ;	W[ 34 ] = W_CALC( 34 ) ;	W[ 35 ] = W_CALC( 35 ) ;
	W[ 36 ] = W_CALC( 36 ) ;	W[ 37 ] = W_CALC( 37 ) ;	W[ 38 ] = W_CALC( 38 ) ;	W[ 39 ] = W_CALC( 39 ) ;
	W[ 40 ] = W_CALC( 40 ) ;	W[ 41 ] = W_CALC( 41 ) ;	W[ 42 ] = W_CALC( 42 ) ;	W[ 43 ] = W_CALC( 43 ) ;
	W[ 44 ] = W_CALC( 44 ) ;	W[ 45 ] = W_CALC( 45 ) ;	W[ 46 ] = W_CALC( 46 ) ;	W[ 47 ] = W_CALC( 47 ) ;
	W[ 48 ] = W_CALC( 48 ) ;	W[ 49 ] = W_CALC( 49 ) ;	W[ 50 ] = W_CALC( 50 ) ;	W[ 51 ] = W_CALC( 51 ) ;
	W[ 52 ] = W_CALC( 52 ) ;	W[ 53 ] = W_CALC( 53 ) ;	W[ 54 ] = W_CALC( 54 ) ;	W[ 55 ] = W_CALC( 55 ) ;
	W[ 56 ] = W_CALC( 56 ) ;	W[ 57 ] = W_CALC( 57 ) ;	W[ 58 ] = W_CALC( 58 ) ;	W[ 59 ] = W_CALC( 59 ) ;
	W[ 60 ] = W_CALC( 60 ) ;	W[ 61 ] = W_CALC( 61 ) ;	W[ 62 ] = W_CALC( 62 ) ;	W[ 63 ] = W_CALC( 63 ) ;

	X[ 0 ] = H[ 0 ] ;
	X[ 1 ] = H[ 1 ] ;
	X[ 2 ] = H[ 2 ] ;
	X[ 3 ] = H[ 3 ] ;
	X[ 4 ] = H[ 4 ] ;
	X[ 5 ] = H[ 5 ] ;
	X[ 6 ] = H[ 6 ] ;
	X[ 7 ] = H[ 7 ] ;
#if 0
	for( i = 0 ; i < 64 ; i ++ )
	{
		u32 temp1 = X[ 7 ] + S1( X[ 4 ] ) + CH( X[ 4 ], X[ 5 ], X[ 6 ] ) + K[ i ] + W[ i ] ;
		u32 temp2 = S0( X[ 0 ] ) + MAJ( X[ 0 ], X[ 1 ], X[ 2 ] ) ;
		X[ 7 ] = X[ 6 ] ;
		X[ 6 ] = X[ 5 ] ;
		X[ 5 ] = X[ 4 ] ;
		X[ 4 ] = X[ 3 ] + temp1 ;
		X[ 3 ] = X[ 2 ] ;
		X[ 2 ] = X[ 1 ] ;
		X[ 1 ] = X[ 0 ] ;
		X[ 0 ] = temp1 + temp2 ;
	}
#else
	for( i = 0 ; i < 64 ; i += 8 )
	{
		u32 temp1 = X[ 7 ] + S1( X[ 4 ] ) + CH( X[ 4 ], X[ 5 ], X[ 6 ] ) + K[ i ] + W[ i ] ;
		u32 temp2 = S0( X[ 0 ] ) + MAJ( X[ 0 ], X[ 1 ], X[ 2 ] ) ;
//		X[ 7 ] = X[ 6 ] ;
//		X[ 6 ] = X[ 5 ] ;
//		X[ 5 ] = X[ 4 ] ;
//		X[ 4 ] = X[ 3 ] + temp1 ;
//		X[ 3 ] = X[ 2 ] ;
//		X[ 2 ] = X[ 1 ] ;
//		X[ 1 ] = X[ 0 ] ;
//		X[ 0 ] = temp1 + temp2 ;
		u32 temp3 = X[ 6 ] + S1( X[ 3 ] + temp1 ) + CH( X[ 3 ] + temp1, X[ 4 ], X[ 5 ] ) + K[ i + 1 ] + W[ i + 1 ] ;
		u32 temp4 = S0( temp1 + temp2 ) + MAJ( temp1 + temp2, X[ 0 ], X[ 1 ] ) ;
//		X[ 7 ] = X[ 5 ] ;
//		X[ 6 ] = X[ 4 ] ;
//		X[ 5 ] = X[ 3 ] + temp1 ;
//		X[ 4 ] = X[ 2 ] + temp3 ;
//		X[ 3 ] = X[ 1 ] ;
//		X[ 2 ] = X[ 0 ] ;
//		X[ 1 ] = temp1 + temp2 ;
//		X[ 0 ] = temp3 + temp4 ;
		u32 temp5 = X[ 5 ] + S1( X[ 2 ] + temp3 ) + CH( X[ 2 ] + temp3, X[ 3 ] + temp1, X[ 4 ] ) + K[ i + 2 ] + W[ i + 2 ] ;
		u32 temp6 = S0( temp3 + temp4 ) + MAJ( temp3 + temp4, temp1 + temp2, X[ 0 ] ) ;
//		X[ 7 ] = X[ 4 ] ;
//		X[ 6 ] = X[ 3 ] + temp1 ;
//		X[ 5 ] = X[ 2 ] + temp3 ;
//		X[ 4 ] = X[ 1 ] + temp5 ;
//		X[ 3 ] = X[ 0 ] ;
//		X[ 2 ] = temp1 + temp2 ;
//		X[ 1 ] = temp3 + temp4 ;
//		X[ 0 ] = temp5 + temp6 ;
		u32 temp7 = X[ 4 ] + S1( X[ 1 ] + temp5 ) + CH( X[ 1 ] + temp5, X[ 2 ] + temp3, X[ 3 ] + temp1 ) + K[ i + 3 ] + W[ i + 3 ] ;
		u32 temp8 = S0( temp5 + temp6 ) + MAJ( temp5 + temp6, temp3 + temp4, temp1 + temp2 ) ;
//		X[ 7 ] = X[ 3 ] + temp1 ;
//		X[ 6 ] = X[ 2 ] + temp3 ;
//		X[ 5 ] = X[ 1 ] + temp5 ;
//		X[ 4 ] = X[ 0 ] + temp7 ;
//		X[ 3 ] = temp1 + temp2 ;
//		X[ 2 ] = temp3 + temp4 ;
//		X[ 1 ] = temp5 + temp6 ;
//		X[ 0 ] = temp7 + temp8 ;
		u32 temp9 = X[ 3 ] + temp1 + S1( X[ 0 ] + temp7 ) + CH( X[ 0 ] + temp7, X[ 1 ] + temp5, X[ 2 ] + temp3 ) + K[ i + 4 ] + W[ i + 4 ] ;
		u32 temp10 = S0( temp7 + temp8 ) + MAJ( temp7 + temp8, temp5 + temp6, temp3 + temp4 ) ;
//		X[ 7 ] = X[ 2 ] + temp3 ;
//		X[ 6 ] = X[ 1 ] + temp5 ;
//		X[ 5 ] = X[ 0 ] + temp7 ;
//		X[ 4 ] = temp1 + temp2 + temp9 ;
//		X[ 3 ] = temp3 + temp4 ;
//		X[ 2 ] = temp5 + temp6 ;
//		X[ 1 ] = temp7 + temp8 ;
//		X[ 0 ] = temp9 + temp10 ;
		u32 temp11 = X[ 2 ] + temp3 + S1( temp1 + temp2 + temp9 ) + CH( temp1 + temp2 + temp9, X[ 0 ] + temp7, X[ 1 ] + temp5 ) + K[ i + 5 ] + W[ i + 5 ] ;
		u32 temp12 = S0( temp9 + temp10 ) + MAJ( temp9 + temp10, temp7 + temp8, temp5 + temp6 ) ;
//		X[ 7 ] = X[ 1 ] + temp5 ;
//		X[ 6 ] = X[ 0 ] + temp7 ;
//		X[ 5 ] = temp1 + temp2 + temp9 ;
//		X[ 4 ] = temp3 + temp4 + temp11 ;
//		X[ 3 ] = temp5 + temp6 ;
//		X[ 2 ] = temp7 + temp8 ;
//		X[ 1 ] = temp9 + temp10 ;
//		X[ 0 ] = temp11 + temp12 ;
		u32 temp13 = X[ 1 ] + temp5 + S1( temp3 + temp4 + temp11 ) + CH( temp3 + temp4 + temp11, temp1 + temp2 + temp9, X[ 0 ] + temp7 ) + K[ i + 6 ] + W[ i + 6 ] ;
		u32 temp14 = S0( temp11 + temp12 ) + MAJ( temp11 + temp12, temp9 + temp10, temp7 + temp8 ) ;
//		X[ 7 ] = X[ 0 ] + temp7 ;
//		X[ 6 ] = temp1 + temp2 + temp9 ;
//		X[ 5 ] = temp3 + temp4 + temp11 ;
//		X[ 4 ] = temp5 + temp6 + temp13 ;
//		X[ 3 ] = temp7 + temp8 ;
//		X[ 2 ] = temp9 + temp10 ;
//		X[ 1 ] = temp11 + temp12 ;
//		X[ 0 ] = temp13 + temp14 ;
		u32 temp15 = X[ 0 ] + temp7 + S1( temp5 + temp6 + temp13 ) + CH( temp5 + temp6 + temp13, temp3 + temp4 + temp11, temp1 + temp2 + temp9 ) + K[ i + 7 ] + W[ i + 7 ] ;
		u32 temp16 = S0( temp13 + temp14 ) + MAJ( temp13 + temp14, temp11 + temp12, temp9 + temp10 ) ;
		X[ 7 ] = temp1 + temp2 + temp9  ;
		X[ 6 ] = temp3 + temp4 + temp11 ;
		X[ 5 ] = temp5 + temp6 + temp13 ;
		X[ 4 ] = temp7 + temp8 + temp15 ;
		X[ 3 ] = temp9 + temp10  ;
		X[ 2 ] = temp11 + temp12 ;
		X[ 1 ] = temp13 + temp14 ;
		X[ 0 ] = temp15 + temp16 ;
	}
#endif
	H[ 0 ] += X[ 0 ] ;
	H[ 1 ] += X[ 1 ] ;
	H[ 2 ] += X[ 2 ] ;
	H[ 3 ] += X[ 3 ] ;
	H[ 4 ] += X[ 4 ] ;
	H[ 5 ] += X[ 5 ] ;
	H[ 6 ] += X[ 6 ] ;
	H[ 7 ] += X[ 7 ] ;
}

void DXArchive::HashSha256( const void *SrcData, size_t SrcDataSize, void *DestBuffer )
{
	u32 H[ 8 ] ;
	u8 Buffer[ 128 ] ;
	const u8 *sp = ( const u8 * )SrcData ;
	size_t i ;
	size_t FullBlockNum = SrcDataSize / 64 ;
	size_t DataSize ;
	size_t FillSize ;
	u64 BitLength = SrcDataSize * 8 ;

	H[ 0 ] = 0x6a09e667 ;
	H[ 1 ] = 0xbb67ae85 ;
	H[ 2 ] = 0x3c6ef372 ;
	H[ 3 ] = 0xa54ff53a ;
	H[ 4 ] = 0x510e527f ;
	H[ 5 ] = 0x9b05688c ;
	H[ 6 ] = 0x1f83d9ab ;
	H[ 7 ] = 0x5be0cd19 ;

	for( i = 0 ; i < FullBlockNum ; i ++ )
	{
		HashSha256_Calc( sp, H ) ;
		sp += 64 ;
	}

	DataSize = SrcDataSize - FullBlockNum * 64 ;
	for( i = 0 ; i < DataSize ; i ++ )
	{
		Buffer[ i ] = sp[ i ] ;
	}
	Buffer[ DataSize ] = 0x80 ;

	if( DataSize + 1 + 8 > 64 )
	{
		FillSize = ( 128 - 8 ) - ( DataSize + 1 ) ;
		for( i = 0 ; i < FillSize ; i ++ )
		{
			Buffer[ DataSize + 1 + i ] = 0 ;
		}

		Buffer[ 127 ] = ( ( u8 * )&BitLength )[ 0 ] ;
		Buffer[ 126 ] = ( ( u8 * )&BitLength )[ 1 ] ;
		Buffer[ 125 ] = ( ( u8 * )&BitLength )[ 2 ] ;
		Buffer[ 124 ] = ( ( u8 * )&BitLength )[ 3 ] ;
		Buffer[ 123 ] = ( ( u8 * )&BitLength )[ 4 ] ;
		Buffer[ 122 ] = ( ( u8 * )&BitLength )[ 5 ] ;
		Buffer[ 121 ] = ( ( u8 * )&BitLength )[ 6 ] ;
		Buffer[ 120 ] = ( ( u8 * )&BitLength )[ 7 ] ;

		HashSha256_Calc( &Buffer[  0 ], H ) ;
		HashSha256_Calc( &Buffer[ 64 ], H ) ;
	}
	else
	{
		FillSize = ( 64 - 8 ) - ( DataSize + 1 ) ;
		for( i = 0 ; i < FillSize ; i ++ )
		{
			Buffer[ DataSize + 1 + i ] = 0 ;
		}

		Buffer[ 63 ] = ( ( u8 * )&BitLength )[ 0 ] ;
		Buffer[ 62 ] = ( ( u8 * )&BitLength )[ 1 ] ;
		Buffer[ 61 ] = ( ( u8 * )&BitLength )[ 2 ] ;
		Buffer[ 60 ] = ( ( u8 * )&BitLength )[ 3 ] ;
		Buffer[ 59 ] = ( ( u8 * )&BitLength )[ 4 ] ;
		Buffer[ 58 ] = ( ( u8 * )&BitLength )[ 5 ] ;
		Buffer[ 57 ] = ( ( u8 * )&BitLength )[ 6 ] ;
		Buffer[ 56 ] = ( ( u8 * )&BitLength )[ 7 ] ;

		HashSha256_Calc( &Buffer[  0 ], H ) ;
	}

	( ( u8 * )DestBuffer )[  0 ] = ( ( u8 * )H )[ 3 ] ;
	( ( u8 * )DestBuffer )[  1 ] = ( ( u8 * )H )[ 2 ] ;
	( ( u8 * )DestBuffer )[  2 ] = ( ( u8 * )H )[ 1 ] ;
	( ( u8 * )DestBuffer )[  3 ] = ( ( u8 * )H )[ 0 ] ;
	( ( u8 * )DestBuffer )[  4 ] = ( ( u8 * )H )[ 7 ] ;
	( ( u8 * )DestBuffer )[  5 ] = ( ( u8 * )H )[ 6 ] ;
	( ( u8 * )DestBuffer )[  6 ] = ( ( u8 * )H )[ 5 ] ;
	( ( u8 * )DestBuffer )[  7 ] = ( ( u8 * )H )[ 4 ] ;
	( ( u8 * )DestBuffer )[  8 ] = ( ( u8 * )H )[ 11 ] ;
	( ( u8 * )DestBuffer )[  9 ] = ( ( u8 * )H )[ 10 ] ;
	( ( u8 * )DestBuffer )[ 10 ] = ( ( u8 * )H )[ 9 ] ;
	( ( u8 * )DestBuffer )[ 11 ] = ( ( u8 * )H )[ 8 ] ;
	( ( u8 * )DestBuffer )[ 12 ] = ( ( u8 * )H )[ 15 ] ;
	( ( u8 * )DestBuffer )[ 13 ] = ( ( u8 * )H )[ 14 ] ;
	( ( u8 * )DestBuffer )[ 14 ] = ( ( u8 * )H )[ 13 ] ;
	( ( u8 * )DestBuffer )[ 15 ] = ( ( u8 * )H )[ 12 ] ;
	( ( u8 * )DestBuffer )[ 16 ] = ( ( u8 * )H )[ 19 ] ;
	( ( u8 * )DestBuffer )[ 17 ] = ( ( u8 * )H )[ 18 ] ;
	( ( u8 * )DestBuffer )[ 18 ] = ( ( u8 * )H )[ 17 ] ;
	( ( u8 * )DestBuffer )[ 19 ] = ( ( u8 * )H )[ 16 ] ;
	( ( u8 * )DestBuffer )[ 20 ] = ( ( u8 * )H )[ 23 ] ;
	( ( u8 * )DestBuffer )[ 21 ] = ( ( u8 * )H )[ 22 ] ;
	( ( u8 * )DestBuffer )[ 22 ] = ( ( u8 * )H )[ 21 ] ;
	( ( u8 * )DestBuffer )[ 23 ] = ( ( u8 * )H )[ 20 ] ;
	( ( u8 * )DestBuffer )[ 24 ] = ( ( u8 * )H )[ 27 ] ;
	( ( u8 * )DestBuffer )[ 25 ] = ( ( u8 * )H )[ 26 ] ;
	( ( u8 * )DestBuffer )[ 26 ] = ( ( u8 * )H )[ 25 ] ;
	( ( u8 * )DestBuffer )[ 27 ] = ( ( u8 * )H )[ 24 ] ;
	( ( u8 * )DestBuffer )[ 28 ] = ( ( u8 * )H )[ 31 ] ;
	( ( u8 * )DestBuffer )[ 29 ] = ( ( u8 * )H )[ 30 ] ;
	( ( u8 * )DestBuffer )[ 30 ] = ( ( u8 * )H )[ 29 ] ;
	( ( u8 * )DestBuffer )[ 31 ] = ( ( u8 * )H )[ 28 ] ;
}


// アーカイブファイルを作成する(ディレクトリ一個だけ)
int DXArchive::EncodeArchiveOneDirectory( char *OutputFileName, char *DirectoryPath, bool Press, const char *KeyString )
{
	int i, FileNum, Result ;
	char **FilePathList, *NameBuffer ;

	// ファイルの数を取得する
	FileNum = GetDirectoryFilePath( DirectoryPath, NULL ) ;
	if( FileNum < 0 ) return -1 ;

	// ファイルの数の分だけファイル名とファイルポインタの格納用のメモリを確保する
	NameBuffer = (char *)malloc( FileNum * ( 256 + sizeof( char * ) ) ) ;

	// ファイルのパスを取得する
	GetDirectoryFilePath( DirectoryPath, NameBuffer ) ;

	// ファイルパスのリストを作成する
	FilePathList = (char **)( NameBuffer + FileNum * 256 ) ;
	for( i = 0 ; i < FileNum ; i ++ )
		FilePathList[i] = NameBuffer + i * 256 ;

	// エンコード
	Result = EncodeArchive( OutputFileName, FilePathList, FileNum, Press, KeyString ) ;

	// 確保したメモリの解放
	free( NameBuffer ) ;

	// 結果を返す
	return Result ;
}

// アーカイブファイルを作成する
int DXArchive::EncodeArchive( char *OutputFileName, char **FileOrDirectoryPath, int FileNum, bool Press, const char *KeyString )
{
	DARC_HEAD Head ;
	DARC_DIRECTORY Directory, *DirectoryP ;
	SIZESAVE SizeSave ;
	FILE *DestFp ;
	u8 *NameP, *FileP, *DirP ;
	int i ;
	u32 Type ;
	void *TempBuffer ;
	u8 KeyV2[DXA_KEYV2_LENGTH] ;
	char KeyV2String[DXA_KEYV2STR_LENGTH + 1] ;
	size_t KeyV2StringBytes ;
	char KeyV2StringBuffer[ DXA_KEYV2_STRING_MAXLENGTH ] ;

	// 鍵の作成
	KeyV2Create( KeyString, KeyV2 ) ;

	// 鍵文字列の保存
	if( KeyString == NULL )
	{
		KeyV2String[ 0 ] = '\0' ;
		KeyV2StringBytes = 0 ;
	}
	else
	{
		KeyV2StringBytes = CL_strlen( CHARCODEFORMAT_ASCII, KeyString ) ;
		if( KeyV2StringBytes > DXA_KEYV2STR_LENGTH )
		{
			KeyV2StringBytes = DXA_KEYV2STR_LENGTH ;
		}
		memcpy( KeyV2String, KeyString, KeyV2StringBytes ) ;
		KeyV2String[ KeyV2StringBytes ] = '\0' ;
	}

	// ファイル読み込みに使用するバッファの確保
	TempBuffer = malloc( DXA_BUFFERSIZE ) ;

	// 出力ファイルを開く
	DestFp = fopen( OutputFileName, "wb" ) ;

	// アーカイブのヘッダを出力する
	{
		Head.Head						= DXA_HEAD ;
		Head.Version					= DXA_VER ;
		Head.HeadSize					= 0xffffffff ;
		Head.DataStartAddress			= sizeof( DARC_HEAD ) ;
		Head.FileNameTableStartAddress	= 0xffffffffffffffff ;
		Head.DirectoryTableStartAddress	= 0xffffffffffffffff ;
		Head.FileTableStartAddress		= 0xffffffffffffffff ;
		Head.CharCodeFormat				= GetACP() ;
		SetFileApisToANSI() ;

		KeyV2ConvFileWrite( &Head, sizeof( DARC_HEAD ), DestFp, KeyV2, 0 ) ;
	}
	
	// 各バッファを確保する
	NameP = ( u8 * )malloc( DXA_BUFFERSIZE ) ;
	if( NameP == NULL ) return -1 ;
	memset( NameP, 0, DXA_BUFFERSIZE ) ;

	FileP = ( u8 * )malloc( DXA_BUFFERSIZE ) ;
	if( FileP == NULL ) return -1 ;
	memset( FileP, 0, DXA_BUFFERSIZE ) ;

	DirP = ( u8 * )malloc( DXA_BUFFERSIZE ) ;
	if( DirP == NULL ) return -1 ;
	memset( DirP, 0, DXA_BUFFERSIZE ) ;

	// サイズ保存構造体にデータをセット
	SizeSave.DataSize		= 0 ;
	SizeSave.NameSize		= 0 ;
	SizeSave.DirectorySize	= 0 ;
	SizeSave.FileSize		= 0 ;
	
	// 最初のディレクトリのファイル情報を書き出す
	{
		DARC_FILEHEAD File ;
		
		memset( &File, 0, sizeof( DARC_FILEHEAD ) ) ;
		File.NameAddress	= SizeSave.NameSize ;
		File.Attributes		= FILE_ATTRIBUTE_DIRECTORY ;
		File.DataAddress	= SizeSave.DirectorySize ;
		File.DataSize		= 0 ;
		File.PressDataSize  = 0xffffffffffffffff ;

		// ディレクトリ名の書き出し
		SizeSave.NameSize += AddFileNameData( ( int )Head.CharCodeFormat, "", NameP + SizeSave.NameSize ) ;

		// ファイル情報の書き出し
		memcpy( FileP + SizeSave.FileSize, &File, sizeof( DARC_FILEHEAD ) ) ;
		SizeSave.FileSize += sizeof( DARC_FILEHEAD ) ;
	}

	// 最初のディレクトリ情報を書き出す
	Directory.DirectoryAddress 			= 0 ;
	Directory.ParentDirectoryAddress 	= 0xffffffffffffffff ;
	Directory.FileHeadNum 				= FileNum ;
	Directory.FileHeadAddress 			= SizeSave.FileSize ;
	memcpy( DirP + SizeSave.DirectorySize, &Directory, sizeof( DARC_DIRECTORY ) ) ;
	DirectoryP = ( DARC_DIRECTORY * )( DirP + SizeSave.DirectorySize ) ;

	// サイズを加算する
	SizeSave.DirectorySize 	+= sizeof( DARC_DIRECTORY ) ;
	SizeSave.FileSize 		+= sizeof( DARC_FILEHEAD ) * FileNum ;

	// 渡されたファイルの数だけ処理を繰り返す
	for( i = 0 ; i < FileNum ; i ++ )
	{
		// 指定されたファイルがあるかどうか検査
		Type = GetFileAttributesA( FileOrDirectoryPath[i] ) ;
		if( ( signed int )Type == -1 ) continue ;

		// ファイルのタイプによって処理を分岐
		if( ( Type & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
		{
			// ディレクトリの場合はディレクトリのアーカイブに回す
			DirectoryEncode( ( int )Head.CharCodeFormat, FileOrDirectoryPath[i], NameP, DirP, FileP, &Directory, &SizeSave, i, DestFp, TempBuffer, Press, KeyV2String, KeyV2StringBytes, KeyV2StringBuffer ) ;
		}
		else
		{
			WIN32_FIND_DATAA FindData ;
			HANDLE FindHandle ;
			DARC_FILEHEAD File ;
			u8 lKeyV2[DXA_KEYV2_LENGTH] ;
			size_t KeyV2StringBufferBytes ;
	
			// ファイルの情報を得る
			FindHandle = FindFirstFileA( FileOrDirectoryPath[i], &FindData ) ;
			if( FindHandle == INVALID_HANDLE_VALUE ) continue ;
			
			// ファイルヘッダをセットする
			{
				File.NameAddress     = SizeSave.NameSize ;
				File.Time.Create     = ( ( ( LONGLONG )FindData.ftCreationTime.dwHighDateTime   ) << 32 ) + FindData.ftCreationTime.dwLowDateTime   ;
				File.Time.LastAccess = ( ( ( LONGLONG )FindData.ftLastAccessTime.dwHighDateTime ) << 32 ) + FindData.ftLastAccessTime.dwLowDateTime ;
				File.Time.LastWrite  = ( ( ( LONGLONG )FindData.ftLastWriteTime.dwHighDateTime  ) << 32 ) + FindData.ftLastWriteTime.dwLowDateTime  ;
				File.Attributes      = FindData.dwFileAttributes ;
				File.DataAddress     = SizeSave.DataSize ;
				File.DataSize        = ( ( ( LONGLONG )FindData.nFileSizeHigh ) << 32 ) + FindData.nFileSizeLow ;
				File.PressDataSize	 = 0xffffffffffffffff ;
			}

			// ファイル名を書き出す
			SizeSave.NameSize += AddFileNameData( ( int )Head.CharCodeFormat, FindData.cFileName, NameP + SizeSave.NameSize ) ;

			// ファイル個別の鍵を作成
			KeyV2StringBufferBytes = CreateKeyV2FileString( ( int )Head.CharCodeFormat, KeyV2String, KeyV2StringBytes, DirectoryP, &File, FileP, DirP, NameP, ( BYTE * )KeyV2StringBuffer ) ;
			KeyV2Create( KeyV2StringBuffer, lKeyV2 , KeyV2StringBufferBytes ) ;

			// ファイルデータを書き出す
			if( File.DataSize != 0 )
			{
				FILE *SrcP ;
				u64 FileSize, WriteSize, MoveSize ;

				// ファイルを開く
				SrcP = fopen( FileOrDirectoryPath[i], "rb" ) ;
				
				// サイズを得る
				_fseeki64( SrcP, 0, SEEK_END ) ;
				FileSize = _ftelli64( SrcP ) ;
				_fseeki64( SrcP, 0, SEEK_SET ) ;
				
				// ファイルサイズが 10MB 以下の場合で、圧縮の指定がある場合は圧縮を試みる
				if( Press == true && File.DataSize < 10 * 1024 * 1024 )
				{
					void *SrcBuf, *DestBuf ;
					u32 DestSize, Len ;
					
					// 一部のファイル形式の場合は予め弾く
					if( ( Len = ( int )strlen( FindData.cFileName ) ) > 4 )
					{
						char *sp ;
						
						sp = &FindData.cFileName[Len-3] ;
						if( StrICmp( sp, "wav" ) == 0 ||
							StrICmp( sp, "jpg" ) == 0 ||
							StrICmp( sp, "png" ) == 0 ||
							StrICmp( sp, "mpg" ) == 0 ||
							StrICmp( sp, "mp3" ) == 0 ||
							StrICmp( sp, "mp4" ) == 0 ||
							StrICmp( sp, "ogg" ) == 0 ||
							StrICmp( sp, "ogv" ) == 0 ||
							StrICmp( sp, "ops" ) == 0 ||
							StrICmp( sp, "wmv" ) == 0 ||
							StrICmp( sp - 1, "jpeg" ) == 0 ) goto NOPRESS ;
					}
					
					// データが丸ごと入るメモリ領域の確保
					SrcBuf  = malloc( ( size_t )( FileSize + FileSize * 2 + 64 ) ) ;
					DestBuf = (u8 *)SrcBuf + FileSize ;
					
					// ファイルを丸ごと読み込む
					fread64( SrcBuf, FileSize, SrcP ) ;
					
					// 圧縮
					DestSize = Encode( SrcBuf, ( u32 )FileSize, DestBuf ) ;
					
					// 殆ど圧縮出来なかった場合は圧縮無しでアーカイブする
					if( (f64)DestSize / (f64)FileSize > 0.90 )
					{
						_fseeki64( SrcP, 0L, SEEK_SET ) ;
						free( SrcBuf ) ;
						goto NOPRESS ;
					}
					
					// 圧縮データを反転して書き出す
					WriteSize = ( DestSize + 3 ) / 4 * 4 ;
					KeyV2ConvFileWrite( DestBuf, WriteSize, DestFp, lKeyV2, File.DataSize ) ;
					
					// メモリの解放
					free( SrcBuf ) ;
					
					// 圧縮データのサイズを保存する
					File.PressDataSize = DestSize ;
				}
				else
				{
NOPRESS:					
					// 転送開始
					WriteSize = 0 ;
					while( WriteSize < FileSize )
					{
						// 転送サイズ決定
						MoveSize = DXA_BUFFERSIZE < FileSize - WriteSize ? DXA_BUFFERSIZE : FileSize - WriteSize ;
						MoveSize = ( MoveSize + 3 ) / 4 * 4 ;	// サイズは４の倍数に合わせる
						
						// ファイルの反転読み込み
						KeyV2ConvFileRead( TempBuffer, MoveSize, SrcP, lKeyV2, File.DataSize + WriteSize ) ;

						// 書き出し
						fwrite64( TempBuffer, MoveSize, DestFp ) ;
						
						// 書き出しサイズの加算
						WriteSize += MoveSize ;
					}
				}
				
				// 書き出したファイルを閉じる
				fclose( SrcP ) ;
			
				// データサイズの加算
				SizeSave.DataSize += WriteSize ;
			}
			
			// ファイルヘッダを書き出す
			memcpy( FileP + Directory.FileHeadAddress + sizeof( DARC_FILEHEAD ) * i, &File, sizeof( DARC_FILEHEAD ) ) ;

			// Find ハンドルを閉じる
			FindClose( FindHandle ) ;
		}
	}
	
	// バッファに溜め込んだ各種ヘッダデータを出力する
	{
		// 出力する順番は ファイルネームテーブル、 DARC_FILEHEAD テーブル、 DARC_DIRECTORY テーブル の順
		KeyV2ConvFileWrite( NameP, SizeSave.NameSize,      DestFp, KeyV2, 0 ) ;
		KeyV2ConvFileWrite( FileP, SizeSave.FileSize,      DestFp, KeyV2, SizeSave.NameSize ) ;
		KeyV2ConvFileWrite( DirP,  SizeSave.DirectorySize, DestFp, KeyV2, SizeSave.NameSize + SizeSave.FileSize ) ;
	}
		
	// 再びアーカイブのヘッダを出力する
	{
		Head.Head                       = DXA_HEAD ;
		Head.Version                    = DXA_VER ;
		Head.HeadSize                   = ( u32 )( SizeSave.NameSize + SizeSave.DirectorySize + SizeSave.FileSize ) ;
		Head.DataStartAddress           = sizeof( DARC_HEAD ) ;
		Head.FileNameTableStartAddress  = SizeSave.DataSize + Head.DataStartAddress ;
		Head.FileTableStartAddress      = SizeSave.NameSize ;
		Head.DirectoryTableStartAddress = Head.FileTableStartAddress + SizeSave.FileSize ;

		_fseeki64( DestFp, 0, SEEK_SET ) ;
		KeyV2ConvFileWrite( &Head, sizeof( DARC_HEAD ), DestFp, KeyV2, 0 ) ;
	}
	
	// 書き出したファイルを閉じる
	fclose( DestFp ) ;
	
	// 確保したバッファを開放する
	free( NameP ) ;
	free( FileP ) ;
	free( DirP ) ;
	free( TempBuffer ) ;

	// 終了
	return 0 ;
}

// アーカイブファイルを展開する
int DXArchive::DecodeArchive( char *ArchiveName, char *OutputPath, const char *KeyString )
{
	u8 *HeadBuffer = NULL ;
	DARC_HEAD Head ;
	u8 *FileP, *NameP, *DirP ;
	FILE *ArcP = NULL ;
	char OldDir[MAX_PATH] ;
	u8 Key[DXA_KEYSTR_LENGTH] ;
	u8 KeyV2[DXA_KEYV2_LENGTH] ;
	char KeyV2String[DXA_KEYV2STR_LENGTH + 1] ;
	size_t KeyV2StringBytes ;
	char KeyV2StringBuffer[ DXA_KEYV2_STRING_MAXLENGTH ] ;

	// 鍵の作成
	KeyCreate( KeyString, Key ) ;
	KeyV2Create( KeyString, KeyV2 ) ;

	// 鍵文字列の保存
	if( KeyString == NULL )
	{
		KeyV2String[ 0 ] = '\0' ;
		KeyV2StringBytes = 0 ;
	}
	else
	{
		KeyV2StringBytes = CL_strlen( CHARCODEFORMAT_ASCII, KeyString ) ;
		if( KeyV2StringBytes > DXA_KEYV2STR_LENGTH )
		{
			KeyV2StringBytes = DXA_KEYV2STR_LENGTH ;
		}
		memcpy( KeyV2String, KeyString, KeyV2StringBytes ) ;
		KeyV2String[ KeyV2StringBytes ] = '\0' ;
	}

	// アーカイブファイルを開く
	ArcP = fopen( ArchiveName, "rb" ) ;
	if( ArcP == NULL ) return -1 ;

	// 出力先のディレクトリにカレントディレクトリを変更する
	GetCurrentDirectoryA( MAX_PATH, OldDir ) ;
	SetCurrentDirectoryA( OutputPath ) ;

	// ヘッダを解析する
	{
		KeyV2ConvFileRead( &Head, sizeof( DARC_HEAD ), ArcP, KeyV2, 0 ) ;
		
		// ＩＤの検査
		if( Head.Head != DXA_HEAD )
		{
			// バージョン６以前か調べる
			fseek( ArcP, 0L, SEEK_SET ) ;
			KeyConvFileRead( &Head, sizeof( DARC_HEAD ), ArcP, Key, 0 ) ;

			if( Head.Head != DXA_HEAD )
			{
				goto ERR ;
			}
		}
		
		// バージョン検査
		if( Head.Version > DXA_VER || Head.Version < 0x0006 ) goto ERR ;
		
		// ヘッダのサイズ分のメモリを確保する
		HeadBuffer = ( u8 * )malloc( ( size_t )Head.HeadSize ) ;
		if( HeadBuffer == NULL ) goto ERR ;
		
		// ヘッダパックをメモリに読み込む
		_fseeki64( ArcP, Head.FileNameTableStartAddress, SEEK_SET ) ;
		if( Head.Version >= DXA_KEYV2_VER )
		{
			KeyV2ConvFileRead( HeadBuffer, Head.HeadSize, ArcP, KeyV2, 0 ) ;
		}
		else
		{
			KeyConvFileRead( HeadBuffer, Head.HeadSize, ArcP, Key, 0 ) ;
		}
		
		// 各アドレスをセットする
		NameP = HeadBuffer ;
		FileP = NameP + Head.FileTableStartAddress ;
		DirP  = NameP + Head.DirectoryTableStartAddress ;
	}

	// アーカイブの展開を開始する
	DirectoryDecode( NameP, DirP, FileP, &Head, ( DARC_DIRECTORY * )DirP, ArcP, Key, KeyV2String, KeyV2StringBytes, KeyV2StringBuffer ) ;
	
	// ファイルを閉じる
	fclose( ArcP ) ;
	
	// ヘッダを読み込んでいたメモリを解放する
	free( HeadBuffer ) ;

	// カレントディレクトリを元に戻す
	SetCurrentDirectoryA( OldDir ) ;

	// 終了
	return 0 ;

ERR :
	if( HeadBuffer != NULL ) free( HeadBuffer ) ;
	if( ArcP != NULL ) fclose( ArcP ) ;

	// カレントディレクトリを元に戻す
	SetCurrentDirectoryA( OldDir ) ;

	// 終了
	return -1 ;
}



// コンストラクタ
DXArchive::DXArchive( char *ArchivePath )
{
	this->fp = NULL ;
	this->HeadBuffer = NULL ;
	this->NameP = this->DirP = this->FileP = NULL ;
	this->CurrentDirectory = NULL ;
	this->CashBuffer = NULL ;

	if( ArchivePath != NULL )
	{
		this->OpenArchiveFile( ArchivePath ) ;
	}
}

// デストラクタ
DXArchive::~DXArchive()
{
	if( this->fp != NULL ) this->CloseArchiveFile() ;

	if( this->fp != NULL ) fclose( this->fp ) ;
	if( this->HeadBuffer != NULL ) free( this->HeadBuffer ) ;
	if( this->CashBuffer != NULL ) free( this->CashBuffer ) ;

	this->fp = NULL ;
	this->HeadBuffer = NULL ;
	this->NameP = this->DirP = this->FileP = NULL ;
	this->CurrentDirectory = NULL ;
}

// 指定のディレクトリデータの暗号化を解除する( 丸ごとメモリに読み込んだ場合用 )
int DXArchive::DirectoryKeyConv( DARC_DIRECTORY *Dir, char *KeyV2StringBuffer )
{
	// メモリイメージではない場合はエラー
	if( this->MemoryOpenFlag == false )
		return -1 ;

	// バージョン 0x0005 より前では何もしない
	if( this->Head.Version < 0x0005 )
		return 0 ;
	
	// 暗号化解除処理開始
	{
		u32 i, FileHeadSize ;
		DARC_FILEHEAD *File ;
		unsigned char lKeyV2[DXA_KEYV2_LENGTH] ;
		size_t KeyV2StringBufferBytes ;

		// 格納されているファイルの数だけ繰り返す
		FileHeadSize = sizeof( DARC_FILEHEAD ) ;
		File = ( DARC_FILEHEAD * )( this->FileP + Dir->FileHeadAddress ) ;
		for( i = 0 ; i < Dir->FileHeadNum ; i ++, File = (DARC_FILEHEAD *)( (u8 *)File + FileHeadSize ) )
		{
			// ディレクトリかどうかで処理を分岐
			if( File->Attributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// ディレクトリの場合は再帰をかける
				DirectoryKeyConv( ( DARC_DIRECTORY * )( this->DirP + File->DataAddress ), KeyV2StringBuffer ) ;
			}
			else
			{
				u8 *DataP ;

				// ファイルの場合は暗号化を解除する
				
				// データがある場合のみ処理
				if( File->DataSize != 0 )
				{
					// データ位置をセットする
					DataP = ( u8 * )this->fp + this->Head.DataStartAddress + File->DataAddress ;

					// データが圧縮されているかどうかで処理を分岐
					if( File->PressDataSize != 0xffffffffffffffff )
					{
						// 圧縮されている場合
						if( this->Head.Version >= DXA_KEYV2_VER )
						{
							// ファイル個別の鍵を作成
							KeyV2StringBufferBytes = CreateKeyV2FileString( ( int )this->Head.CharCodeFormat, this->KeyV2String, this->KeyV2StringBytes, Dir, File, this->FileP, this->DirP, this->NameP, ( BYTE * )KeyV2StringBuffer ) ;
							KeyV2Create( KeyV2StringBuffer, lKeyV2 , KeyV2StringBufferBytes ) ;

							KeyV2Conv( DataP, File->PressDataSize, File->DataSize, lKeyV2 ) ;
						}
						else
						{
							KeyConv( DataP, File->PressDataSize, File->DataSize, this->Key ) ;
						}
					}
					else
					{
						// 圧縮されていない場合
						if( this->Head.Version >= DXA_KEYV2_VER )
						{
							// ファイル個別の鍵を作成
							KeyV2StringBufferBytes = CreateKeyV2FileString( ( int )this->Head.CharCodeFormat, this->KeyV2String, this->KeyV2StringBytes, Dir, File, this->FileP, this->DirP, this->NameP, ( BYTE * )KeyV2StringBuffer ) ;
							KeyV2Create( KeyV2StringBuffer, lKeyV2 , KeyV2StringBufferBytes ) ;

							KeyV2Conv( DataP, File->DataSize, File->DataSize, lKeyV2 ) ;
						}
						else
						{
							KeyConv( DataP, File->DataSize, File->DataSize, this->Key ) ;
						}
					}
				}
			}
		}
	}

	// 終了
	return 0 ;
}

// メモリ上にあるアーカイブイメージを開く( 0:成功  -1:失敗 )
int	DXArchive::OpenArchiveMem( void *ArchiveImage, s64 ArchiveSize, const char *KeyString )
{
	u8 *datp ;

	// 既になんらかのアーカイブを開いていた場合はエラー
	if( this->fp != NULL ) return -1 ;

	// 鍵の作成
	KeyCreate( KeyString, this->Key ) ;
	KeyV2Create( KeyString, this->KeyV2 ) ;

	// 鍵文字列の保存
	if( KeyString == NULL )
	{
		KeyV2String[ 0 ] = '\0' ;
		KeyV2StringBytes = 0 ;
	}
	else
	{
		KeyV2StringBytes = CL_strlen( CHARCODEFORMAT_ASCII, KeyString ) ;
		if( KeyV2StringBytes > DXA_KEYV2STR_LENGTH )
		{
			KeyV2StringBytes = DXA_KEYV2STR_LENGTH ;
		}
		memcpy( KeyV2String, KeyString, KeyV2StringBytes ) ;
		KeyV2String[ KeyV2StringBytes ] = '\0' ;
	}

	// 最初にヘッダの部分を反転する
	memcpy( &this->Head, ArchiveImage, sizeof( DARC_HEAD ) ) ;
	KeyV2Conv( &this->Head, sizeof( DARC_HEAD ), 0, this->KeyV2 ) ;

	// ＩＤが違う場合はエラー
	if( Head.Head != DXA_HEAD )
	{
		// バージョン６か調べる
		memcpy( &this->Head, ArchiveImage, sizeof( DARC_HEAD ) ) ;
		KeyConv( &this->Head, sizeof( DARC_HEAD ), 0, this->Key ) ;

		// ＩＤが違う場合はエラー
		if( Head.Head != DXA_HEAD )
		{
			goto ERR ;
		}
	}

	// ポインタを保存
	this->fp = (FILE *)ArchiveImage ;
	datp = (u8 *)ArchiveImage ;

	// ヘッダを解析する
	{
		datp += sizeof( DARC_HEAD ) ;
		
		// バージョン検査
		if( this->Head.Version > DXA_VER || this->Head.Version < 0x0006 ) goto ERR ;

		// ヘッダパックのアドレスをセットする
		this->HeadBuffer = (u8 *)this->fp + this->Head.FileNameTableStartAddress ;

		// 各アドレスをセットする
		if( this->Head.Version >= DXA_KEYV2_VER )
		{
			KeyV2Conv( this->HeadBuffer, this->Head.HeadSize, 0, this->KeyV2 ) ;
		}
		else
		{
			KeyConv( this->HeadBuffer, this->Head.HeadSize, 0, this->Key ) ;
		}
		this->NameP = this->HeadBuffer ;
		this->FileP = this->NameP + this->Head.FileTableStartAddress ;
		this->DirP = this->NameP + this->Head.DirectoryTableStartAddress ;
	}

	// カレントディレクトリのセット
	this->CurrentDirectory = ( DARC_DIRECTORY * )this->DirP ;

	// メモリイメージから開いているフラグを立てる
	MemoryOpenFlag = true ;

	// 全てのファイルの暗号化を解除する
	{
		char KeyV2StringBuffer[ DXA_KEYV2_STRING_MAXLENGTH ] ;
		DirectoryKeyConv( ( DARC_DIRECTORY * )this->DirP, KeyV2StringBuffer ) ;
	}

	// ユーザーのイメージから開いたフラグを立てる
	UserMemoryImageFlag = true ;

	// サイズも保存しておく
	MemoryImageSize = ArchiveSize ;

	// 終了
	return 0 ;

ERR :
	// 終了
	return -1 ;
}

// アーカイブファイルを開き最初にすべてメモリ上に読み込んでから処理する( 0:成功  -1:失敗 )
int DXArchive::OpenArchiveFileMem( const char *ArchivePath, const char *KeyString )
{
	FILE *fp ;
	u8 *datp ;
	void *ArchiveImage ;
	s64 ArchiveSize ;

	// 既になんらかのアーカイブを開いていた場合はエラー
	if( this->fp != NULL ) return -1 ;

	// 鍵の作成
	KeyCreate( KeyString, this->Key ) ;
	KeyV2Create( KeyString, this->KeyV2 ) ;

	// 鍵文字列の保存
	if( KeyString == NULL )
	{
		KeyV2String[ 0 ] = '\0' ;
		KeyV2StringBytes = 0 ;
	}
	else
	{
		KeyV2StringBytes = CL_strlen( CHARCODEFORMAT_ASCII, KeyString ) ;
		if( KeyV2StringBytes > DXA_KEYV2STR_LENGTH )
		{
			KeyV2StringBytes = DXA_KEYV2STR_LENGTH ;
		}
		memcpy( KeyV2String, KeyString, KeyV2StringBytes ) ;
		KeyV2String[ KeyV2StringBytes ] = '\0' ;
	}

	// メモリに読み込む
	{
		fp = fopen( ArchivePath, "rb" ) ;
		if( fp == NULL ) return -1 ;
		_fseeki64( fp, 0L, SEEK_END ) ;
		ArchiveSize = _ftelli64( fp ) ;
		_fseeki64( fp, 0L, SEEK_SET ) ;
		ArchiveImage = malloc( ( size_t )ArchiveSize ) ;
		if( ArchiveImage == NULL )
		{
			fclose( fp ) ;
			return -1 ;
		}
		fread64( ArchiveImage, ArchiveSize, fp ) ;
		fclose( fp ) ;
	}

	// 最初にヘッダの部分を反転する
	memcpy( &this->Head, ArchiveImage, sizeof( DARC_HEAD ) ) ;
	KeyV2Conv( &this->Head, sizeof( DARC_HEAD ), 0, this->KeyV2 ) ;

	// ＩＤが違う場合はエラー
	if( Head.Head != DXA_HEAD )
	{
		// バージョン６か調べる
		memcpy( &this->Head, ArchiveImage, sizeof( DARC_HEAD ) ) ;
		KeyConv( &this->Head, sizeof( DARC_HEAD ), 0, this->Key ) ;

		// ＩＤが違う場合はエラー
		if( Head.Head != DXA_HEAD )
			return -1 ;
	}

	// ポインタを保存
	this->fp = (FILE *)ArchiveImage ;
	datp = (u8 *)ArchiveImage ;

	// ヘッダを解析する
	{
		datp += sizeof( DARC_HEAD ) ;
		
		// バージョン検査
		if( this->Head.Version > DXA_VER || this->Head.Version < 0x0006 ) goto ERR ;

		// ヘッダパックのアドレスをセットする
		this->HeadBuffer = (u8 *)this->fp + this->Head.FileNameTableStartAddress ;

		// 各アドレスをセットする
		if( this->Head.Version >= DXA_KEYV2_VER )
		{
			KeyV2Conv( this->HeadBuffer, this->Head.HeadSize, 0, this->KeyV2 ) ;
		}
		else
		{
			KeyConv( this->HeadBuffer, this->Head.HeadSize, 0, this->Key ) ;
		}
		this->NameP = this->HeadBuffer ;
		this->FileP = this->NameP + this->Head.FileTableStartAddress ;
		this->DirP = this->NameP + this->Head.DirectoryTableStartAddress ;
	}

	// カレントディレクトリのセット
	this->CurrentDirectory = ( DARC_DIRECTORY * )this->DirP ;

	// メモリイメージから開いているフラグを立てる
	MemoryOpenFlag = true ;

	// 全てのファイルの暗号化を解除する
	{
		char KeyV2StringBuffer[ DXA_KEYV2_STRING_MAXLENGTH ] ;
		DirectoryKeyConv( ( DARC_DIRECTORY * )this->DirP, KeyV2StringBuffer ) ;
	}
	
	// ユーザーのイメージから開いたわけではないのでフラグを倒す
	UserMemoryImageFlag = false ;

	// サイズも保存しておく
	MemoryImageSize = ArchiveSize ;

	// 終了
	return 0 ;

ERR :
	
	// 終了
	return -1 ;
}

// アーカイブファイルを開く
int DXArchive::OpenArchiveFile( const char *ArchivePath, const char *KeyString )
{
	// 既になんらかのアーカイブを開いていた場合はエラー
	if( this->fp != NULL ) return -1 ;

	// アーカイブファイルを開こうと試みる
	this->fp = fopen( ArchivePath, "rb" ) ;
	if( this->fp == NULL ) return -1 ;

	// 鍵文字列の作成
	KeyCreate( KeyString, this->Key ) ;
	KeyV2Create( KeyString, this->KeyV2 ) ;

	// 鍵文字列の保存
	if( KeyString == NULL )
	{
		KeyV2String[ 0 ] = '\0' ;
		KeyV2StringBytes = 0 ;
	}
	else
	{
		KeyV2StringBytes = CL_strlen( CHARCODEFORMAT_ASCII, KeyString ) ;
		if( KeyV2StringBytes > DXA_KEYV2STR_LENGTH )
		{
			KeyV2StringBytes = DXA_KEYV2STR_LENGTH ;
		}
		memcpy( KeyV2String, KeyString, KeyV2StringBytes ) ;
		KeyV2String[ KeyV2StringBytes ] = '\0' ;
	}

	// ヘッダを解析する
	{
		KeyV2ConvFileRead( &this->Head, sizeof( DARC_HEAD ), this->fp, this->KeyV2, 0 ) ;
		
		// ＩＤの検査
		if( this->Head.Head != DXA_HEAD )
		{
			// バージョン６以前か調べる
			fseek( this->fp, 0L, SEEK_SET ) ;
			KeyConvFileRead( &this->Head, sizeof( DARC_HEAD ), this->fp, this->Key, 0 ) ;

			if( this->Head.Head != DXA_HEAD )
			{
				goto ERR ;
			}
		}
		
		// バージョン検査
		if( this->Head.Version > DXA_VER || this->Head.Version < 0x0006 ) goto ERR ;
		
		// ヘッダのサイズ分のメモリを確保する
		this->HeadBuffer = ( u8 * )malloc( ( size_t )this->Head.HeadSize ) ;
		if( this->HeadBuffer == NULL ) goto ERR ;
		
		// ヘッダパックをメモリに読み込む
		_fseeki64( this->fp, this->Head.FileNameTableStartAddress, SEEK_SET ) ;
		if( this->Head.Version >= DXA_KEYV2_VER )
		{
			KeyV2ConvFileRead( this->HeadBuffer, this->Head.HeadSize, this->fp, this->KeyV2, 0 ) ;
		}
		else
		{
			KeyConvFileRead( this->HeadBuffer, this->Head.HeadSize, this->fp, this->Key, 0 ) ;
		}
		
		// 各アドレスをセットする
		this->NameP = this->HeadBuffer ;
		this->FileP = this->NameP + this->Head.FileTableStartAddress ;
		this->DirP = this->NameP + this->Head.DirectoryTableStartAddress ;
	}

	// カレントディレクトリのセット
	this->CurrentDirectory = ( DARC_DIRECTORY * )this->DirP ;

	// メモリイメージから開いている、フラグを倒す
	MemoryOpenFlag = false ;

	// 終了
	return 0 ;

ERR :
	if( this->fp != NULL ){ fclose( this->fp ) ; this->fp = NULL ; }
	if( this->HeadBuffer != NULL ){ free( this->HeadBuffer ) ; this->HeadBuffer = NULL ; }
	
	// 終了
	return -1 ;
}

// アーカイブファイルを閉じる
int DXArchive::CloseArchiveFile( void )
{
	// 既に閉じていたら何もせず終了
	if( this->fp == NULL ) return 0 ;

	// メモリから開いているかどうかで処理を分岐
	if( MemoryOpenFlag == true )
	{
		// アーカイブクラスが読み込んだ場合とそうでない場合で処理を分岐
		if( UserMemoryImageFlag == true )
		{
			char KeyV2StringBuffer[ DXA_KEYV2_STRING_MAXLENGTH ] ;

			// 反転したデータを元に戻す
			DirectoryKeyConv( ( DARC_DIRECTORY * )this->DirP, KeyV2StringBuffer ) ;
			if( this->Head.Version >= DXA_KEYV2_VER )
			{
				KeyV2Conv( this->HeadBuffer, this->Head.HeadSize, 0, this->KeyV2 ) ;
			}
			else
			{
				KeyConv( this->HeadBuffer, this->Head.HeadSize, 0, this->Key ) ;
			}
		}
		else
		{
			// 確保していたメモリを開放する
			free( this->fp ) ;
		}
	}
	else
	{
		// アーカイブファイルを閉じる
		fclose( this->fp ) ;
		
		// ヘッダバッファも解放
		free( this->HeadBuffer ) ;
	}

	// ポインタ初期化
	this->fp = NULL ;
	this->HeadBuffer = NULL ;
	this->NameP = this->DirP = this->FileP = NULL ;
	this->CurrentDirectory = NULL ;

	// 終了
	return 0 ;
}

// アーカイブ内のディレクトリパスを変更する( 0:成功  -1:失敗 )
int	DXArchive::ChangeCurrentDirectoryFast( SEARCHDATA *SearchData )
{
	DARC_FILEHEAD *FileH ;
	int i, j, k, Num ;
	u8 *NameData, *PathData ;
	u16 PackNum, Parity ;

	PackNum  = SearchData->PackNum ;
	Parity   = SearchData->Parity ;
	PathData = SearchData->FileName ;

	// カレントディレクトリから同名のディレクトリを探す
	FileH = ( DARC_FILEHEAD * )( this->FileP + this->CurrentDirectory->FileHeadAddress ) ;
	Num = (s32)this->CurrentDirectory->FileHeadNum ;
	for( i = 0 ; i < Num ; i ++, FileH ++ )
	{
		// ディレクトリチェック
		if( ( FileH->Attributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 ) continue ;

		// 文字列数とパリティチェック
		NameData = this->NameP + FileH->NameAddress ;
		if( PackNum != ((u16 *)NameData)[0] || Parity != ((u16 *)NameData)[1] ) continue ;

		// 文字列チェック
		NameData += 4 ;
		for( j = 0, k = 0 ; j < PackNum ; j ++, k += 4 )
			if( *((u32 *)&PathData[k]) != *((u32 *)&NameData[k]) ) break ;

		// 適合したディレクトリがあったらここで終了
		if( PackNum == j ) break ;
	}

	// 無かったらエラー
	if( i == Num ) return -1 ;

	// 在ったらカレントディレクトリを変更
	this->CurrentDirectory = ( DARC_DIRECTORY * )( this->DirP + FileH->DataAddress ) ;

	// 正常終了
	return 0 ;
}

// アーカイブ内のディレクトリパスを変更する( 0:成功  -1:失敗 )
int DXArchive::ChangeCurrentDir( const char *DirPath )
{
	return ChangeCurrentDirectoryBase( DirPath, true ) ;
}

// アーカイブ内のディレクトリパスを変更する( 0:成功  -1:失敗 )
int	DXArchive::ChangeCurrentDirectoryBase( const char *DirectoryPath, bool ErrorIsDirectoryReset, SEARCHDATA *LastSearchData )
{
	DARC_DIRECTORY *OldDir ;
	SEARCHDATA SearchData ;

	// ここに留まるパスだったら無視
	if( strcmp( DirectoryPath, "." ) == 0 ) return 0 ;

	// 『\』だけの場合はルートディレクトリに戻る
	if( strcmp( DirectoryPath, "\\" ) == 0 )
	{
		this->CurrentDirectory = ( DARC_DIRECTORY * )this->DirP ;
		return 0 ;
	}

	// 下に一つ下がるパスだったら処理を分岐
	if( strcmp( DirectoryPath, ".." ) == 0 )
	{
		// ルートディレクトリに居たらエラー
		if( this->CurrentDirectory->ParentDirectoryAddress == 0xffffffffffffffff ) return -1 ;
		
		// 親ディレクトリがあったらそちらに移る
		this->CurrentDirectory = ( DARC_DIRECTORY * )( this->DirP + this->CurrentDirectory->ParentDirectoryAddress ) ;
		return 0 ;
	}

	// それ以外の場合は指定の名前のディレクトリを探す
	
	// 変更以前のディレクトリを保存しておく
	OldDir = this->CurrentDirectory ;

	// パス中に『\』があるかどうかで処理を分岐
	if( strchr( DirectoryPath, '\\' ) == NULL )
	{
		// ファイル名を検索専用の形式に変換する
		ConvSearchData( &SearchData, DirectoryPath, NULL ) ;

		// ディレクトリを変更
		if( ChangeCurrentDirectoryFast( &SearchData ) < 0 ) goto ERR ;

/*		// \ が無い場合は、同名のディレクトリを探す
		FileH = ( DARC_FILEHEAD * )( this->FileP + this->CurrentDirectory->FileHeadAddress ) ;
		for( i = 0 ;
			 i < (s32)this->CurrentDirectory->FileHeadNum &&
			 StrICmp( ( char * )( this->NameP + FileH->NameAddress ), DirectoryPath ) != 0 ;
			 i ++, FileH ++ ){}
*/
	}
	else
	{
		// \ がある場合は繋がったディレクトリを一つづつ変更してゆく
	
		int Point, StrLength ;

		Point = 0 ;
		// ループ
		for(;;)
		{
			// 文字列を取得する
			ConvSearchData( &SearchData, &DirectoryPath[Point], &StrLength ) ;
			Point += StrLength ;
/*			StrPoint = 0 ;
			while( DirectoryPath[Point] != '\0' && DirectoryPath[Point] != '\\' )
			{
				if( CheckMultiByteChar( &DirectoryPath[Point] ) == TRUE )
				{
					*((u16 *)&String[StrPoint]) = *((u16 *)&DirectoryPath[Point]) ;
					StrPoint += 2 ;
					Point    += 2 ;
				}
				else
				{
					String[StrPoint] = DirectoryPath[Point] ;
					StrPoint ++ ;
					Point    ++ ;
				}
			}
			String[StrPoint] = '\0' ;
*/
			// もし初っ端が \ だった場合はルートディレクトリに落とす
			if( StrLength == 0 && DirectoryPath[Point] == '\\' )
			{
				this->ChangeCurrentDirectoryBase( "\\", false ) ;
			}
			else
			{
				// それ以外の場合は普通にディレクトリ変更
				if( this->ChangeCurrentDirectoryFast( &SearchData ) < 0 )
				{
					// エラーが起きて、更にエラーが起きた時に元のディレクトリに戻せの
					// フラグが立っている場合は元のディレクトリに戻す
					if( ErrorIsDirectoryReset == true ) this->CurrentDirectory = OldDir ;

					// エラー終了
					goto ERR ;
				}
			}

			// もし終端文字で終了した場合はループから抜ける
			// 又はあと \ しかない場合もループから抜ける
			if( DirectoryPath[Point] == '\0' ||
				( DirectoryPath[Point] == '\\' && DirectoryPath[Point+1] == '\0' ) ) break ;
			Point ++ ;
		}
	}

	if( LastSearchData != NULL )
	{
		memcpy( LastSearchData->FileName, SearchData.FileName, SearchData.PackNum * 4 ) ;
		LastSearchData->Parity  = SearchData.Parity ;
		LastSearchData->PackNum = SearchData.PackNum ;
	}

	// 正常終了
	return 0 ;

ERR:
	if( LastSearchData != NULL )
	{
		memcpy( LastSearchData->FileName, SearchData.FileName, SearchData.PackNum * 4 ) ;
		LastSearchData->Parity  = SearchData.Parity ;
		LastSearchData->PackNum = SearchData.PackNum ;
	}

	// エラー終了
	return -1 ;
}
		
// アーカイブ内のカレントディレクトリパスを取得する
int DXArchive::GetCurrentDir( char *DirPathBuffer, int BufferLength )
{
	char DirPath[MAX_PATH] ;
	DARC_DIRECTORY *Dir[200], *DirTempP ;
	int Depth, i ;

	// ルートディレクトリに着くまで検索する
	Depth = 0 ;
	DirTempP = this->CurrentDirectory ;
	while( DirTempP->DirectoryAddress != 0xffffffffffffffff && DirTempP->DirectoryAddress != 0 )
	{
		Dir[Depth] = DirTempP ;
		DirTempP = ( DARC_DIRECTORY * )( this->DirP + DirTempP->ParentDirectoryAddress ) ;
		Depth ++ ;
	}
	
	// パス名を連結する
	DirPath[0] = '\0' ;
	for( i = Depth - 1 ; i >= 0 ; i -- )
	{
		strcat( DirPath, "\\" ) ;
		strcat( DirPath, ( char * )( this->NameP + ( ( DARC_FILEHEAD * )( this->FileP + Dir[i]->DirectoryAddress ) )->NameAddress ) ) ;
	}

	// バッファの長さが０か、長さが足りないときはディレクトリ名の長さを返す
	if( BufferLength == 0 || BufferLength < (s32)strlen( DirPath ) )
	{
		return ( int )( strlen( DirPath ) + 1 ) ;
	}
	else
	{
		// ディレクトリ名をバッファに転送する
		strcpy( DirPathBuffer, DirPath ) ;
	}

	// 終了
	return 0 ;
}



// アーカイブファイル中の指定のファイルをメモリに読み込む
s64 DXArchive::LoadFileToMem( const char *FilePath, void *Buffer, u64 BufferLength )
{
	DARC_FILEHEAD *FileH ;
	char KeyV2StringBuffer[ DXA_KEYV2_STRING_MAXLENGTH ] ;
	size_t KeyV2StringBufferBytes ;
	unsigned char lKeyV2[ DXA_KEYV2_LENGTH ] ;
	DARC_DIRECTORY *Directory ;

	// 指定のファイルの情報を得る
	FileH = this->GetFileInfo( FilePath, &Directory ) ;
	if( FileH == NULL ) return -1 ;

	// ファイルサイズが足りているか調べる、足りていないか、バッファ、又はサイズが０だったらサイズを返す
	if( BufferLength < FileH->DataSize || BufferLength == 0 || Buffer == NULL )
	{
		return ( s64 )FileH->DataSize ;
	}

	// 足りている場合はバッファーに読み込む

	// ファイルが圧縮されているかどうかで処理を分岐
	if( FileH->PressDataSize != 0xffffffffffffffff )
	{
		// 圧縮されている場合

		// メモリ上に読み込んでいるかどうかで処理を分岐
		if( MemoryOpenFlag == true )
		{
			// メモリ上の圧縮データを解凍する
			Decode( (u8 *)this->fp + this->Head.DataStartAddress + FileH->DataAddress, Buffer ) ;
		}
		else
		{
			void *temp ;

			// 圧縮データをメモリに読み込んでから解凍する

			// 圧縮データが収まるメモリ領域の確保
			temp = malloc( ( size_t )FileH->PressDataSize ) ;

			// 圧縮データの読み込み
			_fseeki64( this->fp, this->Head.DataStartAddress + FileH->DataAddress, SEEK_SET ) ;
			if( this->Head.Version >= DXA_KEYV2_VER )
			{
				// ファイル個別の鍵を作成
				KeyV2StringBufferBytes = CreateKeyV2FileString( ( int )this->Head.CharCodeFormat, this->KeyV2String, this->KeyV2StringBytes, Directory, FileH, this->FileP, this->DirP, this->NameP, ( BYTE * )KeyV2StringBuffer ) ;
				KeyV2Create( KeyV2StringBuffer, lKeyV2 , KeyV2StringBufferBytes ) ;

				KeyV2ConvFileRead( temp, FileH->PressDataSize, this->fp, lKeyV2, FileH->DataSize ) ;
			}
			else
			{
				KeyConvFileRead( temp, FileH->PressDataSize, this->fp, this->Key, FileH->DataSize ) ;
			}
			
			// 解凍
			Decode( temp, Buffer ) ;
			
			// メモリの解放
			free( temp ) ;
		}
	}
	else
	{
		// 圧縮されていない場合

		// メモリ上に読み込んでいるかどうかで処理を分岐
		if( MemoryOpenFlag == true )
		{
			// コピー
			memcpy( Buffer, (u8 *)this->fp + this->Head.DataStartAddress + FileH->DataAddress, ( size_t )FileH->DataSize ) ;
		}
		else
		{
			// ファイルポインタを移動
			_fseeki64( this->fp, this->Head.DataStartAddress + FileH->DataAddress, SEEK_SET ) ;

			// 読み込み
			if( this->Head.Version >= DXA_KEYV2_VER )
			{
				// ファイル個別の鍵を作成
				KeyV2StringBufferBytes = CreateKeyV2FileString( ( int )this->Head.CharCodeFormat, this->KeyV2String, this->KeyV2StringBytes, Directory, FileH, this->FileP, this->DirP, this->NameP, ( BYTE * )KeyV2StringBuffer ) ;
				KeyV2Create( KeyV2StringBuffer, lKeyV2 , KeyV2StringBufferBytes ) ;

				KeyV2ConvFileRead( Buffer, FileH->DataSize, this->fp, lKeyV2, FileH->DataSize ) ;
			}
			else
			{
				KeyConvFileRead( Buffer, FileH->DataSize, this->fp, this->Key, FileH->DataSize ) ;
			}
		}
	}
	
	// 終了
	return 0 ;
}

// アーカイブファイル中の指定のファイルをサイズを取得する
s64 DXArchive::GetFileSize( const char *FilePath )
{
	// ファイルサイズを返す
	return this->LoadFileToMem( FilePath, NULL, 0 ) ;
}

// アーカイブファイルをメモリに読み込んだ場合のファイルイメージが格納されている先頭アドレスを取得する( メモリから開いている場合のみ有効 )
void *DXArchive::GetFileImage( void )
{
	// メモリイメージから開いていなかったらエラー
	if( MemoryOpenFlag == false ) return NULL ;

	// 先頭アドレスを返す
	return this->fp ;
}

// アーカイブファイル中の指定のファイルのファイル内の位置とファイルの大きさを得る( -1:エラー )
int DXArchive::GetFileInfo( const char *FilePath, u64 *PositionP, u64 *SizeP )
{
	DARC_FILEHEAD *FileH ;

	// 指定のファイルの情報を得る
	FileH = this->GetFileInfo( FilePath ) ;
	if( FileH == NULL ) return -1 ;

	// ファイルのデータがある位置とファイルサイズを保存する
	if( PositionP != NULL ) *PositionP = this->Head.DataStartAddress + FileH->DataAddress ;
	if( SizeP     != NULL ) *SizeP     = FileH->DataSize ;

	// 成功終了
	return 0 ;
}

// アーカイブファイル中の指定のファイルを、クラス内のバッファに読み込む
void *DXArchive::LoadFileToCash( const char *FilePath )
{
	s64 FileSize ;

	// ファイルサイズを取得する
	FileSize = this->GetFileSize( FilePath ) ;

	// ファイルが無かったらエラー
	if( FileSize < 0 ) return NULL ;

	// 確保しているキャッシュバッファのサイズよりも大きい場合はバッファを再確保する
	if( FileSize > ( s64 )this->CashBufferSize )
	{
		// キャッシュバッファのクリア
		this->ClearCash() ;

		// キャッシュバッファの再確保
		this->CashBuffer = malloc( ( size_t )FileSize ) ;

		// 確保に失敗した場合は NULL を返す
		if( this->CashBuffer == NULL ) return NULL ;

		// キャッシュバッファのサイズを保存
		this->CashBufferSize = FileSize ;
	}

	// ファイルをメモリに読み込む
	this->LoadFileToMem( FilePath, this->CashBuffer, FileSize ) ;

	// キャッシュバッファのアドレスを返す
	return this->CashBuffer ;
}

// キャッシュバッファを開放する
int DXArchive::ClearCash( void )
{
	// メモリが確保されていたら解放する
	if( this->CashBuffer != NULL )
	{
		free( this->CashBuffer ) ;
		this->CashBuffer = NULL ;
		this->CashBufferSize = 0 ;
	}

	// 終了
	return 0 ;
}

	
// アーカイブファイル中の指定のファイルを開き、ファイルアクセス用オブジェクトを作成する
DXArchiveFile *DXArchive::OpenFile( const char *FilePath )
{
	DARC_FILEHEAD *FileH ;
	DARC_DIRECTORY *Directory ;
	DXArchiveFile *CDArc = NULL ;

	// メモリから開いている場合は無効
	if( MemoryOpenFlag == true ) return NULL ;

	// 指定のファイルの情報を得る
	FileH = this->GetFileInfo( FilePath, &Directory ) ;
	if( FileH == NULL ) return NULL ;

	// 新しく DXArchiveFile クラスを作成する
	CDArc = new DXArchiveFile( FileH, Directory, this ) ;
	
	// DXArchiveFile クラスのポインタを返す
	return CDArc ;
}













// コンストラクタ
DXArchiveFile::DXArchiveFile( DARC_FILEHEAD *FileHead, DARC_DIRECTORY *Directory, DXArchive *Archive )
{
	this->FileData  = FileHead ;
	this->Archive   = Archive ;
	this->EOFFlag   = FALSE ;
	this->FilePoint = 0 ;
	this->DataBuffer = NULL ;

	// バージョン７以降の場合は鍵バージョン２を作成する
	if( this->Archive->GetHeader()->Version >= DXA_KEYV2_VER )
	{
		char KeyV2StringBuffer[ DXA_KEYV2_STRING_MAXLENGTH ] ;
		size_t KeyV2StringBufferBytes ;

		KeyV2StringBufferBytes = DXArchive::CreateKeyV2FileString(
			( int )this->Archive->GetHeader()->CharCodeFormat,
			this->Archive->GetKeyV2String(),
			this->Archive->GetKeyV2StringBytes(),
			Directory,
			FileHead,
			this->Archive->GetFileHeadTable(),
			this->Archive->GetDirectoryTable(),
			this->Archive->GetNameTable(),
			( BYTE * )KeyV2StringBuffer
		) ;
		DXArchive::KeyV2Create( KeyV2StringBuffer, KeyV2 , KeyV2StringBufferBytes ) ;
	}
	
	// ファイルが圧縮されている場合はここで読み込んで解凍してしまう
	if( FileHead->PressDataSize != 0xffffffffffffffff )
	{
		void *temp ;

		// 圧縮データが収まるメモリ領域の確保
		temp = malloc( ( size_t )FileHead->PressDataSize ) ;

		// 解凍データが収まるメモリ領域の確保
		this->DataBuffer = malloc( ( size_t )FileHead->DataSize ) ;

		// 圧縮データの読み込み
		_fseeki64( this->Archive->GetFilePointer(), this->Archive->GetHeader()->DataStartAddress + FileHead->DataAddress, SEEK_SET ) ;
		if( this->Archive->GetHeader()->Version >= DXA_KEYV2_VER )
		{
			DXArchive::KeyV2ConvFileRead( temp, FileHead->PressDataSize, this->Archive->GetFilePointer(), KeyV2, FileHead->DataSize ) ;
		}
		else
		{
			DXArchive::KeyConvFileRead( temp, FileHead->PressDataSize, this->Archive->GetFilePointer(), this->Archive->GetKey(), FileHead->DataSize ) ;
		}
		
		// 解凍
		DXArchive::Decode( temp, this->DataBuffer ) ;
		
		// メモリの解放
		free( temp ) ;
	}
}

// デストラクタ
DXArchiveFile::~DXArchiveFile()
{
	// メモリの解放
	if( this->DataBuffer != NULL )
	{
		free( this->DataBuffer ) ;
		this->DataBuffer = NULL ;
	}
}

// ファイルの内容を読み込む
s64 DXArchiveFile::Read( void *Buffer, s64 ReadLength )
{
	s64 ReadSize ;

	// EOF フラグが立っていたら０を返す
	if( this->EOFFlag == TRUE ) return 0 ;
	
	// アーカイブファイルポインタと、仮想ファイルポインタが一致しているか調べる
	// 一致していなかったらアーカイブファイルポインタを移動する
	if( this->DataBuffer == NULL && _ftelli64( this->Archive->GetFilePointer() ) != (s32)( this->FileData->DataAddress + this->Archive->GetHeader()->DataStartAddress + this->FilePoint ) )
	{
		_fseeki64( this->Archive->GetFilePointer(), this->FileData->DataAddress + this->Archive->GetHeader()->DataStartAddress + this->FilePoint, SEEK_SET ) ;
	}
	
	// EOF 検出
	if( this->FileData->DataSize == this->FilePoint )
	{
		this->EOFFlag = TRUE ;
		return 0 ;
	}
	
	// データを読み込む量を設定する
	ReadSize = ReadLength < (s64)( this->FileData->DataSize - this->FilePoint ) ? ReadLength : this->FileData->DataSize - this->FilePoint ;
	
	// データを読み込む
	if( this->DataBuffer == NULL )
	{
		if( this->Archive->GetHeader()->Version >= DXA_KEYV2_VER )
		{
			DXArchive::KeyV2ConvFileRead( Buffer, ReadSize, this->Archive->GetFilePointer(), KeyV2, this->FileData->DataSize + this->FilePoint ) ;
		}
		else
		{
			DXArchive::KeyConvFileRead( Buffer, ReadSize, this->Archive->GetFilePointer(), this->Archive->GetKey(), this->FileData->DataSize + this->FilePoint ) ;
		}
	}
	else
	{
		memcpy( Buffer, (u8 *)this->DataBuffer + this->FilePoint, ( size_t )ReadSize ) ;
	}
	
	// EOF フラグを倒す
	this->EOFFlag = FALSE ;

	// 読み込んだ分だけファイルポインタを移動する
	this->FilePoint += ReadSize ;
	
	// 読み込んだ容量を返す
	return ReadSize ;
}
	
// ファイルポインタを変更する
s64 DXArchiveFile::Seek( s64 SeekPoint, s64 SeekMode )
{
	// シークタイプによって処理を分岐
	switch( SeekMode )
	{
	case SEEK_SET : break ;		
	case SEEK_CUR : SeekPoint += this->FilePoint ; break ;
	case SEEK_END :	SeekPoint = this->FileData->DataSize + SeekPoint ; break ;
	}
	
	// 補正
	if( SeekPoint > (s64)this->FileData->DataSize ) SeekPoint = this->FileData->DataSize ;
	if( SeekPoint < 0 ) SeekPoint = 0 ;
	
	// セット
	this->FilePoint = SeekPoint ;
	
	// EOFフラグを倒す
	this->EOFFlag = FALSE ;
	
	// 終了
	return 0 ;
}

// 現在のファイルポインタを得る
s64 DXArchiveFile::Tell( void )
{
	return this->FilePoint ;
}

// ファイルの終端に来ているか、のフラグを得る
s64 DXArchiveFile::Eof( void )
{
	return this->EOFFlag ;
}

// ファイルのサイズを取得する
s64 DXArchiveFile::Size( void )
{
	return this->FileData->DataSize ;
}



