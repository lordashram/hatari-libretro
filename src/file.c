/*
  Hatari - file.c

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  Common file access functions.
*/
const char File_rcsid[] = "Hatari $Id: file.c,v 1.28 2006-03-02 08:42:02 thothy Exp $";

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <zlib.h>

#include "main.h"
#include "dialog.h"
#include "file.h"
#include "createBlankImage.h"
#include "zip.h"


#if defined(__BEOS__) || (defined(__sun) && defined(__SVR4))
/* The scandir() and alphasort() functions aren't available on
 * BeOS and Solaris, so let's declare them here... */
#include <dirent.h>

#undef DIRSIZ

#define DIRSIZ(dp)                                          \
        ((sizeof(struct dirent) - sizeof(dp)->d_name) +     \
		(((dp)->d_reclen + 1 + 3) &~ 3))

#if defined(__sun) && defined(__SVR4)
# define dirfd(d) ((d)->dd_fd)
#elif defined(__BEOS__)
# define dirfd(d) ((d)->fd)
#endif


/*-----------------------------------------------------------------------*/
/*
  Alphabetic order comparison routine for those who want it.
*/
int alphasort(const void *d1, const void *d2)
{
  return strcmp((*(struct dirent * const *)d1)->d_name, (*(struct dirent * const *)d2)->d_name);
}


/*-----------------------------------------------------------------------*/
/*
  Scan a directory for all its entries
*/
int scandir(const char *dirname, struct dirent ***namelist, int (*sdfilter)(struct dirent *), int (*dcomp)(const void *, const void *))
{
  struct dirent *d, *p, **names;
  struct stat stb;
  size_t nitems;
  size_t arraysz;
  DIR *dirp;

  if ((dirp = opendir(dirname)) == NULL)
    return(-1);

  if (fstat(dirfd(dirp), &stb) < 0)
    return(-1);

  /*
   * estimate the array size by taking the size of the directory file
   * and dividing it by a multiple of the minimum size entry.
   */
  arraysz = (stb.st_size / 24);

  names = (struct dirent **)malloc(arraysz * sizeof(struct dirent *));
  if (names == NULL)
    return(-1);

  nitems = 0;

  while ((d = readdir(dirp)) != NULL) {

     if (sdfilter != NULL && !(*sdfilter)(d))
       continue;       /* just selected names */

     /*
      * Make a minimum size copy of the data
      */

     p = (struct dirent *)malloc(DIRSIZ(d));
     if (p == NULL)
       return(-1);

     p->d_ino = d->d_ino;
     p->d_reclen = d->d_reclen;
     /*p->d_namlen = d->d_namlen;*/
     memcpy(p->d_name, d->d_name, p->d_reclen + 1);

     /*
      * Check to make sure the array has space left and
      * realloc the maximum size.
      */

     if (++nitems >= arraysz) {

       if (fstat(dirfd(dirp), &stb) < 0)
         return(-1);     /* just might have grown */

       arraysz = stb.st_size / 12;

       names = (struct dirent **)realloc((char *)names, arraysz * sizeof(struct dirent *));
       if (names == NULL)
         return(-1);
     }

     names[nitems-1] = p;
   }

   closedir(dirp);

   if (nitems && dcomp != NULL)
     qsort(names, nitems, sizeof(struct dirent *), dcomp);

   *namelist = names;

   return nitems;
}


#endif /* __BEOS__ */



/*-----------------------------------------------------------------------*/
/*
  Remove any '/'s from end of filenames, but keeps / intact
*/
void File_CleanFileName(char *pszFileName)
{
  int len;

  len = strlen(pszFileName);

  /* Security length check: */
  if (len > FILENAME_MAX)
  {
    pszFileName[FILENAME_MAX-1] = 0;
    len = FILENAME_MAX;
  }

  /* Remove end slash from filename! But / remains! Doh! */
  if( len>2 && pszFileName[len-1]=='/' )
    pszFileName[len-1] = 0;
}


/*-----------------------------------------------------------------------*/
/*
  Add '/' to end of filename
*/
void File_AddSlashToEndFileName(char *pszFileName)
{
  /* Check dir/filenames */
  if (strlen(pszFileName)!=0)
  {
    if (pszFileName[strlen(pszFileName)-1]!='/')
      strcat(pszFileName,"/");  /* Must use end slash */
  }
}


/*-----------------------------------------------------------------------*/
/*
  Does filename extension match? If so, return TRUE
*/
BOOL File_DoesFileExtensionMatch(const char *pszFileName, const char *pszExtension)
{
  if ( strlen(pszFileName) < strlen(pszExtension) )
    return(FALSE);
  /* Is matching extension? */
  if ( !strcasecmp(&pszFileName[strlen(pszFileName)-strlen(pszExtension)], pszExtension) )
    return(TRUE);

  /* No */
  return(FALSE);
}


/*-----------------------------------------------------------------------*/
/*
  Check if filename is from root
  
  Return TRUE if filename is '/', else give FALSE
*/
BOOL File_IsRootFileName(char *pszFileName)
{
  if (pszFileName[0]=='\0')     /* If NULL string return! */
    return(FALSE);

  if (pszFileName[0]=='/')
    return(TRUE);

  return(FALSE);
}


/*-----------------------------------------------------------------------*/
/*
  Return string, to remove 'C:' part of filename
*/
const char *File_RemoveFileNameDrive(const char *pszFileName)
{
  if ( (pszFileName[0]!='\0') && (pszFileName[1]==':') )
    return(&pszFileName[2]);
  else
    return(pszFileName);
}


/*-----------------------------------------------------------------------*/
/*
  Check if filename end with a '/'
  
  Return TRUE if filename ends with '/'
*/
BOOL File_DoesFileNameEndWithSlash(char *pszFileName)
{
  if (pszFileName[0]=='\0')    /* If NULL string return! */
    return(FALSE);

  /* Does string end in a '/'? */
  if (pszFileName[strlen(pszFileName)-1]=='/')
    return(TRUE);

  return(FALSE);
}


/*-----------------------------------------------------------------------*/
/*
  Read file from disk into memory, allocate memory for it if need to (pass
  Address as NULL).
*/
void *File_Read(char *pszFileName, void *pAddress, long *pFileSize, const char * const ppszExts[])
{
  void *pFile = NULL;
  long FileSize = 0;

  /* Does the file exist? If not, see if can scan for other extensions and try these */
  if (!File_Exists(pszFileName) && ppszExts)
  {
    /* Try other extensions, if suceeds correct filename is now in 'pszFileName' */
    File_FindPossibleExtFileName(pszFileName, ppszExts);
  }

  /* Is it a gzipped file? */
  if (File_DoesFileExtensionMatch(pszFileName, ".gz"))
  {
    gzFile hGzFile;
    /* Open and read gzipped file */
    hGzFile = gzopen(pszFileName, "rb");
    if (hGzFile != NULL)
    {
      /* Find size of file: */
      do
      {
        /* Seek through the file until we hit the end... */
        gzseek(hGzFile, 1024, SEEK_CUR);
      }
      while (!gzeof(hGzFile));
      FileSize = gztell(hGzFile);
      gzrewind(hGzFile);
      /* Find pointer to where to load, allocate memory if pass NULL */
      if (pAddress)
        pFile = pAddress;
      else
        pFile = malloc(FileSize);
      /* Read in... */
      if (pFile)
        FileSize = gzread(hGzFile, pFile, FileSize);

      gzclose(hGzFile);
    }
  }
  else if (File_DoesFileExtensionMatch(pszFileName, ".zip"))
  {
    /* It is a .ZIP file! -> Try to load the first file in the archive */
    pFile = ZIP_ReadFirstFile(pszFileName, &FileSize, ppszExts);
    if (pFile && pAddress)
    {
      memcpy(pAddress, pFile, FileSize);
      free(pFile);
      pFile = pAddress;
    }
  }
  else          /* It is a normal file */
  {
    FILE *hDiskFile;
    /* Open and read normal file */
    hDiskFile = fopen(pszFileName, "rb");
    if (hDiskFile != NULL)
    {
      /* Find size of file: */
      fseek(hDiskFile, 0, SEEK_END);
      FileSize = ftell(hDiskFile);
      fseek(hDiskFile, 0, SEEK_SET);
      /* Find pointer to where to load, allocate memory if pass NULL */
      if (pAddress)
        pFile = pAddress;
      else
        pFile = malloc(FileSize);
      /* Read in... */
      if (pFile)
        FileSize = fread(pFile, 1, FileSize, hDiskFile);

      fclose(hDiskFile);
    }
  }

  /* Store size of file we read in (or 0 if failed) */
  if (pFileSize)
    *pFileSize = FileSize;

  return(pFile);        /* Return to where read in/allocated */
}


/*-----------------------------------------------------------------------*/
/*
  Save file to disk, return FALSE if errors
*/
BOOL File_Save(char *pszFileName, const void *pAddress, size_t Size, BOOL bQueryOverwrite)
{
  BOOL bRet = FALSE;

  /* Check if need to ask user if to overwrite */
  if (bQueryOverwrite)
  {
    /* If file exists, ask if OK to overwrite */
    if (!File_QueryOverwrite(pszFileName))
      return(FALSE);
  }

  /* Normal file or gzipped file? */
  if (File_DoesFileExtensionMatch(pszFileName, ".gz"))
  {
    gzFile *hGzFile;
    /* Create a gzipped file: */
    hGzFile = gzopen(pszFileName, "wb");
    if (hGzFile != NULL)
    {
      /* Write data, set success flag */
      if (gzwrite(hGzFile, pAddress, Size) == (int)Size)
        bRet = TRUE;

      gzclose(hGzFile);
    }
  }
  else
  {
    FILE *hDiskFile;
    /* Create a normal file: */
    hDiskFile = fopen(pszFileName, "wb");
    if (hDiskFile != NULL)
    {
      /* Write data, set success flag */
      if (fwrite(pAddress, 1, Size, hDiskFile) == Size)
        bRet = TRUE;

      fclose(hDiskFile);
    }
  }

  return(bRet);
}


/*-----------------------------------------------------------------------*/
/*
  Return size of file, -1 if error
*/
int File_Length(const char *pszFileName)
{
  FILE *hDiskFile;
  int FileSize;

  hDiskFile = fopen(pszFileName, "rb");
  if (hDiskFile!=NULL)
  {
    fseek(hDiskFile, 0, SEEK_END);
    FileSize = ftell(hDiskFile);
    fseek(hDiskFile, 0, SEEK_SET);
    fclose(hDiskFile);
    return(FileSize);
  }

  return(-1);
}


/*-----------------------------------------------------------------------*/
/*
  Return TRUE if file exists
*/
BOOL File_Exists(const char *pszFileName)
{
  FILE *hDiskFile;

  /* Attempt to open file */
  hDiskFile = fopen(pszFileName, "rb");
  if (hDiskFile!=NULL)
  {
    fclose(hDiskFile);
    return(TRUE);
  }
  return(FALSE);
}


/*-----------------------------------------------------------------------*/
/*
  Find if file exists, and if so ask user if OK to overwrite
*/
BOOL File_QueryOverwrite(const char *pszFileName)
{
  char szString[FILENAME_MAX + 26];

  /* Try and find if file exists */
  if (File_Exists(pszFileName))
  {
    /* File does exist, are we OK to overwrite? */
    snprintf(szString, sizeof(szString), "File '%s' exists, overwrite?", pszFileName);
	fprintf(stderr, "%s\n", szString);
    return DlgAlert_Query(szString);
  }

  return(TRUE);
}


/*-----------------------------------------------------------------------*/
/*
  Try filename with various extensions and check if file exists - if so return correct name
*/
BOOL File_FindPossibleExtFileName(char *pszFileName, const char * const ppszExts[])
{
  char *szSrcDir, *szSrcName, *szSrcExt;
  char *szTempFileName;
  int i = 0;
  BOOL bFileExists = FALSE;

  /* Allocate temporary memory for strings: */
  szTempFileName = malloc(4 * FILENAME_MAX);
  if (!szTempFileName)
  {
    perror("File_FindPossibleExtFileName");
    return FALSE;
  }
  szSrcDir = szTempFileName + FILENAME_MAX;
  szSrcName = szSrcDir + FILENAME_MAX;
  szSrcExt = szSrcName + FILENAME_MAX;

  /* Split filename into parts */
  File_splitpath(pszFileName, szSrcDir, szSrcName, szSrcExt);

  /* Scan possible extensions */
  while(ppszExts[i] && !bFileExists)
  {
    /* Re-build with new file extension */
    File_makepath(szTempFileName, szSrcDir, szSrcName, ppszExts[i]);
    /* Does this file exist? */
    if (File_Exists(szTempFileName))
    {
      /* Copy name for return */
      strcpy(pszFileName, szTempFileName);
      bFileExists = TRUE;
    }

    /* Next one */
    i++;
  }

  free(szTempFileName);

  return bFileExists;
}


/*-----------------------------------------------------------------------*/
/*
  Split a complete filename into path, filename and extension.
  If pExt is NULL, don't split the extension from the file name!
*/
void File_splitpath(const char *pSrcFileName, char *pDir, char *pName, char *pExt)
{
  char *ptr1, *ptr2;

  /* Build pathname: */
  ptr1 = strrchr(pSrcFileName, '/');
  if( ptr1 )
  {
    strcpy(pDir, pSrcFileName);
    strcpy(pName, ptr1+1);
    pDir[ptr1-pSrcFileName+1] = 0;
  }
  else
  {
    strcpy(pDir, "./");
    strcpy(pName, pSrcFileName);
  }

  /* Build the raw filename: */
  if( pExt!=NULL )
  {
    ptr2 = strrchr(pName+1, '.');
    if( ptr2 )
    {
      pName[ptr2-pName] = 0;
      /* Copy the file extension: */
      strcpy(pExt, ptr2+1);
    }
    else
      pExt[0] = 0;
   }
}


/*-----------------------------------------------------------------------*/
/*
  Build a complete filename from path, filename and extension.
  pExt can also be NULL.
*/
void File_makepath(char *pDestFileName, const char *pDir, const char *pName, const char *pExt)
{
  strcpy(pDestFileName, pDir);
  if( strlen(pDestFileName)==0 )
    strcpy(pDestFileName, "./");
  if( pDestFileName[strlen(pDestFileName)-1]!='/' )
    strcat(pDestFileName, "/");

  strcat(pDestFileName, pName);

  if( pExt!=NULL )
  {
    if( strlen(pExt)>0 && pExt[0]!='.' )
      strcat(pDestFileName, ".");
    strcat(pDestFileName, pExt);
  }
}


/*-----------------------------------------------------------------------*/
/*
  Shrink a file name to a certain length and insert some dots if we cut
  something away (usefull for showing file names in a dialog).
*/
void File_ShrinkName(char *pDestFileName, char *pSrcFileName, int maxlen)
{
  int srclen = strlen(pSrcFileName);
  if( srclen<maxlen )
    strcpy(pDestFileName, pSrcFileName);  /* It fits! */
  else
  {
    strncpy(pDestFileName, pSrcFileName, maxlen/2);
    if(maxlen&1)  /* even or uneven? */
      pDestFileName[maxlen/2-1] = 0;
    else
      pDestFileName[maxlen/2-2] = 0;
    strcat(pDestFileName, "...");
    strcat(pDestFileName, &pSrcFileName[strlen(pSrcFileName)-maxlen/2+1]);
  }
}


/*-----------------------------------------------------------------------*/
/*
  Create a clean absolute file name from a (possibly) relative file name.
  I.e. filter out all occurancies of "./" and "../".
  pFileName needs to point to a buffer of at least FILENAME_MAX bytes.
*/
void File_MakeAbsoluteName(char *pFileName)
{
  char *pTempName;
  int inpos, outpos;

  inpos = 0;
  pTempName = malloc(FILENAME_MAX);
  if (!pTempName)
  {
    perror("File_MakeAbsoluteName - malloc");
	return;
  }

  /* Is it already an absolute name? */
  if(pFileName[0] == '/')
  {
    outpos = 0;
  }
  else
  {
    if (!getcwd(pTempName, FILENAME_MAX))
    {
      perror("File_MakeAbsoluteName - getcwd");
      free(pTempName);
	  return;
    }
    File_AddSlashToEndFileName(pTempName);
    outpos = strlen(pTempName);
  }

  /* Now filter out the relative paths "./" and "../" */
  while (pFileName[inpos] != 0 && outpos < FILENAME_MAX)
  {
    if (pFileName[inpos] == '.' && pFileName[inpos+1] == '/')
    {
      /* Ignore "./" */
      inpos += 2;
    }
    else if (pFileName[inpos] == '.' && pFileName[inpos+1] == '.' && pFileName[inpos+2] == '/')
    {
      /* Handle "../" */
      char *pSlashPos;
      inpos += 3;
      pTempName[outpos - 1] = 0;
      pSlashPos = strrchr(pTempName, '/');
      if (pSlashPos)
      {
        *(pSlashPos + 1) = 0;
        outpos = strlen(pTempName);
      }
      else
      {
        pTempName[0] = '/';
        outpos = 1;
      }
    }
    else
    {
      /* Copy until next slash or end of input string */
      while (pFileName[inpos] != 0 && outpos < FILENAME_MAX)
      {
        pTempName[outpos++] = pFileName[inpos++];
        if (pFileName[inpos - 1] == '/')  break;
      }
    }
  }

  pTempName[outpos] = 0;

  strcpy(pFileName, pTempName);          /* Copy back */
  free(pTempName);
}


/*-----------------------------------------------------------------------*/
/*
  Create a valid path name from a possibly invalid name by erasing invalid
  path parts at the end of the string.
  pPathName needs to point to a buffer of at least FILENAME_MAX bytes.
*/
void File_MakeValidPathName(char *pPathName)
{
  struct stat dirstat;
  char *pLastSlash;

  do
  {
    /* Check for a valid path */
    if (stat(pPathName, &dirstat) == 0 && S_ISDIR(dirstat.st_mode))
    {
      break;
    }

    pLastSlash = strrchr(pPathName, '/');
    if (pLastSlash)
    {
      /* Erase the (probably invalid) part after the last slash */
      *pLastSlash = 0;
    }
    else
    {
      /* Path name seems to be completely invalid -> set to root directory */
      strcpy(pPathName, "/");
    }
  }
  while (pLastSlash);
}
