from pathlib import Path
import json
root=Path(__file__).resolve().parents[1]
d=json.loads((root/'font/vfont_project.json').read_text(encoding='utf-8-sig'))
chars=d['chars'];assert len(set(chars))==len(chars)
assert d['cell_width']==8 and d['cell_height']==14
out=['#include "vfont.h"']
out.append('const uint16_t vfont_codepoints[VFONT_COUNT] = {'+','.join(str(ord(c)) for c in chars)+'};')
out.append('const uint8_t vfont_widths[VFONT_COUNT] = {'+','.join(str(d['widths'][c]) for c in chars)+'};')
out.append('const uint8_t vfont_rows[VFONT_COUNT][14] = {'+','.join('{'+','.join(map(str,d['glyphs'][c]))+'}' for c in chars)+'};')
(root/'src/vfont.cpp').write_text('\n'.join(out))
(root/'include/vfont.h').write_text('#pragma once\n#include <cstdint>\nconstexpr int VFONT_COUNT='+str(len(chars))+';\nconstexpr int VFONT_LINE_STEP='+str(d['line_step'])+';\nextern const uint16_t vfont_codepoints[VFONT_COUNT];\nextern const uint8_t vfont_widths[VFONT_COUNT];\nextern const uint8_t vfont_rows[VFONT_COUNT][14];\n')
print('Imported',len(chars),'glyphs without scaling')
