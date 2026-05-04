/* eapp.c — edge-app instance + field accumulator.
 *
 * An edge app is identified entirely by:
 *   - a manifest (name, subtitle, primitive choice, accent color, field
 *     schema)
 *   - a live accumulator of typed field values
 *
 * No C code per app. Sending a delta = writing a field. The renderer in
 * eapp_render.c recomputes the visible UI from the accumulator on every
 * draw using the universal frame's primitive renderer.
 *
 * This is the v0.9 manifestation of the invariant-frame pitch:
 *   visible_state = invariant_frame ⊕ accumulated_field_deltas
 * which mirrors ATOMiK's hardware reduction:
 *   current_state = initial_state    ⊕ accumulator
 */
#include "atomik_os.h"
#include <stdio.h>
#include <string.h>

void eapp_init(edge_app_t *a, const char *name, const char *subtitle,
               primitive_t prim, pixel_t accent) {
    memset(a, 0, sizeof *a);
    snprintf(a->name,     sizeof a->name,     "%s", name     ? name     : "");
    snprintf(a->subtitle, sizeof a->subtitle, "%s", subtitle ? subtitle : "");
    a->primitive   = prim;
    a->accent      = accent;
    a->field_count = 0;
}

int eapp_add_field(edge_app_t *a, field_type_t t) {
    if (a->field_count >= EAPP_MAX_FIELDS) return -1;
    int id = a->field_count++;
    a->fields[id].type   = t;
    a->fields[id].i      = 0;
    a->fields[id].list_n = 0;
    a->fields[id].str[0] = 0;
    a->fields[id].color  = 0;
    return id;
}

static int valid_field(edge_app_t *a, int id) {
    return id >= 0 && id < a->field_count;
}

void eapp_set_str(edge_app_t *a, int id, const char *s) {
    if (!valid_field(a, id) || a->fields[id].type != FT_STR) return;
    snprintf(a->fields[id].str, FIELD_STR_MAX, "%s", s ? s : "");
}

void eapp_set_int(edge_app_t *a, int id, int v) {
    if (!valid_field(a, id) || a->fields[id].type != FT_INT) return;
    a->fields[id].i = v;
}

void eapp_set_color(edge_app_t *a, int id, pixel_t c) {
    if (!valid_field(a, id) || a->fields[id].type != FT_COLOR) return;
    a->fields[id].color = c;
}

void eapp_clear_list(edge_app_t *a, int id) {
    if (!valid_field(a, id) || a->fields[id].type != FT_LIST) return;
    a->fields[id].list_n = 0;
}

void eapp_list_append(edge_app_t *a, int id, const char *s) {
    if (!valid_field(a, id) || a->fields[id].type != FT_LIST) return;
    field_value_t *fv = &a->fields[id];
    if (fv->list_n >= FIELD_LIST_MAX) return;
    snprintf(fv->list[fv->list_n], FIELD_STR_MAX, "%s", s ? s : "");
    fv->list_n++;
}

const char *eapp_primitive_name(primitive_t p) {
    static const char *NAMES[PRIM_MAX] = {
        [PRIM_LIST]  = "list",
        [PRIM_CARD]  = "card",
        [PRIM_GRID]  = "grid",
        [PRIM_FEED]  = "feed",
        [PRIM_CONVO] = "conversation",
    };
    if (p < 0 || p >= PRIM_MAX) return "?";
    return NAMES[p];
}
