#ifndef _LJ_PROF_SYMTAB
#define _LJ_PROF_SYMTAB

#define LJS_CURRENT_VERSION 0x1

/*
** symtab format:
**
** symtab         := prologue sym*
** prologue       := 'l' 'j' 's' version reserved
** version        := <BYTE>
** reserved       := <BYTE> <BYTE> <BYTE>
** sym            := sym-lua | sym-final
** sym-lua        := sym-header sym-addr sym-chunk sym-line
** sym-header     := <BYTE>
** sym-addr       := <ULEB128>
** sym-chunk      := string
** sym-line       := <ULEB128>
** sym-final      := sym-header
** string         := string-len string-payload
** string-len     := <ULEB128>
** string-payload := <BYTE> {string-len}
**
** <BYTE>   :  A single byte (no surprises here)
** <ULEB128>:  Unsigned integer represented in ULEB128 encoding
**
** (Order of bits below is hi -> lo)
**
** version: [VVVVVVVV]
**  * VVVVVVVV: Byte interpreted as a plain numeric version number
**
** sym-header: [FUUUUUTT]
**  * TT    : 2 bits for representing symbol type
**  * UUUUU : 5 unused bits
**  * F     : 1 bit marking the end of the symtab (final symbol)
*/

#define SYMTAB_LFUNC ((uint8_t)0)
#define SYMTAB_FINAL ((uint8_t)0x80)

void dump_symtab(struct lj_wbuf *out, const struct global_State *g);

#endif /* ifndef _LJ_PROF_SYMTAB */
