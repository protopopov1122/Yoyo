#!/bin/sh

xsltproc opcodes_gen.xsl ../opcodes.xml > ../headers/yoyo/opcodes.h
xsltproc mnemonics_gen.xsl ../opcodes.xml > ../yoyo/mnemonics.c
