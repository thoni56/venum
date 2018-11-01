/* vn.c					Date: 1995-09-05/reibert@bee

   venum -- Version number handling system

   Written by Reibert Arbring, SoftLab ab.

   Copyright © 1988, 1989, 1993 - 1995 SoftLab ab.


   Known deficiencies:

   * Product name must be a "output language" variable name
   * Format language is difficult

   History:

   1995-09-03/reibert@home	4.2	venums verison in version-file, ostype
   1994-12-01/reibert@home	4.1	version-file, number-format, version(1)-support,
   Rez output
   1993-05-19/reibert@home	4.0	Format-strings, completly rewritten
   1989-11-10 RO            3.0	New version numbers: "v.r(c)s"
   1989-11-07 RO            2.00	unix
   1988-02-01 RO			1.01	/m /c

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef THINK_C
#define __mac__

#include <unix.h>

#else

#define SET_ALL

extern char *getlogin();

#endif

#ifdef __mingw32__
#include <windows.h>
#endif

#include "vn.h"
#include "spa.h"
#include "venum.version.h"

#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN FILENAME_MAX
#endif

PRIVATE char
*pname,		/* Name of the product for this execution */
    *reportFormat;	/* Special report format wanted */

PRIVATE VnProduct
p			/* The read product */
=       /* Default values for first time */
    { NULL, NULL, NULL, NULL, NULL,
      { "$v.$r{s $s}{c($c)}", "$n $V", "$H{z -- $z} ($d $t{u, $u{h@$h}}{o/$o})",
        "$n $V $D $T{u $u}{h@$h}{o/$o}" },
      { 1, 0, 0, NULL, 0L}
    },
    set;		/* User modifications */

PRIVATE int
up,			/* How to increment version numbers */
    wr;			/* What to write */

PRIVATE bool
changed = FALSE,	/* Is set if anything changed */
    all,			/* Write all possible output files */
    temporary,		/* Do not update version file */
    verbose;

PRIVATE int /* Language */
out;			/* Output language */


#ifdef DBG
PUBLIC int debug;
#else
#define debug FALSE
#endif


#ifdef __mac__
void setCreator(char *name, OSType creator)
{ /* Set creator of file to venum */
    FInfo fi;
    short vref;
    unsigned char s[256];

    if (GetVol(NULL, &vref)==0) {
        *s = sprintf((char *)s+1, "%s", name);
        if (GetFInfo(s, vref, &fi)==0) {
            fi.fdCreator = creator;
            SetFInfo(s, vref, &fi); /* GDSGD */
        } else spaAlert('E', "GetVol");
    } else spaAlert('E', "GetVol");
}
#endif

/*----------------------------------------------------------------------
  Return new memory
*/
PUBLIC void *New(
                 size_t sz
                 ){
    char * n = malloc(sz);

    if (!n) spaAlert('S', "out of memory, could not allocate %d bytes", sz);
    return n;
}


/*----------------------------------------------------------------------
  Return a fresh string copy
*/
PUBLIC char *strNew(
                    char *inStr
                    ){
    char *n;

    n = New((size_t)(strlen(inStr)+1));
    strcpy(n, inStr);
    return n;
}


/*----------------------------------------------------------------------
  Return ostype as a string
*/
PUBLIC char *ostype(){
    char *o = getenv("OSTYPE");

    if (o && *o) return o;
    else return
#ifdef __mac__
             "MacOS";
#else
#  ifdef __cygwin32__
    "cygwin32";
#  else
#    ifdef __cygwin64__
    "cygwin64";
#    else
#      ifdef __mingw32__
    "mingw32";
#      else
#        ifdef __sunos__
    "sunos";
#        else
#          message "No known OSTYPE"
#        endif
#      endif
#    endif
#  endif
#endif
}

/*----------------------------------------------------------------------
  Set <to> and <changed>, if <from> has a value
  return changed
*/
PRIVATE bool ccset(char **to, char *from, bool *changed)
{
    if (from
#ifdef __mac__
        && *from
#endif
        ) {
        *to = from;
        *changed = TRUE;
    }
    return *changed;
}


/*----------------------------------------------------------------------
  Check if an identifier ([a-zA-Z][a-zA-Z0-9_]*),
  return offending character
*/
PRIVATE char checkName(char *s)
{
    if (s && *s) {
        if (!isalpha(*s)) return *s;
        for (s++; *s; s++) {
            if  (!(isalnum(*s) || *s=='_')) return *s;
        }
    }
    return 0;
}


/*----------------------------------------------------------------------
  Lowercase string in situ
*/
PRIVATE void strlow(char *s)
{
    for (; *s; s++) if (isupper(*s)) *s = tolower(*s);
}


/*----------------------------------------------------------------------
  Return empty string if NULL pointer, owz the string itself
*/
PRIVATE char *safe(char *s) { return (s? s: ""); }


/*----------------------------------------------------------------------
  Format a number to a static string
*/
typedef enum {		/* The various number formats */
    OCT, DEC, hex, HEX, rom, ROM, chr, CHR
} PForm;

PRIVATE char *i2a(int i, PForm p)
{
    static char tmp[16];
    bool lower = FALSE;
    char
        *ones[] = { "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX" },
        *tens[] = { "", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC" },
            *huns[] = { "", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM" },
                *mils[] = { "", "M", "MM", "MMM" };


                switch (p) {
                case hex:
                    lower = TRUE;
                case HEX:
                    sprintf(tmp, "%X", i);
                    break;
                case OCT:
                    sprintf(tmp, "%O", i);
                    break;
                case rom:
                    lower = TRUE;
                case ROM:
                    if (i>3999) sprintf(tmp, "\"%d\"", i);
                    else sprintf(tmp, "%s%s%s%s", mils[i/1000], huns[(i/100)%10],
                                 tens[(i/10)%10], ones[i%10]);
                    break;
                case chr:
                    lower = TRUE;
                case CHR:
                    if (i>25) sprintf(tmp, "%s%c", i2a(i/26-1, p), i%26+'A');
                    else sprintf(tmp, "%c", i+'A');
                    break;
                default:
                    sprintf(tmp, "%d", i);
                    break;
                }
                if (lower) strlow(tmp);
                return tmp;
}

/*----------------------------------------------------------------------
  Format a date to a static string
*/
PRIVATE char *d2a(time_t t, int big)
{
    static char tmp[16];
    struct tm *lt = localtime(&t);

    sprintf(tmp, "%02d-%02d-%02d",lt->tm_year + (big?1900:0), lt->tm_mon+1, lt->tm_mday);
    return tmp;
}

/*----------------------------------------------------------------------
  Format a time to a static string
*/
PRIVATE char *t2a(time_t t, int big)
{
    static char tmp[16];
    struct tm *lt = localtime(&t);

    sprintf(tmp, big? "%02d:%02d:%02d": "%02d:%02d",
            lt->tm_hour, lt->tm_min, lt->tm_sec);
    return tmp;
}

/*----------------------------------------------------------------------
  Format a version to a [P]A[n]-number in a static string
*/
PRIVATE char *v2pa(VnVersion *v)
{
    static char tmp[16];

    if (v->revision) sprintf(tmp, "P%s%d", i2a(v->version, CHR), v->revision);
    else {
        if (v->version) return i2a(v->version-1, CHR);
        else return "PA0";
    }
    return tmp;
}

/*----------------------------------------------------------------------
  NULL-safe line-printing
*/
PRIVATE void sputs(
                   FILE *f,
                   char *s
                   ){
    fprintf(f, "%s\n", safe(s));
}

/*----------------------------------------------------------------------
  Print a product to a version file
*/
PRIVATE void pp(
                FILE *f,
                VnProduct *p
                ){
    fprintf(f, "%d.%d %s # %s\n",
            venum.version.version, venum.version.revision,
            venum.name, venum.shortHeader);
    sputs(f, p->name);
    sputs(f, p->slogan);
    fprintf(f, "%d\n%d\n%d\n",
            p->number.version,
            p->number.revision,
            p->number.correction);
    sputs(f, p->number.state);
    fprintf(f, "%lu %s %s\n", p->number.time - UNIX_ERA,
            d2a(p->number.time, TRUE), t2a(p->number.time, TRUE));
    sputs(f, p->user);
    sputs(f, p->host);
    sputs(f, p->format.number);
    sputs(f, p->format.shrt);
    sputs(f, p->format.lng);
    sputs(f, p->format.xtra);
    sputs(f, p->ostype);
}


/*----------------------------------------------------------------------
  Get a line from a file, returns NULL at EOF or empty line
*/
PRIVATE char * cgp(
                   FILE *f
                   ){
    char buf[1024];

    *buf = 0;
    if (fgets(buf, 1024, f)) {
        int l = strlen(buf);
        if (l && buf[l-1]=='\n') buf[l-1] = 0; /* Strip last nl */
        if (*buf) return strNew(buf);
    }
    return NULL;
}

/*----------------------------------------------------------------------
  Get a time from a file, start of era at errors
*/
PRIVATE time_t tgp(
                   FILE *f
                   ){
    time_t t1 = UNIX_ERA;
    struct tm t;
    char *line = cgp(f);
    int n;

    if (line) {
        n = sscanf(line, "%lu %d-%d-%d %d.%d.%d", &t1,
                   &t.tm_year, &t.tm_mon, &t.tm_mday,
                   &t.tm_hour, &t.tm_min, &t.tm_sec);
        if (n == 7) {
            t.tm_year -= 1900;
            t.tm_mon--;
            t.tm_isdst = -1; /* ???? */
            t1 = mktime(&t);
        } else if (n<=0) t1 = UNIX_ERA;
        free(line);
    }
    return t1;
}

/*----------------------------------------------------------------------
  Get a number from a file, 0 at errors
*/
PRIVATE int igp(
                FILE *f
                ){
    int i = 0;
    char *line = cgp(f);

    if (line) { i = atoi(line); free(line); }
    return i;
}

/*----------------------------------------------------------------------
  Get all product info from a file
*/
PRIVATE void gp(
                FILE *f,
                VnProduct *p
                ){
    char *vs = cgp(f);
    int v = 0, r = 0;

    if (vs && isdigit(*vs)) {
        /* If a digit first then really check for correct version */
        if (sscanf(vs, "%d.%d", &v, &r)<2) {
            spaAlert('E', "Malformed version line in venum db file");
            v = r = 0;
        } else {
            if (v>venum.version.version) {
                spaAlert('F', "venum db file version error (%d found - %d wanted)",
                         v, venum.version.version);
            }
            if (v==venum.version.version && r>venum.version.revision) {
                spaAlert('E', "venum db file revision warning (%d.%d found - %d.%d wanted)",
                         v, r, venum.version.version, venum.version.revision);
            }
        }
        p->name = cgp(f);
    } else {
        /* Let's hope it's an old file, with name on the first line */
        p->name = vs;
    }
    p->slogan = cgp(f);
    p->number.version = igp(f);
    p->number.revision = igp(f);
    p->number.correction = igp(f);
    p->number.state = cgp(f);
    p->number.time = tgp(f);
    p->user = cgp(f);
    p->host = cgp(f);
    p->format.number = cgp(f);
    p->format.shrt = cgp(f);
    p->format.lng = cgp(f);
    p->format.xtra = cgp(f);
    if (v>=4 && r>=2) p->ostype = cgp(f); else p->ostype = ostype();
}


/*----------------------------------------------------------------------
  A static character array (instance) for adding things
*/
#define SCA_SZ 1024
PRIVATE char _sca[SCA_SZ+1];
PRIVATE int _scap;
PRIVATE bool _scaa;

PRIVATE void scaClear() { _scap = 0; _scaa = TRUE; }
PRIVATE void scaStore(int s) { _scaa = s; }
PRIVATE int scaStoring() { return _scaa; }
PRIVATE char* scaAsString() { _sca[_scap] = 0; return _sca; }

PRIVATE void scaAdd(char c) { if (_scap<SCA_SZ && _scaa) _sca[_scap++] = c; }
PRIVATE void scaAddString(register char *s) { if (s) while (*s) scaAdd(*s++); }


/*----------------------------------------------------------------------

 */
PRIVATE char * istrfvers(
                         char *format,
                         VnProduct *p,
                         int lev
                         ){
    register char *fmt = format;
    int i;
    PForm pi = DEC;

    if (fmt) for (; *fmt; fmt++) {
            switch (*fmt) {
            case '$':	/* Write a variable */
                switch (*++fmt) {
                case 'n': scaAddString(p->name); break;
                case 'z': scaAddString(p->slogan); break;
                case 'u': scaAddString(p->user); break;
                case 'h': scaAddString(p->host); break;
                case 'o': scaAddString(p->ostype); break;
                case 's': scaAddString(p->number.state); break;
                case 'v': scaAddString(i2a(p->number.version, pi)); break;
                case 'r': scaAddString(i2a(p->number.revision, pi)); break;
                case 'c': scaAddString(i2a(p->number.correction, pi)); break;
                case 'd': scaAddString(d2a(p->number.time, FALSE)); break;
                case 't': scaAddString(t2a(p->number.time, FALSE)); break;
                case 'D': scaAddString(d2a(p->number.time, TRUE)); break;
                case 'T': scaAddString(t2a(p->number.time, TRUE)); break;
                case 'P': scaAddString(v2pa(&p->number)); break;
                case 'V':
                    if (format == p->format.number) spaAlert('F', "Recursive call of number format");
                    istrfvers(p->format.number, p, lev);
                    break;
                case 'H':
                    if (format == p->format.shrt) spaAlert('F', "Recursive call of header format");
                    if (format == p->format.number) spaAlert('F', "Illegal call of header format");
                    istrfvers(p->format.shrt, p, lev);
                    break;
                case '$':
                case '%':
                case '{':
                case '|':
                case '}':
                    scaAdd(*fmt);
                    break;
                default: scaAdd('$'); scaAdd(*fmt);
                }
                break;
            case '%':        /* Set integer print form */
                switch (*++fmt) {
                case 'd': pi = DEC; break;
                case 'o': pi = OCT; break;
                case 'x': pi = hex; break;
                case 'X': pi = HEX; break;
                case 'c': pi = chr; break;
                case 'C': pi = CHR; break;
                case 'r': pi = rom; break;
                case 'R': pi = ROM; break;
                default: scaAdd('%'); scaAdd(*fmt);
                }
                break;
            case '{':		/* Conditional print  */
                switch (*++fmt) {	/* Set i to 1 if non-zero, 0 if zero, -1 if undefined */
                case 'n': i = p->name!=NULL; break;
                case 'z': i = p->slogan!=NULL; break;
                case 'u': i = p->user!=NULL; break;
                case 'h': i = p->host!=NULL; break;
                case 'o': i = p->ostype!=NULL; break;
                case 's': i = p->number.state!=NULL; break;
                case 'v': i = p->number.version!=0; break;
                case 'r': i = p->number.revision!=0; break;
                case 'c': i = p->number.correction!=0; break;
                case 'd':
                case 't': i = p->number.time!=0L; break;
                default:  i = -1;
                }
                if (i==-1) { scaAdd('{'); scaAdd(*fmt); }
                else {
                    bool printing = scaStoring();

                    scaStore(i==1 && printing);
                    fmt = istrfvers(++fmt, p, lev+1);
                    scaStore(i==0 && printing);
                    if (*fmt=='|') {
                        fmt = istrfvers(++fmt, p, lev+1);
                    }
                    scaStore(printing);
                }
                break;
            case '|':
            case '}':
                if (lev>0) return fmt;
            default:
                scaAdd(*fmt);
                break;
            }
        }
    return fmt;
}

/*======================================================================
  Return a string according to <format>
*/
char *strfvers(
               char *format,
               VnProduct *p
               ){
    scaClear();
    istrfvers(format, p, 0);
    return scaAsString();
}


/*======================================================================
  ARGUMENT PART
*/
PRIVATE char
*lan[] = { "C", "IMP", "REZ", "-", NULL },	/* Output target languages */
    *nPart[] = { "version", "revision", "correction", "time", "-", NULL },
    /* Update options */
    *wrs[] = {"no", "change", "always", NULL}; /* Output control */

/*----------------------------------------------------------------------
 */
SPA_FUN(argTooMany) { spaAlert('W', "Superflouos argument ignored: %s", rawName); }

/*----------------------------------------------------------------------
 */
SPA_FUN(version) {
    printf("%s\n\n", verbose? venum.longHeader: venum.shortHeader);
    spaExit(0);
}

/*----------------------------------------------------------------------
 */
SPA_FUN(setChanged) {
    changed = TRUE;
}

/*----------------------------------------------------------------------
 */
PRIVATE SPA_FUN(usage) {
    printf("Usage: venum [-help|-<option>] <product> [<update>]\n");
}

/*----------------------------------------------------------------------
 */
PRIVATE SPA_FUN(xit) {
    spaExit(0);
}

/*----------------------------------------------------------------------
 */
#ifdef DBG
SPA_FUN(setDbgAlertLevel) { SpaAlertLevel = on? 'D': (verbose? 'I': 'W'); }
SPA_FUN(setAlertLevel) { SpaAlertLevel = debug? 'D': (on? 'I': 'W'); }
#else
SPA_FUN(setAlertLevel) { SpaAlertLevel = on? 'I': 'W'; }
#endif


/*----------------------------------------------------------------------
 */
SPA_DECLARE(arguments)
SPA_STRING("product", "name of product", pname, NULL, NULL)
SPA_KEYWORD("update", "what to update\nmajor version number\nminor ...\nerrata number\nlast modified time\nnothing\n",
            up, nPart, 4, NULL)
SPA_FUNCTION("", "", argTooMany)
SPA_END

SPA_DECLARE(options)
SPA_KEYWORD("output", "type of output\nANSI-C\nSoftLabs macro processor\nmacintosh resource compilers\nNo output",
            out, lan, 0, NULL)
SPA_KEYWORD("write", "when to write to output file(s)\nnever write\nwrite only when necessary\nalways write",
            wr, wrs, 1, NULL)
SPA_STRING("print", "report with this format", reportFormat, NULL, NULL)
SPA_FLAG("all", "write all output files", all, FALSE, NULL)
SPA_FLAG("volatile", "the update is not made permanent", temporary, FALSE, NULL)
SPA_FLAG("verbose", "talkative/report mode", verbose, TRUE, setAlertLevel)
#ifdef DBG
SPA_FLAG("debug", "debug mode", debug, FALSE, setDbgAlertLevel)
#endif
SPA_FUNCTION("version", "write venums version", version)
SPA_HELP("help", "this help", usage, xit)
SPA_COMMENT("  -- Set options --")
SPA_STRING("name", "set product name", set.name, NULL, setChanged)
SPA_STRING("slogan", "set product slogan", set.slogan, NULL, setChanged)
     SPA_STRING("state", "set product state", set.number.state, NULL, setChanged)
#ifdef SET_ALL
SPA_STRING("user", "set user name", set.user, NULL, setChanged)
SPA_STRING("host", "set host name", set.host, NULL, setChanged)
SPA_STRING("ostype", "set OS type", set.ostype, NULL, setChanged)
SPA_COMMENT("  -- Set format options --")
SPA_STRING("number", "set version number format", set.format.number, NULL, setChanged)
SPA_STRING("short", "set short header format", set.format.shrt, NULL, setChanged)
SPA_STRING("long", "set long header format", set.format.lng, NULL, setChanged)
SPA_STRING("extra", "set extra header format", set.format.xtra, NULL, setChanged)
#endif
SPA_END


/*======================================================================
 */
PUBLIC int main(
            int argc,
            char *argv[]
            ){
    int stdIn;
    FILE *db;
    char dbName[FILENAME_MAX];
    char c;
    time_t now;

#ifdef __mac__		/* Minimal drag & drop support */
    short msg, files;
    AppFile af;
    char pdef[256];
    char *dot;
    OSErr oe;

    CountAppFiles(&msg, &files);
    if (msg==0 && files>0) {		/* Dragged & dropped items */
        GetAppFiles(1, &af);
        if (af.fType=='TEXT') {
            sprintf(pdef, "%#s", af.fName);
            dot = strchr(pdef, '.');
            if (dot) {
                *dot++ = 0;
                if (strcmp(dot, "version")!=0) dot = NULL;
            }
            if (dot) {
                arguments[0].s = pdef;
                oe = SetVol(NULL, af.vRefNum);
                if (oe) { printf("Could not reach volume for %#s (error %d)\n", af.fName, oe); return; }
            } else { printf("%#s is not a '.version' file\n", af.fName); return; }
        } else { printf("%#s is not a 'TEXT' file\n", af.fName); return; }
        if (files>1) printf("Too many files, will only use %#s\n", af.fName);
    }
#endif
    time(&now);
    changed = FALSE;

    spaProcess(argc, argv, arguments, options, NULL);

    if (!pname)
        spaAlert('F', "Product not given, use \"%s -help\" to get help", venum.name);

    c = checkName(pname);
    if (c) spaAlert('F', "Malformed product name, the first offending character is %c", c);
    sprintf(dbName, "%s.version", pname);

    if (!(db = fopen(dbName, "r"))) {
        spaAlert('W', "Version file %s not found, using default to initilize ...", dbName);
        /* Use most default values (stored in p) */
        p.name = pname;
        up = 3;			/* Change time, i.e. set now as default */
        changed = TRUE;
        all = TRUE;		/* Write all possible files */
    } else {
        gp(db, &p);
        fclose(db);
    }

    ccset(&p.name, set.name, &changed);
    ccset(&p.slogan, set.slogan, &changed);
    if (ccset(&p.number.state, set.number.state, &changed)) {
        /* If change of state, reset correction and stamp time */
        if (up>3) {
            p.number.correction = 0;
            up = 3;
        }
    }
#ifdef SET_ALL
    ccset(&p.user, set.user, &changed);
    ccset(&p.host, set.host, &changed);
    ccset(&p.format.number, set.format.number, &changed);
    ccset(&p.format.shrt, set.format.shrt, &changed);
    ccset(&p.format.lng, set.format.lng, &changed);
    ccset(&p.format.xtra, set.format.xtra, &changed);
#endif

    switch (up) {	/* Do the updating */
        char hostname[1000];
    case 0:
        p.number.version++;
        p.number.revision = -1;
    case 1:
        p.number.revision++;
        p.number.correction = -1;
    case 2:
        p.number.correction++;
    case 3:
        p.number.time = now;
        p.user = getlogin() /*getenv("USER")*/;
        /* p.host = getenv("HOST"); */
#ifdef __mingw32__
        DWORD size = 1000;
        GetComputerNameEx(ComputerNameNetBIOS, hostname, &size);
#else
        gethostname(hostname, 1000);
#endif
        p.host = hostname;
        p.ostype = ostype();
        changed = TRUE;
        break;
    }

#ifdef DBG
    if (debug) {
        puts("---"); pp(stdout, &p); puts("---");
        printf("VERS = '%s'\n", strfvers(p.format.number, &p));
        printf("SHRT = '%s'\n", strfvers(p.format.shrt, &p));
        printf("LONG = '%s'\n", strfvers(p.format.lng, &p));
        printf("XTRA = '%s'\n", strfvers(p.format.xtra, &p));
        puts("---");
    }
#endif

    if (changed && !temporary) {	/* Store in version DB */
        if (!(db = fopen(dbName, "w")))
            spaAlert('F', "write open error: %s", dbName);
        else {
            pp(db, &p);
            fclose(db);
#ifdef __mac__
            setCreator(dbName, 'VNUM');
#endif
        }
    }

    if ((wr==1 && changed) || wr==2) {	/* Write output files */
        writeProd(pname, &p, out, all);
    }

    if (reportFormat && *reportFormat) {
        printf("%s\n", strfvers(reportFormat, &p));
    } else if (verbose || debug) {
        printf("%s\n",
               strfvers((p.format.lng? p.format.lng:
                         "$n $v.$r{s $s}{c.$c} - $d $t{u $u{h@$h}}"),
                        &p));
    }

    return 0;
}
