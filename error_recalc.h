#ifndef ERROR_RECALC_H
#define ERROR_RECALC_H

#ifndef WIN32 // compiles actual C++ code for win32 file picker
    #ifdef __cplusplus
        extern "C" {
    #endif
#endif

#include <stdint.h>

// Function prototypes
extern void eccedc_init(void);
extern void eccedc_generate(uint8_t* sector);
extern int edc_verify(const uint8_t* sector);

#ifndef WIN32 // compiles actual C++ code for win32 file picker
    #ifdef __cplusplus
        }
    #endif
#endif

#endif // ERROR_RECALC_H