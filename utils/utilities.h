#ifndef __UTIL_H
#define __UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "C.tab.h"
#include "nodes.h"
#include "conversion.h"
#include "token.h"

/* Fn prototypes */
void assert(int, char *);
void type_check_assignment(value *, value *, int);
void type_check_return(value *, int);
value *void_value(void);

#endif