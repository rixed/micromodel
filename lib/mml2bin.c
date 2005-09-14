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
#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <libcnt/cnt.h>
#include <libcnt/log.h>
#include <libcnt/vec.h>
#include "libmicromodel/mml2bin.h"
#include "libmicromodel/mcommander.h"

/* Private Functions */

static unsigned char *write_param(unsigned char *dst, const char *src, MCom_param_type type, unsigned *src_len) {
	unsigned len = 0;
	char *after;
	float f;
	switch (type) {
		case MCom_SEL:
			if (0!=strncmp("new", src, 3)) return NULL;
			len = 3;
			// output nothing
			break;
		case MCom_INT:
			*dst++ = strtoul(src, &after, 0);
			len = after - src;
			break;
		case MCom_BOOL:
			*dst++ = !!strtoul(src, &after, 0);
			len = after - src;
			break;
		case MCom_GEOMTYPE:
			;
			unsigned i;
			for (i=0; i<3; i++) {
				unsigned l = strlen(MCom_query_selType_name(i));
				if (0==strncmp(MCom_query_selType_name(i), src, l)) {
					*dst++ = i;
					len = l;
					break;
				}
			}
			if (i>=3) return NULL;
			break;
		case MCom_VEC:
			f = strtod(src, &after);
			len = after - src;
			if (src[len] != ',') return NULL;
			len++;
			memcpy(dst, &f, sizeof(f));
			dst += sizeof(f);
			f = strtod(src+len, &after);
			len = after - src;
			if (src[len] != ',') return NULL;
			len++;
			memcpy(dst, &f, sizeof(f));
			dst += sizeof(f);
			f = strtod(src+len, &after);
			len = after - src;
			memcpy(dst, &f, sizeof(f));
			dst += sizeof(f);
			break;
		case MCom_REAL:
			f = strtod(src, &after);
			len = after - src;
			memcpy(dst, &f, sizeof(f));
			dst += sizeof(f);
			break;
		case MCom_INDEX:
			;
			unsigned long index = strtoul(src, &after, 0);
			len = after - src;
			memcpy(dst, &index, sizeof(index));
			dst += sizeof(index);
			break;
		case MCom_VERSION:
			;
			unsigned short v = strtoul(src, &after, 0);
			len = after - src;
			memcpy(dst, &v, sizeof(v));
			dst += sizeof(v);
			break;
		default:
			assert(0);
	}
	if (src_len) *src_len = len;
	return dst;
}

static int seta(unsigned char *begin, unsigned offset, const char *src, unsigned max_size, mmlPatchSet **patchset, unsigned *nb_max_patches, int *err) {	// terminated by \0 or \n
	assert(begin && src && max_size>=2);
	unsigned command, len = 0;
	unsigned char *dst = begin+offset;
	while (src[len]!='\t' && src[len]!=' ' && src[len]!='\n' && src[len]!='\0') len++;
	for (command=0; command < MCom_query_nb_commands(); command++) {
		if (0==strncmp(MCom_query_command_instruction(command), src, strlen(MCom_query_command_instruction(command)))) break;
	}
	if (command >= MCom_query_nb_commands()) {
err_quit:
		*err = 1;
		return dst-begin;
	};
	unsigned char *end = dst+max_size;
	*dst++ = command;
	unsigned char *was_backref = dst++;
	*was_backref = 0;
	char *after;
	for (unsigned param = 0; dst<end && param<MCom_query_nb_params(command); param++) {
		if (src[len]!='\t' && src[len]!=' ') goto err_quit;
		len ++;
		if (src[len] == '\\') {
			len ++;
			*was_backref |= (1<<param);
			*dst++ = strtoul(src+len, &after, 0);
			len = after - src;
		} else {
			if (src[len] == '$') {
				len ++;
				unsigned long patch_num = strtoul(src+len, &after, 0);
				len = after - src;
				if (src[len] != '=') goto err_quit;
				len ++;
				if (*patchset) {
					if (patch_num >= *nb_max_patches) {
						const unsigned new_nb_max_p = *nb_max_patches*2;
						mmlPatchSet *tmp = mem_realloc(*patchset, sizeof(mmlPatchSet)+new_nb_max_p*sizeof(struct mmlPatch));
						if (!tmp) goto err_quit;
						*patchset = tmp;
						*nb_max_patches = new_nb_max_p;
					}
					(*patchset)->patches[patch_num].offset = dst-begin;
					(*patchset)->patches[patch_num].type = MCom_query_param_type(command, param);
					if (patch_num >= (*patchset)->size) (*patchset)->size = patch_num + 1;
				}
			}
			unsigned src_len;
			unsigned char *new_dst = write_param(dst, src+len, MCom_query_param_type(command, param), &src_len);
			if (! new_dst) goto err_quit;
			dst = new_dst;
			len += src_len;
		}
	}
	if (dst>=end || (src[len]!='\n' && src[len]!='\0')) goto err_quit;
	*err = 0;
	return dst-begin-offset;
}

static const char *find_value(const char *values, unsigned num) {
	// Look through the values text buffer for the begining of value num
	// (that is, "$num=", which cannot appear in a value whatsoever).
	char needle[16];
	unsigned len = snprintf(needle, sizeof(needle), "$%u=", num);
	if (len >= sizeof(needle)) return NULL;
	const char *s = strstr(values, needle);
	if (s) return s+len;
	else return NULL;
}

/* Public Functions */

unsigned char *mml2bin(const char *mml, unsigned *size, mmlPatchSet **patchset) {
	assert(mml);
	const char *begin = mml, *end;
	unsigned offset = 0, bufsize = 1024;
	unsigned char *buf = mem_alloc(bufsize);
	if (!buf) return NULL;
	unsigned nb_max_patches = 50;
	if (patchset) {
		*patchset = mem_alloc(sizeof(mmlPatchSet)+nb_max_patches*sizeof(struct mmlPatch));
		if (!*patchset) {
			mem_unregister(buf);
			return NULL;
		}
		(*patchset)->size = 0;
	}
	unsigned lineno = 1;
	do {
		while (*begin == '\n') {
			lineno++;
			begin++;
		}
		if (! *begin) break;
		end = begin;
		while (*end && *end!='\n') end++;
		if (*begin != '#') {
			if (bufsize - offset < MAX_BIN_LINELEN) {
				unsigned new_size = bufsize + MAX_BIN_LINELEN + (bufsize>>1);
				unsigned char *tmp = mem_realloc(buf, new_size);
				if (!tmp) {
errQuit:
					mem_unregister(buf);
					if (patchset) mem_unregister(*patchset), *patchset = NULL;
					return NULL;
				}
				buf = tmp;
				bufsize = new_size;
			}
			int err;
			unsigned len = seta(buf, offset, begin, bufsize - offset, patchset, &nb_max_patches, &err);
			if (err) {
				log_warning(LOG_IMPORTANT, "MML Syntax Error line %u", lineno);
				goto errQuit;
			}
			offset += len;
		}
		begin = end;
		while (*begin == '\n') {
			lineno++;
			begin++;
		}
	} while (*begin);
	if (patchset) {
		mmlPatchSet *tmp = mem_realloc(*patchset, sizeof_patchset(*patchset));
		if (tmp) *patchset = tmp;
	}
	if (size) *size = offset;
	return buf;
}

int patchbina(unsigned char * const bin, mmlPatchSet *patchset, const char *values) {
	assert(bin && patchset && values);
	for (unsigned p = 0; p < patchset->size; p++) {
		const char *v = find_value(values, p);
		if (!v || NULL == write_param(bin + patchset->patches[p].offset, v, patchset->patches[p].type, NULL)) {
			log_warning(LOG_DEBUG, "Cannot apply patch %u", p);
			return 0;
		}
	}
	return 1;
}

// vi:ts=3:sw=3
