# Credits and source origins

## JACE

Development and maintenance: Jeffrey S. Groby.
JACE began from Bluefever Software's VICE tutorial engine. Jeffrey confirmed
this origin and reported no code copied from other engines during the release
review on 2026-10-15. JACE contains subsequent changes to search, evaluation,
move ordering, correctness, UCI handling, portability, testing, and packaging.

## VICE / Bluefever Software

- Upstream: https://github.com/bluefeversoft/vice
- Tutorial: the VICE chess-engine programming series by Bluefever Software.
- License: MIT; Copyright (c) 2022 bluefeversoft.
- License reviewed: https://github.com/bluefeversoft/vice/blob/main/LICENSE

VICE is the source foundation, not merely an inspiration. The upstream MIT
copyright and permission notice is retained in JACE's LICENSE and in binary
packages. The release review compared JACE main ec50b539 with the upstream
VICE tree a89f82dcab34b74481d6504312e3d52bbba44320. The polykeys.c file was
identical; attack.c, hashkeys.c, and polybook.c retained substantial shared code.
This comparison corroborates the stated origin; it is not a complete legal audit.

## Polyglot

The Polyglot opening-book format and its standard 781-entry random-key table
originate with Polyglot by Fabien Letouzey. JACE's implementation and table
were inherited through VICE. Supporting that format does not require the
Polyglot executable. This attribution does not claim ownership of the format
or grant rights to unrelated Polyglot distributions.

The optional performance.bin opening book is excluded from binary packages
and Git source archives (via .gitattributes). It remains in repository history
and checkouts. Its source and redistribution terms have not been established in
this review; the JACE MIT license is not a license grant for that book.
Users may supply a book they have permission to use.
