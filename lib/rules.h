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
#ifndef RULES_H_050427
#define RULES_H_050427

#include <stdbool.h>
#include "libmicromodel/vertex.h"
#include "libmicromodel/edge.h"
#include "libmicromodel/facet.h"

bool rule_v1(const Vertex *v);
bool rule_v2(const Vertex *v);
bool rule_v3(const Vertex *v);
bool rule_e1(const Edge *v);
bool rule_e2(const Edge *v);
bool rule_e3(const Edge *v);
bool rule_e4(const Edge *v);
bool rule_f1(const Facet *v);
bool rule_f2(const Facet *v);
bool rule_f3(const Facet *v);

#endif
