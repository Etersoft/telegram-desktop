#!/bin/sh

OFILE=Telegram/cmake/TelegramSourceFiles.cmake

cat <<EOF >$OFILE

# generated with $0 from $OFILE
set (SOURCE_FILES
EOF

grep "^<(src_loc)" Telegram/gyp/telegram_sources.txt | \
    grep -v "platform/mac" | grep -v "platform/win" | \
    sed -e "s|<(src_loc)|    SourceFiles|g" >> $OFILE

for i in lottie ffmpeg ; do
    grep "<(src_loc)" Telegram/gyp/lib_$i.gyp | \
        grep -v "platform/mac" | grep -v "platform/win" | \
        grep -v "pch_" | grep "\." | \
        sed -e "s|.*<(src_loc)\(.*\)',.*|    SourceFiles\1|g" >> $OFILE
done

cat <<EOF >>$OFILE
)
EOF
