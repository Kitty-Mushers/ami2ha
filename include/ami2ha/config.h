/*
 * ami2ha -- dashboard configuration
 *
 * Portable C99: the parser has no OS dependencies, so it is covered by the
 * host test suite. src/config/ only supplies the file I/O.
 *
 * The format is deliberately a plain text file rather than an IFF prefs
 * chunk. It is meant to be readable, diffable and shareable -- someone
 * should be able to post their dashboard on a forum and have it work.
 *
 *   # comments start with # or ;
 *
 *   host       homeassistant.local
 *   port       8123
 *   tls        no          # yes for https; needs AmiSSL
 *   tlsverify  yes         # no only for a self-signed certificate
 *   tokenfile  S:ha.token
 *   savedir    Work:ami2ha-shots
 *   savesize   1280 720    # a camera's "Save at" size; or native
 *   columns    2
 *
 *   group "Wohnzimmer"          # placed in a column automatically
 *       sensor sensor.wohnzimmer_temperatur label "Temperatur"
 *       gauge  sensor.wohnzimmer_co2        label "CO2" min 400 max 2000
 *       toggle light.wohnzimmer             label "Licht"
 *   end
 *
 *   group "Kameras" column 2    # or pinned to one
 *       camera camera.einfahrt  label "Einfahrt" width 320 height 180
 *   end
 *
 *   group "Szenen"
 *       button scene.gute_nacht  label "Gute Nacht"
 *       button light.turn_on     label "Hell"  entity light.kueche \
 *              data {"brightness":255}
 *   end
 */
#ifndef AMI2HA_CONFIG_H
#define AMI2HA_CONFIG_H

#include <stddef.h>

#include "ami2ha/buf.h"
#include "ami2ha/entity.h"

#define CFG_LABEL_MAX    40
#define CFG_TITLE_MAX    40
#define CFG_SERVICE_MAX  48
#define CFG_DATA_MAX     128
#define CFG_HOST_MAX     96
#define CFG_PATH_MAX     128
#define CFG_ERR_MAX      120
#define CFG_TOKEN_MAX    256

/* Bounds exist to keep a mistyped or hostile file from exhausting memory. */
#define CFG_MAX_GROUPS   32
#define CFG_MAX_COLUMNS  8
#define CFG_SAVE_W       1280   /* savesize when the file says nothing */
#define CFG_SAVE_H       720
#define CFG_MAX_WIDGETS  128

/* Widgets are allocated in blocks of this many as the file is read. */
#define CFG_WIDGET_CHUNK 16

typedef enum {
    W_SENSOR = 0, /* read-only value with optional unit    */
    W_TOGGLE,     /* checkbox bound to an on/off entity    */
    W_BUTTON,     /* fires a service call                  */
    W_GAUGE,      /* value drawn as a bar between min/max  */
    W_TEXT,       /* static caption, no entity             */
    W_CAMERA,     /* snapshot from a camera entity          */
    W_MEDIA,      /* media player: what is playing, + transport */
    W_DIMMER,     /* light brightness on a slider, 0..100%   */
    W_COLOR,      /* rgb light: three sliders and a swatch   */
    W_COVER       /* blind or shutter: open, stop, close     */
} widget_kind;

typedef struct {
    widget_kind kind;
    char        entity[HA_ENTITY_ID_MAX];
    char        label[CFG_LABEL_MAX];
    char        service[CFG_SERVICE_MAX]; /* button: "domain.service" */
    char        data[CFG_DATA_MAX];       /* button: raw JSON service data */
    long        min, max;                 /* gauge range   */
    int         decimals;                 /* gauge scaling */
    /*
     * Camera only. The size is asked of Home Assistant rather than scaled
     * here: it will resize server-side, which turns a 31 KB 1280x720 frame
     * into about 6 KB at 320x180 and saves the Amiga both the transfer and
     * the decoding. Both dimensions must be sent -- a width on its own is
     * silently ignored.
     *
     * cam_refresh is seconds between automatic updates; 0 means only when
     * asked, which is the sensible default when decoding a JPEG costs real
     * time on a 68k.
     */
    int         cam_w, cam_h;
    int         cam_refresh;
    /*
     * Caption the tile with the time the snapshot arrived. Optional
     * because most cameras burn their own timestamp into the picture, and
     * two of them on one tile is just clutter -- but that one is the
     * camera's clock, and this one says when ami2ha actually looked.
     */
    int         cam_stamp;
    /*
     * The label was written in the file rather than derived from the entity
     * id. Home Assistant's friendly name is the better caption for a widget
     * that was discovered by label -- but not for one someone named
     * themselves, and without this the two are indistinguishable.
     */
    int         label_explicit;
    /*
     * The kind was chosen for us from the domain, not written in the file.
     * Only such a widget may be refined once the states arrive: a `toggle`
     * someone typed for a colour lamp is a decision, not a guess.
     */
    int         kind_auto;
    int         group;                    /* owning group index */
} a2h_widget;

typedef struct {
    char title[CFG_TITLE_MAX];
    int  first_widget; /* index into cfg->widgets */
    int  nwidgets;
    /*
     * Which dashboard column the group sits in, counting from 1; 0 lets
     * cfg_place_groups choose. Kept as written even when it is beyond the
     * current column count, so lowering `columns` and raising it again
     * does not lose the arrangement -- placement clamps it instead.
     */
    int  column;
} a2h_group;

typedef struct {
    char host[CFG_HOST_MAX];
    int  port;
    /*
     * Whether `port` was actually named, as opposed to being the 8123 that
     * cfg_init fills in. Without this a caller cannot tell the default from
     * a deliberate 8123, and so cannot pick a different default of its own
     * -- which is exactly what a TLS connection needs to do for 443.
     */
    int  port_explicit;
    /*
     * Reach the server over TLS. Off by default: Home Assistant's own
     * default is plain http on 8123, and most people run ami2ha on the
     * same network as the server.
     */
    int  tls;
    /*
     * Check the certificate against the trusted roots, and that it names
     * the host being connected to. On unless explicitly turned off, which
     * is what a self-signed certificate needs -- that keeps the traffic
     * encrypted but stops proving who is answering.
     */
    int  tls_verify;
    char tokenfile[CFG_PATH_MAX];
    /*
     * Where "Save snapshot" on a camera tile puts its picture. Empty means
     * the default, PROGDIR:snapshots -- resolved where it is used rather than
     * filled in here, so a configuration written back out stays as short as
     * the one that was read.
     */
    char savedir[CFG_PATH_MAX];
    /*
     * The size a camera's "Save at ..." asks Home Assistant for, which
     * scales it on the server. 1280x720 by default: large enough to keep,
     * and kind to whoever the picture is passed on to on a smaller
     * machine. 0x0 is the camera's native frame, `savesize native`.
     */
    int  save_w, save_h;
    char token[CFG_TOKEN_MAX];
    /*
     * When set, the entities come from Home Assistant: everything tagged
     * with this label, rather than the widgets listed below. Lets the
     * selection be made in the HA UI instead of in this file.
     */
    char label[48];

    int  columns;      /* dashboard columns; 0 = let MUI decide */
    int  refresh_secs; /* application-level ping interval       */

    a2h_group   groups[CFG_MAX_GROUPS];
    int         ngroups;

    /*
     * Grown on demand rather than embedded. Embedding CFG_MAX_WIDGETS
     * widgets made a2h_config roughly 80 KB, which is both absurd on a 2 MB
     * machine for a dashboard holding a dozen controls, and large enough to
     * land in territory where the allocation misbehaved on real hardware.
     * A typical dashboard now costs a few KB.
     */
    a2h_widget *widgets;
    int         nwidgets;
    int         widget_cap;
} a2h_config;

/* Populate with defaults (port 8123, one column, no widgets). */
void cfg_init(a2h_config *cfg);

/* Release the widget array. Safe on a zeroed or already-freed config. */
void cfg_free(a2h_config *cfg);

/*
 * Parse `text`. Returns 1 on success. On failure returns 0 and writes a
 * message naming the offending line into `err`.
 */
int cfg_parse(a2h_config *cfg, const char *text, size_t len,
              char *err, size_t errsz);

/*
 * Serialise a configuration back to the dashboard format, so the editor can
 * save what the user arranged. Parsing the result must reproduce the same
 * configuration.
 *
 * Note that this regenerates the file: comments and hand-made layout are
 * not preserved, because there is nothing in the parsed form to reconstruct
 * them from.
 */
int cfg_write(const a2h_config *cfg, a2h_buf *out);

/*
 * Append a widget, inferring its kind from the entity's domain. Used when
 * the dashboard is discovered from Home Assistant rather than written out.
 * Returns 0 if the cap or memory is reached.
 */
/*
 * Improve the kinds that were guessed from the domain, now that the states
 * are in. A light is a checkmark until Home Assistant says it can do more.
 * Only touches widgets whose kind came from discovery.
 */
void cfg_refine_kinds(a2h_config *cfg, ha_store *store);

int cfg_add_discovered(a2h_config *cfg, const char *entity_id,
                       const char *label);

/*
 * Sort a dashboard built by cfg_add_discovered into one group per kind of
 * thing -- lights, switches, covers, scenes, media, sensors, cameras, in
 * that order, each keeping the order Home Assistant gave -- instead of the
 * single group discovery fills. Empty kinds get no group.
 *
 * Not only tidier. One group holding a whole label's worth of entities is
 * taller than the screen, and MUI 3.8 does not draw a group taller than
 * the visible part of a scrolling window: the frame and labels never
 * appear, only the values that update themselves later.
 */
void cfg_arrange_discovered(a2h_config *cfg);

/*
 * Decide which column each group goes in, writing a 0-based column index
 * per group into `col_of` (cfg->ngroups entries). A group pinned with
 * `column` goes where it says, clamped to the column count. The others
 * are taken in file order, each joining whichever column is shortest at
 * that point (the leftmost on a tie) -- the masonry rule, judged on an
 * estimate of each group's height. Pinned groups are counted first, so an
 * unpinned one never lands under a tall pinned one by accident.
 */
void cfg_place_groups(const a2h_config *cfg, int *col_of);

/*
 * Swap group `g` with its neighbour, one place earlier (`delta` -1) or
 * later (+1), taking its widgets along. Earlier means higher up in its
 * column and placed sooner, since placement goes in file order. Returns
 * the group's new index, or -1 when there is nowhere to go.
 */
int cfg_move_group(a2h_config *cfg, int g, int delta);

/* Human-readable name of a widget kind, for error messages and the editor. */
const char *cfg_widget_kind_name(widget_kind k);

/*
 * Write a starter configuration covering everything in `store`, grouped by
 * domain, into `out`. Explicit per-entity config is only tolerable if you
 * do not have to type two hundred entity IDs by hand: this produces a file
 * to prune rather than a blank page.
 *
 * Widget kinds are inferred from the domain (lights and switches become
 * toggles, scenes and scripts become buttons, everything else a readout).
 * Gauges are never guessed -- their ranges are a judgement call, so the
 * generated file explains how to convert a sensor into one instead.
 */
int cfg_generate(a2h_buf *out, const ha_store *store, const a2h_config *base);

/*
 * Build the leading part of a saved snapshot's file name from a widget's
 * label, so a picture is called "Einfahrt-..." rather than
 * "driveway_f-...". The label is what the user reads on screen, so it is
 * what they will look for in the drawer.
 *
 * `label` may be empty or unusable, in which case `entity`'s local part is
 * used instead -- a file must still get a name. Anything a filesystem or a
 * shell would trip over becomes '_': path separators, wildcards, quotes and
 * control characters. Latin-1 accents are kept, since that is what the
 * Amiga's filesystem and its fonts both use.
 *
 * Writes at most `outsz` bytes including the NUL, and never a leading or
 * trailing '_'. Returns the length written.
 */
size_t cfg_label_filename(char *out, size_t outsz,
                          const char *label, const char *entity);

#endif /* AMI2HA_CONFIG_H */
