/*$T tools.c GC 1.139 12/15/04 23:59:09 */


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

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_MoveTokenBefore(FileDes *pfile, token *_pst_ToMove, token *pref)
{
	/* Unlink */
	if(_pst_ToMove->pst_Prev) _pst_ToMove->pst_Prev->pst_Next = _pst_ToMove->pst_Next;
	if(_pst_ToMove->pst_Next) _pst_ToMove->pst_Next->pst_Prev = _pst_ToMove->pst_Prev;

	/* Link */
	_pst_ToMove->pst_Next = pref;
	_pst_ToMove->pst_Prev = pref->pst_Prev;
	if(pref->pst_Prev)
		pref->pst_Prev->pst_Next = _pst_ToMove;
	else
		pfile->pst_RootToken = _pst_ToMove;
	pref->pst_Prev = _pst_ToMove;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_MoveTokenAfter(FileDes *pfile, token *_pst_ToMove, token *pref)
{
	/* Unlink */
	if(_pst_ToMove->pst_Prev)
	{
		_pst_ToMove->pst_Prev->pst_Next = _pst_ToMove->pst_Next;
	}

	if(_pst_ToMove->pst_Next)
	{
		_pst_ToMove->pst_Next->pst_Prev = _pst_ToMove->pst_Prev;
	}

	/* Link */
	_pst_ToMove->pst_Prev = pref;
	_pst_ToMove->pst_Next = pref->pst_Next;
	if(pref->pst_Next)
		pref->pst_Next->pst_Prev = _pst_ToMove;
	else
		pfile->pst_LastToken = _pst_ToMove;
	pref->pst_Next = _pst_ToMove;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_UnlinkToken(FileDes *pfile, token *pcur)
{
	/*~~~~~~~~~~~~~~~~~~*/
	token	*pnext, *pret;
	/*~~~~~~~~~~~~~~~~~~*/

	pret = pcur->pst_Next;
	pnext = pcur->pst_Prev;
	if(pnext)
		pnext->pst_Next = pcur->pst_Next;
	else
		pfile->pst_RootToken = pcur->pst_Next;
	pnext = pcur->pst_Next;
	if(pnext)
		pnext->pst_Prev = pcur->pst_Prev;
	else
		pfile->pst_LastToken = pcur->pst_Prev;
	free(pcur->pc_Value);
	free(pcur);
	return pret;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void TokenString(token *pcur)
{
	switch(pcur->i_ID)
	{
	case TOKEN_BREAKLINE:	pcur->pc_Value = _strdup("\\"); break;
	case TOKEN_LBRACE:		pcur->pc_Value = _strdup("{"); break;
	case TOKEN_RBRACE:		pcur->pc_Value = _strdup("}"); break;
	case TOKEN_LPAREN:		pcur->pc_Value = _strdup("("); break;
	case TOKEN_RPAREN:		pcur->pc_Value = _strdup(")"); break;
	}

	if(pcur->ForceEOLAfter) pcur->ForceEOLAfter = 1;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_InsertTokenBefore(FileDes *pfile, token *_pst_Ref, int type)
{
	/*~~~~~~~*/
	token	*p;
	/*~~~~~~~*/

	p = (token *) __malloc__(sizeof(token));
	if(!p) Fatal("Not enough memory", NULL);
	memcpy(p, _pst_Ref, sizeof(token));
	p->pst_Next = _pst_Ref;
	p->pst_Prev = _pst_Ref->pst_Prev;
	if(p->pst_Prev == NULL)
		pfile->pst_RootToken = p;
	else
		p->pst_Prev->pst_Next = p;
	_pst_Ref->pst_Prev = p;
	p->i_ID = type;
	TokenString(p);
	return p;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_InsertTokenAfter(FileDes *pfile, token *_pst_Ref, int type)
{
	/*~~~~~~~*/
	token	*p;
	/*~~~~~~~*/

	p = (token *) __malloc__(sizeof(token));
	if(!p) Fatal("Not enough memory", NULL);
	memcpy(p, _pst_Ref, sizeof(token));
	p->pst_Next = _pst_Ref->pst_Next;
	p->pst_Prev = _pst_Ref;
	if(p->pst_Next == NULL)
		pfile->pst_LastToken = p;
	else
		p->pst_Next->pst_Prev = p;
	_pst_Ref->pst_Next = p;
	p->i_ID = type;
	TokenString(p);
	return p;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
char LastOnLineWithCmt(token *pcur)
{
	if(!pcur) return 1;
	if(pcur->ForceEOLAfter) return 1;
	while(!pcur->ForceEOLAfter)
	{
		pcur = NextToken(pcur);
		if(pcur->i_ID != TOKEN_CCMT) return 0;
	}

	return 1;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_ComputeCol(FileDes *pfile)
{
	/*~~~~~~~~~~~~~*/
	token	*pcur;
	int		col, eol;
	/*~~~~~~~~~~~~~*/

	col = 0;
	eol = 1;
	for(pcur = pfile->pst_RootToken; pcur; pcur = NextToken(pcur))
	{
		if(eol)
		{
			col += (pcur->StmtLevel * Config.TabSize) + (pcur->IndentLevel * Config.TabSize);
		}

		if(pcur->pst_Prev && pcur->pst_Prev->ForceEOLAfter) col += pcur->pst_Prev->AddSpaceAfter;
		pcur->Col = col;
		col += strlen(pcur->pc_Value);
		eol = 0;
		if(pcur->ForceEOLAfter)
		{
			eol = 1;
			col = 0;
			if(pcur->NoIndent) col += pcur->WasSpaceAfter;
		}
		else if(pcur->NoIndent)
			col += pcur->WasSpaceAfter;
		else
		{
			col += pcur->ForceSpaceAfter;
		}
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int Tool_LenTokens(token *pdeb, token *pend)
{
	/*~~~~*/
	int len;
	/*~~~~*/

	len = 0;
	if(!pend) pend = pdeb;
	while(pdeb && pdeb != pend->pst_Next)
	{
		len += strlen(pdeb->pc_Value) + pdeb->ForceSpaceAfter;
		pdeb = NextToken(pdeb);
	}

	return len;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_ToBegLine(token *pcur)
{
	if(!PrevToken(pcur)) return pcur;
	pcur = PrevToken(pcur);
	while(pcur && !pcur->ForceEOLAfter)
	{
		if(!PrevToken(pcur)) return pcur;
		pcur = PrevToken(pcur);
	}

	pcur = NextToken(pcur);
	return pcur;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_ToEndLine(token *pcur)
{
	while(pcur && pcur->i_ID && !pcur->ForceEOLAfter) pcur = NextToken(pcur);
	return pcur;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_PrevValid(token *pcur)
{
	if(!pcur) return NULL;
	do
	{
		pcur = PrevToken(pcur);
	} while(pcur && pcur->i_ID == TOKEN_CCMT);
	return pcur;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_NextValid(token *pcur)
{
	if(!pcur) return NULL;
	do
	{
		pcur = NextToken(pcur);
	} while(pcur && ((pcur->i_ID == TOKEN_CCMT) || (pcur->i_ID == TOKEN_CPPCMT)));
	return pcur;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_SearchToken(token *pcur, int id)
{
	while(pcur && pcur->i_ID != id) pcur = NextToken(pcur);
	return pcur;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_ToRelationNext(token *pcur)
{
	/*~~~~~~~*/
	int count;
	int c1, c2;
	/*~~~~~~~*/

	count = 1;
	c1 = c2 = pcur->i_ID;
	if(c1 == TOKEN_LPAREN)
		c2 = TOKEN_RPAREN;
	else if(c1 == TOKEN_LBRACE)
		c2 = TOKEN_RBRACE;
	else if(c1 == TOKEN_LESS)
		c2 = TOKEN_GREAT;
	else if(c1 == TOKEN_LARRAY)
		c2 = TOKEN_RARRAY;
	pcur = NextToken(pcur);
	while(count && pcur)
	{
		if(pcur->i_ID == c1) count++;
		if(pcur->i_ID == c2) count--;
		if(count) pcur = NextToken(pcur);
	}

	return pcur;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *Tool_ToRelationPrev(token *pcur)
{
	/*~~~~~~~*/
	int count;
	int c1, c2;
	/*~~~~~~~*/

	count = 1;
	c1 = c2 = pcur->i_ID;
	if(c1 == TOKEN_RPAREN)
		c2 = TOKEN_LPAREN;
	else if(c1 == TOKEN_RBRACE)
		c2 = TOKEN_LBRACE;
	else if(c1 == TOKEN_GREAT)
		c2 = TOKEN_LESS;
	pcur = PrevToken(pcur);
	while(count && pcur)
	{
		if(pcur->i_ID == c1) count++;
		if(pcur->i_ID == c2) count--;
		if(count) pcur = PrevToken(pcur);
	}

	return pcur;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
token *EOLIfWeCan(token *pcur)
{
	/*~~~~~~~~~~~*/
	token	*pnext;
	/*~~~~~~~~~~~*/

	/*
	 * Force EOL to be placed after some special tokens. For example, never put an EOL
	 * before a semicolon.
	 */
	if(!pcur->WasEOLAfter)
	{
		pnext = NextToken(pcur);
		while((pnext->i_ID == TOKEN_COMMA) || ((pnext->i_ID == TOKEN_SEMICOL) && (!pnext->InParen)))
		{
			pcur = pnext;
			pnext = NextToken(pcur);
		}
	}

	return pcur;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_ForceEOLAfterComments(token *pcur, int num)
{
	while(NextToken(pcur) && NextToken(pcur)->i_ID == TOKEN_CCMT)
	{
		if(pcur->WasEOLAfter) break;
		pcur = NextToken(pcur);
	}

	Tool_ForceEOLAfter(pcur, num);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_ForceEOLAfter(token *pcur, int num)
{
	pcur = EOLIfWeCan(pcur);
	if(pcur->ForceEOLAfter < num) pcur->ForceEOLAfter = (char) num;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_ForceExactEOLAfter(token *pcur, int num)
{
	pcur = EOLIfWeCan(pcur);
	if(!pcur) return;
	pcur->ForceEOLAfter = (char) num;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_ForceEOLBefore(token *pcur, int num)
{
	pcur = PrevToken(pcur);
	if(!pcur) return;
	Tool_ForceEOLAfter(pcur, num);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_ForceExactEOLBefore(token *pcur, int num)
{
	pcur = PrevToken(pcur);
	if(!pcur) return;
	Tool_ForceExactEOLAfter(pcur, num);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_ForceSpaceAfter(token *pcur, int num)
{
	if(!pcur) return;
	pcur->ForceSpaceAfter = (char) num;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_ForceSpaceBefore(token *pcur, int num)
{
	/*~~~~~~~~~~~*/
	token	*pprev;
	/*~~~~~~~~~~~*/

	pprev = PrevToken(pcur);
	if(pprev) Tool_ForceSpaceAfter(pprev, num);
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_ForceEmptyLineAfter(token *pcur)
{
	/*~~~~~~~~~~~*/
	token	*pnext;
	token	*pp;
	/*~~~~~~~~~~~*/

	pcur = EOLIfWeCan(pcur);

	/* Never add empty lines after some tokens */
	switch(pcur->i_ID)
	{
	case TOKEN_LBRACE:
	case TOKEN_LPAREN:
	case TOKEN_COLON:
	case TOKEN_RPAREN:
		if(!pcur->InPP || !pcur->pst_Next || pcur->pst_Next->InPP) return;
	}

	switch(pcur->i_SubID)
	{
	case TOKEN_W_ELSE:	return;
	}

	pnext = NextToken(pcur);

	/* Don't add empty line between 2 lines that are not at the same indent level */
	pp = Tool_ToBegLine(pcur);
	if(pp->StmtLevel < pnext->StmtLevel)
	{
		if(pp->i_ID != TOKEN_PP || pp->i_SubID != TOKEN_PP_ENDIF) return;
	}

	if(pnext->i_ID == TOKEN_PP && pnext->i_SubID == TOKEN_PP_ENDIF) return;

	/* Never add empty lines before some tokens */
	switch(pnext->i_ID)
	{
	case TOKEN_LBRACE:
	case TOKEN_LPAREN:
	case TOKEN_RBRACE:
	case TOKEN_RPAREN:
		return;

	case TOKEN_SPECWORD:
		switch(pnext->i_SubID)
		{
		case TOKEN_W_ELSE:	return;
		}
		break;
	}

	if(pnext->WizardCmt) return;

	pcur->ForceEOLAfter = 2;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_ForceEmptyLineBefore(token *pcur)
{
	/*~~~~~~~~~~~~~~~~~~~*/
	token	*pnext, *pprev;
	/*~~~~~~~~~~~~~~~~~~~*/

	pprev = PrevToken(pcur);
	if(pprev)
	{
		/* Don't add an empty line after a comment */
		if(pprev->i_ID == TOKEN_CCMT && pcur->i_ID != TOKEN_CCMT)
		{
			return;
		}

		/* Don't add an empty line before a comment that is just before a right brace. */
		if(pprev->i_ID == TOKEN_CCMT)
		{
			pnext = NextToken(pcur);
			switch(pnext->i_ID)
			{
			case TOKEN_RBRACE:	return;
			}
		}

		if(pprev->WizardCmt) return;

		Tool_ForceEmptyLineAfter(pprev);
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_SetFlag(token *pbeg, token *pend, char *flag, char set)
{
	/*~~~*/
	int of;
	/*~~~*/

	if(!pbeg) return;
	if(!pend) return;
	of = (char *) flag - (char *) pbeg;
	for(; pbeg != pend->pst_Next; pbeg = NextToken(pbeg)) *(((char *) pbeg) + of) = set;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void Tool_IncIndentLevel(token *pbeg, token *pend, int num)
{
	if(!pbeg) return;
	if(!pend) return;
	pbeg->IndentLevel += num;
	while(pbeg != pend)
	{
		pbeg = NextToken(pbeg);
		if(!pbeg) break;
		pbeg->IndentLevel += num;
	}
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
int Tool_ToTab(int tab)
{
	/*~~*/
	int l;
	/*~~*/

	l = (tab / Config.TabSize) * Config.TabSize;
	if(l < tab) l += Config.TabSize;
	return l;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
char Tool_IsSpecialWord(char *p)
{
	/*~~*/
	int i;
	/*~~*/

	for(i = 0; i < gi_NumCmtCateg; i++)
	{
		if(!strcmp(p, gast_CmtCateg[i].mpsz_Name)) return (char) gast_CmtCateg[i].mi_Type;
	}

	return 0;
}

/*
 =======================================================================================================================
    returns Pointer to the level character or NULL if there is no Level number
 =======================================================================================================================
 */
char *Tool_pLevelComment(token *pcur)
{
	if(LevelCommentEx(pcur, 0))
		return(&pcur->pc_Value[3]);
	else if(LevelCommentEx(pcur, 1))
		return(&pcur->pc_Value[4]);
	else
		return NULL;
}
