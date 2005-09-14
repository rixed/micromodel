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
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <libcnt/mem.h>
#include "srcbuf.h"

// Il faut garder un buffer MML (copié vers le buffer de source_textbox lors
// du reset, ou lu depuis ce buffer lors du accept_cb de cette fenetre.
// Après une action, il faudrait modifier ce buffer pour ajouter une ligne à la fin.
// Undo efface la dernière ligne, Redo la remet (cherche le \0 suivant).
// Truc : Le format de ce buffer est : l0\nl1\n...lN\0r0\0r1\0..rR\0\0...garbage...
// On peut ainsi facilement, avec un pointeur sur le premier \0, appender, undoer,
// redoer... On va encore gagner en perfs !


/* Data Definition */

struct SrcBuf {
	unsigned data_size;
	unsigned length;
	char *data;
	char first_char;	// to store the first char when the first line was undo
};

/* Private Functions */

static void SrcBuf_reset(SrcBuf *this) {
	assert(this && this->data_size>=2);
	this->length = 0;
	this->data[0] = this->data[1] = '\0';
}

static int SrcBuf_resize(SrcBuf *this, unsigned new_size) {
	assert(this && this->data_size>=2);
	char *tmp = mem_realloc(this->data, new_size);
	if (!tmp) return 0;
	this->data = tmp;
	return 1;
}

/* Public Functions */

SrcBuf *SrcBuf_new(unsigned size) {
	size += 2;	// at least 2 nul chars
	SrcBuf *this = mem_alloc(sizeof(*this));
	if (!this) return NULL;
	this->data = mem_alloc(size);
	if (!this->data) {
		mem_unregister(this);
		return NULL;
	}
	this->data_size = size;
	SrcBuf_reset(this);
	return this;
}

void SrcBuf_del(SrcBuf *this) {
	assert(this && this->data_size>=2);
	mem_unregister(this->data);
	mem_unregister(this);
}

char *SrcBuf_get_text(SrcBuf *this) {
	assert(this && this->data_size>=2);
	return this->data;
}

int SrcBuf_set_text(SrcBuf *this, const char *txt) {
	assert(this && this->data_size>=2);
	SrcBuf_reset(this);
	return SrcBuf_append(this, txt);
}

int SrcBuf_append(SrcBuf *this, const char *txt) {
	assert(this && this->data_size>=3);
	unsigned len = strlen(txt);
	if (this->data_size < this->length+len+3) {
		if (!SrcBuf_resize(this, (this->length+len)*2+3)) return 0;
	}
	if (this->length && this->data[this->length]!='\n') {
		this->data[this->length++] = '\n';
		this->data[this->length] = '\0';
	}
	memcpy(this->data+this->length, txt, len+1);
	this->length += len;
	this->data[this->length+1] = '\0';
	return 1;
}

void SrcBuf_undo(SrcBuf *this) {
	assert(this && this->data_size>=2);
	if (0 == this->length) return;
	while (this->length && this->data[--this->length] == '\n') ;
	while (this->length && this->data[--this->length] != '\n') ;
	if (0 == this->length) this->first_char = this->data[0];
	this->data[this->length] = '\0';
}

void SrcBuf_redo(SrcBuf *this) {
	assert(this && this->data_size>=2);
	if (this->data[this->length+1] == '\0') return;
	if (0 == this->length) this->data[0] = this->first_char;
	else this->data[this->length] = '\n';
	do this->length++; while (this->data[this->length] != '\0');
	assert(this->length < this->data_size);
}

