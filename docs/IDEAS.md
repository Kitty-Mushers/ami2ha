# Ideas not yet built

## Choosing entities from inside Home Assistant

Today the dashboard is a text file on the Amiga. It would be nicer to pick
the entities in Home Assistant itself and have the Amiga follow that, so a
change does not mean editing a file on a machine that may be in another
room.

Three ways to do it, cheapest first.

### Labels (no custom component) -- IMPLEMENTED

Built. Set `label <name>` in the dashboard file; see the README.

Measured on a real installation before choosing this route:

    config/entity_registry/list                 2,433,832 bytes (3494 entities)
    render_template label_entities('amiga')           164 bytes

The registry route is not merely wasteful, it is impossible: it exceeds the
2 MB WebSocket message cap on its own. What follows is the original note.



Home Assistant has labels. Tag entities with `amiga`, and ami2ha asks the
entity registry which entities carry that label:

```
{"id":N,"type":"config/entity_registry/list"}
```

then filters on `labels` and feeds the result straight into
`subscribe_entities`. Nothing to install on the HA side, and the selection
UI already exists -- it is the normal label picker.

The registry list is large on a big installation (it describes every
entity), so it would want fetching once at startup and parsing streamed
rather than buffered whole. Widget *kind* would still have to be inferred
from the domain, as the generator already does, or overridden locally.

### A group or input_text helper

Put the entity ids in a `group`, or a comma-separated `input_text`. Both are
tiny to read and need no custom code. Cruder than labels, but the payload is
tiny and the parsing trivial.

### A custom integration with a config flow

A proper `ami2ha` integration with a config flow: pick entities, choose
widget kinds and gauge ranges, and expose the lot as one JSON document the
Amiga fetches. The nicest to use and by far the most work -- and it puts a
Python component between the Amiga and a protocol it can already speak.

### Worth keeping either way

The local file should not go away. It works with no server-side setup, it
can be shared, and it is the only option if someone is pointing an Amiga at
a Home Assistant they do not administer. Server-side selection is better as
an additional source, not a replacement.

## Settings window: the group order as a visible list -- IMPLEMENTED

Built in 0.5: the settings window's Layout page lists the selected tab's
groups in order, numbered, beside the list of tabs. What follows is the
original note.


Groups are reordered with Move up / Move down beside the Group cycle, and
the cycle's entries carry each group's position and column ("3. Claude
(column 2)"), so the order can be read by opening the cycle. That was
judged enough for now (Oct 2026).

If it ever is not: replace or accompany the cycle with a small list of all
groups, always visible, with the selected one highlighted -- the order and
the columns at a glance, and possibly drag to reorder. The cost is height
in a window that already needs most of the screen for the two entity
lists, which is why it was not done first.

## A colour wheel that is exactly round

On the dashboard the colour control is given a box measured in text --
fourteen lines by sixteen M's -- so the wheel inside comes out about 50
pixels across. It is near round rather than round, because lines and M's
keep different proportions in different fonts: 49x51 on the 4000T, 57x51
on the Pi (Oct 2026).

Exact would mean a small subclass of Coloradjust that sizes itself in
MUIM_AskMinMax, once the font is known, working out how much of its width
and height are the wheel. It would be ami2ha's first custom MUI class,
which is why it waited until after 0.5.
