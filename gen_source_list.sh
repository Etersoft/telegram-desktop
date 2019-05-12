#!/bin/sh

OFILE=Telegram/cmake/TelegramSourceFiles.cmake

cat <<EOF >$OFILE

# generated with $0 from $OFILE
set (SOURCE_FILES
EOF

grep "^<(src_loc)" Telegram/gyp/telegram_sources.txt | \
    grep -v "platform/mac" | grep -v "platform/win" | \
    sed -e "s|<(src_loc)|    SourceFiles|g" >> $OFILE

cat <<EOF >>$OFILE
)
EOF
