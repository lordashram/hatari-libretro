/*
	DSP M56001 emulation
	Dummy emulation, Hatari glue

	(C) 2001-2008 ARAnyM developer team
	Adaption to Hatari (C) 2008 by Thomas Huth

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef DSP_H
#define DSP_H

#if ENABLE_DSP_EMU
# include "dsp_core.h"
#endif

extern bool bDspEnabled;

extern void DSP_Init(void);
extern void DSP_UnInit(void);
extern void DSP_MemorySnapShot_Capture(bool bSave);

extern void DSP_Reset(void);
extern void DSP_Run(int nHostCycles);
extern Uint16 DSP_GetPC(void);
extern Uint32 DSP_SsiReadTxValue(void);
extern void DSP_SsiReceiveSerialClock(void);

extern void DSP_HandleReadAccess(void);
extern void DSP_HandleWriteAccess(void);

#endif /* DSP_H */
