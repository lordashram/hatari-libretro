/*
  Hatari
*/

#ifndef SCREEN_H
#define SCREEN_H

#include <SDL_video.h>    /* for SDL_Surface */


/* Frame buffer, used to store details in screen conversion */
typedef struct {
  unsigned short int HBLPalettes[(NUM_VISIBLE_LINES+1)*16];  /* 1x16 colour palette per screen line, +1 line as may write after line 200 */
  unsigned long HBLPaletteMasks[NUM_VISIBLE_LINES+1];        /* Bit mask of palette colours changes, top bit set is resolution change */
  unsigned char *pSTScreen;          /* Copy of screen built up during frame(copy each line on HBL to simulate monitor raster) */
  unsigned char *pSTScreenCopy;      /* Previous frames copy of above  */
  int OverscanModeCopy;              /* Previous screen overscan mode */
  BOOL bFullUpdate;                  /* Set TRUE to cause full update on next draw */
} FRAMEBUFFER;
#define NUM_FRAMEBUFFERS  2

/* Details for each display screen - both Window and FullScreen */
typedef struct {
  int STScreenLeftSkipBytes,STScreenWidthBytes;     /* Bytes to skip on left and number of bytes of screen to draw */
  int STScreenStartHorizLine,STScreenEndHorizLine;  /* Start/End points in ST screen(28 is top of normal screen) */
  int PCStartHorizLine,PCStartXOffset;              /* Source ST lines/bytes to skip, Destination screen lines/bytes to skip */
} SCREENDRAW_OVERSCAN;

typedef struct {
  void *pDrawFunction;              /* Draw function */
  /*int DirectDrawMode;*/           /* Mode required for DirectDraw. eg MODE_320x200x256 */
  int Width,Height,BitDepth,VertPixelsPerLine;
  SCREENDRAW_OVERSCAN Overscan[4];  /* Details for starting offset for each overscan mode(none,top,bottom,both) */
} SCREENDRAW;

typedef struct {
  SCREENDRAW *pLowRes, *pAltLowRes;
  SCREENDRAW *pMediumRes, *pAltMediumRes;
  SCREENDRAW *pHighRes, *pAltHighRes;
  SCREENDRAW *pLowMediumMixRes, *pAltLowMediumMixRes;
} SCREENDRAW_DISPLAYOPTIONS;

/* ST Resolution defines */
enum {
  ST_LOW_RES,
  ST_MEDIUM_RES,
  ST_HIGH_RES,
  ST_LOWMEDIUM_MIX_RES
};

/* Update Palette defines */
enum {
  UPDATE_PALETTE_NONE,
  UPDATE_PALETTE_UPDATE,
  UPDATE_PALETTE_FULLUPDATE
};

/* Palette mask values for 'HBLPaletteMask[]' */
#define PALETTEMASK_RESOLUTION  0x00040000
#define PALETTEMASK_PALETTE     0x0000ffff
#define PALETTEMASK_UPDATERES   0x20000000
#define PALETTEMASK_UPDATEPAL   0x40000000
#define PALETTEMASK_UPDATEFULL  0x80000000
#define PALETTEMASK_UPDATEMASK  (PALETTEMASK_UPDATEFULL|PALETTEMASK_UPDATEPAL|PALETTEMASK_UPDATERES)

/* Overscan values */
enum {
  OVERSCANMODE_NONE,     /* 0x00 */
  OVERSCANMODE_TOP,      /* 0x01 */
  OVERSCANMODE_BOTTOM    /* 0x02 (Top+Bottom) 0x03 */
};

/* Available fullscreen modes */
#define NUM_DISPLAYMODEOPTIONS	6
enum {
	DISPLAYMODE_16COL_LOWRES,		/* (fastest) */
	DISPLAYMODE_16COL_HIGHRES,
	DISPLAYMODE_16COL_FULL,
	DISPLAYMODE_HICOL_LOWRES,
	DISPLAYMODE_HICOL_HIGHRES,
	DISPLAYMODE_HICOL_FULL			/* (slowest) */
};


/* For palette we don't go from colour '0' as the whole background would change, so go from this value */
#define  BASECOLOUR       0x0A
#define  BASECOLOUR_LONG  0x0A0A0A0A

extern SCREENDRAW ScreenDrawWindow[4];
extern SCREENDRAW ScreenDrawFullScreen[4];
extern FRAMEBUFFER *pFrameBuffer;
extern unsigned char *pScreenBitmap;
extern unsigned char *pSTScreen,*pSTScreenCopy;
extern unsigned char *pPCScreenDest;
extern int STScreenStartHorizLine,STScreenEndHorizLine;
extern int PCScreenBytesPerLine,STScreenWidthBytes,STScreenLeftSkipBytes;
extern BOOL bInFullScreen;
extern BOOL bFullScreenHold;
extern BOOL bScreenContentsChanged;
extern int STRes,PrevSTRes;
extern int STScreenLineOffset[NUM_VISIBLE_LINES];
extern unsigned long STRGBPalette[16];
extern unsigned long ST2RGB[2048];
extern SDL_Surface *sdlscrn;
extern BOOL bGrabMouse;

extern void Screen_Init(void);
extern void Screen_UnInit(void);
extern void Screen_Reset(void);
extern void Screen_SetScreenLineOffsets(void);
extern void Screen_SetFullUpdate(void);
extern void Screen_SetupRGBTable(BOOL bFullScreen);
extern void Screen_EnterFullScreen(void);
extern void Screen_ReturnFromFullScreen(void);
extern void Screen_ClearScreen(void);
extern void Screen_SetDrawModes(void);
extern void Screen_SetWindowRes(int Width,int Height,int BitCount);
extern void Screen_Blit(BOOL bSwapScreen);
extern FRAMEBUFFER *Screen_GetOtherFrameBuffer(void);
extern void Screen_DrawFrame(BOOL bForceFlip);
extern void Screen_Draw(void);

#endif  /* ifndef SCREEN_H */
