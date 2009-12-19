#ifndef __OUTP_H
#define __OUTP_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include "C.tab.h"
#include "nodes.h"
#include "token.h"
#include "environment.h"
#include "conversion.h"

void fatal(char *, ...);
void debug(char *);
void print_return_value(environment *, value *);

#endif