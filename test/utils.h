#ifndef UTILS_H__
#define UTILS_H__

typedef uint8_t byte;
typedef enum {false, true} dboolean;

#ifdef __GNUC__
void doom_printf(const char *msg, ...) __attribute__ ((format (printf, 1, 2)));
void I_Error(const char *msg, ...) __attribute__ ((format (printf, 1, 2)));
#else
void doom_printf(const char *msg, ...);
void I_Error(const char *msg, ...);
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#endif

