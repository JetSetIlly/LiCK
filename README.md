# Lick

Lick was a project I dabbled with many years ago as an experiment in
Burrows-Wheeler Transform (BWT) based compression. It is an exension of work I
did in the second year of a Software Engineering degree. The original work has
long since been lost. I present it here for educational purposes.

It is written in ANSI C99 and should compile on any UNIX like system. There is
provision for platform specific code and there seems to be some support for
Amiga FFS protection bits.

## Compilation

Compilation is best achieved with the `make` command. A `Makefile` is provided.

The binaries produced by the `make` procedure are primarily `flick` and `lick`.

`flick` is a file compressor, producing files with the `.flk` file extension. 

`lick` is a file archiver although this seems to be unfinished. Primarily, the
compression code, as used in `flick`, has not been plumbed in. It should be
easy to add.

Both `flick` and `lick` support GNU style options. Use `-h` for the available
help.

Other files left in the `bin/` folder as a result of `make` are testing
binaries. Two scripts, in the project's root folder, are provided to help
automate testing.

`TEST` with the argument `all` will test each component of the compression
library "chain" (see below).

`FLICK_TEST` runs the `flick` binary and compresses/decompresses the contents of
the `text.compression.corpus` directory. If my memory is correct, this is the
[`Calgary Corpus`](https://en.wikipedia.org/wiki/Calgary_corpus)


## Compression Method

The compression method employed by the project is the `BWT` with a pre `RLE`
stage; a `MTF` stage and post-`RLE` stage. Output is `huffman` encoded.

Research has no doubt moved on a lot since 2001 and the method outlined above
was by no means state-of-the-art even then. I experimented with `range
encoding` as an alternative but have removed that code from this release
because it failed the tests when I tried.

I still have printed copies of the papers I was using to help me. There may
have been others, but these are the ones I still have references to.

	A Block-sorting Lossless Data Compression Algorithm
	M. Burrows and D. J. Wheeler
	May 10 1994

	Modifications of the Burrows and Wheeler Data Compression Algorithm
	Balkenhol, Kurtz, Shtarkov
	March 1999

	Second Step Algorithms In The Burrows-Wheeler Compression Algorithm
	Sebastian Deorowicz
	November 22 2001

	Reflections of the Burrows Wheeler transform
	Fenwick
	July 2004

Interestingly, the last two papers post-date the date string in the `version.h`
file. This suggests I did more work on this project after 2001. If given the
opportunity I will look further on old hard drives for any updates
