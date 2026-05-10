#include "kernel/early_print.h"
#include "kernel/terminal.h"
#include "kernel/screen.h"
#include "utils/utils.h"
#include "arg.h"

#define MAX_STRING_EARLY_PRINT 1000

/* declare the vsprintf function */
int vsprintf(char *buf, const char *fmt, va_list args);

static char buf[MAX_STRING_EARLY_PRINT+1];  /* temporary buffer for early_printf */
static terminal_t terminal;                 /* internal terminal used for early prints */

void early_print_init() {
    terminal_init(&terminal);
}

void early_printf(const char* format, ...) {
    va_list args;
    int i;

    memset(buf, 0, (MAX_STRING_EARLY_PRINT + 1) * sizeof(char));
    
    va_start(args, format);
    
	i=vsprintf(buf, format,args);
	va_end(args);

    terminal_write_string(&terminal, buf);
}