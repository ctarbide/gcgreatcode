/*$T out.c GC 1.139 12/15/04 23:58:58 */


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
#include "stdio.h"
#include "string.h"
#include "sys/stat.h"
#include "io.h"
#include "stdlib.h"
#include "string.h"
#include "config.h"
#include "in.h"
#include "lexi.h"
#include "error.h"
#include "tools.h"

/*$4
 ***********************************************************************************************************************
 ***********************************************************************************************************************
 */

int gi_NumLines = 0;
int gi_NumChars = 0;

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void WriteToken(FILE *h, FileDes *pfile, token *pcur)
{
	/*~~~~~~~~~~~~~~~~*/
	int		i, jj;
	int		col, col1;
	int		len;
	char	*pz, *plast;
	/*~~~~~~~~~~~~~~~~*/

	Tool_ComputeCol(pfile);
	while(pcur)
	{
		/* Unix style for EOL */
		if(Config.UnixEOL)
		{
			pz = strchr(pcur->pc_Value, '\r');
			while(pz)
			{
				if(pz[1] == '\n')
				{
					memmove(pz, pz + 1, strlen(pz) - 1);
					pz[strlen(pz) - 1] = 0;
				}

				pz = strchr(pz + 1, '\r');
			}
		}

		/* Dos style */
		else
		{
			len = strlen(pcur->pc_Value);
			pz = strchr(pcur->pc_Value, '\n');
			while(pz)
			{
				if(pz[-1] != '\r')
				{
					plast = pcur->pc_Value;
					pcur->pc_Value = (char *) __realloc__(pcur->pc_Value, sizeof(char) * (len + 2));
					pz = pcur->pc_Value + (pz - plast);
					memmove(pz + 1, pz, strlen(pz) + 1);
					*pz = '\r';
					pz++;
					len++;
				}

				pz = strchr(pz + 1, '\n');
			}
		}

		/* Remove trailing spaces */
		len = strlen(pcur->pc_Value);
		if(pcur->ForceEOLAfter)
		{
			while(pcur->pc_Value[len - 1] == ' ')
			{
				pcur->pc_Value[len - 1] = 0;
				len--;
			}
		}

		/* Write token */
		fwrite(pcur->pc_Value, 1, strlen(pcur->pc_Value), h);
		gi_NumChars += strlen(pcur->pc_Value);

		/* Write EOL after the token */
		if(pcur->pst_Next && pcur->pst_Next->NoIndent) pcur->ForceEOLAfter = (char) pcur->WasEOLAfter;
		if(pcur->pst_Next && !pcur->pst_Next->i_ID) pcur->ForceEOLAfter = (char) Config.EndBlanks;

		for(i = 0; i < pcur->ForceEOLAfter; i++)
		{
			if(!Config.UnixEOL) fputc('\r', h);
			fputc('\n', h);
			gi_NumLines++;
		}

		/* Write spaces after the token */
		if(pcur->pst_Next && pcur->pst_Next->NoIndent) pcur->ForceSpaceAfter = (char) pcur->WasSpaceAfter;

		if(Config.OutTab && (pcur->ForceSpaceAfter > 1))
		{
			if(pcur->ForceSpaceAfter && pcur->pst_Next)
			{
				col = (char) pcur->Col;
				if(!pcur->ForceEOLAfter)
					col = pcur->Col + strlen(pcur->pc_Value);
				else
					col = 0;
				col1 = Tool_ToTab(col);
				if(col1 == col) col1 += Config.TabSize;
				if(col1 - col <= pcur->ForceSpaceAfter)
				{
					if(pcur->ForceSpaceAfter == 3) pcur->ForceSpaceAfter = 3;
					fputc('\t', h);
					gi_NumChars++;
					jj = pcur->ForceSpaceAfter;
					jj -= (col1 - col);
					pcur->ForceSpaceAfter = (char) jj;
					while(pcur->ForceSpaceAfter / Config.TabSize)
					{
						fputc('\t', h);
						gi_NumChars++;
						jj = pcur->ForceSpaceAfter;
						jj -= Config.TabSize;
						pcur->ForceSpaceAfter = (char) jj;
					}
				}

				for(i = 0; i < pcur->ForceSpaceAfter; i++)
				{
					fputc(' ', h);
					gi_NumChars++;
				}
			}
		}
		else
		{
			for(i = 0; i < pcur->ForceSpaceAfter; i++)
			{
				fputc(' ', h);
				gi_NumChars++;
			}
		}

		pcur = NextToken(pcur);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void WriteFile(FileDes *pfile)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/
	char	asz_Temp[_MAX_PATH];
	char	name[_MAX_PATH];
	char	name1[_MAX_PATH];
	char	*pz_Temp;
	FILE	*h;
	/*~~~~~~~~~~~~~~~~~~~~~~~~*/

	if(!Config.CanOut) return;
	if(_access(pfile->psz_FileName, 2) && !Config.OutTest)
	{
		if(Config.ReadOnly)
		{
		}
		else
		{
			Warning("Source file is read only, can't write", NULL);
			return;
		}
	}

	/* Save current file with .bak extension */
	strcpy(asz_Temp, pfile->psz_FileName);
	strcat(asz_Temp, ".bak");
	if(_access(asz_Temp, 2)) _chmod(asz_Temp, _S_IWRITE);
	_unlink(asz_Temp);
	if(Config.CanBak)
	{
		if(rename(pfile->psz_FileName, asz_Temp))
		{
			if(_access(pfile->psz_FileName, 2)) _chmod(pfile->psz_FileName, _S_IWRITE);
			if(rename(pfile->psz_FileName, asz_Temp)) Warning("Can't rename file", pfile->psz_FileName);
			return;
		}

		_unlink(pfile->psz_FileName);
	}

	/* Open file to write */
	strcpy(name, pfile->psz_FileName);
	if(Config.OutTest)
	{
		pz_Temp = strrchr(name, '.');
		*name1 = 0;
		if(pz_Temp) strcpy(name1, pz_Temp);
		*pz_Temp = 0;
		strcat(name, "_test");
		strcat(name, name1);
	}
	else
	{
		_chmod(name, _S_IWRITE);
	}

	h = fopen(name, "wb");
	if(!h)
	{
		Fatal("Can't write to file", name);
	}

	/* Parse all tokens */
	WriteToken(h, pfile, pfile->pst_RootToken);

	/* Close file */
	fflush(h);
	fclose(h);
}
