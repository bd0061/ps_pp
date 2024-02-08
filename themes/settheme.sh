#!/bin/bash

(( $# == 0 )) && echo "Usage: $0 [configfile]" && exit 1

[[ -v XDG_HOME ]] && path=$XDG_HOME/ps_pp.conf || path=$HOME/.config/ps_pp.conf

cat $1 > $path
