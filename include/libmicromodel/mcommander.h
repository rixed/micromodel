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
#ifndef MCOMMANDER_H_041208
#define MCOMMANDER_H_041208

#define MAX_NB_PARAMS 6
#define MAX_MML_LINELEN 200	// empirical max MML line length
#define MAX_BIN_LINELEN 4+13*sizeof(float)

/* Pour piloter Grid */

void MCom_reset(void);
int MCom_begin(unsigned char command);
int MCom_param_backref(unsigned char backref);	// replace a grid, a vec, a real or a sel
int MCom_param_vec(float x, float y, float z);
int MCom_param_index(unsigned long index);
int MCom_param_int(unsigned char i);
int MCom_param_version(unsigned short v);
int MCom_param_real(float r);
int MCom_param_bool(unsigned char n);	// true/false, South/North, West/East, Min/Max
int MCom_param_geomtype(unsigned char t);	// VERTEX/EDGE/FACET
int MCom_end(void);
int MCom_binexec(unsigned char *bin, unsigned max_size, unsigned *read_size);
unsigned MCom_binexec_all(unsigned char *bin, unsigned bin_size);

/* Pour piloter le modeleur */

typedef enum {
	MCom_VEC=0,
	MCom_SEL,
	MCom_INDEX,
	MCom_INT,
	MCom_VERSION,
	MCom_REAL,
	MCom_BOOL,
	MCom_GEOMTYPE,
	MCom_BASIS,
	MCom_COLOR,
	MCom_NB_TYPES
} MCom_param_type;

unsigned char MCom_query_nb_commands(void);
unsigned char MCom_query_nb_groups(void);
unsigned char MCom_query_sizeof_group(unsigned char group);
const char *MCom_query_group_name(unsigned char group);
const char *MCom_query_command_name(unsigned char command);
const char *MCom_query_command_description(unsigned char command);
const char *MCom_query_command_instruction(unsigned char command);
const char *MCom_query_selType_name(unsigned t);
unsigned char MCom_query_nb_params(unsigned char command);
MCom_param_type MCom_query_param_type(unsigned char command, unsigned char param);
const char *MCom_query_type_name(MCom_param_type type);
const char *MCom_query_param_name(unsigned char command, unsigned char param);
unsigned MCom_query_nb_backrefs(MCom_param_type type);
const char *MCom_query_backref_name(MCom_param_type type, unsigned backref);
const char *MCom_query_backref_sel_name(unsigned backref);
const unsigned char *Mcom_last_cmd_get(unsigned *size);
const char *MCom_last_cmd_geta(void);

#endif
// vi:ts=3:sw=3
