/*$T tools_cmt.c GC 1.139 12/15/04 23:59:15 */


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
#include "malloc.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"
#include "config.h"
#include "in.h"
#include "lexi.h"
#include "error.h"
#include "tools.h"
#include "time.h"

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void CmtMark(char **xp, char **xgn)
{
	/*~~~~~~~~*/
	char	*p;
	char	*gn;
	/*~~~~~~~~*/

	p = *xp;
	gn = *xgn;
	if(p[2] == CMTMARK)
	{
		p += 2;
		if(isdigit(p[1]) && (p[2] == '-'))
		{
			*gn++ = *p++;
			*gn++ = *p++;
			*gn++ = *p++;
		}
		else
		{
			while(!_isspace(*p) && *p != '*') *gn++ = *p++;
		}
	}
	else if(p[3] == CMTMARK)
	{
		p += 3;
		if(isdigit(p[1]) && (p[2] == '-'))
		{
			*gn++ = *p++;
			*gn++ = *p++;
			*gn++ = *p++;
		}
		else
		{
			while(!_isspace(*p) && *p != '*') *gn++ = *p++;
		}
	}
	else
		p += 2;

	*xp = p;
	*xgn = gn;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int CommentMark(char a)
{
	if(a == '-' || a == '*' || a == '#' || a == '=' || a == '~' || a == '+' || a == '_' || a == '/') return 1;
	if
	(
		a == Config.CharCmt1
	||	a == Config.CharCmt2
	||	a == Config.CharCmt3
	||	a == Config.CharCmt4
	||	a == Config.CharCmt5
	||	a == Config.CharCmt6
	) return 1;
	return 0;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int Tool_DeleteCommentMarks(token *pcur, char **pp, int markCnt)
{
	/*~~~~~~~*/
	int		i;
	char	*p;
	/*~~~~~~~*/

	p = *pp;

	/* Delete comment marks */
	for(i = 0; i < markCnt; i++)
	{
		if(!CommentMark(p[i]) || (p[0] != p[i])) return 0;
	}

	while(CommentMark(*p))
	{
		if(p[0] == '*' && p[1] == '/') break;
		p++;
	}

	if(pcur->CppComment && p[0] == ' ' && p[1] == Config.CharSplit) p += 2;

	*pp = p;
	return 1;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_UnSplitCmt(token *pcur)
{
	/*~~~~~~~~~~~~*/
	char	*p, *p1;
	char	*gn;
	char	*memo;
	int		style;
	/*~~~~~~~~~~~~*/

	p = pcur->pc_Value;

	/* Do not indent a fixed comment */
	if(FixedComment(pcur)) return;

	/* Allocate for the new comment */
	memo = (char *) __malloc__(strlen(p) + 4);

	/* Begin mark */
	gn = memo;
	*gn++ = '/';
	*gn++ = '*';

	/* Skip $X marks */
	CmtMark(&p, &gn);

	/* Delete leading comment marks of length skipping the "/" */
	Tool_DeleteCommentMarks(pcur, &p, 1);

	*gn++ = ' ';
	for(;;)
	{
		/* Zap spaces and '*' character at the beginning of a new line */
		while(_isspace(*p))
		{
			if(*p == '\n')
			{
				while(_isspace(*p) || (*p == '*'))
				{
					if(p[0] == '*' && p[1] == '/') goto End;
					p++;
				}
				break;
			}

			p++;
		}

		/* Handle marks at the end of the line, assume this is a box comment */
		if(CommentMark(*p) && (p[1] == '\n') && _isspace(p[-1]))
		{
			p++;
			continue;
		}

		if(p[0] == '*' && p[1] == '/') goto End;

		/* Copy comment (non space) */
		while(*p && !_isspace(*p))
		{
			if(*p == Config.CharSplit) pcur->NeedSplit = 1;

			/* Delete comment marks */
			if(Tool_DeleteCommentMarks(pcur, &p, 3)) break;
			if(p[0] == '*' && p[1] == '/') goto End;

			/* Style */
			if(*p == '$')
			{
				style = 0;
re:
				*gn++ = *p++;
				p1 = p;
				switch(*p)
				{
				case 'A':
					style |= 1;
					goto re;

				case 'a':
					style |= 2;
					goto re;

				case 'c':
				case 'C':
					goto re;
				}

				if(style)
				{
					while(*p != '\n' && (p[1] != '*' || p[2] != '/') && *p != Config.CharSplit)
					{
						*gn = *p;
						if(style & 1) *gn = (char) toupper(*gn);
						if(style & 2) *gn = (char) tolower(*gn);
						gn++;
						p++;
					}
				}
			}
			else
			{
				*gn++ = *p++;
			}
		}

		/* Copy one space */
		if(gn[-1] != ' ') *gn++ = ' ';
	}

End:
	/* Add an empty space at the end */
	if(gn[-1] != ' ') *gn++ = ' ';

	/* End mark */
	*gn++ = '*';
	*gn++ = '/';
	*gn++ = 0;

	/* Replace old comment with new one */
	free(pcur->pc_Value);
	pcur->pc_Value = memo;
}

/*
 =======================================================================================================================
    This routine deletes leading '*' and keeps eol and could be called instead of Tool_UnSplitCmt.
 =======================================================================================================================
 */
void Tool_UnSplitCmt2(token *pcur)
{
	/*~~~~~~~~~~*/
	char	*p;
	char	*gn;
	char	*memo;
	/*~~~~~~~~~~*/

	p = pcur->pc_Value;

	/* Do not indent a fixed comment */
	if(FixedComment(pcur)) return;

	/* Allocate for the new comment */
	memo = (char *) __malloc__(strlen(p) + 4);

	/* Begin mark */
	gn = memo;
	*gn++ = '/';
	*gn++ = '*';

	/* Skip $X marks */
	CmtMark(&p, &gn);

	/* Delete leading comment marks of length skipping the "/" */
	Tool_DeleteCommentMarks(pcur, &p, 1);

	*gn++ = ' ';
	for(;;)
	{
		if(*p == '\n')
		{
			while(_isspace(*p))
			{
				p++;
			}

			while(*p == '*')
			{
				if(p[0] == '*' && p[1] == '/') goto End;
				p++;
			}
		}

		/* Check for "*\n" */
		if(*p == '\n') continue;
		if(p[0] == '*' && p[1] == '/') goto End;
		if(Tool_DeleteCommentMarks(pcur, &p, 3)) break;

		/* Copy to end of line */
		while(*p)
		{
			if(p[0] == '*' && p[1] == '/') goto End;

			if(*p == '\n')
			{
				/* eat trailing '*' (i.e. boxed comments) */
				if(p[-1] == '*') gn--;

				/* store EOL and out of loop */
				*gn++ = '\n';
				break;
			}
			else
			{
				*gn++ = *p++;
			}
		}
	}

End:
	/* Add an empty space at the end */
	if(gn[-1] != ' ') *gn++ = ' ';

	/* End mark */
	*gn++ = '*';
	*gn++ = '/';
	*gn++ = 0;

	/* Replace old comment with new one */
	free(pcur->pc_Value);
	pcur->pc_Value = memo;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_ConcatCmt(FileDes *pfile, token *pcur)
{
	/*~~~~~~~*/
	char	*p;
	/*~~~~~~~*/

	p = (char *) __malloc__(strlen(pcur->pc_Value) + strlen(pcur->pst_Next->pc_Value) + 10);
	strcpy(p, pcur->pc_Value);
	p[strlen(p) - 2] = (char) Config.CharSplit;
	p[strlen(p) - 1] = 0;
	strcat(p, pcur->pst_Next->pc_Value + 3);
	Tool_UnlinkToken(pfile, pcur->pst_Next);
	free(pcur->pc_Value);
	pcur->pc_Value = p;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_SplitCmtFirstLine(token *pcur)
{
	/*~~~~~~~~~~~~~~~~~~~~*/
	char	*p;
	char	*gn, *memospace;
	char	*memo;
	int		colcur, colbeg;
	int		colend;
	char	first;
	/*~~~~~~~~~~~~~~~~~~~~*/

	p = pcur->pc_Value;
	first = 1;

	/* Do not indent a fixed comment */
	if(FixedComment(pcur)) return;
	memo = (char *) __malloc__(strlen(p) * 2);

	/* Begin mark */
	gn = memo;
	*gn++ = '/';
	*gn++ = '*';
	if(Config.JavaDoc) *gn++ = '*';

	/* Marked comments */
	CmtMark(&p, &gn);
	colbeg = pcur->Col + 5;
	if(!Config.CmtFirstLineBreakFirst) goto mark1;
	for(;;)
	{
forceeol:
		if(_isspace(*p)) p++;
		if(p[0] == '*' && p[1] == '/') goto End;
		*gn++ = '\n';
		if(pcur->CppComment && Config.CmtKeepCpp)
		{
		}
		else
		{
			*gn++ = ' ';
			if(Config.CmtFirstLineFillStar)
				*gn++ = '*';
			else
				*gn++ = ' ';
		}

		*gn++ = ' ';
mark1:
		colcur = colbeg;
		memospace = NULL;
		for(;;)
		{
			if(*p == Config.CharSplit)
			{
				*gn++ = *p++;
				goto forceeol;
			}

			/* Keeping EOL */
			if(*p == '\n')
			{
				goto forceeol;
			}

			if(_isspace(*p))
			{
				colend = colbeg + Config.LineLenCmtFirstLine;
				if(colend > Config.LineLenCmt) colend = Config.LineLenCmt;
				if(colcur >= colend) break;
				memospace = p;
			}

			if(p[0] == '*' && p[1] == '/') goto End;
			*gn++ = *p++;
			first = 0;
			colcur++;
		}

		colend = colbeg + Config.LineLenCmtFirstLine;
		if(colend > Config.LineLenCmt) colend = Config.LineLenCmt;
		if((colcur >= colend) && memospace)
		{
			gn -= (p - memospace);
			p = memospace;
		}
	}

End:
	while(_isspace(gn[-1])) gn--;

	/* End mark */
	if(Config.CmtFirstLineBreakLast) *gn++ = '\n';
	*gn++ = ' ';
	*gn++ = '*';
	*gn++ = '/';
	*gn++ = 0;

	/* Replace old comment with new one */
	free(pcur->pc_Value);
	pcur->pc_Value = memo;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_SplitCmtFct(token *pcur, int sep)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	int		lenw;
	char	*p, *p1, *p2;
	char	*gn, *memospace;
	int		len;
	char	*memo, mem;
	int		colcur, i;
	char	okmotnew;
	char	first;
	int		lencateg, coldes;
	char	justaftercateg;
	int		offset, fin;
	int		style, colc, colc1;
	char	*pp1, *pp2;
	int		isEmpty;
	int		isNewAt;
	int		firstCol;
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	/* FIXME: Flag to decide if the comment is empty?? */
	isEmpty = strlen(pcur->pc_Value) < 6;

	/* Turn off cpp comment convertions if we're doing java doc */
	if(Config.FctJavaDoc) pcur->CppComment = 0;

	lenw = 0;
	p = pcur->pc_Value;
	first = 1;
	lencateg = 0;
	coldes = Config.TabSize;
	if(Config.FctJavaDoc)
		coldes = 1;
	else
		coldes = Config.TabSize;
	pp1 = pp2 = NULL;
	colc1 = 0;
	memospace = 0;

	/* Do not indent a fixed comment */
	if(FixedComment(pcur)) return;
	len = strlen(p) * 8;
	if(len < 1024) len = 1024;
	memo = (char *) __malloc__(len);

	/* Begin mark */
	gn = memo;

	*gn++ = '/';
	*gn++ = '*';
	if(Config.FctJavaDoc) *gn++ = '*';

	/* Marked comments */
	CmtMark(&p, &gn);
	offset = 0;

	/* Separator */
	if(Config.CmtSepBreak)
		*gn++ = '\n';
	else
		offset = gn - memo;
	*gn++ = ' ';

	fin = Config.LineLenCmt - (pcur->StmtLevel + pcur->IndentLevel) * Config.TabSize - 1;
	for(i = offset; i < fin; i++) *gn++ = (char) sep;

	justaftercateg = 1;
	isNewAt = 1;
	for(;;)
	{
forceeol:
		*gn++ = '\n';
		colcur = 0;
		if(Config.CmtSeparatorStar)
		{
			*gn++ = ' ';
			*gn++ = '*';
			colcur = 2;
		}

		for(i = 0; i < coldes; i++)
		{
			colcur++;
			*gn++ = ' ';
		}

		if(_isspace(*p))
		{
			memospace = p;
			p++;
		}

		/* Detect a keyword */
		memospace = NULL;
		firstCol = colcur;
		for(;;)
		{
			p1 = p;
			if(*p == Config.CharSplit)
			{
				*gn++ = *p++;
				goto forceeol;
			}

			/* Keeping EOL */
			if(*p == '\n')
			{
				goto forceeol;
			}

			if((*p == '@') && Config.SplitBeforeAtInFctCmts)
			{
				if(isNewAt && (firstCol != colcur))
				{
					/* New AT not at beginning of a line so force an EOL */
					isNewAt = 0;
					goto forceeol;
				}
				else
				{
					/* Now output '@' at start watching for another @ */
					*gn++ = *p++;
					isNewAt = 1;
				}
			}

			if(!Lisword(*p) && !_isspace(*p)) goto copy;
			while(Lisword(*p1)) p1++;
			p2 = p1;
			while(_isspace(*p2)) p2++;
			if(*p2 == ':')
			{
				if(p2 != p1) *p2 = ' ';
				mem = *p1;
				*p1 = 0;
				lenw = strlen(p);
				okmotnew = Tool_IsSpecialWord(p);
				if(p2 != p1) *p2 = ':';
				*p1 = mem;
				if(!okmotnew) goto nomot;
				*p2 = ' ';
				*p1 = ':';
				gn--;
				while(*gn == ' ') gn--;
				gn++;
				if((gn[-1] != '\n') || !first)
				{
					if(Config.CmtCategCtyle == 0)
						if(gn[-1] != '\n') *gn++ = '\n';
					*gn++ = '\n';
					memospace = NULL;
				}

				/* Set first indentation by inserting spaces */
				for(i = 0; i < Config.TabSize; i++) *gn++ = ' ';
				colcur = Config.TabSize;

				*gn++ = *p++;

				/* Copy the category */
				colcur++;
				while(*p != ':')
				{
					*gn++ = *p++;
					colcur++;
				}

				colcur++;
				*gn++ = *p++;

				/* Add spaces to indent text of category */
				coldes = Tool_ToTab(colcur);
				if(Config.CmtCategCtyle == 1)
					coldes += Config.TabSize;
				else
				{
					if(coldes == colcur)
						coldes += Config.TabSize;
					else if(lenw < Config.TabSize)
						coldes += Config.TabSize;
				}

				for(i = colcur; i < coldes; i++)
				{
					*gn++ = ' ';
					colcur++;
				}

				if(_isspace(*p)) p++;
				if(okmotnew != 2) justaftercateg = 1;
			}
			else
			{
nomot:
				if(colcur < Config.LineLenCmt)
				{
					while(Lisword(*p))
					{
						*gn++ = *p++;
						justaftercateg = 0;
						colcur++;
					}
				}

copy:
				first = 0;
				if(colcur < Config.LineLenCmt)
				{
					/*$F SWG 12/04/2001 */
					while(!Lisword(*p) && !_isspace(*p) && *p != Config.CharSplit)
					{
						if(p[0] == '*' && p[1] == '/') goto End;

						/* Format */
						if(*p == '$')
						{
							style = 0;
re:
							*gn++ = *p++;
							colcur++;
							switch(*p)
							{
							case 'a':
							case 'A':
								goto re;

							case 'c':
							case 'C':
								style = 1;
								goto re;

							case 'r':
							case 'R':
								style = 2;
								goto re;
							}

							if(style)
							{
								*gn++ = ' ';
								colcur++;

								pp1 = p;
								while(_isspace(*pp1)) pp1++;
								pp2 = pp1;
								while(*pp1 != '\n' && *pp1 != Config.CharSplit && (*pp1 != '*' || pp1[1] != '/'))
									pp1++;
							}

							/* Center */
							if(style)
							{
								colc = pp1 - pp2 - 1;
								if(style == 1)
								{
									colc1 = fin / 2;
									colc1 -= colc / 2;
									colc1 -= colcur;
								}

								/* Right */
								else if(style == 2)
								{
									colc1 = fin - colc + 1;
									colc1 -= colcur;
								}

								while(colc1 > 0)
								{
									*gn++ = ' ';
									colcur++;
									colc1--;
								}

								while(pp2 != pp1)
								{
									*gn++ = *pp2++;
									colcur++;
								}

								p = pp2;
							}
						}
						else
						{
							*gn++ = *p++;
							colcur++;
						}
					}
				}

				/* Keeping EOL */
				if(*p == '\n') goto forceeol;

				/* Copy spaces */
				if(colcur < Config.LineLenCmt)
				{
					if(_isspace(*p))
					{
						if(!_isspace(gn[-1]))
						{
							memospace = p;
							*gn++ = *p++;
							colcur++;
						}
						else
							p++;
					}
				}
			}

			if(colcur >= Config.LineLenCmt)
			{
				if(memospace)
				{
					gn -= (p - memospace);
					p = memospace;
				}
				break;
			}

			if(p[0] == '*' && p[1] == '/') goto End;
		}

		if(gn >= (memo + len))
		{
			memo[30] = '.';
			memo[31] = '.';
			memo[32] = '.';
			memo[34] = 0;
			Fatal("Comment too long", memo);
		}
	}

End:
	gn--;
	while(*gn == ' ') gn--;
	gn++;
	if(gn[-1] != '\n') *gn++ = '\n';
	*gn++ = ' ';

	offset = 0;
	if((!Config.CmtSepBreak) && (pcur->CppComment) && (Config.CmtKeepCpp)) offset = 2;
	for(i = offset; i < Config.LineLenCmt - (pcur->StmtLevel + pcur->IndentLevel) * Config.TabSize - 1; i++)
		*gn++ = (char) sep;

	if(Config.CmtSepBreak) *gn++ = '\n';
	*gn++ = ' ';
	*gn++ = '*';
	*gn++ = '/';
	*gn++ = 0;

	/* Replace old comment with new one */
	free(pcur->pc_Value);
	pcur->pc_Value = memo;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_SplitEndLineCmt(token *pcur)
{
	/*~~~~~~~~~~~~~~~*/
	char	*memo;
	char	*p, *gn;
	char	*memospace;
	int		colcur;
	/*~~~~~~~~~~~~~~~*/

	memospace = NULL;
	colcur = pcur->Col;

	/* Do not indent a fixed comment */
	if(FixedComment(pcur)) return;
	memo = (char *) __malloc__(strlen(pcur->pc_Value) * 50);

	/* Begin mark */
	gn = memo;
	p = pcur->pc_Value;
	*gn++ = *p++;
	*gn++ = *p++;
	*gn++ = *p++;
	colcur += 3;
	for(;;)
	{
		while(!_isspace(*p))
		{
			if(*p == Config.CharSplit)
			{
				*gn++ = *p++;
				goto forceeol;
			}

			if(p[0] == '*' && p[1] == '/') break;
			*gn++ = *p++;
			colcur++;
		}

		if(colcur < Config.LineLenCmt)
		{
			while(_isspace(*p))
			{
				*gn++ = *p++;
				colcur++;
				memospace = p;
			}
		}

		if(colcur >= Config.LineLenCmt)
		{
forceeol:
			if(memospace)
			{
				gn -= (p - memospace);
				p = memospace;
			}

			gn--;
			while(_isspace(*gn)) gn--;
			gn++;

			/*
			 * Set indent level of comment, so when it will be splitter later, it will be
			 * correctly indent.
			 */
			*gn++ = '\n';
			memospace = NULL;
			pcur->IndentLevel = (pcur->Col / Config.TabSize) - pcur->StmtLevel;
			colcur = pcur->Col + 3;
			if(!pcur->CppComment || !Config.CmtKeepCpp)
			{
				*gn++ = ' ';
				*gn++ = '*';
				*gn++ = ' ';
			}
			else
			{
				*gn++ = ' ';
			}

			if(_isspace(*p))
			{
				p++;
				colcur++;
			}
		}

		if(p[0] == '*' && p[1] == '/') break;
	}

	/* End mark */
	if(gn[-1] != ' ') *gn++ = ' ';
	*gn++ = '*';
	*gn++ = '/';
	*gn++ = 0;

	/* Replace old comment with new one */
	free(pcur->pc_Value);
	pcur->pc_Value = memo;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_InCmt(token *pcur)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	char	*p, *p1, *memin, *memo, *gn, *cgn;
	char	*memospace;
	char	*params[100];
	int		paramsseen[100];
	int		i, inumpar;
	int		maxlen;
	int		colcur, colind, tmp;
	char	cmem;
	token	*pnext;
	char	first;
	char	res;
	int		lenin, totallen;
	char	justafterparam;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	inumpar = 0;
	maxlen = 0;
	first = 1;
	lenin = 0;
	totallen = 0;
	memospace = 0;

	/* Do not indent a fixed comment */
	if(FixedComment(pcur)) return;
	p = pcur->pc_Value;
	while(*p)
	{
		if(Lisword(*p))
		{
			memin = p;
			while(Lisword(*p)) p++;
			p1 = p;
			while(_isspace(*p1)) p1++;
			if(*p1 == ':')
			{
				*p = 0;
				res = Tool_IsSpecialWord(memin);
				if(res == 2)
				{
					lenin = strlen(memin) + 1;
					*p = ':';
					goto foundin;
				}

				*p = ':';
			}
		}

		p++;
	}

	return;
foundin:
	justafterparam = 0;

	/*
	 * Get all params names. Search first the beginning of the following definition
	 * (function def, function decl or macro).
	 */
	pnext = pcur;
	while(!pnext->InFctDef && !pnext->InFctDecl && !pnext->MacroName) pnext = NextToken(pnext);
	while(!pnext->EndListParam)
	{
		while(!pnext->EndListParam && !pnext->IsFctParamName) pnext = NextToken(pnext);
		if(pnext->IsFctParamName)
		{
			paramsseen[inumpar] = 0;
			params[inumpar++] = pnext->pc_Value;
			if((int) strlen(pnext->pc_Value) >= maxlen) maxlen = strlen(pnext->pc_Value);
			totallen += strlen(pnext->pc_Value);
		}

		if(!pnext->EndListParam) pnext = NextToken(pnext);
	}

	if(!inumpar) return;
	if(maxlen % Config.TabSize == 0) maxlen += Config.TabSize;
	if(maxlen % Config.TabSize == 3) maxlen += Config.TabSize;
	memin = pcur->pc_Value;
	memo = (char *) __malloc__(strlen(pcur->pc_Value) * maxlen * 50);
	gn = memo;

	/* Copy all the comment from the beginning */
	p++;
	while(p != memin) *gn++ = *memin++;
	p = memin;

	/* Indent to common column */
	colcur = Config.TabSize + lenin;
	tmp = colcur;
	if(lenin < Config.TabSize) tmp += Config.TabSize;
	if(Config.CmtCategCtyle == 1) tmp += Config.TabSize;
	tmp = Tool_ToTab(tmp);
	while(colcur != tmp)
	{
		*gn++ = ' ';
		colcur++;
	}

	colind = colcur + maxlen;
	colind = Tool_ToTab(colind);

	/* Special character ? */
	while(_isspace(*p)) p++;
	if(*p == CMTMARK)
	{
		p++;
		*gn = 0;
		for(i = 0; i < inumpar; i++)
		{
			while(colcur != tmp)
			{
				strcat(gn, " ");
				colcur++;
			}

			strcat(gn, params[i]);
			strcat(gn, "\n");
			colcur = 0;
		}

		gn += strlen(gn);
	}

	for(;;)
	{
		/* Get param name */
		if(Lisword(*p) || *p == '.')
		{
			if(*p == Config.CharSplit)
			{
				*gn++ = *p++;
				goto forceeol;
			}

			memin = p;
			while(Lisword(*memin) || *memin == '.') memin++;
			cmem = *memin;
			*memin = 0;
			for(i = 0; i < inumpar; i++)
			{
				if(!paramsseen[i] && !_stricmp(p, params[i]))
				{
					paramsseen[i] = 1;
					break;
				}

				if(Tool_IsSpecialWord(p) && cmem == ':')
				{
					*memin = cmem;
					*gn++ = '\n';
					*gn++ = '\n';
					memospace = NULL;
					colcur = 0;
					while(colcur != Config.TabSize)
					{
						*gn++ = ' ';
						colcur++;
					}

					goto End;
				}
			}

			*memin = cmem;

			/* Find a param name ? */
			if(i != inumpar)
			{
				justafterparam = 1;
				if(!first)
				{
					cgn = gn - 1;
					while(cgn > memo && _isspace(*cgn))
					{
						if(*cgn == '\n') break;
						cgn--;
					}

					gn = cgn + 1;
					if(gn[-1] != '\n') *gn++ = '\n';
					memospace = NULL;
					colcur = 0;
					while(colcur != tmp)
					{
						*gn++ = ' ';
						colcur++;
					}
				}

				first = 0;
				while(p != memin)
				{
					*gn++ = *p++;
					colcur++;
				}

				/* Add spaces to indent on column for var explanations */
				while(colcur < colind)
				{
					*gn++ = ' ';
					colcur++;
				}

				while(_isspace(*p)) p++;
			}
			else
			{
				/*
				 * We found a word just after category that is not a valid param. We force the
				 * next real good param to be alone on line.
				 */
				if(first) first = 0;
				while(p != memin)
				{
					if(Lisword(*p) && justafterparam)
					{
						*gn++ = *p++;
						justafterparam = 0;
					}
					else
						*gn++ = *p++;
					colcur++;
				}
			}
		}
		else
		{
			/* Copy param */
			while(!Lisword(*p) && *p != '.')
			{
				if(p[0] == '*' && p[1] == '/') goto End;
				if((p[0] == Config.CharCmt3) && (p[1] == Config.CharCmt3)) goto End;
				if(p[0] == '\n' && p[1] == ' ' && p[2] == Config.CharCmt3 && p[3] == Config.CharCmt3) goto End;
				if(_isspace(*p))
				{
					if(colcur >= Config.LineLenCmt) break;
					memospace = p;
					if(*p == '\n')
					{
						p++;
						continue;
					}

					if((*p == ' ') && (gn[-1] == ' '))
					{
						p++;
						continue;
					}
				}

				*gn++ = *p++;
				colcur++;
			}
		}

		if(p[0] == '*' && p[1] == '/') goto End;
		if((p[0] == Config.CharCmt3) && (p[1] == Config.CharCmt3)) goto End;
		if(p[0] == '\n' && p[1] == ' ' && (p[2] == Config.CharCmt3) && (p[3] == Config.CharCmt3)) goto End;
		if(colcur >= Config.LineLenCmt)
		{
			if(memospace)
			{
				gn -= (p - memospace);
				p = memospace;
			}

forceeol:
			*gn++ = '\n';
			memospace = NULL;
			colcur = 0;
			while(colcur < colind)
			{
				*gn++ = ' ';
				colcur++;
			}

			while(_isspace(*p)) p++;
		}
	}

End:
	if((p[0] == Config.CharCmt3) && (p[1] == Config.CharCmt3))
	{
		*gn++ = '\n';
		*gn++ = ' ';
	}

	if(gn[-1] == '\n' && *p == '\n') p++;
	while(*p) *gn++ = *p++;
	*gn++ = 0;
	free(pcur->pc_Value);
	pcur->pc_Value = memo;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_InsertSepAfter(FileDes *pfile, token *pcur, token *pref, int cchar, int num)
{
	/*~~~~~~~~~~~~*/
	token	*pnext1;
	int		i;
	/*~~~~~~~~~~~~*/

	Tool_InsertTokenAfter(pfile, pcur, TOKEN_CCMT);
	pnext1 = pcur->pst_Next;
	pnext1->AddCmt = 1;
	pnext1->StmtLevel = pref->StmtLevel;
	pnext1->IndentLevel = pref->IndentLevel;
	pnext1->ForceEOLAfter = pref->ForceEOLAfter;
	if(num == -1) num = Config.LineLenCmt;
	pnext1->pc_Value = (char *) __malloc__(num * 2);
	pnext1->pc_Value[0] = '/';
	pnext1->pc_Value[1] = '*';
	for(i = 2; i < num - (pnext1->StmtLevel + pnext1->IndentLevel) * Config.TabSize - 2; i++)
		pnext1->pc_Value[i] = (char) cchar;
	pnext1->pc_Value[i] = '*';
	pnext1->pc_Value[i + 1] = '/';
	pnext1->pc_Value[i + 2] = 0;
	pnext1->CmtIndent = 1;
	Tool_ForceEOLBefore(pnext1, 1);
	Tool_ForceEOLAfter(pnext1, 1);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_InsertSingleSep(FileDes *pfile, token *pcur, char sep)
{
	/*~~~~~~~~~~~~~~*/
	char	*memo;
	int		i, i_End;
	char	*ptmp;
	char	hasinsert;
	/*~~~~~~~~~~~~~~*/

	pfile = pfile;
	memo = (char *) __malloc__(Config.LineLenCmt + 10);
	memo[0] = '/';
	memo[1] = '*';
	memo[2] = pcur->pc_Value[2];
	memo[3] = pcur->pc_Value[3];
	memo[4] = pcur->pc_Value[4];
	ptmp = pcur->pc_Value + 5;
	hasinsert = 0;
	i_End = Config.LineLenCmt - 2;
	if(!pcur->pst_Next) return;
	i_End -= (pcur->pst_Next->IndentLevel * Config.TabSize) + (pcur->pst_Next->StmtLevel * Config.TabSize);
	for(i = 5; i < i_End; i++)
	{
		if(ptmp && (ptmp[1] != sep) && (ptmp[1] != '*' || ptmp[2] != '/'))
		{
			memo[i] = *(ptmp++);
			hasinsert = 1;
		}
		else
		{
			ptmp = NULL;
			if(hasinsert)
				memo[i] = ' ';
			else
				memo[i] = sep;
			hasinsert = 0;
		}
	}

	memo[i] = '*';
	memo[i + 1] = '/';
	memo[i + 2] = 0;
	free(pcur->pc_Value);
	pcur->pc_Value = memo;
	Tool_ForceEOLAfter(pcur, 1);
	pcur->CmtIndent = 1;
	pcur->StmtLevel = pcur->pst_Next->StmtLevel;
	pcur->IndentLevel = pcur->pst_Next->IndentLevel;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_AddTag(FileDes *pfile, token *pcur)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	char	*p;
	char	*memo;
	char	asz_Temp[128], *psz_Temp;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	p = pcur->pc_Value;
	if(p[2] != CMTMARK || p[3] != 'T') return;
	memo = (char *) __malloc__(512);
	memo[0] = '/';
	memo[1] = '*';
	memo[2] = CMTMARK;
	memo[3] = 'T';
	memo[4] = ' ';
	psz_Temp = strrchr(pfile->psz_FileName, '\\');
	if(!psz_Temp)
		sprintf(&memo[5], pfile->psz_FileName);
	else
		sprintf(&memo[5], psz_Temp + 1);
	strcat(memo, " GC");
	sprintf(memo, "%s %d.%03d ", memo, VERSION, REVISION);
	strcat(memo, _strdate(asz_Temp));
	strcat(memo, " ");
	strcat(memo, _strtime(asz_Temp));
	strcat(memo, " */");
	free(pcur->pc_Value);
	pcur->pc_Value = memo;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_AddEmpty(FileDes *pfile, token *pcur, char *val, int level, int subid)
{
	/*~~~~~~~~~~~*/
	token	*pprev;
	char	ac;
	/*~~~~~~~~~~~*/

	ac = 1;
	if(pcur->pst_Prev) ac = pcur->pst_Prev->ForceEOLAfter;
	Tool_InsertTokenBefore(pfile, pcur, TOKEN_CCMT);
	pprev = PrevToken(pcur);
	pprev->i_SubID = subid;
	pprev->pc_Value = _strdup(val);
	if(pprev->pst_Prev && pcur->In_PP_Define)
		pprev->InPP = pprev->pst_Prev->InPP;
	else
		pprev->InPP = 0;
	pprev->InFctDef = 0;
	pprev->CmtLevel = (char) level;
	pprev->NoIndent = 0;
	pprev->CppComment = 0;
	Tool_ForceEOLBefore(pprev, 1);
	Tool_ForceEmptyLineBefore(pprev);
	Tool_ForceEOLAfter(pprev, ac);
	pprev->StmtLevel = pprev->pst_Next->StmtLevel;
	pprev->IndentLevel = pprev->pst_Next->IndentLevel;
	pprev->WasEOLAfter = 1;
	return pprev;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_AddEmptyCmtAfterIfMissing(FileDes *pfile, token *pcur)
{
	/*~~~~~~~~~~~*/
	token	*pprev;
	/*~~~~~~~~~~~*/

	if(pcur->pst_Next && (pcur->pst_Next->i_ID != TOKEN_CCMT) && (pcur->pst_Next->i_ID != TOKEN_CPPCMT))
	{
		pprev = pcur;
		pcur = Tool_InsertTokenAfter(pfile, pcur, TOKEN_CCMT);
		pcur->i_SubID = TOKEN_W_CMT_FUNC_PARAM;
		pcur->pc_Value = _strdup(gz_FixmeComment);
		pcur->CppComment = 0;
		pcur->EndDecl = 0;
		pprev->ForceEOLAfter = 0;
	}

	return pcur;
}
