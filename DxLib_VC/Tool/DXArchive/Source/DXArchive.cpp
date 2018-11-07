// -------------------------------------------------------------------------------
// 
// 		�c�w���C�u�����A�[�J�C�o
// 
//	Creator			: �R�c �I
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

#define MIN_COMPRESS		(4)						// �Œሳ�k�o�C�g��
#define MAX_SEARCHLISTNUM	(64)					// �ő��v����T���ׂ̃��X�g��H��ő吔
#define MAX_SUBLISTNUM		(65536)					// ���k���ԒZ�k�̂��߂̃T�u���X�g�̍ő吔
#define MAX_COPYSIZE 		(0x1fff + MIN_COMPRESS)	// �Q�ƃA�h���X����R�s�[�o�؂�ő�T�C�Y( ���k�R�[�h���\���ł���R�s�[�T�C�Y�̍ő�l + �Œሳ�k�o�C�g�� )
#define MAX_ADDRESSLISTNUM	(1024 * 1024 * 1)		// �X���C�h�����̍ő�T�C�Y
#define MAX_POSITION		(1 << 24)				// �Q�Ɖ\�ȍő告�΃A�h���X( 16MB )

// struct -----------------------------

// ���k���ԒZ�k�p���X�g
typedef struct LZ_LIST
{
	LZ_LIST *next, *prev ;
	u32 address ;
} LZ_LIST ;

// class code -------------------------

// �t�@�C�������ꏏ�ɂȂ��Ă���ƕ������Ă���p�X������t�@�C���p�X�ƃf�B���N�g���p�X�𕪊�����
// �t���p�X�ł���K�v�͖���
int DXArchive::GetFilePathAndDirPath( char *Src, char *FilePath, char *DirPath )
{
	int i, Last ;
	
	// �t�@�C�����𔲂��o��
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
	
	// �f�B���N�g���p�X�𔲂��o��
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
	
	// �I��
	return 0 ;
}

// �t�@�C���̏��𓾂�
DARC_FILEHEAD *DXArchive::GetFileInfo( const char *FilePath, DARC_DIRECTORY **DirectoryP )
{
	DARC_DIRECTORY *OldDir ;
	DARC_FILEHEAD *FileH ;
	u8 *NameData ;
	int i, j, k, Num, FileHeadSize ;
	SEARCHDATA SearchData ;

	// ���̃f�B���N�g����ۑ����Ă���
	OldDir = this->CurrentDirectory ;

	// �t�@�C���p�X�� \ ���܂܂�Ă���ꍇ�A�f�B���N�g���ύX���s��
	if( strchr( FilePath, '\\' ) != NULL )
	{
		// �J�����g�f�B���N�g����ړI�̃t�@�C��������f�B���N�g���ɕύX����
		if( this->ChangeCurrentDirectoryBase( FilePath, false, &SearchData ) >= 0 )
		{
			// �G���[���N���Ȃ������ꍇ�̓t�@�C�������f�B���N�g�����������ƂɂȂ�̂ŃG���[
			goto ERR ;
		}
	}
	else
	{
		// �t�@�C�����������p�f�[�^�ɕϊ�����
		ConvSearchData( &SearchData, FilePath, NULL ) ;
	}

	// �����̃t�@�C����T��
	FileHeadSize = sizeof( DARC_FILEHEAD ) ;
	FileH = ( DARC_FILEHEAD * )( this->FileP + this->CurrentDirectory->FileHeadAddress ) ;
	Num = ( int )this->CurrentDirectory->FileHeadNum ;
	for( i = 0 ; i < Num ; i ++, FileH = (DARC_FILEHEAD *)( (u8 *)FileH + FileHeadSize ) )
	{
		// �f�B���N�g���`�F�b�N
		if( ( FileH->Attributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 ) continue ;

		// �����񐔂ƃp���e�B�`�F�b�N
		NameData = this->NameP + FileH->NameAddress ;
		if( SearchData.PackNum != ((u16 *)NameData)[0] || SearchData.Parity != ((u16 *)NameData)[1] ) continue ;

		// ������`�F�b�N
		NameData += 4 ;
		for( j = 0, k = 0 ; j < SearchData.PackNum ; j ++, k += 4 )
			if( *((u32 *)&SearchData.FileName[k]) != *((u32 *)&NameData[k]) ) break ;

		// �K�������t�@�C�����������炱���ŏI��
		if( SearchData.PackNum == j ) break ;
	}

	// ����������G���[
	if( i == Num ) goto ERR ;

	// �f�B���N�g���̃A�h���X��ۑ�����w�肪�������ꍇ�͕ۑ�
	if( DirectoryP != NULL )
	{
		*DirectoryP = this->CurrentDirectory ;
	}
	
	// �f�B���N�g�������ɖ߂�
	this->CurrentDirectory = OldDir ;
	
	// �ړI�̃t�@�C���̃A�h���X��Ԃ�
	return FileH ;
	
ERR :
	// �f�B���N�g�������ɖ߂�
	this->CurrentDirectory = OldDir ;
	
	// �G���[�I��
	return NULL ;
}

// �A�[�J�C�u���̃J�����g�f�B���N�g���̏����擾����
DARC_DIRECTORY *DXArchive::GetCurrentDirectoryInfo( void )
{
	return CurrentDirectory ;
}

// �ǂ��炪�V���������r����
DXArchive::DATE_RESULT DXArchive::DateCmp( DARC_FILETIME *date1, DARC_FILETIME *date2 )
{
	if( date1->LastWrite == date2->LastWrite ) return DATE_RESULT_DRAW ;
	else if( date1->LastWrite > date2->LastWrite ) return DATE_RESULT_LEFTNEW ;
	else return DATE_RESULT_RIGHTNEW ;
}

// ��r�ΏƂ̕����񒆂̑啶�����������Ƃ��Ĉ�����r����( 0:������  1:�Ⴄ )
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

	// �����܂ŗ��ď��߂ē�����
	return 0 ;
}

// �����񒆂̉p���̏�������啶���ɕϊ�
int DXArchive::ConvSearchData( SEARCHDATA *SearchData, const char *Src, int *Length )
{
	int i, StringLength ;
	u16 ParityData ;

	ParityData = 0 ;
	for( i = 0 ; Src[i] != '\0' && Src[i] != '\\' ; )
	{
		if( CheckMultiByteChar( &Src[i] ) == TRUE )
		{
			// �Q�o�C�g�����̏ꍇ�͂��̂܂܃R�s�[
			*((u16 *)&SearchData->FileName[i]) = *((u16 *)&Src[i]) ;
			ParityData += (u8)Src[i] + (u8)Src[i+1] ;
			i += 2 ;
		}
		else
		{
			// �������̏ꍇ�͑啶���ɕϊ�
			if( Src[i] >= 'a' && Src[i] <= 'z' )	SearchData->FileName[i] = (u8)Src[i] - 'a' + 'A' ;
			else									SearchData->FileName[i] = Src[i] ;
			ParityData += (u8)SearchData->FileName[i] ;
			i ++ ;
		}
	}

	// ������̒�����ۑ�
	if( Length != NULL ) *Length = i ;

	// �S�̔{���̈ʒu�܂łO����
	StringLength = ( ( i + 1 ) + 3 ) / 4 * 4 ;
	memset( &SearchData->FileName[i], 0, StringLength - i ) ;

	// �p���e�B�f�[�^�̕ۑ�
	SearchData->Parity = ParityData ;

	// �p�b�N�f�[�^���̕ۑ�
	SearchData->PackNum = StringLength / 4 ;

	// ����I��
	return 0 ;
}

// �t�@�C�����f�[�^��ǉ�����( �߂�l�͎g�p�����f�[�^�o�C�g�� )
int DXArchive::AddFileNameData( int CharCodeFormat, const char *FileName, u8 *FileNameTable )
{
	int PackNum, Length, i, j ;
	u32 Parity ;

	// �T�C�Y���Z�b�g
	Length = ( int )CL_strlen( CharCodeFormat, FileName ) * GetCharCodeFormatUnitSize( CharCodeFormat ) ;

	// �ꕶ�������������ꍇ�̏���
	if( Length == 0 )
	{
		// �p�b�N���ƃp���e�B���̂ݕۑ�
		*((u32 *)&FileNameTable[0]) = 0 ;

		// �g�p�T�C�Y��Ԃ�
		return 4 ;
	}
	Length ++ ;

	PackNum = ( Length + 3 ) / 4 ;

	// �p�b�N����ۑ�
	*((u16 *)&FileNameTable[0]) = PackNum ;

	// �o�b�t�@�̏�����
	memset( &FileNameTable[4], 0, PackNum * 4 * 2 ) ;

	// ��������R�s�[
	CL_strcpy( CharCodeFormat, (char *)&FileNameTable[4 + PackNum * 4], FileName ) ;

	// �p���̏�������S�đ啶���ɕϊ������t�@�C������ۑ�
	Parity = 0 ;
	for( i = 0 ; FileName[i] != '\0' ; )
	{
		int Bytes = GetCharBytes( &FileName[i], CharCodeFormat ) ;

		// 1�o�C�g�������ǂ����ŏ����𕪊�
		if( Bytes == 1 )
		{
			// �P�o�C�g����
			if( FileName[i] >= 'a' && FileName[i] <= 'z' )
			{
				// �������̏ꍇ�͑啶���ɕϊ�
				FileNameTable[4 + i] = (u8)FileName[i] - 'a' + 'A' ;
			}
			else
			{
				// �����ł͂Ȃ��ꍇ�͕��ʂɃR�s�[
				FileNameTable[4 + i] = (u8)FileName[i] ;
			}
			Parity += FileNameTable[4 + i] ;
			i ++ ;
		}
		else
		{
			// �}���`�o�C�g����
			for( j = 0 ; j < Bytes ; j ++ )
			{
				FileNameTable[4 + i + j] = (u8)FileName[i + j] ;
				Parity += (u8)FileName[i + j] ;
			}
			i += Bytes ;
		}
	}

	// �p���e�B����ۑ�
	*((u16 *)&FileNameTable[2]) = (u16)Parity ;

	// �g�p�����T�C�Y��Ԃ�
	return PackNum * 4 * 2 + 4 ;
}

// �t�@�C�����f�[�^���猳�̃t�@�C�����̕�������擾����
const char *DXArchive::GetOriginalFileName( u8 *FileNameTable )
{
	return (char *)FileNameTable + *((u16 *)&FileNameTable[0]) * 4 + 4 ;
}

// �W���X�g���[���Ƀf�[�^����������( 64bit�� )
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

// �W���X�g���[������f�[�^��ǂݍ���( 64bit�� )
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

// �f�[�^�𔽓]������֐�
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


// �f�[�^�𔽓]�����ăt�@�C���ɏ����o���֐�
void DXArchive::NotConvFileWrite( void *Data, s64 Size, FILE *fp )
{
	// �f�[�^�𔽓]����
	NotConv( Data, Size ) ;

	// �����o��
	fwrite64( Data, Size, fp ) ;

	// �Ăє��]
	NotConv( Data, Size ) ;
}

// �f�[�^�𔽓]�����ăt�@�C������ǂݍ��ފ֐�
void DXArchive::NotConvFileRead( void *Data, s64 Size, FILE *fp )
{
	// �ǂݍ���
	fread64( Data, Size, fp ) ;

	// �f�[�^�𔽓]
	NotConv( Data, Size ) ;
}

// �J�����g�f�B���N�g���ɂ���w��̃t�@�C���̌��o�[�W�����Q�p�̕�������쐬����A�߂�l�͕�����̒���( �P�ʁFByte )( FileString �� DXA_KEYV2_STRING_MAXLENGTH �̒������K�v )
size_t DXArchive::CreateKeyV2FileString( int CharCodeFormat, const char *KeyV2String, size_t KeyV2StringBytes, DARC_DIRECTORY *Directory, DARC_FILEHEAD *FileHead, u8 *FileTable, u8 *DirectoryTable, u8 *NameTable, u8 *FileString )
{
	size_t StartAddr ;

	// �ŏ��Ƀp�X���[�h�̕�������Z�b�g
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

	// ���Ƀt�@�C�����̕�������Z�b�g
	CL_strcat_s( CharCodeFormat, ( char * )&FileString[ StartAddr ], DXA_KEYV2_STRING_MAXLENGTH - StartAddr, ( char * )( NameTable + FileHead->NameAddress + 4 ) ) ;

	// ���̌�Ƀf�B���N�g���̕�������Z�b�g
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

// ����������쐬
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
			// �������� DXA_KEYSTR_LENGTH ���Z�������烋�[�v����
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

// ���o�[�W�����Q��������쐬
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

// ����������g�p���� Xor ���Z( Key �͕K�� DXA_KEYSTR_LENGTH �̒������Ȃ���΂Ȃ�Ȃ� )
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

// ���o�[�W�����Q��������g�p���� Xor ���Z( Key �͕K�� DXA_KEYV2_LENGTH �̒������Ȃ���΂Ȃ�Ȃ� )
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

// �f�[�^������������g�p���� Xor ���Z������t�@�C���ɏ����o���֐�( Key �͕K�� DXA_KEYSTR_LENGTH �̒������Ȃ���΂Ȃ�Ȃ� )
void DXArchive::KeyConvFileWrite( void *Data, s64 Size, FILE *fp, unsigned char *Key, s64 Position )
{
	s64 pos ;

	// �t�@�C���̈ʒu���擾���Ă���
	if( Position == -1 ) pos = _ftelli64( fp ) ;
	else                 pos = Position ;

	// �f�[�^������������g���� Xor ���Z����
	KeyConv( Data, Size, pos, Key ) ;

	// �����o��
	fwrite64( Data, Size, fp ) ;

	// �Ă� Xor ���Z
	KeyConv( Data, Size, pos, Key ) ;
}

// �f�[�^�����o�[�W�����Q��������g�p���� Xor ���Z������t�@�C���ɏ����o���֐�( Key �͕K�� DXA_KEYV2_LENGTH �̒������Ȃ���΂Ȃ�Ȃ� )
void DXArchive::KeyV2ConvFileWrite( void *Data, s64 Size, FILE *fp, unsigned char *Key, s64 Position )
{
	// �f�[�^������������g���� Xor ���Z����
	KeyV2Conv( Data, Size, Position, Key ) ;

	// �����o��
	fwrite64( Data, Size, fp ) ;

	// �Ă� Xor ���Z
	KeyV2Conv( Data, Size, Position, Key ) ;
}

// �t�@�C������ǂݍ��񂾃f�[�^������������g�p���� Xor ���Z����֐�( Key �͕K�� DXA_KEYSTR_LENGTH �̒������Ȃ���΂Ȃ�Ȃ� )
void DXArchive::KeyConvFileRead( void *Data, s64 Size, FILE *fp, unsigned char *Key, s64 Position )
{
	s64 pos ;

	// �t�@�C���̈ʒu���擾���Ă���
	if( Position == -1 ) pos = _ftelli64( fp ) ;
	else                 pos = Position ;

	// �ǂݍ���
	fread64( Data, Size, fp ) ;

	// �f�[�^������������g���� Xor ���Z
	KeyConv( Data, Size, pos, Key ) ;
}

// �t�@�C������ǂݍ��񂾃f�[�^�����o�[�W�����Q��������g�p���� Xor ���Z����֐�( Key �͕K�� DXA_KEYV2_LENGTH �̒������Ȃ���΂Ȃ�Ȃ� )
void DXArchive::KeyV2ConvFileRead( void *Data, s64 Size, FILE *fp, unsigned char *Key, s64 Position )
{
	// �ǂݍ���
	fread64( Data, Size, fp ) ;

	// �f�[�^������������g���� Xor ���Z
	KeyV2Conv( Data, Size, Position, Key ) ;
}

/*
// �Q�o�C�g���������ׂ�( TRUE:�Q�o�C�g���� FALSE:�P�o�C�g���� )
int DXArchive::CheckMultiByteChar( const char *Buf )
{
	return  ( (unsigned char)*Buf >= 0x81 && (unsigned char)*Buf <= 0x9F ) || ( (unsigned char)*Buf >= 0xE0 && (unsigned char)*Buf <= 0xFC ) ;
}
*/

// �w��̃f�B���N�g���ɂ���t�@�C�����A�[�J�C�u�f�[�^�ɓf���o��
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

	// �f�B���N�g���̏��𓾂�
	FindHandle = FindFirstFileA( DirectoryName, &FindData ) ;
	if( FindHandle == INVALID_HANDLE_VALUE ) return 0 ;
	
	// �f�B���N�g�������i�[����t�@�C���w�b�_���Z�b�g����
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

	// �f�B���N�g�����������o��
	Size->NameSize += AddFileNameData( CharCodeFormat, FindData.cFileName, NameP + Size->NameSize ) ;

	// �f�B���N�g����񂪓������t�@�C���w�b�_�������o��
	memcpy( FileP + ParentDir->FileHeadAddress + DataNumber * sizeof( DARC_FILEHEAD ),
			&File, sizeof( DARC_FILEHEAD ) ) ;

	// Find �n���h�������
	FindClose( FindHandle ) ;

	// �w��̃f�B���N�g���ɃJ�����g�f�B���N�g�����ڂ�
	GetCurrentDirectoryA( MAX_PATH, DirPath ) ;
	SetCurrentDirectoryA( DirectoryName ) ;

	// �f�B���N�g�����̃Z�b�g
	{
		Dir.DirectoryAddress = ParentDir->FileHeadAddress + DataNumber * sizeof( DARC_FILEHEAD ) ;
		Dir.FileHeadAddress  = Size->FileSize ;

		// �e�f�B���N�g���̏��ʒu���Z�b�g
		if( ParentDir->DirectoryAddress != 0xffffffffffffffff && ParentDir->DirectoryAddress != 0 )
		{
			Dir.ParentDirectoryAddress = ((DARC_FILEHEAD *)( FileP + ParentDir->DirectoryAddress ))->DataAddress ;
		}
		else
		{
			Dir.ParentDirectoryAddress = 0 ;
		}

		// �f�B���N�g�����̃t�@�C���̐����擾����
		Dir.FileHeadNum = GetDirectoryFilePath( "", NULL ) ;
	}

	// �f�B���N�g���̏����o�͂���
	memcpy( DirP + Size->DirectorySize, &Dir, sizeof( DARC_DIRECTORY ) ) ;	
	DirectoryP = ( DARC_DIRECTORY * )( DirP + Size->DirectorySize ) ;

	// �A�h���X�𐄈ڂ�����
	Size->DirectorySize += sizeof( DARC_DIRECTORY ) ;
	Size->FileSize      += sizeof( DARC_FILEHEAD ) * Dir.FileHeadNum ;
	
	// �t�@�C�������������ꍇ�͂����ŏI��
	if( Dir.FileHeadNum == 0 )
	{
		// ���Ƃ̃f�B���N�g�����J�����g�f�B���N�g���ɃZ�b�g
		SetCurrentDirectoryA( DirPath ) ;
		return 0 ;
	}

	// �t�@�C�������o�͂���
	{
		int i ;
		
		i = 0 ;
		
		// �񋓊J�n
		FindHandle = FindFirstFileA( "*", &FindData ) ;
		do
		{
			// ��̃f�B���N�g���ɖ߂����肷�邽�߂̃p�X�͖�������
			if( strcmp( FindData.cFileName, "." ) == 0 || strcmp( FindData.cFileName, ".." ) == 0 ) continue ;

			// �t�@�C���ł͂Ȃ��A�f�B���N�g���������ꍇ�͍ċA����
			if( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// �f�B���N�g���������ꍇ�̏���
				if( DirectoryEncode( CharCodeFormat, FindData.cFileName, NameP, DirP, FileP, &Dir, Size, i, DestP, TempBuffer, Press, KeyV2String, KeyV2StringBytes, KeyV2StringBuffer ) < 0 ) return -1 ;
			}
			else
			{
				// �t�@�C���������ꍇ�̏���

				// �t�@�C���̃f�[�^���Z�b�g
				File.NameAddress     = Size->NameSize ;
				File.Time.Create     = ( ( ( LONGLONG )FindData.ftCreationTime.dwHighDateTime   ) << 32 ) + FindData.ftCreationTime.dwLowDateTime   ;
				File.Time.LastAccess = ( ( ( LONGLONG )FindData.ftLastAccessTime.dwHighDateTime ) << 32 ) + FindData.ftLastAccessTime.dwLowDateTime ;
				File.Time.LastWrite  = ( ( ( LONGLONG )FindData.ftLastWriteTime.dwHighDateTime  ) << 32 ) + FindData.ftLastWriteTime.dwLowDateTime  ;
				File.Attributes      = FindData.dwFileAttributes ;
				File.DataAddress     = Size->DataSize ;
				File.DataSize        = ( ( ( LONGLONG )FindData.nFileSizeHigh ) << 32 ) + FindData.nFileSizeLow ;
				File.PressDataSize   = 0xffffffffffffffff ;

				// �t�@�C�����������o��
				Size->NameSize += AddFileNameData( CharCodeFormat, FindData.cFileName, NameP + Size->NameSize ) ;

				// �t�@�C���ʂ̌����쐬
				KeyV2StringBufferBytes = CreateKeyV2FileString( CharCodeFormat, KeyV2String, KeyV2StringBytes, DirectoryP, &File, FileP, DirP, NameP, ( BYTE * )KeyV2StringBuffer ) ;
				KeyV2Create( KeyV2StringBuffer, lKeyV2 , KeyV2StringBufferBytes ) ;
				
				// �t�@�C���f�[�^�������o��
				if( File.DataSize != 0 )
				{
					FILE *SrcP ;
					u64 FileSize, WriteSize, MoveSize ;

					// �t�@�C�����J��
					SrcP = fopen( FindData.cFileName, "rb" ) ;
					
					// �T�C�Y�𓾂�
					_fseeki64( SrcP, 0, SEEK_END ) ;
					FileSize = _ftelli64( SrcP ) ;
					_fseeki64( SrcP, 0, SEEK_SET ) ;
					
					// �t�@�C���T�C�Y�� 10MB �ȉ��̏ꍇ�ŁA���k�̎w�肪����ꍇ�͈��k�����݂�
					if( Press == true && File.DataSize < 10 * 1024 * 1024 )
					{
						void *SrcBuf, *DestBuf ;
						u32 DestSize, Len ;
						
						// �ꕔ�̃t�@�C���`���̏ꍇ�͗\�ߒe��
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
						
						// �f�[�^���ۂ��Ɠ��郁�����̈�̊m��
						SrcBuf  = malloc( ( size_t )( FileSize + FileSize * 2 + 64 ) ) ;
						DestBuf = (u8 *)SrcBuf + FileSize ;
						
						// �t�@�C�����ۂ��Ɠǂݍ���
						fread64( SrcBuf, FileSize, SrcP ) ;
						
						// ���k
						DestSize = Encode( SrcBuf, ( u32 )FileSize, DestBuf ) ;
						
						// �w�ǈ��k�o���Ȃ������ꍇ�͈��k�����ŃA�[�J�C�u����
						if( (f64)DestSize / (f64)FileSize > 0.90 )
						{
							_fseeki64( SrcP, 0L, SEEK_SET ) ;
							free( SrcBuf ) ;
							goto NOPRESS ;
						}
						
						// ���k�f�[�^�𔽓]���ď����o��
						WriteSize = ( DestSize + 3 ) / 4 * 4 ;
						KeyV2ConvFileWrite( DestBuf, WriteSize, DestP, lKeyV2, File.DataSize ) ;
						
						// �������̉��
						free( SrcBuf ) ;
						
						// ���k�f�[�^�̃T�C�Y��ۑ�����
						File.PressDataSize = DestSize ;
					}
					else
					{
NOPRESS:					
						// �]���J�n
						WriteSize = 0 ;
						while( WriteSize < FileSize )
						{
							// �]���T�C�Y����
							MoveSize = DXA_BUFFERSIZE < FileSize - WriteSize ? DXA_BUFFERSIZE : FileSize - WriteSize ;
							MoveSize = ( MoveSize + 3 ) / 4 * 4 ;	// �T�C�Y�͂S�̔{���ɍ��킹��
							
							// �t�@�C���̔��]�ǂݍ���
							KeyV2ConvFileRead( TempBuffer, MoveSize, SrcP, lKeyV2, File.DataSize + WriteSize ) ;

							// �����o��
							fwrite64( TempBuffer, MoveSize, DestP ) ;
							
							// �����o���T�C�Y�̉��Z
							WriteSize += MoveSize ;
						}
					}
					
					// �����o�����t�@�C�������
					fclose( SrcP ) ;
				
					// �f�[�^�T�C�Y�̉��Z
					Size->DataSize += WriteSize ;
				}
				
				// �t�@�C���w�b�_�������o��
				memcpy( FileP + Dir.FileHeadAddress + sizeof( DARC_FILEHEAD ) * i, &File, sizeof( DARC_FILEHEAD ) ) ;
			}
			
			i ++ ;
		}
		while( FindNextFileA( FindHandle, &FindData ) != 0 ) ;
		
		// Find �n���h�������
		FindClose( FindHandle ) ;
	}
						
	// ���Ƃ̃f�B���N�g�����J�����g�f�B���N�g���ɃZ�b�g
	SetCurrentDirectoryA( DirPath ) ;

	// �I��
	return 0 ;
}

// �w��̃f�B���N�g���f�[�^�ɂ���t�@�C����W�J����
int DXArchive::DirectoryDecode( u8 *NameP, u8 *DirP, u8 *FileP, DARC_HEAD *Head, DARC_DIRECTORY *Dir, FILE *ArcP, unsigned char *Key, const char *KeyV2String, size_t KeyV2StringBytes, char *KeyV2StringBuffer )
{
	char DirPath[MAX_PATH] ;
	
	// ���݂̃J�����g�f�B���N�g����ۑ�
	GetCurrentDirectoryA( MAX_PATH, DirPath ) ;

	// �f�B���N�g����񂪂���ꍇ�́A�܂��W�J�p�̃f�B���N�g�����쐬����
	if( Dir->DirectoryAddress != 0xffffffffffffffff && Dir->ParentDirectoryAddress != 0xffffffffffffffff )
	{
		DARC_FILEHEAD *DirFile ;
		
		// DARC_FILEHEAD �̃A�h���X���擾
		DirFile = ( DARC_FILEHEAD * )( FileP + Dir->DirectoryAddress ) ;
		
		// �f�B���N�g���̍쐬
		CreateDirectoryA( GetOriginalFileName( NameP + DirFile->NameAddress ), NULL ) ;
		
		// ���̃f�B���N�g���ɃJ�����g�f�B���N�g�����ڂ�
		SetCurrentDirectoryA( GetOriginalFileName( NameP + DirFile->NameAddress ) ) ;
	}

	// �W�J�����J�n
	{
		u32 i, FileHeadSize ;
		DARC_FILEHEAD *File ;
		size_t KeyV2StringBufferBytes ;
		unsigned char lKeyV2[ DXA_KEYV2_LENGTH ] ;

		// �i�[����Ă���t�@�C���̐������J��Ԃ�
		FileHeadSize = sizeof( DARC_FILEHEAD ) ;
		File = ( DARC_FILEHEAD * )( FileP + Dir->FileHeadAddress ) ;
		for( i = 0 ; i < Dir->FileHeadNum ; i ++, File = (DARC_FILEHEAD *)( (u8 *)File + FileHeadSize ) )
		{
			// �f�B���N�g�����ǂ����ŏ����𕪊�
			if( File->Attributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// �f�B���N�g���̏ꍇ�͍ċA��������
				DirectoryDecode( NameP, DirP, FileP, Head, ( DARC_DIRECTORY * )( DirP + File->DataAddress ), ArcP, Key, KeyV2String, KeyV2StringBytes, KeyV2StringBuffer ) ;
			}
			else
			{
				FILE *DestP ;
				void *Buffer ;
			
				// �t�@�C���̏ꍇ�͓W�J����
				
				// �o�b�t�@���m�ۂ���
				Buffer = malloc( DXA_BUFFERSIZE ) ;
				if( Buffer == NULL ) return -1 ;

				// �t�@�C�����J��
				DestP = fopen( GetOriginalFileName( NameP + File->NameAddress ), "wb" ) ;

				// �t�@�C���ʂ̌����쐬
				KeyV2StringBufferBytes = CreateKeyV2FileString( ( int )Head->CharCodeFormat, KeyV2String, KeyV2StringBytes, Dir, File, FileP, DirP, NameP, ( BYTE * )KeyV2StringBuffer ) ;
				KeyV2Create( KeyV2StringBuffer, lKeyV2 , KeyV2StringBufferBytes ) ;
				
				// �f�[�^������ꍇ�̂ݓ]��
				if( File->DataSize != 0 )
				{
					// �����ʒu���Z�b�g����
					if( _ftelli64( ArcP ) != ( s32 )( Head->DataStartAddress + File->DataAddress ) )
						_fseeki64( ArcP, Head->DataStartAddress + File->DataAddress, SEEK_SET ) ;
						
					// �f�[�^�����k����Ă��邩�ǂ����ŏ����𕪊�
					if( File->PressDataSize != 0xffffffffffffffff )
					{
						void *temp ;
						
						// ���k����Ă���ꍇ

						// ���k�f�[�^�����܂郁�����̈�̊m��
						temp = malloc( ( size_t )( File->PressDataSize + File->DataSize ) ) ;

						// ���k�f�[�^�̓ǂݍ���
						if( Head->Version >= DXA_KEYV2_VER )
						{
							KeyV2ConvFileRead( temp, File->PressDataSize, ArcP, lKeyV2, File->DataSize ) ;
						}
						else
						{
							KeyConvFileRead( temp, File->PressDataSize, ArcP, Key, File->DataSize ) ;
						}
						
						// ��
						Decode( temp, (u8 *)temp + File->PressDataSize ) ;
						
						// �����o��
						fwrite64( (u8 *)temp + File->PressDataSize, File->DataSize, DestP ) ;
						
						// �������̉��
						free( temp ) ;
					}
					else
					{
						// ���k����Ă��Ȃ��ꍇ
					
						// �]�������J�n
						{
							u64 MoveSize, WriteSize ;
							
							WriteSize = 0 ;
							while( WriteSize < File->DataSize )
							{
								MoveSize = File->DataSize - WriteSize > DXA_BUFFERSIZE ? DXA_BUFFERSIZE : File->DataSize - WriteSize ;

								// �t�@�C���̔��]�ǂݍ���
								if( Head->Version >= DXA_KEYV2_VER )
								{
									KeyV2ConvFileRead( Buffer, MoveSize, ArcP, lKeyV2, File->DataSize + WriteSize ) ;
								}
								else
								{
									KeyConvFileRead( Buffer, MoveSize, ArcP, Key, File->DataSize + WriteSize ) ;
								}

								// �����o��
								fwrite64( Buffer, MoveSize, DestP ) ;
								
								WriteSize += MoveSize ;
							}
						}
					}
				}
				
				// �t�@�C�������
				fclose( DestP ) ;

				// �o�b�t�@���J������
				free( Buffer ) ;

				// �t�@�C���̃^�C���X�^���v��ݒ肷��
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

				// �t�@�C��������t����
				SetFileAttributesA( GetOriginalFileName( NameP + File->NameAddress ), ( u32 )File->Attributes ) ;
			}
		}
	}
	
	// �J�����g�f�B���N�g�������ɖ߂�
	SetCurrentDirectoryA( DirPath ) ;

	// �I��
	return 0 ;
}

// �f�B���N�g�����̃t�@�C���p�X���擾����
int DXArchive::GetDirectoryFilePath( const char *DirectoryPath, char *FileNameBuffer )
{
	WIN32_FIND_DATAA FindData ;
	HANDLE FindHandle ;
	int FileNum ;
	char DirPath[256], String[256] ;

	// �f�B���N�g�����ǂ������`�F�b�N����
	if( DirectoryPath[0] != '\0' )
	{
		FindHandle = FindFirstFileA( DirectoryPath, &FindData ) ;
		if( FindHandle == INVALID_HANDLE_VALUE || ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 ) return -1 ;
		FindClose( FindHandle ) ;
	}

	// �w��̃t�H���_�̃t�@�C���̖��O���擾����
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
			// ��̃f�B���N�g���ɖ߂����肷�邽�߂̃p�X�͖�������
			if( strcmp( FindData.cFileName, "." ) == 0 || strcmp( FindData.cFileName, ".." ) == 0 ) continue ;

			// �t�@�C���p�X��ۑ�����
			if( FileNameBuffer != NULL )
			{
				strcpy( FileNameBuffer, DirPath ) ;
				strcat( FileNameBuffer, FindData.cFileName ) ;
				FileNameBuffer += 256 ;
			}

			// �t�@�C���̐��𑝂₷
			FileNum ++ ;
		}
		while( FindNextFileA( FindHandle, &FindData ) != 0 ) ;
		FindClose( FindHandle ) ;
	}

	// ����Ԃ�
	return FileNum ;
}

// �G���R�[�h( �߂�l:���k��̃T�C�Y  -1 �̓G���[  Dest �� NULL �����邱�Ƃ��\ )
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
	
	// �T�u���X�g�̃T�C�Y�����߂�
	{
			 if( SrcSize < 100 * 1024 )			sublistmaxnum = 1 ;
		else if( SrcSize < 3 * 1024 * 1024 )	sublistmaxnum = MAX_SUBLISTNUM / 3 ;
		else									sublistmaxnum = MAX_SUBLISTNUM ;
	}

	// ���X�g�̃T�C�Y�����߂�
	{
		maxlistnum = MAX_ADDRESSLISTNUM ;
		if( maxlistnum > SrcSize )
		{
			while( ( maxlistnum >> 1 ) > 0x100 && ( maxlistnum >> 1 ) > SrcSize )
				maxlistnum >>= 1 ;
		}
		maxlistnummask = maxlistnum - 1 ;
	}

	// �������̊m��
	usesublistflagtable   = (u8 *)malloc(
		sizeof( void * )  * 65536 +					// ���C�����X�g�̐擪�I�u�W�F�N�g�p�̈�
		sizeof( LZ_LIST ) * maxlistnum +			// ���C�����X�g�p�̈�
		sizeof( u8 )      * 65536 +					// �T�u���X�g���g�p���Ă��邩�t���O�p�̈�
		sizeof( void * )  * 256 * sublistmaxnum ) ;	// �T�u���X�g�p�̈�
		
	// �A�h���X�̃Z�b�g
	listfirsttable =     usesublistflagtable + sizeof(     u8 ) * 65536 ;
	sublistbuf     =          listfirsttable + sizeof( void * ) * 65536 ;
	listbuf        = (LZ_LIST *)( sublistbuf + sizeof( void * ) * 256 * sublistmaxnum ) ;
	
	// ������
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

	// ���k���f�[�^�̒��ň�ԏo���p�x���Ⴂ�o�C�g�R�[�h����������
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

	// ���k���̃T�C�Y���Z�b�g
	((u32 *)destp)[0] = SrcSize ;

	// �L�[�R�[�h���Z�b�g
	destp[8] = keycode ;

	// ���k����
	dp               = destp + 9 ;
	sp               = srcp ;
	srcaddress       = 0 ;
	dstsize          = 0 ;
	listaddp         = 0 ;
	sublistnum       = 0 ;
	nextprintaddress = 1024 * 100 ;
	while( srcaddress < SrcSize )
	{
		// �c��T�C�Y���Œሳ�k�T�C�Y�ȉ��̏ꍇ�͈��k���������Ȃ�
		if( srcaddress + MIN_COMPRESS >= SrcSize ) goto NOENCODE ;

		// ���X�g���擾
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

		// ��Ԉ�v���̒����R�[�h��T��
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

		// ���X�g�ɓo�^
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

		// ��v�R�[�h��������Ȃ�������񈳏k�R�[�h�Ƃ��ďo��
		if( maxconbo == -1 )
		{
NOENCODE:
			// �L�[�R�[�h�������ꍇ�͂Q��A���ŏo�͂���
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
			// ���������ꍇ�͌������ʒu�ƒ������o�͂���
			
			// �L�[�R�[�h�ƌ������ʒu�ƒ������o��
			if( destp != NULL )
			{
				// �L�[�R�[�h�̏o��
				*dp++ = keycode ;

				// �o�͂���A�����͍Œ� MIN_COMPRESS ���邱�Ƃ��O��Ȃ̂� - MIN_COMPRESS �������̂��o�͂���
				maxconbo -= MIN_COMPRESS ;

				// �A�����O�`�S�r�b�g�ƘA�����A���΃A�h���X�̃r�b�g�����o��
				*dp = (u8)( ( ( maxconbo & 0x1f ) << 3 ) | ( maxconbosize << 2 ) | maxaddresssize ) ;

				// �L�[�R�[�h�̘A���̓L�[�R�[�h�ƒl�̓������񈳏k�R�[�h��
				// ���f���邽�߁A�L�[�R�[�h�̒l�ȏ�̏ꍇ�͒l���{�P����
				if( *dp >= keycode ) dp[0] += 1 ;
				dp ++ ;

				// �A�����T�`�P�Q�r�b�g���o��
				if( maxconbosize == 1 )
					*dp++ = (u8)( ( maxconbo >> 5 ) & 0xff ) ;

				// maxconbo �͂܂��g������ - MIN_COMPRESS ��������߂�
				maxconbo += MIN_COMPRESS ;

				// �o�͂��鑊�΃A�h���X�͂O��( ���݂̃A�h���X�|�P )��}���̂ŁA�|�P�������̂��o�͂���
				maxaddress -- ;

				// ���΃A�h���X���o��
				*dp++ = (u8)( maxaddress ) ;
				if( maxaddresssize > 0 )
				{
					*dp++ = (u8)( maxaddress >> 8 ) ;
					if( maxaddresssize == 2 )
						*dp++ = (u8)( maxaddress >> 16 ) ;
				}
			}
			
			// �o�̓T�C�Y�����Z
			dstsize += 3 + maxaddresssize + maxconbosize ;
			
			// ���X�g�ɏ���ǉ�
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

		// ���k���̕\��
		if( nextprintaddress < srcaddress )
		{
			nextprintaddress = srcaddress + 100 * 1024 ;
		}
	}

	// ���k��̃f�[�^�T�C�Y��ۑ�����
	*((u32 *)&destp[4]) = dstsize + 9 ;

	// �m�ۂ����������̉��
	free( usesublistflagtable ) ;

	// �f�[�^�̃T�C�Y��Ԃ�
	return dstsize + 9 ;
}

// �f�R�[�h( �߂�l:�𓀌�̃T�C�Y  -1 �̓G���[  Dest �� NULL �����邱�Ƃ��\ )
int DXArchive::Decode( void *Src, void *Dest )
{
	u32 srcsize, destsize, code, indexsize, keycode, conbo, index ;
	u8 *srcp, *destp, *dp, *sp ;

	destp = (u8 *)Dest ;
	srcp  = (u8 *)Src ;
	
	// �𓀌�̃f�[�^�T�C�Y�𓾂�
	destsize = *((u32 *)&srcp[0]) ;

	// ���k�f�[�^�̃T�C�Y�𓾂�
	srcsize = *((u32 *)&srcp[4]) - 9 ;

	// �L�[�R�[�h
	keycode = srcp[8] ;
	
	// �o�͐悪�Ȃ��ꍇ�̓T�C�Y�����Ԃ�
	if( Dest == NULL )
		return destsize ;
	
	// �W�J�J�n
	sp  = srcp + 9 ;
	dp  = destp ;
	while( srcsize )
	{
		// �L�[�R�[�h�������ŏ����𕪊�
		if( sp[0] != keycode )
		{
			// �񈳏k�R�[�h�̏ꍇ�͂��̂܂܏o��
			*dp = *sp ;
			dp      ++ ;
			sp      ++ ;
			srcsize -- ;
			continue ;
		}
	
		// �L�[�R�[�h���A�����Ă����ꍇ�̓L�[�R�[�h���̂��o��
		if( sp[1] == keycode )
		{
			*dp = (u8)keycode ;
			dp      ++ ;
			sp      += 2 ;
			srcsize -= 2 ;
			
			continue ;
		}

		// ���o�C�g�𓾂�
		code = sp[1] ;

		// �����L�[�R�[�h�����傫�Ȓl�������ꍇ�̓L�[�R�[�h
		// �Ƃ̃o�b�e�B���O�h�~�ׂ̈Ɂ{�P���Ă���̂Ł|�P����
		if( code > keycode ) code -- ;

		sp      += 2 ;
		srcsize -= 2 ;

		// �A�������擾����
		conbo = code >> 3 ;
		if( code & ( 0x1 << 2 ) )
		{
			conbo |= *sp << 5 ;
			sp      ++ ;
			srcsize -- ;
		}
		conbo += MIN_COMPRESS ;	// �ۑ����Ɍ��Z�����ŏ����k�o�C�g���𑫂�

		// �Q�Ƒ��΃A�h���X���擾����
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
		index ++ ;		// �ۑ����Ɂ|�P���Ă���̂Ł{�P����

		// �W�J
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

	// �𓀌�̃T�C�Y��Ԃ�
	return (int)destsize ;
}

// �o�C�i���f�[�^������ SHA-256 �̃n�b�V���l���v�Z����( DestBuffer �̎����A�h���X��擪�� 32byte �n�b�V���l���������܂�܂� )
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


// �A�[�J�C�u�t�@�C�����쐬����(�f�B���N�g�������)
int DXArchive::EncodeArchiveOneDirectory( char *OutputFileName, char *DirectoryPath, bool Press, const char *KeyString )
{
	int i, FileNum, Result ;
	char **FilePathList, *NameBuffer ;

	// �t�@�C���̐����擾����
	FileNum = GetDirectoryFilePath( DirectoryPath, NULL ) ;
	if( FileNum < 0 ) return -1 ;

	// �t�@�C���̐��̕������t�@�C�����ƃt�@�C���|�C���^�̊i�[�p�̃��������m�ۂ���
	NameBuffer = (char *)malloc( FileNum * ( 256 + sizeof( char * ) ) ) ;

	// �t�@�C���̃p�X���擾����
	GetDirectoryFilePath( DirectoryPath, NameBuffer ) ;

	// �t�@�C���p�X�̃��X�g���쐬����
	FilePathList = (char **)( NameBuffer + FileNum * 256 ) ;
	for( i = 0 ; i < FileNum ; i ++ )
		FilePathList[i] = NameBuffer + i * 256 ;

	// �G���R�[�h
	Result = EncodeArchive( OutputFileName, FilePathList, FileNum, Press, KeyString ) ;

	// �m�ۂ����������̉��
	free( NameBuffer ) ;

	// ���ʂ�Ԃ�
	return Result ;
}

// �A�[�J�C�u�t�@�C�����쐬����
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

	// ���̍쐬
	KeyV2Create( KeyString, KeyV2 ) ;

	// ��������̕ۑ�
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

	// �t�@�C���ǂݍ��݂Ɏg�p����o�b�t�@�̊m��
	TempBuffer = malloc( DXA_BUFFERSIZE ) ;

	// �o�̓t�@�C�����J��
	DestFp = fopen( OutputFileName, "wb" ) ;

	// �A�[�J�C�u�̃w�b�_���o�͂���
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
	
	// �e�o�b�t�@���m�ۂ���
	NameP = ( u8 * )malloc( DXA_BUFFERSIZE ) ;
	if( NameP == NULL ) return -1 ;
	memset( NameP, 0, DXA_BUFFERSIZE ) ;

	FileP = ( u8 * )malloc( DXA_BUFFERSIZE ) ;
	if( FileP == NULL ) return -1 ;
	memset( FileP, 0, DXA_BUFFERSIZE ) ;

	DirP = ( u8 * )malloc( DXA_BUFFERSIZE ) ;
	if( DirP == NULL ) return -1 ;
	memset( DirP, 0, DXA_BUFFERSIZE ) ;

	// �T�C�Y�ۑ��\���̂Ƀf�[�^���Z�b�g
	SizeSave.DataSize		= 0 ;
	SizeSave.NameSize		= 0 ;
	SizeSave.DirectorySize	= 0 ;
	SizeSave.FileSize		= 0 ;
	
	// �ŏ��̃f�B���N�g���̃t�@�C�����������o��
	{
		DARC_FILEHEAD File ;
		
		memset( &File, 0, sizeof( DARC_FILEHEAD ) ) ;
		File.NameAddress	= SizeSave.NameSize ;
		File.Attributes		= FILE_ATTRIBUTE_DIRECTORY ;
		File.DataAddress	= SizeSave.DirectorySize ;
		File.DataSize		= 0 ;
		File.PressDataSize  = 0xffffffffffffffff ;

		// �f�B���N�g�����̏����o��
		SizeSave.NameSize += AddFileNameData( ( int )Head.CharCodeFormat, "", NameP + SizeSave.NameSize ) ;

		// �t�@�C�����̏����o��
		memcpy( FileP + SizeSave.FileSize, &File, sizeof( DARC_FILEHEAD ) ) ;
		SizeSave.FileSize += sizeof( DARC_FILEHEAD ) ;
	}

	// �ŏ��̃f�B���N�g�����������o��
	Directory.DirectoryAddress 			= 0 ;
	Directory.ParentDirectoryAddress 	= 0xffffffffffffffff ;
	Directory.FileHeadNum 				= FileNum ;
	Directory.FileHeadAddress 			= SizeSave.FileSize ;
	memcpy( DirP + SizeSave.DirectorySize, &Directory, sizeof( DARC_DIRECTORY ) ) ;
	DirectoryP = ( DARC_DIRECTORY * )( DirP + SizeSave.DirectorySize ) ;

	// �T�C�Y�����Z����
	SizeSave.DirectorySize 	+= sizeof( DARC_DIRECTORY ) ;
	SizeSave.FileSize 		+= sizeof( DARC_FILEHEAD ) * FileNum ;

	// �n���ꂽ�t�@�C���̐������������J��Ԃ�
	for( i = 0 ; i < FileNum ; i ++ )
	{
		// �w�肳�ꂽ�t�@�C�������邩�ǂ�������
		Type = GetFileAttributesA( FileOrDirectoryPath[i] ) ;
		if( ( signed int )Type == -1 ) continue ;

		// �t�@�C���̃^�C�v�ɂ���ď����𕪊�
		if( ( Type & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
		{
			// �f�B���N�g���̏ꍇ�̓f�B���N�g���̃A�[�J�C�u�ɉ�
			DirectoryEncode( ( int )Head.CharCodeFormat, FileOrDirectoryPath[i], NameP, DirP, FileP, &Directory, &SizeSave, i, DestFp, TempBuffer, Press, KeyV2String, KeyV2StringBytes, KeyV2StringBuffer ) ;
		}
		else
		{
			WIN32_FIND_DATAA FindData ;
			HANDLE FindHandle ;
			DARC_FILEHEAD File ;
			u8 lKeyV2[DXA_KEYV2_LENGTH] ;
			size_t KeyV2StringBufferBytes ;
	
			// �t�@�C���̏��𓾂�
			FindHandle = FindFirstFileA( FileOrDirectoryPath[i], &FindData ) ;
			if( FindHandle == INVALID_HANDLE_VALUE ) continue ;
			
			// �t�@�C���w�b�_���Z�b�g����
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

			// �t�@�C�����������o��
			SizeSave.NameSize += AddFileNameData( ( int )Head.CharCodeFormat, FindData.cFileName, NameP + SizeSave.NameSize ) ;

			// �t�@�C���ʂ̌����쐬
			KeyV2StringBufferBytes = CreateKeyV2FileString( ( int )Head.CharCodeFormat, KeyV2String, KeyV2StringBytes, DirectoryP, &File, FileP, DirP, NameP, ( BYTE * )KeyV2StringBuffer ) ;
			KeyV2Create( KeyV2StringBuffer, lKeyV2 , KeyV2StringBufferBytes ) ;

			// �t�@�C���f�[�^�������o��
			if( File.DataSize != 0 )
			{
				FILE *SrcP ;
				u64 FileSize, WriteSize, MoveSize ;

				// �t�@�C�����J��
				SrcP = fopen( FileOrDirectoryPath[i], "rb" ) ;
				
				// �T�C�Y�𓾂�
				_fseeki64( SrcP, 0, SEEK_END ) ;
				FileSize = _ftelli64( SrcP ) ;
				_fseeki64( SrcP, 0, SEEK_SET ) ;
				
				// �t�@�C���T�C�Y�� 10MB �ȉ��̏ꍇ�ŁA���k�̎w�肪����ꍇ�͈��k�����݂�
				if( Press == true && File.DataSize < 10 * 1024 * 1024 )
				{
					void *SrcBuf, *DestBuf ;
					u32 DestSize, Len ;
					
					// �ꕔ�̃t�@�C���`���̏ꍇ�͗\�ߒe��
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
					
					// �f�[�^���ۂ��Ɠ��郁�����̈�̊m��
					SrcBuf  = malloc( ( size_t )( FileSize + FileSize * 2 + 64 ) ) ;
					DestBuf = (u8 *)SrcBuf + FileSize ;
					
					// �t�@�C�����ۂ��Ɠǂݍ���
					fread64( SrcBuf, FileSize, SrcP ) ;
					
					// ���k
					DestSize = Encode( SrcBuf, ( u32 )FileSize, DestBuf ) ;
					
					// �w�ǈ��k�o���Ȃ������ꍇ�͈��k�����ŃA�[�J�C�u����
					if( (f64)DestSize / (f64)FileSize > 0.90 )
					{
						_fseeki64( SrcP, 0L, SEEK_SET ) ;
						free( SrcBuf ) ;
						goto NOPRESS ;
					}
					
					// ���k�f�[�^�𔽓]���ď����o��
					WriteSize = ( DestSize + 3 ) / 4 * 4 ;
					KeyV2ConvFileWrite( DestBuf, WriteSize, DestFp, lKeyV2, File.DataSize ) ;
					
					// �������̉��
					free( SrcBuf ) ;
					
					// ���k�f�[�^�̃T�C�Y��ۑ�����
					File.PressDataSize = DestSize ;
				}
				else
				{
NOPRESS:					
					// �]���J�n
					WriteSize = 0 ;
					while( WriteSize < FileSize )
					{
						// �]���T�C�Y����
						MoveSize = DXA_BUFFERSIZE < FileSize - WriteSize ? DXA_BUFFERSIZE : FileSize - WriteSize ;
						MoveSize = ( MoveSize + 3 ) / 4 * 4 ;	// �T�C�Y�͂S�̔{���ɍ��킹��
						
						// �t�@�C���̔��]�ǂݍ���
						KeyV2ConvFileRead( TempBuffer, MoveSize, SrcP, lKeyV2, File.DataSize + WriteSize ) ;

						// �����o��
						fwrite64( TempBuffer, MoveSize, DestFp ) ;
						
						// �����o���T�C�Y�̉��Z
						WriteSize += MoveSize ;
					}
				}
				
				// �����o�����t�@�C�������
				fclose( SrcP ) ;
			
				// �f�[�^�T�C�Y�̉��Z
				SizeSave.DataSize += WriteSize ;
			}
			
			// �t�@�C���w�b�_�������o��
			memcpy( FileP + Directory.FileHeadAddress + sizeof( DARC_FILEHEAD ) * i, &File, sizeof( DARC_FILEHEAD ) ) ;

			// Find �n���h�������
			FindClose( FindHandle ) ;
		}
	}
	
	// �o�b�t�@�ɗ��ߍ��񂾊e��w�b�_�f�[�^���o�͂���
	{
		// �o�͂��鏇�Ԃ� �t�@�C���l�[���e�[�u���A DARC_FILEHEAD �e�[�u���A DARC_DIRECTORY �e�[�u�� �̏�
		KeyV2ConvFileWrite( NameP, SizeSave.NameSize,      DestFp, KeyV2, 0 ) ;
		KeyV2ConvFileWrite( FileP, SizeSave.FileSize,      DestFp, KeyV2, SizeSave.NameSize ) ;
		KeyV2ConvFileWrite( DirP,  SizeSave.DirectorySize, DestFp, KeyV2, SizeSave.NameSize + SizeSave.FileSize ) ;
	}
		
	// �ĂуA�[�J�C�u�̃w�b�_���o�͂���
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
	
	// �����o�����t�@�C�������
	fclose( DestFp ) ;
	
	// �m�ۂ����o�b�t�@���J������
	free( NameP ) ;
	free( FileP ) ;
	free( DirP ) ;
	free( TempBuffer ) ;

	// �I��
	return 0 ;
}

// �A�[�J�C�u�t�@�C����W�J����
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

	// ���̍쐬
	KeyCreate( KeyString, Key ) ;
	KeyV2Create( KeyString, KeyV2 ) ;

	// ��������̕ۑ�
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

	// �A�[�J�C�u�t�@�C�����J��
	ArcP = fopen( ArchiveName, "rb" ) ;
	if( ArcP == NULL ) return -1 ;

	// �o�͐�̃f�B���N�g���ɃJ�����g�f�B���N�g����ύX����
	GetCurrentDirectoryA( MAX_PATH, OldDir ) ;
	SetCurrentDirectoryA( OutputPath ) ;

	// �w�b�_����͂���
	{
		KeyV2ConvFileRead( &Head, sizeof( DARC_HEAD ), ArcP, KeyV2, 0 ) ;
		
		// �h�c�̌���
		if( Head.Head != DXA_HEAD )
		{
			// �o�[�W�����U�ȑO�����ׂ�
			fseek( ArcP, 0L, SEEK_SET ) ;
			KeyConvFileRead( &Head, sizeof( DARC_HEAD ), ArcP, Key, 0 ) ;

			if( Head.Head != DXA_HEAD )
			{
				goto ERR ;
			}
		}
		
		// �o�[�W��������
		if( Head.Version > DXA_VER || Head.Version < 0x0006 ) goto ERR ;
		
		// �w�b�_�̃T�C�Y���̃��������m�ۂ���
		HeadBuffer = ( u8 * )malloc( ( size_t )Head.HeadSize ) ;
		if( HeadBuffer == NULL ) goto ERR ;
		
		// �w�b�_�p�b�N���������ɓǂݍ���
		_fseeki64( ArcP, Head.FileNameTableStartAddress, SEEK_SET ) ;
		if( Head.Version >= DXA_KEYV2_VER )
		{
			KeyV2ConvFileRead( HeadBuffer, Head.HeadSize, ArcP, KeyV2, 0 ) ;
		}
		else
		{
			KeyConvFileRead( HeadBuffer, Head.HeadSize, ArcP, Key, 0 ) ;
		}
		
		// �e�A�h���X���Z�b�g����
		NameP = HeadBuffer ;
		FileP = NameP + Head.FileTableStartAddress ;
		DirP  = NameP + Head.DirectoryTableStartAddress ;
	}

	// �A�[�J�C�u�̓W�J���J�n����
	DirectoryDecode( NameP, DirP, FileP, &Head, ( DARC_DIRECTORY * )DirP, ArcP, Key, KeyV2String, KeyV2StringBytes, KeyV2StringBuffer ) ;
	
	// �t�@�C�������
	fclose( ArcP ) ;
	
	// �w�b�_��ǂݍ���ł������������������
	free( HeadBuffer ) ;

	// �J�����g�f�B���N�g�������ɖ߂�
	SetCurrentDirectoryA( OldDir ) ;

	// �I��
	return 0 ;

ERR :
	if( HeadBuffer != NULL ) free( HeadBuffer ) ;
	if( ArcP != NULL ) fclose( ArcP ) ;

	// �J�����g�f�B���N�g�������ɖ߂�
	SetCurrentDirectoryA( OldDir ) ;

	// �I��
	return -1 ;
}



// �R���X�g���N�^
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

// �f�X�g���N�^
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

// �w��̃f�B���N�g���f�[�^�̈Í�������������( �ۂ��ƃ������ɓǂݍ��񂾏ꍇ�p )
int DXArchive::DirectoryKeyConv( DARC_DIRECTORY *Dir, char *KeyV2StringBuffer )
{
	// �������C���[�W�ł͂Ȃ��ꍇ�̓G���[
	if( this->MemoryOpenFlag == false )
		return -1 ;

	// �o�[�W���� 0x0005 ���O�ł͉������Ȃ�
	if( this->Head.Version < 0x0005 )
		return 0 ;
	
	// �Í������������J�n
	{
		u32 i, FileHeadSize ;
		DARC_FILEHEAD *File ;
		unsigned char lKeyV2[DXA_KEYV2_LENGTH] ;
		size_t KeyV2StringBufferBytes ;

		// �i�[����Ă���t�@�C���̐������J��Ԃ�
		FileHeadSize = sizeof( DARC_FILEHEAD ) ;
		File = ( DARC_FILEHEAD * )( this->FileP + Dir->FileHeadAddress ) ;
		for( i = 0 ; i < Dir->FileHeadNum ; i ++, File = (DARC_FILEHEAD *)( (u8 *)File + FileHeadSize ) )
		{
			// �f�B���N�g�����ǂ����ŏ����𕪊�
			if( File->Attributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// �f�B���N�g���̏ꍇ�͍ċA��������
				DirectoryKeyConv( ( DARC_DIRECTORY * )( this->DirP + File->DataAddress ), KeyV2StringBuffer ) ;
			}
			else
			{
				u8 *DataP ;

				// �t�@�C���̏ꍇ�͈Í�������������
				
				// �f�[�^������ꍇ�̂ݏ���
				if( File->DataSize != 0 )
				{
					// �f�[�^�ʒu���Z�b�g����
					DataP = ( u8 * )this->fp + this->Head.DataStartAddress + File->DataAddress ;

					// �f�[�^�����k����Ă��邩�ǂ����ŏ����𕪊�
					if( File->PressDataSize != 0xffffffffffffffff )
					{
						// ���k����Ă���ꍇ
						if( this->Head.Version >= DXA_KEYV2_VER )
						{
							// �t�@�C���ʂ̌����쐬
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
						// ���k����Ă��Ȃ��ꍇ
						if( this->Head.Version >= DXA_KEYV2_VER )
						{
							// �t�@�C���ʂ̌����쐬
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

	// �I��
	return 0 ;
}

// ��������ɂ���A�[�J�C�u�C���[�W���J��( 0:����  -1:���s )
int	DXArchive::OpenArchiveMem( void *ArchiveImage, s64 ArchiveSize, const char *KeyString )
{
	u8 *datp ;

	// ���ɂȂ�炩�̃A�[�J�C�u���J���Ă����ꍇ�̓G���[
	if( this->fp != NULL ) return -1 ;

	// ���̍쐬
	KeyCreate( KeyString, this->Key ) ;
	KeyV2Create( KeyString, this->KeyV2 ) ;

	// ��������̕ۑ�
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

	// �ŏ��Ƀw�b�_�̕����𔽓]����
	memcpy( &this->Head, ArchiveImage, sizeof( DARC_HEAD ) ) ;
	KeyV2Conv( &this->Head, sizeof( DARC_HEAD ), 0, this->KeyV2 ) ;

	// �h�c���Ⴄ�ꍇ�̓G���[
	if( Head.Head != DXA_HEAD )
	{
		// �o�[�W�����U�����ׂ�
		memcpy( &this->Head, ArchiveImage, sizeof( DARC_HEAD ) ) ;
		KeyConv( &this->Head, sizeof( DARC_HEAD ), 0, this->Key ) ;

		// �h�c���Ⴄ�ꍇ�̓G���[
		if( Head.Head != DXA_HEAD )
		{
			goto ERR ;
		}
	}

	// �|�C���^��ۑ�
	this->fp = (FILE *)ArchiveImage ;
	datp = (u8 *)ArchiveImage ;

	// �w�b�_����͂���
	{
		datp += sizeof( DARC_HEAD ) ;
		
		// �o�[�W��������
		if( this->Head.Version > DXA_VER || this->Head.Version < 0x0006 ) goto ERR ;

		// �w�b�_�p�b�N�̃A�h���X���Z�b�g����
		this->HeadBuffer = (u8 *)this->fp + this->Head.FileNameTableStartAddress ;

		// �e�A�h���X���Z�b�g����
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

	// �J�����g�f�B���N�g���̃Z�b�g
	this->CurrentDirectory = ( DARC_DIRECTORY * )this->DirP ;

	// �������C���[�W����J���Ă���t���O�𗧂Ă�
	MemoryOpenFlag = true ;

	// �S�Ẵt�@�C���̈Í�������������
	{
		char KeyV2StringBuffer[ DXA_KEYV2_STRING_MAXLENGTH ] ;
		DirectoryKeyConv( ( DARC_DIRECTORY * )this->DirP, KeyV2StringBuffer ) ;
	}

	// ���[�U�[�̃C���[�W����J�����t���O�𗧂Ă�
	UserMemoryImageFlag = true ;

	// �T�C�Y���ۑ����Ă���
	MemoryImageSize = ArchiveSize ;

	// �I��
	return 0 ;

ERR :
	// �I��
	return -1 ;
}

// �A�[�J�C�u�t�@�C�����J���ŏ��ɂ��ׂă�������ɓǂݍ���ł��珈������( 0:����  -1:���s )
int DXArchive::OpenArchiveFileMem( const char *ArchivePath, const char *KeyString )
{
	FILE *fp ;
	u8 *datp ;
	void *ArchiveImage ;
	s64 ArchiveSize ;

	// ���ɂȂ�炩�̃A�[�J�C�u���J���Ă����ꍇ�̓G���[
	if( this->fp != NULL ) return -1 ;

	// ���̍쐬
	KeyCreate( KeyString, this->Key ) ;
	KeyV2Create( KeyString, this->KeyV2 ) ;

	// ��������̕ۑ�
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

	// �������ɓǂݍ���
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

	// �ŏ��Ƀw�b�_�̕����𔽓]����
	memcpy( &this->Head, ArchiveImage, sizeof( DARC_HEAD ) ) ;
	KeyV2Conv( &this->Head, sizeof( DARC_HEAD ), 0, this->KeyV2 ) ;

	// �h�c���Ⴄ�ꍇ�̓G���[
	if( Head.Head != DXA_HEAD )
	{
		// �o�[�W�����U�����ׂ�
		memcpy( &this->Head, ArchiveImage, sizeof( DARC_HEAD ) ) ;
		KeyConv( &this->Head, sizeof( DARC_HEAD ), 0, this->Key ) ;

		// �h�c���Ⴄ�ꍇ�̓G���[
		if( Head.Head != DXA_HEAD )
			return -1 ;
	}

	// �|�C���^��ۑ�
	this->fp = (FILE *)ArchiveImage ;
	datp = (u8 *)ArchiveImage ;

	// �w�b�_����͂���
	{
		datp += sizeof( DARC_HEAD ) ;
		
		// �o�[�W��������
		if( this->Head.Version > DXA_VER || this->Head.Version < 0x0006 ) goto ERR ;

		// �w�b�_�p�b�N�̃A�h���X���Z�b�g����
		this->HeadBuffer = (u8 *)this->fp + this->Head.FileNameTableStartAddress ;

		// �e�A�h���X���Z�b�g����
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

	// �J�����g�f�B���N�g���̃Z�b�g
	this->CurrentDirectory = ( DARC_DIRECTORY * )this->DirP ;

	// �������C���[�W����J���Ă���t���O�𗧂Ă�
	MemoryOpenFlag = true ;

	// �S�Ẵt�@�C���̈Í�������������
	{
		char KeyV2StringBuffer[ DXA_KEYV2_STRING_MAXLENGTH ] ;
		DirectoryKeyConv( ( DARC_DIRECTORY * )this->DirP, KeyV2StringBuffer ) ;
	}
	
	// ���[�U�[�̃C���[�W����J�����킯�ł͂Ȃ��̂Ńt���O��|��
	UserMemoryImageFlag = false ;

	// �T�C�Y���ۑ����Ă���
	MemoryImageSize = ArchiveSize ;

	// �I��
	return 0 ;

ERR :
	
	// �I��
	return -1 ;
}

// �A�[�J�C�u�t�@�C�����J��
int DXArchive::OpenArchiveFile( const char *ArchivePath, const char *KeyString )
{
	// ���ɂȂ�炩�̃A�[�J�C�u���J���Ă����ꍇ�̓G���[
	if( this->fp != NULL ) return -1 ;

	// �A�[�J�C�u�t�@�C�����J�����Ǝ��݂�
	this->fp = fopen( ArchivePath, "rb" ) ;
	if( this->fp == NULL ) return -1 ;

	// ��������̍쐬
	KeyCreate( KeyString, this->Key ) ;
	KeyV2Create( KeyString, this->KeyV2 ) ;

	// ��������̕ۑ�
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

	// �w�b�_����͂���
	{
		KeyV2ConvFileRead( &this->Head, sizeof( DARC_HEAD ), this->fp, this->KeyV2, 0 ) ;
		
		// �h�c�̌���
		if( this->Head.Head != DXA_HEAD )
		{
			// �o�[�W�����U�ȑO�����ׂ�
			fseek( this->fp, 0L, SEEK_SET ) ;
			KeyConvFileRead( &this->Head, sizeof( DARC_HEAD ), this->fp, this->Key, 0 ) ;

			if( this->Head.Head != DXA_HEAD )
			{
				goto ERR ;
			}
		}
		
		// �o�[�W��������
		if( this->Head.Version > DXA_VER || this->Head.Version < 0x0006 ) goto ERR ;
		
		// �w�b�_�̃T�C�Y���̃��������m�ۂ���
		this->HeadBuffer = ( u8 * )malloc( ( size_t )this->Head.HeadSize ) ;
		if( this->HeadBuffer == NULL ) goto ERR ;
		
		// �w�b�_�p�b�N���������ɓǂݍ���
		_fseeki64( this->fp, this->Head.FileNameTableStartAddress, SEEK_SET ) ;
		if( this->Head.Version >= DXA_KEYV2_VER )
		{
			KeyV2ConvFileRead( this->HeadBuffer, this->Head.HeadSize, this->fp, this->KeyV2, 0 ) ;
		}
		else
		{
			KeyConvFileRead( this->HeadBuffer, this->Head.HeadSize, this->fp, this->Key, 0 ) ;
		}
		
		// �e�A�h���X���Z�b�g����
		this->NameP = this->HeadBuffer ;
		this->FileP = this->NameP + this->Head.FileTableStartAddress ;
		this->DirP = this->NameP + this->Head.DirectoryTableStartAddress ;
	}

	// �J�����g�f�B���N�g���̃Z�b�g
	this->CurrentDirectory = ( DARC_DIRECTORY * )this->DirP ;

	// �������C���[�W����J���Ă���A�t���O��|��
	MemoryOpenFlag = false ;

	// �I��
	return 0 ;

ERR :
	if( this->fp != NULL ){ fclose( this->fp ) ; this->fp = NULL ; }
	if( this->HeadBuffer != NULL ){ free( this->HeadBuffer ) ; this->HeadBuffer = NULL ; }
	
	// �I��
	return -1 ;
}

// �A�[�J�C�u�t�@�C�������
int DXArchive::CloseArchiveFile( void )
{
	// ���ɕ��Ă����牽�������I��
	if( this->fp == NULL ) return 0 ;

	// ����������J���Ă��邩�ǂ����ŏ����𕪊�
	if( MemoryOpenFlag == true )
	{
		// �A�[�J�C�u�N���X���ǂݍ��񂾏ꍇ�Ƃ����łȂ��ꍇ�ŏ����𕪊�
		if( UserMemoryImageFlag == true )
		{
			char KeyV2StringBuffer[ DXA_KEYV2_STRING_MAXLENGTH ] ;

			// ���]�����f�[�^�����ɖ߂�
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
			// �m�ۂ��Ă������������J������
			free( this->fp ) ;
		}
	}
	else
	{
		// �A�[�J�C�u�t�@�C�������
		fclose( this->fp ) ;
		
		// �w�b�_�o�b�t�@�����
		free( this->HeadBuffer ) ;
	}

	// �|�C���^������
	this->fp = NULL ;
	this->HeadBuffer = NULL ;
	this->NameP = this->DirP = this->FileP = NULL ;
	this->CurrentDirectory = NULL ;

	// �I��
	return 0 ;
}

// �A�[�J�C�u���̃f�B���N�g���p�X��ύX����( 0:����  -1:���s )
int	DXArchive::ChangeCurrentDirectoryFast( SEARCHDATA *SearchData )
{
	DARC_FILEHEAD *FileH ;
	int i, j, k, Num ;
	u8 *NameData, *PathData ;
	u16 PackNum, Parity ;

	PackNum  = SearchData->PackNum ;
	Parity   = SearchData->Parity ;
	PathData = SearchData->FileName ;

	// �J�����g�f�B���N�g�����瓯���̃f�B���N�g����T��
	FileH = ( DARC_FILEHEAD * )( this->FileP + this->CurrentDirectory->FileHeadAddress ) ;
	Num = (s32)this->CurrentDirectory->FileHeadNum ;
	for( i = 0 ; i < Num ; i ++, FileH ++ )
	{
		// �f�B���N�g���`�F�b�N
		if( ( FileH->Attributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 ) continue ;

		// �����񐔂ƃp���e�B�`�F�b�N
		NameData = this->NameP + FileH->NameAddress ;
		if( PackNum != ((u16 *)NameData)[0] || Parity != ((u16 *)NameData)[1] ) continue ;

		// ������`�F�b�N
		NameData += 4 ;
		for( j = 0, k = 0 ; j < PackNum ; j ++, k += 4 )
			if( *((u32 *)&PathData[k]) != *((u32 *)&NameData[k]) ) break ;

		// �K�������f�B���N�g�����������炱���ŏI��
		if( PackNum == j ) break ;
	}

	// ����������G���[
	if( i == Num ) return -1 ;

	// �݂�����J�����g�f�B���N�g����ύX
	this->CurrentDirectory = ( DARC_DIRECTORY * )( this->DirP + FileH->DataAddress ) ;

	// ����I��
	return 0 ;
}

// �A�[�J�C�u���̃f�B���N�g���p�X��ύX����( 0:����  -1:���s )
int DXArchive::ChangeCurrentDir( const char *DirPath )
{
	return ChangeCurrentDirectoryBase( DirPath, true ) ;
}

// �A�[�J�C�u���̃f�B���N�g���p�X��ύX����( 0:����  -1:���s )
int	DXArchive::ChangeCurrentDirectoryBase( const char *DirectoryPath, bool ErrorIsDirectoryReset, SEARCHDATA *LastSearchData )
{
	DARC_DIRECTORY *OldDir ;
	SEARCHDATA SearchData ;

	// �����ɗ��܂�p�X�������疳��
	if( strcmp( DirectoryPath, "." ) == 0 ) return 0 ;

	// �w\�x�����̏ꍇ�̓��[�g�f�B���N�g���ɖ߂�
	if( strcmp( DirectoryPath, "\\" ) == 0 )
	{
		this->CurrentDirectory = ( DARC_DIRECTORY * )this->DirP ;
		return 0 ;
	}

	// ���Ɉ������p�X�������珈���𕪊�
	if( strcmp( DirectoryPath, ".." ) == 0 )
	{
		// ���[�g�f�B���N�g���ɋ�����G���[
		if( this->CurrentDirectory->ParentDirectoryAddress == 0xffffffffffffffff ) return -1 ;
		
		// �e�f�B���N�g�����������炻����Ɉڂ�
		this->CurrentDirectory = ( DARC_DIRECTORY * )( this->DirP + this->CurrentDirectory->ParentDirectoryAddress ) ;
		return 0 ;
	}

	// ����ȊO�̏ꍇ�͎w��̖��O�̃f�B���N�g����T��
	
	// �ύX�ȑO�̃f�B���N�g����ۑ����Ă���
	OldDir = this->CurrentDirectory ;

	// �p�X���Ɂw\�x�����邩�ǂ����ŏ����𕪊�
	if( strchr( DirectoryPath, '\\' ) == NULL )
	{
		// �t�@�C������������p�̌`���ɕϊ�����
		ConvSearchData( &SearchData, DirectoryPath, NULL ) ;

		// �f�B���N�g����ύX
		if( ChangeCurrentDirectoryFast( &SearchData ) < 0 ) goto ERR ;

/*		// \ �������ꍇ�́A�����̃f�B���N�g����T��
		FileH = ( DARC_FILEHEAD * )( this->FileP + this->CurrentDirectory->FileHeadAddress ) ;
		for( i = 0 ;
			 i < (s32)this->CurrentDirectory->FileHeadNum &&
			 StrICmp( ( char * )( this->NameP + FileH->NameAddress ), DirectoryPath ) != 0 ;
			 i ++, FileH ++ ){}
*/
	}
	else
	{
		// \ ������ꍇ�͌q�������f�B���N�g������ÂύX���Ă䂭
	
		int Point, StrLength ;

		Point = 0 ;
		// ���[�v
		for(;;)
		{
			// ��������擾����
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
			// ���������[�� \ �������ꍇ�̓��[�g�f�B���N�g���ɗ��Ƃ�
			if( StrLength == 0 && DirectoryPath[Point] == '\\' )
			{
				this->ChangeCurrentDirectoryBase( "\\", false ) ;
			}
			else
			{
				// ����ȊO�̏ꍇ�͕��ʂɃf�B���N�g���ύX
				if( this->ChangeCurrentDirectoryFast( &SearchData ) < 0 )
				{
					// �G���[���N���āA�X�ɃG���[���N�������Ɍ��̃f�B���N�g���ɖ߂���
					// �t���O�������Ă���ꍇ�͌��̃f�B���N�g���ɖ߂�
					if( ErrorIsDirectoryReset == true ) this->CurrentDirectory = OldDir ;

					// �G���[�I��
					goto ERR ;
				}
			}

			// �����I�[�����ŏI�������ꍇ�̓��[�v���甲����
			// ���͂��� \ �����Ȃ��ꍇ�����[�v���甲����
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

	// ����I��
	return 0 ;

ERR:
	if( LastSearchData != NULL )
	{
		memcpy( LastSearchData->FileName, SearchData.FileName, SearchData.PackNum * 4 ) ;
		LastSearchData->Parity  = SearchData.Parity ;
		LastSearchData->PackNum = SearchData.PackNum ;
	}

	// �G���[�I��
	return -1 ;
}
		
// �A�[�J�C�u���̃J�����g�f�B���N�g���p�X���擾����
int DXArchive::GetCurrentDir( char *DirPathBuffer, int BufferLength )
{
	char DirPath[MAX_PATH] ;
	DARC_DIRECTORY *Dir[200], *DirTempP ;
	int Depth, i ;

	// ���[�g�f�B���N�g���ɒ����܂Ō�������
	Depth = 0 ;
	DirTempP = this->CurrentDirectory ;
	while( DirTempP->DirectoryAddress != 0xffffffffffffffff && DirTempP->DirectoryAddress != 0 )
	{
		Dir[Depth] = DirTempP ;
		DirTempP = ( DARC_DIRECTORY * )( this->DirP + DirTempP->ParentDirectoryAddress ) ;
		Depth ++ ;
	}
	
	// �p�X����A������
	DirPath[0] = '\0' ;
	for( i = Depth - 1 ; i >= 0 ; i -- )
	{
		strcat( DirPath, "\\" ) ;
		strcat( DirPath, ( char * )( this->NameP + ( ( DARC_FILEHEAD * )( this->FileP + Dir[i]->DirectoryAddress ) )->NameAddress ) ) ;
	}

	// �o�b�t�@�̒������O���A����������Ȃ��Ƃ��̓f�B���N�g�����̒�����Ԃ�
	if( BufferLength == 0 || BufferLength < (s32)strlen( DirPath ) )
	{
		return ( int )( strlen( DirPath ) + 1 ) ;
	}
	else
	{
		// �f�B���N�g�������o�b�t�@�ɓ]������
		strcpy( DirPathBuffer, DirPath ) ;
	}

	// �I��
	return 0 ;
}



// �A�[�J�C�u�t�@�C�����̎w��̃t�@�C�����������ɓǂݍ���
s64 DXArchive::LoadFileToMem( const char *FilePath, void *Buffer, u64 BufferLength )
{
	DARC_FILEHEAD *FileH ;
	char KeyV2StringBuffer[ DXA_KEYV2_STRING_MAXLENGTH ] ;
	size_t KeyV2StringBufferBytes ;
	unsigned char lKeyV2[ DXA_KEYV2_LENGTH ] ;
	DARC_DIRECTORY *Directory ;

	// �w��̃t�@�C���̏��𓾂�
	FileH = this->GetFileInfo( FilePath, &Directory ) ;
	if( FileH == NULL ) return -1 ;

	// �t�@�C���T�C�Y������Ă��邩���ׂ�A����Ă��Ȃ����A�o�b�t�@�A���̓T�C�Y���O��������T�C�Y��Ԃ�
	if( BufferLength < FileH->DataSize || BufferLength == 0 || Buffer == NULL )
	{
		return ( s64 )FileH->DataSize ;
	}

	// ����Ă���ꍇ�̓o�b�t�@�[�ɓǂݍ���

	// �t�@�C�������k����Ă��邩�ǂ����ŏ����𕪊�
	if( FileH->PressDataSize != 0xffffffffffffffff )
	{
		// ���k����Ă���ꍇ

		// ��������ɓǂݍ���ł��邩�ǂ����ŏ����𕪊�
		if( MemoryOpenFlag == true )
		{
			// ��������̈��k�f�[�^���𓀂���
			Decode( (u8 *)this->fp + this->Head.DataStartAddress + FileH->DataAddress, Buffer ) ;
		}
		else
		{
			void *temp ;

			// ���k�f�[�^���������ɓǂݍ���ł���𓀂���

			// ���k�f�[�^�����܂郁�����̈�̊m��
			temp = malloc( ( size_t )FileH->PressDataSize ) ;

			// ���k�f�[�^�̓ǂݍ���
			_fseeki64( this->fp, this->Head.DataStartAddress + FileH->DataAddress, SEEK_SET ) ;
			if( this->Head.Version >= DXA_KEYV2_VER )
			{
				// �t�@�C���ʂ̌����쐬
				KeyV2StringBufferBytes = CreateKeyV2FileString( ( int )this->Head.CharCodeFormat, this->KeyV2String, this->KeyV2StringBytes, Directory, FileH, this->FileP, this->DirP, this->NameP, ( BYTE * )KeyV2StringBuffer ) ;
				KeyV2Create( KeyV2StringBuffer, lKeyV2 , KeyV2StringBufferBytes ) ;

				KeyV2ConvFileRead( temp, FileH->PressDataSize, this->fp, lKeyV2, FileH->DataSize ) ;
			}
			else
			{
				KeyConvFileRead( temp, FileH->PressDataSize, this->fp, this->Key, FileH->DataSize ) ;
			}
			
			// ��
			Decode( temp, Buffer ) ;
			
			// �������̉��
			free( temp ) ;
		}
	}
	else
	{
		// ���k����Ă��Ȃ��ꍇ

		// ��������ɓǂݍ���ł��邩�ǂ����ŏ����𕪊�
		if( MemoryOpenFlag == true )
		{
			// �R�s�[
			memcpy( Buffer, (u8 *)this->fp + this->Head.DataStartAddress + FileH->DataAddress, ( size_t )FileH->DataSize ) ;
		}
		else
		{
			// �t�@�C���|�C���^���ړ�
			_fseeki64( this->fp, this->Head.DataStartAddress + FileH->DataAddress, SEEK_SET ) ;

			// �ǂݍ���
			if( this->Head.Version >= DXA_KEYV2_VER )
			{
				// �t�@�C���ʂ̌����쐬
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
	
	// �I��
	return 0 ;
}

// �A�[�J�C�u�t�@�C�����̎w��̃t�@�C�����T�C�Y���擾����
s64 DXArchive::GetFileSize( const char *FilePath )
{
	// �t�@�C���T�C�Y��Ԃ�
	return this->LoadFileToMem( FilePath, NULL, 0 ) ;
}

// �A�[�J�C�u�t�@�C�����������ɓǂݍ��񂾏ꍇ�̃t�@�C���C���[�W���i�[����Ă���擪�A�h���X���擾����( ����������J���Ă���ꍇ�̂ݗL�� )
void *DXArchive::GetFileImage( void )
{
	// �������C���[�W����J���Ă��Ȃ�������G���[
	if( MemoryOpenFlag == false ) return NULL ;

	// �擪�A�h���X��Ԃ�
	return this->fp ;
}

// �A�[�J�C�u�t�@�C�����̎w��̃t�@�C���̃t�@�C�����̈ʒu�ƃt�@�C���̑傫���𓾂�( -1:�G���[ )
int DXArchive::GetFileInfo( const char *FilePath, u64 *PositionP, u64 *SizeP )
{
	DARC_FILEHEAD *FileH ;

	// �w��̃t�@�C���̏��𓾂�
	FileH = this->GetFileInfo( FilePath ) ;
	if( FileH == NULL ) return -1 ;

	// �t�@�C���̃f�[�^������ʒu�ƃt�@�C���T�C�Y��ۑ�����
	if( PositionP != NULL ) *PositionP = this->Head.DataStartAddress + FileH->DataAddress ;
	if( SizeP     != NULL ) *SizeP     = FileH->DataSize ;

	// �����I��
	return 0 ;
}

// �A�[�J�C�u�t�@�C�����̎w��̃t�@�C�����A�N���X���̃o�b�t�@�ɓǂݍ���
void *DXArchive::LoadFileToCash( const char *FilePath )
{
	s64 FileSize ;

	// �t�@�C���T�C�Y���擾����
	FileSize = this->GetFileSize( FilePath ) ;

	// �t�@�C��������������G���[
	if( FileSize < 0 ) return NULL ;

	// �m�ۂ��Ă���L���b�V���o�b�t�@�̃T�C�Y�����傫���ꍇ�̓o�b�t�@���Ċm�ۂ���
	if( FileSize > ( s64 )this->CashBufferSize )
	{
		// �L���b�V���o�b�t�@�̃N���A
		this->ClearCash() ;

		// �L���b�V���o�b�t�@�̍Ċm��
		this->CashBuffer = malloc( ( size_t )FileSize ) ;

		// �m�ۂɎ��s�����ꍇ�� NULL ��Ԃ�
		if( this->CashBuffer == NULL ) return NULL ;

		// �L���b�V���o�b�t�@�̃T�C�Y��ۑ�
		this->CashBufferSize = FileSize ;
	}

	// �t�@�C�����������ɓǂݍ���
	this->LoadFileToMem( FilePath, this->CashBuffer, FileSize ) ;

	// �L���b�V���o�b�t�@�̃A�h���X��Ԃ�
	return this->CashBuffer ;
}

// �L���b�V���o�b�t�@���J������
int DXArchive::ClearCash( void )
{
	// ���������m�ۂ���Ă�����������
	if( this->CashBuffer != NULL )
	{
		free( this->CashBuffer ) ;
		this->CashBuffer = NULL ;
		this->CashBufferSize = 0 ;
	}

	// �I��
	return 0 ;
}

	
// �A�[�J�C�u�t�@�C�����̎w��̃t�@�C�����J���A�t�@�C���A�N�Z�X�p�I�u�W�F�N�g���쐬����
DXArchiveFile *DXArchive::OpenFile( const char *FilePath )
{
	DARC_FILEHEAD *FileH ;
	DARC_DIRECTORY *Directory ;
	DXArchiveFile *CDArc = NULL ;

	// ����������J���Ă���ꍇ�͖���
	if( MemoryOpenFlag == true ) return NULL ;

	// �w��̃t�@�C���̏��𓾂�
	FileH = this->GetFileInfo( FilePath, &Directory ) ;
	if( FileH == NULL ) return NULL ;

	// �V���� DXArchiveFile �N���X���쐬����
	CDArc = new DXArchiveFile( FileH, Directory, this ) ;
	
	// DXArchiveFile �N���X�̃|�C���^��Ԃ�
	return CDArc ;
}













// �R���X�g���N�^
DXArchiveFile::DXArchiveFile( DARC_FILEHEAD *FileHead, DARC_DIRECTORY *Directory, DXArchive *Archive )
{
	this->FileData  = FileHead ;
	this->Archive   = Archive ;
	this->EOFFlag   = FALSE ;
	this->FilePoint = 0 ;
	this->DataBuffer = NULL ;

	// �o�[�W�����V�ȍ~�̏ꍇ�͌��o�[�W�����Q���쐬����
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
	
	// �t�@�C�������k����Ă���ꍇ�͂����œǂݍ���ŉ𓀂��Ă��܂�
	if( FileHead->PressDataSize != 0xffffffffffffffff )
	{
		void *temp ;

		// ���k�f�[�^�����܂郁�����̈�̊m��
		temp = malloc( ( size_t )FileHead->PressDataSize ) ;

		// �𓀃f�[�^�����܂郁�����̈�̊m��
		this->DataBuffer = malloc( ( size_t )FileHead->DataSize ) ;

		// ���k�f�[�^�̓ǂݍ���
		_fseeki64( this->Archive->GetFilePointer(), this->Archive->GetHeader()->DataStartAddress + FileHead->DataAddress, SEEK_SET ) ;
		if( this->Archive->GetHeader()->Version >= DXA_KEYV2_VER )
		{
			DXArchive::KeyV2ConvFileRead( temp, FileHead->PressDataSize, this->Archive->GetFilePointer(), KeyV2, FileHead->DataSize ) ;
		}
		else
		{
			DXArchive::KeyConvFileRead( temp, FileHead->PressDataSize, this->Archive->GetFilePointer(), this->Archive->GetKey(), FileHead->DataSize ) ;
		}
		
		// ��
		DXArchive::Decode( temp, this->DataBuffer ) ;
		
		// �������̉��
		free( temp ) ;
	}
}

// �f�X�g���N�^
DXArchiveFile::~DXArchiveFile()
{
	// �������̉��
	if( this->DataBuffer != NULL )
	{
		free( this->DataBuffer ) ;
		this->DataBuffer = NULL ;
	}
}

// �t�@�C���̓��e��ǂݍ���
s64 DXArchiveFile::Read( void *Buffer, s64 ReadLength )
{
	s64 ReadSize ;

	// EOF �t���O�������Ă�����O��Ԃ�
	if( this->EOFFlag == TRUE ) return 0 ;
	
	// �A�[�J�C�u�t�@�C���|�C���^�ƁA���z�t�@�C���|�C���^����v���Ă��邩���ׂ�
	// ��v���Ă��Ȃ�������A�[�J�C�u�t�@�C���|�C���^���ړ�����
	if( this->DataBuffer == NULL && _ftelli64( this->Archive->GetFilePointer() ) != (s32)( this->FileData->DataAddress + this->Archive->GetHeader()->DataStartAddress + this->FilePoint ) )
	{
		_fseeki64( this->Archive->GetFilePointer(), this->FileData->DataAddress + this->Archive->GetHeader()->DataStartAddress + this->FilePoint, SEEK_SET ) ;
	}
	
	// EOF ���o
	if( this->FileData->DataSize == this->FilePoint )
	{
		this->EOFFlag = TRUE ;
		return 0 ;
	}
	
	// �f�[�^��ǂݍ��ޗʂ�ݒ肷��
	ReadSize = ReadLength < (s64)( this->FileData->DataSize - this->FilePoint ) ? ReadLength : this->FileData->DataSize - this->FilePoint ;
	
	// �f�[�^��ǂݍ���
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
	
	// EOF �t���O��|��
	this->EOFFlag = FALSE ;

	// �ǂݍ��񂾕������t�@�C���|�C���^���ړ�����
	this->FilePoint += ReadSize ;
	
	// �ǂݍ��񂾗e�ʂ�Ԃ�
	return ReadSize ;
}
	
// �t�@�C���|�C���^��ύX����
s64 DXArchiveFile::Seek( s64 SeekPoint, s64 SeekMode )
{
	// �V�[�N�^�C�v�ɂ���ď����𕪊�
	switch( SeekMode )
	{
	case SEEK_SET : break ;		
	case SEEK_CUR : SeekPoint += this->FilePoint ; break ;
	case SEEK_END :	SeekPoint = this->FileData->DataSize + SeekPoint ; break ;
	}
	
	// �␳
	if( SeekPoint > (s64)this->FileData->DataSize ) SeekPoint = this->FileData->DataSize ;
	if( SeekPoint < 0 ) SeekPoint = 0 ;
	
	// �Z�b�g
	this->FilePoint = SeekPoint ;
	
	// EOF�t���O��|��
	this->EOFFlag = FALSE ;
	
	// �I��
	return 0 ;
}

// ���݂̃t�@�C���|�C���^�𓾂�
s64 DXArchiveFile::Tell( void )
{
	return this->FilePoint ;
}

// �t�@�C���̏I�[�ɗ��Ă��邩�A�̃t���O�𓾂�
s64 DXArchiveFile::Eof( void )
{
	return this->EOFFlag ;
}

// �t�@�C���̃T�C�Y���擾����
s64 DXArchiveFile::Size( void )
{
	return this->FileData->DataSize ;
}



