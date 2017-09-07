/* vnw.c			Date: 1995-09-04/reibert@softlab.se

   vnw -- write to files for venum 4

   1.0 - 1993-05-19
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spa.h"
#include "vn.h"
#include "venum.version.h"

#ifndef FILENAME_MAX
#define FILENAME_MAX 1024
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN FILENAME_MAX
#endif

/*----------------------------------------------------------------------
  A static character array (instance) for adding things
*/
#define SCA_SZ 1024
PRIVATE char _sca[SCA_SZ+1];
PRIVATE int _scap;
#ifdef NOINLINE
PRIVATE void scaClear() { _scap = 0; }
PRIVATE char* scaAsString() { _sca[_scap] = 0; return _sca; }
#else
#define scaClear()   (_scap = 0)
#define scaAsString() (_sca[_scap] = 0, _sca)
#endif
PRIVATE void scaAdd(char c) { if (_scap<SCA_SZ) _sca[_scap++] = c; }
PRIVATE void scaAddString(register char *s) { if (s) while (*s) scaAdd(*s++); }


/*----------------------------------------------------------------------
  Open file <name>.<ext> for read, or stdout if failed
*/
PRIVATE char fName[FILENAME_MAX];

PRIVATE FILE *fOpen(
                    char *name,
                    char *ext
                    ){
    FILE *fp;

    sprintf(fName, "%s.%s", name, ext);

    fp= fopen(fName, "w");
    if (fp==NULL) {
        spaAlert('E', "Could not open %s for output", fName);
        fp= stdout;
    }

    return fp;
}


/*----------------------------------------------------------------------
  Make a string safe for C output as C string.
*/
PRIVATE char *csafe(
                    char *s
                    ){
    scaClear();
    if (s) for (; *s; s++) {
            switch (*s) {
            case '"':
            case '\\':
                scaAdd('\\');
            default:
                scaAdd(*s);
            }
        }
    return scaAsString();
}

/*----------------------------------------------------------------------
  Write header files for C
*/
PRIVATE void write_h(
                     char *name,
                     VnProduct *p
                     ){
    FILE *fp= fOpen("version", "h");

    fprintf(fp,
            "#ifndef _version_h_\n"
            "#define _version_h_ %d\n"
            "\n"
            "#include <time.h>\n"
            "\n"
            "typedef struct {\n"
            "  char*  string;\n"
            "  int    version;\n"
            "  int    revision;\n"
            "  int    correction;\n"
            "  time_t time;\n"
            "  char*  state;\n"
            "} Version;\n"
            "\n"
            "typedef struct {\n"
            "  char*   name;\n"
            "  char*   slogan;\n"
            "  char*   shortHeader;\n"
            "  char*   longHeader;\n"
            "  char*   date;\n"
            "  char*   time;\n"
            "  char*   user;\n"
            "  char*   host;\n"
            "  char*   ostype;\n"
            "  Version version;\n"
            "} Product;\n"
            "\n"
            "#endif\n"
            , p->number.version
            );

    if (fp!=stdout) {
        fclose(fp);
#ifdef THINK_C
        setCreator(fName, 'KAHL');
#endif
    }


    fp= fOpen(name, "version.h");

    fprintf(fp, "#ifndef _%s_version_h_\n", name);
    fprintf(fp, "#define _%s_version_h_ %d\n\n", name, p->number.version);

    fprintf(fp,
            "#include \"version.h\"\n"
            "\n"
            "extern Product %s;\n"
            "\n"
            "#endif\n"
            , name
            );

    if (fp!=stdout) {
        fclose(fp);
#ifdef THINK_C
        setCreator(fName, 'KAHL');
#endif
    }

}


/*----------------------------------------------------------------------
  Write data file for C
  Will not be compilable if containing ".
*/
PRIVATE void fprcdata(
                      FILE *f,
                      char *val,
                      VnProduct *p
                      ){
    fprintf(f, "  \"%s\",\n", csafe(strfvers(val, p)));
}

PRIVATE void write_c(
                     char *name,
                     VnProduct *p
                     ){
    FILE *fp= fOpen(name, "version.c");

    fprintf(fp,
            "/* %s.c - Created by %s */\n"
            "\n"
            "#include \"%s.version.h\"\n"
            "\n",
            name, csafe(venum.shortHeader), name
            );

    fprintf(fp, "Product %s = {\n", name);
    fprcdata(fp, "$n", p);
    fprcdata(fp, "$z", p);
    fprcdata(fp, p->format.shrt, p);
    fprcdata(fp, p->format.lng, p);
    fprcdata(fp, "$D", p);
    fprcdata(fp, "$T", p);
    fprcdata(fp, "$u", p);
    fprcdata(fp, "$h", p);
    fprcdata(fp, "$o", p);
    fprintf(fp, "  {\"%s\", %d, %d, %d, %lu",
            csafe(strfvers(p->format.number, p)),
            p->number.version,
            p->number.revision,
            p->number.correction,
            p->number.time - UNIX_ERA
            );
    fprintf(fp,
            ", \"%s\"}\n"
            "};\n"
            , csafe(strfvers("$s", p))
            );

    fprintf(fp,
            "\n"
            "char *%sId =\n"
            "  \"%c(#)RELEASE %s\";\n"
            , name, '@', csafe(strfvers(p->format.xtra, p))
            );

    if (fp!=stdout) {
        fclose(fp);
#ifdef THINK_C
        setCreator(fName, 'KAHL');
#endif
    }

}


/*----------------------------------------------------------------------
  Make a string safe for IMP output as IMP string.
*/
PRIVATE char *isafe(
                    char *s
                    ){
    scaClear();
    if (s) for (; *s; s++) {
            switch (*s) {
            case '%':
                if (*(s+1)=='%') { scaAddString("%%(\"%%\")"); s++; }
                else scaAdd(*s);
                break;
            case '"':
                scaAdd('"');
            default:
                scaAdd(*s);
            }
        }
    return scaAsString();
}

/*----------------------------------------------------------------------
  Write script file for IMP
  Will not be processable if containing ".
*/
PRIVATE void fprset(
                    FILE *f,
                    char *name,
                    char *var,
                    char *val,
                    VnProduct *p
                    ){
    fprintf(f, "%%%%SET %s_%-14s(\"%s\")\n",
            name, var, (p? isafe(strfvers(val, p)): isafe(val)));
}

PRIVATE void fprseti(
                     FILE *f,
                     char *name,
                     char *var,
                     int i
                     ){
    fprintf(f, "%%%%SET %s_%-14s(%d)\n", name, var, i);
}

PRIVATE void write_macro(
                         char *name,
                         VnProduct *p
                         ){
    FILE *fp= fOpen(name, "version.imp");

    fprintf(fp, "%%%%-- %s.version.imp - Created by %s\n", name, isafe(venum.shortHeader));
    fprset(fp, name, "product", name, NULL);
    fprset(fp, name, "name", "$n", p);
    fprset(fp, name, "slogan", "$z", p);
    fprset(fp, name, "shortHeader", p->format.shrt, p);
    fprset(fp, name, "longHeader", p->format.lng, p);
    fprset(fp, name, "extraHeader", p->format.xtra, p);
    fprset(fp, name, "date", "$D", p);
    fprset(fp, name, "time", "$T", p);
    fprset(fp, name, "user", "$u", p);
    fprset(fp, name, "host", "$h", p);
    fprset(fp, name, "ostype", "$o", p);
    fprset(fp, name, "versionString", p->format.number, p);
    fprseti(fp, name, "version", p->number.version);
    fprseti(fp, name, "revision", p->number.revision);
    fprseti(fp, name, "correction", p->number.correction);
    fprset(fp, name, "state", "$s", p);

    if (fp!=stdout) {
        fclose(fp);
#ifdef __mac__
        setCreator(fName, 'ttxt');
#endif
    }

}


/*----------------------------------------------------------------------
  Make a string safe for Rez output as comments.
*/
PRIVATE char *rsafe(
                    char *s
                    ){
    scaClear();
    if (s) for (; *s; s++) {
            switch (*s) {
            case '*':
                if (*(s+1)=='/') { scaAddString("*//*"); s++; }
                else scaAdd(*s);
                break;
            default:
                scaAdd(*s);
            }
        }
    return scaAsString();
}

/*----------------------------------------------------------------------
  Write data file for Rez compilers
  Somewhat quick and dirty
*/
PRIVATE char *bcd(int i) {
    static char buf[16];

    sprintf(buf, "%02d", i);
    buf[2] = 0; /* Only two first get written */
    /*    sprintf(buf, "%d%d", (i/10)%10, i%10); /* Only two last get written */
    return buf;
}

PRIVATE void wrhexstr(FILE *f, char *str) {
    int l = strlen(str), i;

    if (l>255) l=255;            /* Only 255 first get written */

    fprintf(f, " $\"%02X", l);
    if (l>0) for (i = 0; ; ) {
            fprintf(f, "%02X", (unsigned char)str[i]);
            i++;
            if (i>=l) break;
            if ((i%32==31)) fprintf(f, "\"\n $\"");
            else if (i&1) fprintf(f, " ");
        }
    fprintf(f, "\"\n /* %s */\n", rsafe(str));
}

PRIVATE void wrvers(
                    FILE *fp,
                    int no,
                    char *name,
                    VnProduct *p
                    ){
    int vt;

    if (name) fprintf(fp, "data 'vers' (%d, \"%s\") {\n", no, name);
    else  fprintf(fp, "data 'vers' (%d) {\n", no);

    fprintf(fp, " $\"%s", bcd(p->number.version));
    fprintf(fp, "%s ", bcd(p->number.revision));

    if (p->number.state) {
        switch (*p->number.state) {
        case 'a': case 'A': vt = 40; break;	/* alpha */
        case 'b': case 'B': vt = 60; break;	/* beta */
        default: vt = 20;			/* development */
        }
    } else vt = 80;				/* released */
    fprintf(fp, "%s", bcd(vt));
    fprintf(fp, "%s %04d\"\n", bcd(p->number.correction), 7); /* 7 = Swedish, 0 = US */

    wrhexstr(fp, strfvers("$V", p));

    if (name) wrhexstr(fp, strfvers(p->format.xtra, p));
    else wrhexstr(fp, strfvers("$z", p));

    fprintf(fp, "};\n");
}

PRIVATE void write_rez(
                       char *name,
                       VnProduct *p
                       ){
    FILE *fp= fOpen(name, "r");

    fprintf(fp, "/* %s.r - Created by %s */\n\n", name, rsafe(venum.shortHeader));
    wrvers(fp, 1, name, p);

    fprintf(fp, "\n");
    wrvers(fp, 2, NULL, p);

    if (fp!=stdout) {
        fclose(fp);
#ifdef THINK_C
        setCreator(fName, 'KAHL');
#endif
    }

}


/*======================================================================
  Write output file(s)
*/
PUBLIC void writeProd(
                      char *name,
                      VnProduct *p,
                      Language lang,
                      bool writeAll
                      ){
    switch (lang) {
    case C:
        if (writeAll) write_h(name, p);
        write_c(name, p);
        break;
    case IMP:
        write_macro(name, p);
        break;
    case REZ:
        write_rez(name, p);
        break;
    default:
        break;
    }
}

/* -- EoF -- */
