//
//  operations.c
//  hfsinspect
//
//  Created by Adam Knight on 4/1/14.
//  Copyright (c) 2014 Adam Knight. All rights reserved.
//

#include "operations.h"

String BTreeOptionCatalog    = "catalog";
String BTreeOptionExtents    = "extents";
String BTreeOptionAttributes = "attributes";
String BTreeOptionHotfiles   = "hotfiles";

void set_mode(HIOptions* options, int mode)     { options->mode |= (1 << mode); }
void clear_mode(HIOptions* options, int mode)   { options->mode &= ~(1 << mode); }
bool check_mode(HIOptions* options, int mode)   { return (options->mode & (1 << mode)); }
