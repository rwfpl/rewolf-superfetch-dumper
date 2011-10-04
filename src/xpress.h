/********************************************************************************
 *
 * Xpress Huffman decompression alogrithm
 *
 * Written by ReWolf (2011, http://blog.rewolf.pl/)
 *
 * Based on original specification from MSDN:
 * http://msdn.microsoft.com/en-us/library/dd644740(v=PROT.13).aspx
 * "Error checking and handling has been omitted from all algorithms in the
 *  interests of clarity."
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

#ifndef _XPRESS_H_
#define _XPRESS_H_

#include <Windows.h>

struct XPRESS_PREFIX_CODE_NODE 
{
	USHORT symbol;
	BOOL leaf;
	XPRESS_PREFIX_CODE_NODE* child[2];
};

struct XPRESS_PREFIX_CODE_SYMBOL
{
	USHORT symbol;
	USHORT length;
};

struct XPRESS_BITSTRING
{
	UCHAR* source;
	ULONG index;
	ULONG mask;
	LONG bits;
};

#define XPRESS_HUFFSIZE 256
#define XPRESS_SYMCNT 512
#define XPRESS_TREECNT 1024

void XpressDecompress(UCHAR* input, ULONG inputSize, UCHAR* output, ULONG outputSize);

#endif //_XPRESS_H_
