"""Hearthstone keepsakes: copies of Hearthstone (8690, category 1176, so they share its cooldown and the chosen
cooldown length) with the casting look of another stock spell. Writes the rows into client/patch-T.MPQ Spell.dbc and
data/sql/db-world/base/mod_temucraft_travel_keepsakes_dbc.sql (server spell_dbc). Re-runnable.
usage: python3 tools/build_keepsakes.py   (from the module folder)"""
import re, struct

# spell id, name, description, look from (stock spell: its SpellVisual and icon)
KEEPSAKES = [
    (97180, 'Shard of the Dark Portal', 'Returns you to your home, through a flash of fel fire.', 30146),
    (97181, "Arcanist's Waystone", 'Returns you to your home the way the magi travel.', 3561),
    (97182, 'Death Gate Shard', 'Returns you to your home through a gate of the dead.', 50977),
    (97183, 'Ember of the Imp', 'Returns you to your home in a puff of imp fire.', 688),
    (97184, 'Voidtouched Stone', 'Returns you to your home, folded through the void.', 697),
    (97185, 'Lightbound Stone', 'Returns you to your home on a beam of holy light.', 2006),
    (97186, "Spiritwalker's Stone", 'Returns you to your home along the spirit paths.', 2008),
]
F_ID, F_VISUAL, F_ICON, F_NAME, F_DESC = 0, 131, 133, 136, 170
STRING_FIELDS = set(range(136, 204))

DBC = 'client/patch-T.MPQ/DBFilesClient/Spell.dbc'
d = bytearray(open(DBC, 'rb').read())
n, fc, rs, ss = struct.unpack('<4I', d[4:20])
rows = [bytearray(d[20 + i * rs:20 + (i + 1) * rs]) for i in range(n)]
strings = bytearray(d[20 + n * rs:])
def u(r, i): return struct.unpack('<I', r[i * 4:i * 4 + 4])[0]
def setu(r, i, v): r[i * 4:i * 4 + 4] = struct.pack('<I', v)
def s_at(o): return strings[o:strings.index(b'\0', o)].decode('latin1')
def add_str(t):
    o = len(strings); strings.extend(t.encode('latin1') + b'\0'); return o
by_id = {u(r, 0): k for k, r in enumerate(rows)}
base = rows[by_id[8690]]
new_ids = {k[0] for k in KEEPSAKES}
rows = [r for r in rows if u(r, 0) not in new_ids]
by_id = {u(r, 0): k for k, r in enumerate(rows)}
out_rows = []
for sid, name, desc, src in KEEPSAKES:
    r = bytearray(base); s = rows[by_id[src]]
    setu(r, F_ID, sid); setu(r, F_VISUAL, u(s, F_VISUAL)); setu(r, F_ICON, u(s, F_ICON))
    for f in range(136, 204): setu(r, f, 0)
    setu(r, F_NAME, add_str(name)); setu(r, F_DESC, add_str(desc))
    setu(r, 152, 0xFF01FE)  # Name flags (enUS)
    setu(r, 186, 0xFF01FE)  # Description flags
    rows.append(r); out_rows.append(r)
rows.sort(key=lambda r: u(r, 0))
hdr = struct.pack('<4s4I', b'WDBC', len(rows), fc, rs, len(strings))
open(DBC, 'wb').write(hdr + b''.join(rows) + bytes(strings))

# server rows: column names and number types from the Homecoming rows already in our dbc SQL
src = open('data/sql/db-world/base/mod_temucraft_travel_dbc.sql').read()
i = src.index('INSERT INTO `spell_dbc` (')
cols = re.findall(r'`(\w+)`', src[i + 25:src.index(') VALUES', i)])
tmpl = src[src.index('(97151,', i):]
vals = re.findall(r"'(?:[^'\\]|\\.|'')*'|[^,()\n]+", tmpl[1:tmpl.index('\n')].rstrip(',;)'))
assert len(cols) == fc == len(vals), (len(cols), len(vals))
def sqlval(r, f):
    if f in STRING_FIELDS:
        v = u(r, f); 
        if vals[f].startswith("'"): return "'" + (s_at(v) if v else '').replace("'", "''") + "'"
        return str(v)
    if '.' in vals[f]: return repr(round(struct.unpack('<f', r[f * 4:f * 4 + 4])[0], 6))
    v = u(r, f); return str(v - (1 << 32) if v >= 1 << 31 and vals[f].startswith('-') else v if v < 1 << 31 else v - (1 << 32))
lines = ['-- mod-temucraft-travel: hearthstone keepsake spells (copies of 8690 with other casting looks).',
         '-- Made by tools/build_keepsakes.py; the client rows are in client/patch-T.MPQ Spell.dbc.',
         'DELETE FROM `spell_dbc` WHERE `ID` BETWEEN 97180 AND 97186;',
         'INSERT INTO `spell_dbc` (' + ', '.join('`%s`' % c for c in cols) + ') VALUES']
lines.append(',\n'.join('(' + ','.join(sqlval(r, f) for f in range(fc)) + ')' for r in out_rows) + ';')
open('data/sql/db-world/base/mod_temucraft_travel_keepsakes_dbc.sql', 'w').write('\n'.join(lines) + '\n')
print('ok', len(out_rows))
