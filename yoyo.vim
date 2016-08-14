" Vim syntax file
" Yoyo language

if exists("b:current_syntax")
  finish
endif

syn keyword yoyoKeyword while do for foreach switch try catch finally if else pass break return throw continue with using case default
syn keyword yoyoKeyword null interface object overload
syn keyword yoyoKeyword def method var 

syn region brackets start="\[" end="]" fold transparent
syn region braces start="{" end="}" fold transparent
syn region parentheses start="(" end=")" fold transparent

syn match yoyoIdentifier '\i\+'

syn keyword yoyoBoolean true false
syn region yoyoString start='"' end='"'
syn match yoyoInteger '\v[-+]?[0-9_]+'
syn match yoyoInteger '\v[-+]?0x[A-Fa-f0-9_]+'
syn match yoyoInteger '\v[-+]?0b[0-1]+'
syn match yoyoFloat '\v[-+]?[0-9]+\.[0-9]+'

syn match yoyoOperator '\v\*'
syn match yoyoOperator '\v/'
syn match yoyoOperator '\v\+'
syn match yoyoOperator '\v-'
syn match yoyoOperator '\v\%'
syn match yoyoOperator '\v\<'
syn match yoyoOperator '\v\>'
syn match yoyoOperator '\v\&'
syn match yoyoOperator '\v\|'
syn match yoyoOperator '\v\^'
syn match yoyoOperator '\v\='
syn match yoyoOperator '\v!'
syn match yoyoOperator '\v\,'
syn match yoyoOperator '\v\~'
syn match yoyoOperator '\v\:'
syn match yoyoOperator '\v\?'
syn match yoyoOperator '\v\$'
syn match yoyoOperator '\v\.'

syn match yoyoComment "\v//.*$"
syn region multilineYoyoComment start="/\*" end="\*/"

syn keyword yoyoFunction print eval read input import sys

let b:current_syntax = "yoyo"

hi def link yoyoKeyword Statement
hi def link yoyoComment Comment
hi def link multilineYoyoComment Comment
hi def link yoyoString Constant
hi def link yoyoBoolean Constant
hi def link yoyoInteger Constant
hi def link yoyoFloat Constant
hi def link yoyoOperator Operator
hi def link parentheses Operator
hi def link yoyoIdentifier Identifier
hi def link yoyoFunction Function
