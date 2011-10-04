/********************************************************************************
 *
 * SuperFetch files dumper (Ag*.db)
 *
 * Copyright (C) 2011 ReWolf
 * http://blog.rewolf.pl/
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ********************************************************************************/

#include <cstdio>
#include <windows.h>
#include "xpress.h"

typedef NTSTATUS (__stdcall *__RtlDecompressBuffer)(USHORT CompressionFormat, PUCHAR UncompressedBuffer, ULONG UncompressedBufferSize, PUCHAR CompressedBuffer, ULONG CompressedBufferSize, PULONG FinalUncompressedSize);

enum CompType { Comp_None, Comp_Xpress, Comp_LZNT1 };

#pragma pack(1)
struct PfFileParams
{
	DWORD sizes[9];
};

struct PfFileHeader
{
	DWORD magic;				// = 0xE; ?
	DWORD fileSize;
	DWORD headerSize;			// align this value to 8 after read
	DWORD fileType;				// index to PfDbDatabaseParamsForFileType table
	PfFileParams fileParams;		// 9 dwords
	DWORD volumesCounter;			// number of volumes in file
	DWORD totalEntriesInVolumes;		// ??
	//rest of header is unknown at this moment
};

struct PfVolumeHeader_38	//size 0x38
{
	DWORD unk01;
	DWORD unk02;
	DWORD numOfEntries;
	DWORD unk03;
	DWORD unk04;
	DWORD unk05;
	FILETIME timestamp;
	DWORD volumeID;
	DWORD unk07;
	DWORD unk08;
	WORD nameLen;
	WORD unk09;
	DWORD unk10;
	DWORD unk11;
};

struct PfVolumeHeader_48	//size 0x38
{
	DWORD64 unk01;
	DWORD64 unk02;
	DWORD numOfEntries;
	DWORD unk03;
	DWORD unk04;
	DWORD unk05;
	FILETIME timestamp;
	DWORD volumeID;
	DWORD unk07;
	DWORD64 unk08;
	WORD nameLen;
	WORD unk09;
	DWORD padding;
	DWORD unk10;
	DWORD unk11;
};

struct PfRecordHeader_24
{
	DWORD unk01;
	DWORD nameHash;
	DWORD numOfSubEntries;
	DWORD unk02[4];
	DWORD nameLenExt;		//nameLen multiplied by 4, do shift before use
	DWORD unk03;
};

struct PfRecordHeader_34
{
	DWORD unk01;
	DWORD nameHash;
	DWORD numOfSubEntries;
	DWORD unk02[6];
	DWORD nameLenExt;		//nameLen multiplied by 4, do shift before use
	DWORD unk03[2];
};

struct PfRecordHeader_40
{
	DWORD64 unk01;
	DWORD nameHash;
	DWORD unk02;
	DWORD numOfSubEntries;
	DWORD unk03[7];
	DWORD nameLenExt;		//nameLen multiplied by 4, do shift before use
	DWORD unk04[3];
};

struct PfRecordHeader_48
{
	DWORD unk01;
	DWORD nameHash;
	DWORD numOfSubEntries;
	DWORD unk02[4];
	DWORD nameLenExt;		//nameLen multiplied by 4, do shift before use
	DWORD unk03[10];
};

struct PfRecordHeader_58
{
	DWORD64 unk01;
	DWORD nameHash;
	DWORD unk02;
	DWORD numOfSubEntries;
	DWORD unk03[7];
	DWORD nameLenExt;		//nameLen multiplied by 4, do shift before use
	DWORD unk04[9];
};

struct PfRecordHeader_70
{
	DWORD64 unk01;
	DWORD nameHash;
	DWORD unk02;
	DWORD numOfSubEntries;
	DWORD unk03[7];
	DWORD nameLenExt;		//nameLen multiplied by 4, do shift before use
	DWORD unk04[15];
};
#pragma pack()

int hashStr(wchar_t* str, int strLen)
{
	BYTE* strBuf = (BYTE*)str;
	int curVal = 314159;
	int index = (2*strLen) >> 3;
	int bytesLeft = 2*strLen - (index << 3);
	if (2*strLen >= 8)
	{
		do
		{
			curVal = 442596621*strBuf[0] + 37*(strBuf[6] + 37*(strBuf[5] + 37*(strBuf[4] + 37*(strBuf[3] + 37*(strBuf[2] + 37*strBuf[1]))))) - 803794207*curVal + strBuf[7];
			strBuf += 8;
			index--;
		}
		while ( index );
	}

	for (int i = 0; i < bytesLeft; i++)
		curVal = *strBuf++ + 37*curVal;

	return curVal;
}

void printTimestamp(FILETIME& tmstp)
{
	SYSTEMTIME st = { 0 };
	FileTimeToSystemTime(&tmstp, &st);
	wprintf(L"Timestamp: %04d-%02d-%02d, %02d:%02d:%02d (%d)\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

template <class T, class U, int align>
void dump_T(BYTE* outputBuffer, PfFileHeader* fh)
{
	DWORD crPtr = fh->headerSize;
	for (DWORD i = 0; i < fh->volumesCounter; i++)
	{
		crPtr += 7;
		crPtr &= 0xFFFFFFF8;
		DWORD numOfEntries = 0;
		T* vh = (T*)(outputBuffer + crPtr);

		wchar_t* vol_name = (wchar_t*)(outputBuffer + crPtr + fh->fileParams.sizes[0]);
		int str_len = 0;
		while (vol_name[str_len++]);
		wprintf(L"Volume: (%08X) (%08X) %s\n", hashStr(vol_name, str_len - 1), str_len - 1, vol_name);
		wprintf(L"Volume ID: %04X-%04X\n", vh->volumeID >> 16, vh->volumeID & 0xFFFF);
		printTimestamp(vh->timestamp);
		for (DWORD x = 0; x < sizeof(T)/4; x++)
			wprintf(L"%08X ", *(DWORD*)(outputBuffer + crPtr + x*4));
		wprintf(L"\n\n");

		crPtr += fh->fileParams.sizes[0] + 2*str_len;

		for (DWORD j = 0; j < vh->numOfEntries; j++)
		{
			crPtr += (align - 1);
			crPtr &= (-align);
			U* rh = (U*)(outputBuffer + crPtr);

			wchar_t* rec_name = (wchar_t*)(outputBuffer + crPtr + fh->fileParams.sizes[1]);
			str_len = 0;
			while (rec_name[str_len++]);
			wprintf(L"  File: (%08X) (%08X) %s\n  ", hashStr(rec_name, str_len - 1), str_len - 1, rec_name);
			for (DWORD x = 0; x < sizeof(U)/4; x++)
				wprintf(L"%08X ", *(DWORD*)(outputBuffer + crPtr + x*4));
			wprintf(L"\n\n");

			crPtr += fh->fileParams.sizes[1] + 2*str_len;

			for (DWORD k = 0; k < rh->numOfSubEntries; k++)
			{
				crPtr += (align - 1);
				crPtr &= (-align);

				DWORD skip = *(DWORD*)(outputBuffer + crPtr + align);		//temporary test, it should be changed to separate value or struct
				skip >>= 5;
				skip &= 3;
				skip = fh->fileParams.sizes[3 + skip];
				wprintf(L"    ");
				for (DWORD x = 0; x < skip/4; x++)
					wprintf(L"%08X ", *(DWORD*)(outputBuffer + crPtr + x*4));
				wprintf(L"\n");
				crPtr += skip;
			}
			wprintf(L"\n");
		}
	}
}

void dump(BYTE* outputBuffer, DWORD totalSize)
{
	PfFileHeader* fh = (PfFileHeader*)outputBuffer;
	wprintf(L"magic          : %08X\nfile size      : %08X\nheader size    : %08X\nfile type      : %08X\nvolumes counter: %08X\nunknown        : %08X\n", fh->magic, fh->fileSize, (fh->headerSize + 7) & 0xFFFFFFF8, fh->fileType, fh->volumesCounter, fh->totalEntriesInVolumes);
	
	for (int i = 0; i < _countof(fh->fileParams.sizes); i++)
		wprintf(L"\tparam %02X: %08X\n", i, fh->fileParams.sizes[i]);
	wprintf(L"\n");

	if ((fh->fileParams.sizes[0] == 0x38) && (fh->fileParams.sizes[1] == 0x34))
	{
		dump_T<PfVolumeHeader_38, PfRecordHeader_34, 4>(outputBuffer, fh);
	}
	else if ((fh->fileParams.sizes[0] == 0x38) && (fh->fileParams.sizes[1] == 0x24))
	{
		dump_T<PfVolumeHeader_38, PfRecordHeader_24, 4>(outputBuffer, fh);
	}
	else if ((fh->fileParams.sizes[0] == 0x38) && (fh->fileParams.sizes[1] == 0x48))
	{
		dump_T<PfVolumeHeader_38, PfRecordHeader_48, 4>(outputBuffer, fh);
	}
	else if ((fh->fileParams.sizes[0] == 0x48) && (fh->fileParams.sizes[1] == 0x58))
	{
		dump_T<PfVolumeHeader_48, PfRecordHeader_58, 8>(outputBuffer, fh);
	}
	else if ((fh->fileParams.sizes[0] == 0x48) && (fh->fileParams.sizes[1] == 0x70))
	{
		dump_T<PfVolumeHeader_48, PfRecordHeader_70, 8>(outputBuffer, fh);
	}
	else if ((fh->fileParams.sizes[0] == 0x48) && (fh->fileParams.sizes[1] == 0x40))
	{
		dump_T<PfVolumeHeader_48, PfRecordHeader_40, 8>(outputBuffer, fh);
	}
	else
	{
		wprintf(L"format not supported yet...\n");
	}
}

void LZNT1Decompress(UCHAR* input, ULONG inputSize, UCHAR* output, ULONG outputSize)
{
	static HMODULE hNtDll = 0;
	if (0 == hNtDll)
	{
		hNtDll = GetModuleHandle(L"ntdll.dll");
		if (0 == hNtDll)
			return;
	}

	static __RtlDecompressBuffer _RtlDecompressBuffer = 0;
	if (0 == _RtlDecompressBuffer)
	{
		_RtlDecompressBuffer = (__RtlDecompressBuffer)GetProcAddress(hNtDll, "RtlDecompressBuffer");
		if (0 == _RtlDecompressBuffer)
			return;
	}

	ULONG dummy;
	_RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, output, outputSize, input, inputSize, &dummy);
}

int wmain(int argc, wchar_t *argv[])
{
	wprintf(L"SuperFetch files dumper (Ag*.db) v1.0\nCopyright (C) 2011 ReWolf\nhttp://blog.rewolf.pl/\n\n");

	if (2 != argc)
	{
		wprintf(L"Usage:\n  SuperFetch input_file\n");
		return 0;
	}

	HANDLE hFile = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		wprintf(L"Can't open %s.\n", argv[1]);
		return 0;
	}

	DWORD fSize = GetFileSize(hFile, 0);
	BYTE* fileBuffer = (BYTE*)malloc(fSize);
	if (0 == fileBuffer)
	{
		wprintf(L"Can't allocate %d bytes of memory.\n", fSize);
		CloseHandle(hFile);
		return 0;
	}

	DWORD tmp;
	if ((0 == ReadFile(hFile, fileBuffer, fSize, &tmp, 0)) || (tmp != fSize))
	{
		wprintf(L"ReadFile error.\n");
		CloseHandle(hFile);
		return 0;
	}
	CloseHandle(hFile);

	char headTag[5];
	memmove(headTag, fileBuffer, 4);
	headTag[4] = 0;
	wprintf(L"Tag: %08X (\"%S\")\n", *(DWORD*)fileBuffer, headTag);

	CompType compt;
	switch (*(DWORD*)fileBuffer)
	{
		case 0x0E: compt = Comp_None; break;
		case 0x304D454D: compt = Comp_Xpress; break;
		case 0x4F4D454D: compt = Comp_LZNT1; break;
		default:
		{
			wprintf(L"Invalid input file, go away...\n");
			return 0;
		}
	}

	if (compt != Comp_None)
	{
		DWORD totalSize = *(DWORD*)(fileBuffer + 4);
		wprintf(L"Total output size: %d (%08X)\n", totalSize, totalSize);

		BYTE* outputBuffer = (BYTE*)malloc(totalSize+0x100);
		if (0 == outputBuffer)
		{
			wprintf(L"Can't allocate %d bytes of memory.\n", totalSize);
			free(fileBuffer);
			return 0;
		}

		if (Comp_Xpress == compt)
		{
			DWORD crPtr = 8;
			DWORD outPtr = 0;
			while (crPtr < fSize)
			{
				DWORD chunkSize = *(DWORD*)(fileBuffer + crPtr);
				wprintf(L"  chunk size: %d (%08X)\n", chunkSize, chunkSize);

				const ULONG outSize = min(totalSize - outPtr, 0x10000);
				XpressDecompress(fileBuffer + crPtr + 4, chunkSize, outputBuffer + outPtr, outSize);

				crPtr += chunkSize + 4;
				outPtr += 0x10000;
			}
		}
		else
		{
			//LZNT1
			LZNT1Decompress(fileBuffer + 8, fSize - 8, outputBuffer, totalSize);
		}
		free(fileBuffer);

		wchar_t outName[0x1000];
		swprintf_s(outName, L"%s.unp", argv[1]);
		HANDLE hOutFile = CreateFile(outName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		WriteFile(hOutFile, outputBuffer, totalSize, &tmp, 0);
		CloseHandle(hOutFile);

		wprintf(L"\ndumping data...\n\n");
		dump(outputBuffer, totalSize);

		free(outputBuffer);
	}
	else
	{
		wprintf(L"\ndumping data...\n\n");
		dump(fileBuffer, *(DWORD*)(fileBuffer + 4));
	}

	return 0;
}
