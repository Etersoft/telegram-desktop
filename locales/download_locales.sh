#!/bin/sh
# https://tlgrm.ru/files/locales/tdesktop/Russian.strings
URL=https://tlgrm.ru/files/locales/tdesktop
for i in Belarusian Russian Ukrainian French Turkish Czech ; do
    target=$(echo "$i" | cut -b1-2 | tr "[A-Z]" "[a-z]")
    echo "Downloading $URL/$i.strings ... to lang_$target.strings"
    curl $URL/$i.strings | iconv -f UCS2 -t utf8 > lang_$target.strings
done

#Telegram/Resources/langs/

# be ru uk fr tu cz