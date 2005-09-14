/* This file is part of MicroModel.
 *
 * Copyright (C) 2005 Cedric Cellier.
 *
 * MicroModel is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * MicroModel is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MicroModel; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef MML2BIN_H_041213
#define MML2BIN_H_041213

#include <libcnt/cnt.h>
#include <libmicromodel/mcommander.h>

struct mmlPatch {
	unsigned offset;
	MCom_param_type type;
};

typedef struct {
	unsigned short size;
	struct mmlPatch patches[];
} mmlPatchSet;

#include <assert.h>
static inline size_t sizeof_patchset(mmlPatchSet *patchset) {
	assert(patchset);
	return sizeof(mmlPatchSet)+patchset->size*sizeof(struct mmlPatch);
}

// Allocated space must be freed with mem_free
// patchset is set only if not NULL and no error were encountered
unsigned char *mml2bin(const char *mml, unsigned *size, mmlPatchSet **patchset);

int patchbina(unsigned char * const bin, mmlPatchSet *patchset, const char *values);

#endif
// vi:ts=3:sw=3
