#ifndef _LJ_SYSPROF_SOTAB_H
#define _LJ_SYSPROF_SOTAB_H

#include "../lj_def.h"
#include "../lj_wbuf.h"
#include <stdint.h>

#define LJSO_CURRENT_VERSION 0x1

/*
** so table format:
**
** sotab          := prologue sym*
** prologue       := 'l' 'j' 's' 'o' version reserved
** version        := <BYTE>
** reserved       := <BYTE> <BYTE> <BYTE>
** so             := so-shared | so-final
** so-shared      := so-header so-addr so-path
** so-header      := <BYTE>
** so-addr        := <ULEB128>
** so-path        := string
** so-final       := sym-header
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
** so-header: [FUUUUUTT]
**  * TT    : 2 bits for representing symbol type
**  * UUUUU : 5 unused bits
**  * F     : 1 bit marking the end of the sotab (final symbol)
*/

#define SOTAB_SHARED ((uint8_t)0)
#define SOTAB_FINAL  ((uint8_t)0x80)

void lj_prof_dump_sotab(struct lj_wbuf *buf);

#endif
