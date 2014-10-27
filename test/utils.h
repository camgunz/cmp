#ifndef UTILS_H__
#define UTILS_H__

void error_and_exit(const char *msg);

#ifdef __GNUC__
void errorf_and_exit(const char *msg, ...)
    __attribute__ ((format (printf, 1, 2)));
#else
void errorf_and_exit(const char *msg, ...);
#endif

char* _strdup(const char *s);

#ifndef strdup
#define strdup _strdup
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#endif

