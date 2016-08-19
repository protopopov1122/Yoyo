#!/bin/sh

find . -not -name "archive.sh" \
				-and -not -name "Makefile" \
				-and -not -name "opcodes_gen.xsl" \
				-and -not -name "gen_opcodes.sh" \
				-and -not -name "mnemonics_gen.xsl" \
				-and -not -name "clean.sh" \
				-and -not -name "PGO.sh" \
				-and -not -name "Win32" \
				-and -not -path "./Win32/*" -delete 

