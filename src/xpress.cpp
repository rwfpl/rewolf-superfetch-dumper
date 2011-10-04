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

#include <cstdlib>
#include "xpress.h"

int XpressCompareSymbols(const void *arg1, const void *arg2)
{
	XPRESS_PREFIX_CODE_SYMBOL& a = *(XPRESS_PREFIX_CODE_SYMBOL*)arg1;
	XPRESS_PREFIX_CODE_SYMBOL& b = *(XPRESS_PREFIX_CODE_SYMBOL*)arg2;

	if (a.length < b.length)
		return -1;
	else if (a.length > b.length)
		return 1;
	else if (a.symbol < b.symbol)
		return -1;
	else if (a.symbol > b.symbol)
		return 1;
	else
		return 0;
}

void XpressSortSymbols(XPRESS_PREFIX_CODE_SYMBOL* symbols)
{
	qsort(symbols, XPRESS_SYMCNT, sizeof(XPRESS_PREFIX_CODE_SYMBOL), XpressCompareSymbols);
}

int XpressPrefixCodeTreeAddLeaf(XPRESS_PREFIX_CODE_NODE* treeNodes, ULONG leafIndex, ULONG mask, ULONG bits)
{
	XPRESS_PREFIX_CODE_NODE* node = treeNodes;
	ULONG i = leafIndex + 1;
	ULONG childIndex;

	while (bits > 1)
	{
		bits--;
		childIndex = (mask >> bits) & 1;
		if (NULL == node->child[childIndex])
		{
			node->child[childIndex] = &treeNodes[i];
			treeNodes[i].leaf = FALSE;
			i++;
		}
		node = node->child[childIndex];
	}
	node->child[mask & 1] = &treeNodes[leafIndex];
	return i;
}

XPRESS_PREFIX_CODE_NODE* XpressPrefixCodeTreeRebuild(UCHAR* input, XPRESS_PREFIX_CODE_NODE* treeNodes)
{
	XPRESS_PREFIX_CODE_SYMBOL symbolInfo[XPRESS_SYMCNT];
	ULONG i;
	ULONG j;
	ULONG mask;
	ULONG bits;

	for (i = 0; i < XPRESS_TREECNT; i++)
	{	
		treeNodes[i].symbol = 0;
		treeNodes[i].leaf = FALSE;
		treeNodes[i].child[0] = NULL;
		treeNodes[i].child[1] = NULL;
	}

	for (i = 0; i < XPRESS_HUFFSIZE; i++)
	{
		symbolInfo[2*i].symbol = (USHORT)(2*i);
		symbolInfo[2*i].length = input[i] & 15;
		symbolInfo[2*i+1].symbol = (USHORT)(2*i + 1);
		symbolInfo[2*i+1].length = input[i] >> 4;
	}

	XpressSortSymbols(symbolInfo);

	i = 0;
	while ((i < XPRESS_SYMCNT) && (0 == symbolInfo[i].length))
		i++;

	mask = 0;
	bits = 1;

	treeNodes[0].leaf = FALSE;

	j = 1;
	for (; i < XPRESS_SYMCNT; i++)
	{
		treeNodes[j].symbol = symbolInfo[i].symbol;
		treeNodes[j].leaf = TRUE;
		mask <<= (symbolInfo[i].length - bits);
		bits = symbolInfo[i].length;
		j = XpressPrefixCodeTreeAddLeaf(treeNodes, j, mask, bits);
		mask++;
	}
	
	return treeNodes;
}

void XpressBitstringInit(XPRESS_BITSTRING& bstr, UCHAR* source, ULONG index)
{
	bstr.mask = *(USHORT*)(source + index);
	bstr.mask <<= 16;
	index += 2;
	bstr.mask = bstr.mask | *(USHORT*)(source + index);
	index += 2;
	bstr.bits = 32;
	bstr.source = source;
	bstr.index = index;
}

ULONG XpressBitstringLookup(XPRESS_BITSTRING& bstr, LONG n)
{
	if (0 == n)
		return 0;
	else
		return bstr.mask >> (32 - n);
}

void XpressBitstringSkip(XPRESS_BITSTRING& bstr, LONG n)
{
	bstr.mask <<= n;
	bstr.bits -= n;
	if (bstr.bits < 16)
	{
		bstr.mask += (*(USHORT*)(bstr.source + bstr.index) << (16 - bstr.bits));
		bstr.index += 2;
		bstr.bits += 16;
	}
}

ULONG XpressPrefixCodeTreeDecodeSymbol(XPRESS_BITSTRING& bstr, XPRESS_PREFIX_CODE_NODE* root)
{
	ULONG bit;
	XPRESS_PREFIX_CODE_NODE* node = root;

	do
	{
		bit = XpressBitstringLookup(bstr, 1);
		XpressBitstringSkip(bstr, 1);
		node = node->child[bit];
	}
	while (FALSE == node->leaf);
	
	return node->symbol;
}

void XpressDecompress(UCHAR* input, ULONG inputSize, UCHAR* output, ULONG outputSize)
{
	XPRESS_PREFIX_CODE_NODE prefixCodeTreeNodes[XPRESS_TREECNT];

	XPRESS_BITSTRING bstr;
	XPRESS_PREFIX_CODE_NODE* root = XpressPrefixCodeTreeRebuild(input, prefixCodeTreeNodes);

	XpressBitstringInit(bstr, input, XPRESS_HUFFSIZE);

	ULONG i = 0;
	ULONG stopIndex = i + outputSize;
	while (i < stopIndex)
	{
		ULONG symbol = XpressPrefixCodeTreeDecodeSymbol(bstr, root);
		if (symbol < 256)
		{
			output[i] = (UCHAR)symbol;
			i++;
		}
		else
		{
			symbol -= 256;
			ULONG length = symbol & 15;
			symbol >>= 4;

			LONG offset = (1 << symbol) + XpressBitstringLookup(bstr, symbol);
			offset = -1 * offset;

			if (15 == length)
			{
				length = bstr.source[bstr.index] + 15;
				bstr.index++;

				if (270 == length)
				{
					length = *(USHORT*)(bstr.source + bstr.index);
					bstr.index += 2;
				}
			}

			XpressBitstringSkip(bstr, symbol);

			length += 3;
			do
			{
				output[i] = output[i + offset];
				i++;
				length--;
			}
			while (0 != length);
		}
	}
}
