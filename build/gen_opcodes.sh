#!/bin/sh

xsltproc opcodes_gen.xsl ../opcodes.xml > ../yoyo-vm/headers/opcodes.h
xsltproc mnemonics_gen.xsl ../opcodes.xml > ../yoyo-vm/mnemonics.c
