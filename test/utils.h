#ifndef UTILS_H__
#define UTILS_H__

typedef uint8_t byte;
typedef enum {false, true} dboolean;

void doom_printf(const char *msg, ...);
void I_Error(const char *msg, ...);

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#endif

