#ifndef __CONV_H
#define __CONV_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "C.tab.h"
#include "nodes.h"
#include "token.h"
#include "environment.h"

#define UNDEFINED INT_MIN + 3

char *to_string(value *);
int to_int(value *);
TOKEN * cast_from_node(NODE *);
int type_of(NODE *);

#endif