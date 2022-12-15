

#include "common.h"
//#include "sys.h"
#include "qclib/progslib.h"
#include "qclib/csqc.h"

pubprogfuncs_t *QCLib;


#include <stdio.h>
//copy file into buffer. note that the buffer will have been sized to fit the file (obtained via FileSize)
void *PDECL Sys_ReadFile(const char *fname, unsigned char *(PDECL *buf_get)(void *ctx, size_t len), void *buf_ctx, size_t *out_size, pbool issourcefile)
{
	void *buffer;
	int len;
	FILE *f;
	if (!strncmp(fname, "src/", 4))
		fname += 4;	//skip the src part
	f = fopen(fname, "rb");
	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	len = ftell(f);

	buffer = buf_get(buf_ctx, len);
	fseek(f, 0, SEEK_SET);
	fread(buffer, 1, len, f);
	fclose(f);

	*out_size = len;
	return buffer;
}
//Finds the size of a file.
int Sys_FileSize(const char *fname)
{
	int len;
	FILE *f;
	if (!strncmp(fname, "src/", 4))
		fname += 4;	//skip the src part
	f = fopen(fname, "rb");
	if (!f)
		return -1;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fclose(f);
	return len;
}
//Writes a file.
pbool Sys_WriteFile(const char *fname, void *data, int len)
{
	FILE *f;
	f = fopen(fname, "wb");
	if (!f)
		return 0;
	fwrite(data, 1, len, f);
	fclose(f);
	return 1;
}
//Called when the qc library has some sort of serious error.
static void CSQC_Abort(char *format, ...)	//an error occured.
{
	va_list		argptr;
	char		string[1024];

	va_start(argptr, format);
	vsnprintf(string, sizeof(string) - 1, format, argptr);
	va_end(argptr);

	Con_Printf("CSQC_Abort: %s\nShutting down csqc\n", string);
}




void PF_puts(pubprogfuncs_t *prinst, struct globalvars_s *gvars)
{
	char *s;
	s = prinst->VarString(prinst, 0);

	Con_Printf("%s", s);
}

void PF_putv(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_Printf("%f %f %f\n", G_FLOAT(OFS_PARM0 + 0), G_FLOAT(OFS_PARM0 + 1), G_FLOAT(OFS_PARM0 + 2));
}

void PF_putf(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_Printf("%f\n", G_FLOAT(OFS_PARM0));
}

#ifdef _WIN32
#define Q_snprintfz _snprintf
#define Q_vsnprintf _vsnprintf
#else
#define Q_snprintfz snprintf
#define Q_vsnprintf vsnprintf
#endif
void QCBUILTIN PF_sprintf_internal(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals, const char *s, int firstarg, char *outbuf, int outbuflen)
{
	const char *s0;
	char *o = outbuf, *end = outbuf + outbuflen, *err;
	int width, precision, thisarg, flags;
	char formatbuf[16];
	char *f;
	int argpos = firstarg;
	int isfloat;
	static int dummyivec[3] = { 0, 0, 0 };
	static float dummyvec[3] = { 0, 0, 0 };

#define PRINTF_ALTERNATE 1
#define PRINTF_ZEROPAD 2
#define PRINTF_LEFT 4
#define PRINTF_SPACEPOSITIVE 8
#define PRINTF_SIGNPOSITIVE 16

	formatbuf[0] = '%';

#define GETARG_FLOAT(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_FLOAT(OFS_PARM0 + 3 * (a))) : 0)
#define GETARG_VECTOR(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_VECTOR(OFS_PARM0 + 3 * (a))) : dummyvec)
#define GETARG_INT(a) (((a)>=firstarg && (a)<prinst->callargc) ? (G_INT(OFS_PARM0 + 3 * (a))) : 0)
#define GETARG_INTVECTOR(a) (((a)>=firstarg && (a)<prinst->callargc) ? ((int*) G_VECTOR(OFS_PARM0 + 3 * (a))) : dummyivec)
#define GETARG_STRING(a) (((a)>=firstarg && (a)<prinst->callargc) ? (PR_GetStringOfs(prinst, OFS_PARM0 + 3 * (a))) : "")

	for (;;)
	{
		s0 = s;
		switch (*s)
		{
		case 0:
			goto finished;
		case '%':
			++s;

			if (*s == '%')
				goto verbatim;

			// complete directive format:
			// %3$*1$.*2$ld

			width = -1;
			precision = -1;
			thisarg = -1;
			flags = 0;
			isfloat = -1;

			// is number following?
			if (*s >= '0' && *s <= '9')
			{
				width = strtol(s, &err, 10);
				if (!err)
				{
					printf("PF_sprintf: bad format string: %s\n", s0);
					goto finished;
				}
				if (*err == '$')
				{
					thisarg = width + (firstarg - 1);
					width = -1;
					s = err + 1;
				}
				else
				{
					if (*s == '0')
					{
						flags |= PRINTF_ZEROPAD;
						if (width == 0)
							width = -1; // it was just a flag
					}
					s = err;
				}
			}

			if (width < 0)
			{
				for (;;)
				{
					switch (*s)
					{
					case '#': flags |= PRINTF_ALTERNATE; break;
					case '0': flags |= PRINTF_ZEROPAD; break;
					case '-': flags |= PRINTF_LEFT; break;
					case ' ': flags |= PRINTF_SPACEPOSITIVE; break;
					case '+': flags |= PRINTF_SIGNPOSITIVE; break;
					default:
						goto noflags;
					}
					++s;
				}
			noflags:
				if (*s == '*')
				{
					++s;
					if (*s >= '0' && *s <= '9')
					{
						width = strtol(s, &err, 10);
						if (!err || *err != '$')
						{
							printf("PF_sprintf: invalid format string: %s\n", s0);
							goto finished;
						}
						s = err + 1;
					}
					else
						width = argpos++;
					width = GETARG_FLOAT(width);
					if (width < 0)
					{
						flags |= PRINTF_LEFT;
						width = -width;
					}
				}
				else if (*s >= '0' && *s <= '9')
				{
					width = strtol(s, &err, 10);
					if (!err)
					{
						printf("PF_sprintf: invalid format string: %s\n", s0);
						goto finished;
					}
					s = err;
					if (width < 0)
					{
						flags |= PRINTF_LEFT;
						width = -width;
					}
				}
				// otherwise width stays -1
			}

			if (*s == '.')
			{
				++s;
				if (*s == '*')
				{
					++s;
					if (*s >= '0' && *s <= '9')
					{
						precision = strtol(s, &err, 10);
						if (!err || *err != '$')
						{
							printf("PF_sprintf: invalid format string: %s\n", s0);
							goto finished;
						}
						s = err + 1;
					}
					else
						precision = argpos++;
					precision = GETARG_FLOAT(precision);
				}
				else if (*s >= '0' && *s <= '9')
				{
					precision = strtol(s, &err, 10);
					if (!err)
					{
						printf("PF_sprintf: invalid format string: %s\n", s0);
						goto finished;
					}
					s = err;
				}
				else
				{
					printf("PF_sprintf: invalid format string: %s\n", s0);
					goto finished;
				}
			}

			for (;;)
			{
				switch (*s)
				{
				case 'h': isfloat = 1; break;
				case 'l': isfloat = 0; break;
				case 'L': isfloat = 0; break;
				case 'j': break;
				case 'z': break;
				case 't': break;
				default:
					goto nolength;
				}
				++s;
			}
		nolength:

			// now s points to the final directive char and is no longer changed
			if (*s == 'p' || *s == 'P')
			{
				//%p is slightly different from %x.
				//always 8-bytes wide with 0 padding, always ints.
				flags |= PRINTF_ZEROPAD;
				if (width < 0) width = 8;
				if (isfloat < 0) isfloat = 0;
			}
			else if (*s == 'i')
			{
				//%i defaults to ints, not floats.
				if (isfloat < 0) isfloat = 0;
			}

			//assume floats, not ints.
			if (isfloat < 0)
				isfloat = 1;

			if (thisarg < 0)
				thisarg = argpos++;

			if (o < end - 1)
			{
				f = &formatbuf[1];
				if (*s != 's' && *s != 'c')
					if (flags & PRINTF_ALTERNATE) *f++ = '#';
				if (flags & PRINTF_ZEROPAD) *f++ = '0';
				if (flags & PRINTF_LEFT) *f++ = '-';
				if (flags & PRINTF_SPACEPOSITIVE) *f++ = ' ';
				if (flags & PRINTF_SIGNPOSITIVE) *f++ = '+';
				*f++ = '*';
				if (precision >= 0)
				{
					*f++ = '.';
					*f++ = '*';
				}
				if (*s == 'p')
					*f++ = 'x';
				else if (*s == 'P')
					*f++ = 'X';
				else
					*f++ = *s;
				*f++ = 0;

				if (width < 0) // not set
					width = 0;

				switch (*s)
				{
				case 'd': case 'i':
					if (precision < 0) // not set
						Q_snprintfz(o, end - o, formatbuf, width, (isfloat ? (int)GETARG_FLOAT(thisarg) : (int)GETARG_INT(thisarg)));
					else
						Q_snprintfz(o, end - o, formatbuf, width, precision, (isfloat ? (int)GETARG_FLOAT(thisarg) : (int)GETARG_INT(thisarg)));
					o += strlen(o);
					break;
				case 'o': case 'u': case 'x': case 'X': case 'p': case 'P':
					if (precision < 0) // not set
						Q_snprintfz(o, end - o, formatbuf, width, (isfloat ? (unsigned int)GETARG_FLOAT(thisarg) : (unsigned int)GETARG_INT(thisarg)));
					else
						Q_snprintfz(o, end - o, formatbuf, width, precision, (isfloat ? (unsigned int)GETARG_FLOAT(thisarg) : (unsigned int)GETARG_INT(thisarg)));
					o += strlen(o);
					break;
				case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
					if (precision < 0) // not set
						Q_snprintfz(o, end - o, formatbuf, width, (isfloat ? (double)GETARG_FLOAT(thisarg) : (double)GETARG_INT(thisarg)));
					else
						Q_snprintfz(o, end - o, formatbuf, width, precision, (isfloat ? (double)GETARG_FLOAT(thisarg) : (double)GETARG_INT(thisarg)));
					o += strlen(o);
					break;
				case 'v': case 'V':
					f[-2] += 'g' - 'v';
					if (precision < 0) // not set
						Q_snprintfz(o, end - o, va("%s %s %s", /* NESTED SPRINTF IS NESTED */ formatbuf, formatbuf, formatbuf),
							width, (isfloat ? (double)GETARG_VECTOR(thisarg)[0] : (double)GETARG_INTVECTOR(thisarg)[0]),
							width, (isfloat ? (double)GETARG_VECTOR(thisarg)[1] : (double)GETARG_INTVECTOR(thisarg)[1]),
							width, (isfloat ? (double)GETARG_VECTOR(thisarg)[2] : (double)GETARG_INTVECTOR(thisarg)[2])
						);
					else
						Q_snprintfz(o, end - o, va("%s %s %s", /* NESTED SPRINTF IS NESTED */ formatbuf, formatbuf, formatbuf),
							width, precision, (isfloat ? (double)GETARG_VECTOR(thisarg)[0] : (double)GETARG_INTVECTOR(thisarg)[0]),
							width, precision, (isfloat ? (double)GETARG_VECTOR(thisarg)[1] : (double)GETARG_INTVECTOR(thisarg)[1]),
							width, precision, (isfloat ? (double)GETARG_VECTOR(thisarg)[2] : (double)GETARG_INTVECTOR(thisarg)[2])
						);
					o += strlen(o);
					break;
				case 'c':
					//UTF-8-FIXME: figure it out yourself
//							if(flags & PRINTF_ALTERNATE)
				{
					if (precision < 0) // not set
						Q_snprintfz(o, end - o, formatbuf, width, (isfloat ? (unsigned int)GETARG_FLOAT(thisarg) : (unsigned int)GETARG_INT(thisarg)));
					else
						Q_snprintfz(o, end - o, formatbuf, width, precision, (isfloat ? (unsigned int)GETARG_FLOAT(thisarg) : (unsigned int)GETARG_INT(thisarg)));
					o += strlen(o);
				}
				/*							else
											{
												unsigned int c = (isfloat ? (unsigned int) GETARG_FLOAT(thisarg) : (unsigned int) GETARG_INT(thisarg));
												char charbuf16[16];
												const char *buf = u8_encodech(c, NULL, charbuf16);
												if(!buf)
													buf = "";
												if(precision < 0) // not set
													precision = end - o - 1;
												o += u8_strpad(o, end - o, buf, (flags & PRINTF_LEFT) != 0, width, precision);
											}
				*/							break;
				case 's':
					//UTF-8-FIXME: figure it out yourself
//							if(flags & PRINTF_ALTERNATE)
				{
					if (precision < 0) // not set
						Q_snprintfz(o, end - o, formatbuf, width, GETARG_STRING(thisarg));
					else
						Q_snprintfz(o, end - o, formatbuf, width, precision, GETARG_STRING(thisarg));
					o += strlen(o);
				}
				/*							else
											{
												if(precision < 0) // not set
													precision = end - o - 1;
												o += u8_strpad(o, end - o, GETARG_STRING(thisarg), (flags & PRINTF_LEFT) != 0, width, precision);
											}
				*/							break;
				default:
					printf("PF_sprintf: invalid format string: %s\n", s0);
					goto finished;
				}
			}
			++s;
			break;
		default:
		verbatim:
			if (o < end - 1)
				*o++ = *s;
			s++;
			break;
		}
	}
finished:
	*o = 0;
}

void PF_printf(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char outbuf[4096];
	PF_sprintf_internal(prinst, pr_globals, PR_GetStringOfs(prinst, OFS_PARM0), 1, outbuf, sizeof(outbuf));
	Con_Printf("%s", outbuf);
}

void PF_bad(pubprogfuncs_t *prinst, struct globalvars_s *gvars)
{
	Con_Printf("bad builtin\n");
}

//too specific to the prinst's builtins.
static void QCBUILTIN PF_Fixme(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int binum;
	char fname[MAX_QPATH];
	if (!prinst->GetBuiltinCallInfo(prinst, &binum, fname, sizeof(fname)))
	{
		binum = 0;
		strcpy(fname, "?unknown?");
	}

	Con_Printf("\n");

	prinst->RunError(prinst, "\nBuiltin %i:%s not implemented.\nCSQC is not compatible.", binum, fname);
	//PR_BIError(prinst, "bulitin not implemented");
}







static struct {
	char *name;
	builtin_t bifunc;
	int ebfsnum;
}  BuiltinList[] = {
	{"makevectors",				PF_bad, 1},		// #1 void() makevectors (QUAKE)
	{"setorigin",				PF_bad, 2},		// #2 void(entity e, vector org) setorigin (QUAKE)
	{"setmodel",				PF_bad, 3},			// #3 void(entity e, string modl) setmodel (QUAKE)
	{"setsize",					PF_bad, 4},			// #4 void(entity e, vector mins, vector maxs) setsize (QUAKE)
	{"setsize",					PF_bad, 4},			// #4 void(entity e, vector mins, vector maxs) setsize (QUAKE)
	{"print",					PF_printf, 339},			// #339 void(string s, ...) print (QUAKE)
	{NULL}
};




static builtin_t csqc_builtin[800];

static int PDECL PR_CSQC_MapNamedBuiltin(pubprogfuncs_t *progfuncs, int headercrc, const char *builtinname)
{
	int i, binum;
	for (i = 0; BuiltinList[i].name; i++)
	{
		if (!strcmp(BuiltinList[i].name, builtinname) && BuiltinList[i].bifunc != PF_Fixme)
		{
			for (binum = sizeof(csqc_builtin) / sizeof(csqc_builtin[0]); --binum; )
			{
				if (csqc_builtin[binum] && csqc_builtin[binum] != PF_Fixme && BuiltinList[i].bifunc)
					continue;
				csqc_builtin[binum] = BuiltinList[i].bifunc;
				return binum;
			}
			Con_Printf("No more builtin slots to allocate for %s\n", builtinname);
			break;
		}
	}
	//if (!csqc_nogameaccess)
	//	Con_DPrintf("Unknown csqc builtin: %s\n", builtinname);
	return 0;
}


csqcworld_t csqc;

static void PDECL CSQC_EntSpawn(csqcedict_t *ent, int loading)
{
	
}

static pbool PDECL CSQC_EntFree(csqcedict_t *ent)
{
	ent->v->solid = 0;
	ent->v->movetype = 0;
	ent->v->modelindex = 0;
	ent->v->think = 0;
	ent->v->nextthink = 0;

	return true;
}

void QCLIB_Init(void)
{
	void	*dll;
	progsnum_t pn;
	func_t func;
	pubprogfuncs_t*(*InitProgs) (progparms_t *parms);


#ifdef WIN32
#ifdef _WIN64
	dll = Sys_DLOpen("qclib64.dll");
#else
	dll = Sys_DLOpen("qclib.dll");
#endif
#else
#if __x86_64__ || __ppc64__
	dll = Sys_DLOpen("qclib64.so");
#else
	dll = Sys_DLOpen("qclib.so");
#endif
#endif

	if (dll == NULL)
	{
		Con_Printf("QCLIB DLL not found or not able to initialize.\n");
		return;
	}

	InitProgs = Sys_DLProc(dll, "InitProgs");

	if (InitProgs == NULL)
	{
		Con_Printf("QCLIB DLL is invalid.\n");
		return;
	}

	
	// build builtin list
	int i;
	for (i = 0; i < sizeof(csqc_builtin) / sizeof(csqc_builtin[0]); i++)
		csqc_builtin[i] = PF_Fixme;
	for (i = 0; BuiltinList[i].bifunc; i++)
	{
		if (BuiltinList[i].ebfsnum)
			csqc_builtin[BuiltinList[i].ebfsnum] = BuiltinList[i].bifunc;
	}
	//



	progparms_t parms;
	memset(&parms, 0, sizeof(parms));

	parms.progsversion = PROGSTRUCT_VERSION;
	parms.ReadFile = Sys_ReadFile;
	parms.WriteFile = Sys_WriteFile;
	parms.FileSize = Sys_FileSize;
	parms.Abort = CSQC_Abort;
	parms.memalloc = malloc;
	parms.memfree = free;
	parms.Printf = Con_Printf;
	parms.MapNamedBuiltin = PR_CSQC_MapNamedBuiltin;

	parms.entspawn = CSQC_EntSpawn;
	parms.entcanfree = CSQC_EntFree;

	parms.edictsize = sizeof(csqcedict_t);
	parms.edicts = csqc.edicts;
	parms.num_edicts = &csqc.num_edicts;

	parms.gametime = &csqc.time;
	csqc.time = Sys_DoubleTime();

	parms.user = &csqc;

	parms.numglobalbuiltins = sizeof(csqc_builtin) / sizeof(csqc_builtin[0]);
	parms.globalbuiltins = csqc_builtin;

	QCLib = InitProgs(&parms);

	QCLib->Configure(QCLib, 1024 * 1024, 1, false);	//memory quantity of 1mb. Maximum progs loadable into the instance of 1

//If you support multiple progs types, you should tell the VM the offsets here, via RegisterFieldVar
	pn = QCLib->LoadProgs(QCLib, "test.dat");	//load the progs.
	if (pn < 0)
		Con_Printf("test: Failed to load progs \"%s\"\n", "test.dat");
	else
	{
		//allocate qc-acessable strings here for 64bit cpus. (allocate via AddString, tempstringbase is a holding area not used by the actual vm)
		//you can call functions before InitEnts if you want. it's not really advised for anything except naming additional progs. This sample only allows one max.

		//QCLib->InitEnts(QCLib, 10);		//Now we know how many fields required, we can say how many maximum ents we want to allow. 10 in this case. This can be huge without too many problems.

//now it's safe to ED_Alloc.

		func = QCLib->FindFunction(QCLib, "CSQC_Init", PR_ANY);	//find the function 'main' in the first progs that has it.
		if (!func)
			Con_Printf("Couldn't find function\n");
		else
			QCLib->ExecuteProgram(QCLib, func);			//call the function
	}
	QCLib->Shutdown(QCLib);


	
}








