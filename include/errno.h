#ifndef ERRNO_H
#define ERRNO_H

// https://github.com/mentos-team/MentOS/blob/main/libc/inc/errno.h

#define ENO             0       // No Error, OK.
#define EGENERAL        1       // General Error.
#define EIRQ            2       // IRQ Error.
#define EPF             3       // Page Fault.
#define EGPF            4       // General protection fault

#endif // ERRNO_H