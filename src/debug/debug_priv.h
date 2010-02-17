/*
  Hatari - debug.h

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  Internal header used by debugger files.
*/
#ifndef HATARI_DEBUG_PRIV_H
#define HATARI_DEBUG_PRIV_H

/* DebugUI command structure */
typedef struct
{
	int (*pFunction)(int argc, char *argv[]);
	const char *sLongName;
	const char *sShortName;
	const char *sShortDesc;
	const char *sUsage;
	bool bNoParsing;
} dbgcommand_t;

/* Output file debugger output */
extern FILE *debugOutput;

extern void DebugUI_PrintCmdHelp(const char *psCmd);

extern void DebugCpu_SetDebugging(void);
extern void DebugCpu_MemorySnapShot_Capture(bool bSave);
extern int DebugCpu_Init(const dbgcommand_t **table);
extern void DebugCpu_InitSession(void);

#ifdef ENABLE_DSP_EMU
extern void DebugDsp_SetDebugging(void);
extern void DebugDsp_MemorySnapShot_Capture(bool bSave);
extern int DebugDsp_Init(const dbgcommand_t **table);
extern void DebugDsp_InitSession(void);
#else /* !ENABLE_DSP_EMU */
#define DebugDsp_SetDebugging()
#define DebugDsp_MemorySnapShot_Capture(x)
static inline int DebugDsp_Init(const dbgcommand_t **t) { t = NULL; return 0; }
#define DebugDsp_InitSession()
#endif /* !ENABLE_DSP_EMU */

#endif /* HATARI_DEBUG_PRIV_H */
