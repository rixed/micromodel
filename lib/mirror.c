#include <libcnt/hash.h>
#include "gridsel.h"
#include "grid.h"

/*
 * Data Definitions
 */

static cntHash *mir_geom = NULL;
Vec center, normal;

/*
 * Private Functions
 */

static void mirror_r(GridSel *result, Facet *facet) {
	if (cntHash_get(mir_geom, (cntHashkey){ .ptr=facet })) return;
	Facet **f = cntHash_put(mir_geom, (cntHashkey){ .ptr=facet }, NULL);
	assert(f);
	const unsigned facet_size = Facet_size(facet);
	Edge *edges[facet_size];
	for (unsigned i=0; i<facet_size; i++) {
		Edge *orig_e = Facet_get_edge(facet, i);
		assert(orig_e);
		Edge **mir_e = (Edge **)cntHash_get(mir_geom, (cntHashkey){ .ptr=orig_e });
		if (mir_e) {
			edges[i] = *mir_e;
		} else {	
			Vertex *v[NB_POLES];
			for (EdgePole pole=SOUTH; pole<NB_POLES; pole++) {
				Vertex *orig_v = Edge_get_vertex(orig_e, pole);
				Vertex **mir_v = (Vertex **)cntHash_get(mir_geom, (cntHashkey){ .ptr=orig_v });
				if (mir_v) {
					v[pole] = *mir_v;
				} else {
					Vec pos;
					Vec_sub3(&pos, Vertex_position(orig_v), &center);
					double s = Vec_scalar(&pos, &normal);
					pos = *Vertex_position(orig_v);
					Vec_add_scale(&pos, -2.*s, &normal);
					v[pole] = Grid_vertex_new(&pos, Vertex_basis(orig_v), Vertex_skin_ratio(orig_v), Vertex_uv_x(orig_v), Vertex_uv_y(orig_v));
					Vertex_set_color(v[pole], Vertex_color(orig_v));
					if (result && GridSel_VERTEX == result->type) GridSel_add(result, v[pole]);
					cntHash_put(mir_geom, (cntHashkey){ .ptr=orig_v }, v+pole);
				}
			}
			edges[i] = Grid_edge_new(v[SOUTH], v[NORTH]);
			if (result && GridSel_EDGE == result->type) GridSel_add(result, edges[i]);
			cntHash_put(mir_geom, (cntHashkey){ .ptr=orig_e }, edges+i);
		}
	}
	*f = Grid_facet_new(facet_size, edges, false);
	if (result && GridSel_FACET == result->type) GridSel_add(result, *f);
	for (unsigned i=0; i<facet_size; i++) {
		Facet *neig_f = Facet_get_facet(facet, i);
		if (neig_f == *f) continue;
		mirror_r(result, neig_f);
	}
}

/*
 * Public Functions
 */

int Grid_mirror(unsigned name, unsigned result_selection) {
	assert(!mir_geom);
	if (!Grid_get() || !name) return 0;
	GridSel *sel = Grid_get_selection(name);
	if (!sel || sel->type != GridSel_FACET) return 0;
	GridSel_reset(sel);
	Facet *mir_facet = GridSel_each(sel);
	if (!mir_facet) return 0;
	GridSel my_result, *result = NULL;
	if (result_selection > 0) {
		result = Grid_get_selection(result_selection);
	}
	GridSel_construct(&my_result, result ? result->type : GridSel_FACET);
	mir_geom = cntHash_new(sizeof(void *), Grid_get_carac_size(), 3, cntHash_PTRKEYS, 0);
	assert(mir_geom);
	center = *Facet_center(mir_facet);
	normal = *Facet_normal(mir_facet);
	cntHash_put(mir_geom, (cntHashkey){ .ptr=mir_facet }, NULL);
	Facet *facets[Facet_size(mir_facet)];
	for (unsigned i=0; i<Facet_size(mir_facet); i++) {
		Vertex *v = Facet_get_vertex(mir_facet, i);
		Edge *e = Facet_get_edge(mir_facet, i);
		assert(v && e);
		cntHash_put(mir_geom, (cntHashkey){ .ptr=v }, &v);
		cntHash_put(mir_geom, (cntHashkey){ .ptr=e }, &e);
		// we must unlink the edges from mir_facet
		facets[i] = Facet_get_facet(mir_facet, i);
		Edge_remove_facet(e, mir_facet);
	}
	Grid_replace_facet(mir_facet, NULL);
	for (unsigned i=0; i<Facet_size(mir_facet); i++) {
		mirror_r(&my_result, facets[i]);
	}
	cntHash_del(mir_geom);
	mir_geom = NULL;
	output_selection(name, sel, result_selection, &my_result);
	return 1;
}

