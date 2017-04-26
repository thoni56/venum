/* vn.h				Date: 1995-09-03/reibert@home

   venum 4 header file
*/
#ifndef _VN_H_
#define _VN_H_ 4

#include <time.h>

#ifdef THINK_C
#define NDBG
#define UNIX_ERA 2082848400L	/* MacOs starts at 1904-01-01 00:00 GMT */
#else
#define UNIX_ERA 0L		/* Unix starts at 1970-01-01 00:00 GMT */
				/* MS-Dos starts at 1980-01-01 00:00 GMT */
#endif

#include <stdbool.h>

#define PRIVATE static
#define PUBLIC

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif


/*======================================================================
    Internal data structures for the selected products version info,
    parsed from <product>.version
*/
typedef struct vers {
    int version;
    int revision;
    int correction;
    char *state;
    time_t time;
} VnVersion;

typedef struct form {
    char *number;
    char *shrt;
    char *lng;
    char *xtra;
} VnFormat;

typedef struct prod {
    char *name;
    char *slogan;
    char *user;
    char *host;
    char *ostype;
    VnFormat format;
    VnVersion number;
} VnProduct;


/*======================================================================
    Return a formatted string with version info
*/
extern char *strfvers(
    char *format,
    VnProduct *p
);

/*======================================================================
    Write output file(s)
*/

typedef enum language { C, IMP, REZ, NONE, hhjashj=257 /* Think-C glitch */ } Language;

extern void writeProd(
    char *name,
    VnProduct *p,
    Language lang,
    bool writeAll
);

#endif
