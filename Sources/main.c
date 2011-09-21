/*$T main.c GC 1.139 12/15/04 23:58:53 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */


/*$F
    *****************************************************************************************
    GC GreatCode
    Original code by Christophe Beaudet
    e-mail: cbeaudet@club-internet.fr
    *****************************************************************************************
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <ctype.h>

#ifdef _WIN32
#include <direct.h>
#else
#endif

#include "config.h"
#include "in.h"
#include "out.h"
#include "lexi.h"
#include "error.h"
#include "grammar.h"
#include "indent.h"
#include "tools.h"
#include "debug.h"
#include "os.h"

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void ProcessFiles(void)
{
	/*~~~~~~~~~*/
	int		i, j;
	char	res;
	/*~~~~~~~~~*/

	/* Create list of files */
	for(i = 0; i < gi_NumRecurseDirs; i++) CreateFilesInDir(gpsz_RecurseDirs[i]);
	for(i = 0; i < gi_NumScanFiles; i++) CreateFile(gpsz_ScanFiles[i]);
	if(Config.Verbose) printf("\n");

	/* Process each file */
	for(i = 0; i < gi_NumFiles; i++)
	{
		res = OpenFile(i);
		if(res != 2 && Config.Verbose) printf("Processing %s   ", gpst_Files[i].psz_FileName);
		if(!res)
		{
			memcpy(&Config, &Config1, sizeof(tdst_Config));
			Lexi(&gpst_Files[i]);
			Grammar(&gpst_Files[i]);

			/* printTokensWithTypes(&gpst_Files[i]); */
			Indent(&gpst_Files[i]);

			/* printTokensWithTypes(&gpst_Files[i]); */
			WriteFile(&gpst_Files[i]);
			if(Config.Verbose) printf("(%d lines, %d characters)\n", gi_NumLines, gi_NumChars);
		}
		else if(res == 1)
			gpst_Files[i].psz_FileName = NULL;
	}

	/* Dependencies */
	if(Config.Dependencies)
	{
		for(j = 0; j < gi_NumIncludes; j++)
		{
			memset(&gpst_Files[gi_NumFiles + j], 0, sizeof(FileDes));
			gpst_Files[gi_NumFiles + j].psz_FileName = gast_Includes[j].asz_Name;
			res = OpenFile(gi_NumFiles + j);
			if(res != 2 && Config.Verbose) printf("Processing %s\n", gpst_Files[gi_NumFiles + j].psz_FileName);
			if(!res)
			{
				Lexi(&gpst_Files[gi_NumFiles + j]);
				Grammar(&gpst_Files[gi_NumFiles + j]);
			}
			else if(res == 1)
				gpst_Files[gi_NumFiles + j].psz_FileName = NULL;
		}
	}

	/* Depend treat */
	Depend();

	/* Close all files */
	for(i = 0; i < gi_NumFiles; i++)
	{
		if(gpst_Files[i].psz_FileName) CloseFile(i);
	}

	if(Config.Dependencies)
	{
		for(j = 0; j < gi_NumIncludes; j++)
		{
			if(gpst_Files[i + j].psz_FileName) CloseFile(i + j);
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int main(int argc, char *argv[])
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/
	int		i;
	char	asz_Temp[_MAX_PATH];
	char	*psz_Temp;
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/

	gi_NumIncludesDirs = 0;

	/* Print header */
	printf("GC GreatCode %d.%03d by Christophe Beaudet\n", VERSION, REVISION);
	printf("****************************************\n");
	printf("GC is available on SourceForge.net\n");
	printf("****************************************\n");
	printf("Type GC -help for options\n\n");
	memset(&Config, 0, sizeof(Config));

	/*
	 * Retreive the path where GC.exe is run. This will use to find the configurations
	 * files.
	 */
	strcpy(asz_Temp, argv[0]);
	psz_Temp = strrchr(asz_Temp, '/');
	if(psz_Temp)
	{
		*psz_Temp = 0;
	}
	else
	{
		psz_Temp = strrchr(asz_Temp, '\\');
		if(psz_Temp)
			*psz_Temp = 0;
		else
			strcpy(asz_Temp, ".");
	}

	/* Search for -help to print usage */
	for(i = 1; i < argc; i++)
	{
		if(!strcasecmp(argv[i], "-help"))
		{
			Usage();
			return 0;
		}
	}

	/* Init default options */
	Default();

	/* Treat options */
	Options(argc - 1, argv + 1);

	/* Check if options are valid */
	CheckConfig();

	/* Copy config buffer */
	memcpy(&Config1, &Config, sizeof(tdst_Config));

	/* Read all files */
	ProcessFiles();
	return 0;
}
