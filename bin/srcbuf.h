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
#ifndef SRCBUF_H_050501
#define SRCBUF_H_050501

typedef struct SrcBuf SrcBuf;

SrcBuf *SrcBuf_new(unsigned size);
void SrcBuf_del(SrcBuf *this);

char *SrcBuf_get_text(SrcBuf *this);
int SrcBuf_set_text(SrcBuf *this, const char *txt);
int SrcBuf_append(SrcBuf *this, const char *txt);
void SrcBuf_undo(SrcBuf *this);
void SrcBuf_redo(SrcBuf *this);

#endif
