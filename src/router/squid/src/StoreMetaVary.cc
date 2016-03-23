/*
 * Copyright (C) 1996-2015 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 20    Storage Manager Swapfile Metadata */

#include "squid.h"
#include "MemObject.h"
#include "Store.h"
#include "StoreMetaVary.h"

bool
StoreMetaVary::checkConsistency(StoreEntry *e) const
{
    assert (getType() == STORE_META_VARY_HEADERS);

    if (!e->mem_obj->vary_headers) {
        /* XXX separate this mutator from the query */
        /* Assume the object is OK.. remember the vary request headers */
        e->mem_obj->vary_headers = xstrdup((char *)value);
        return true;
    }

    if (strcmp(e->mem_obj->vary_headers, (char *)value) != 0)
        return false;

    return true;
}

