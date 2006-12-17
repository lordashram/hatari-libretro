/*
  Hatari - ioMemTables.h

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.
*/

#ifndef HATARI_IOMEMTABLES_H
#define HATARI_IOMEMTABLES_H

/* Hardware address details */
typedef struct
{
	Uint32 Address;              /* ST hardware address */
	int SpanInBytes;             /* E.g. SIZE_BYTE, SIZE_WORD or SIZE_LONG */
	void (*ReadFunc)(void);      /* Read function */
	void (*WriteFunc)(void);     /* Write function */
} INTERCEPT_ACCESS_FUNC;

extern const INTERCEPT_ACCESS_FUNC IoMemTable_ST[];
extern const INTERCEPT_ACCESS_FUNC IoMemTable_STE[];
extern const INTERCEPT_ACCESS_FUNC IoMemTable_TT[];
extern const INTERCEPT_ACCESS_FUNC IoMemTable_Falcon[];

extern void IoMemTabFalcon_NoDSP(void (**readtab)(void), void (**writetab)(void));
#if ENABLE_FALCON
extern void IoMemTabFalcon_EnableDSP(void (**readtab)(void), void (**writetab)(void));
#endif

#endif
