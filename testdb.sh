#!/bin/sh

TITLE="Peters Blag"
HEAD="Wer sch&ouml;ne Verschw&ouml;rungslinks f&uuml;r mich hat: ab an sirdzstic (at) gmail.com!"
TAIL="Proudly made without PHP, Java, Perl, MySQL and Postgres<br><a href=impressum.html>Impressum</a>"
DESC="Ein Testblog fuer happyblog"
URL="http://chelloveck.vellocet.net/cgi-bin/blag.cgi"

bin/createdb -t "$TITLE" -e "$HEAD" -f "$TAIL" -o $1
bin/initrss -d "$DESC" -b "$URL" -o $1
