#! /bin/bash
set -e

if [ "$COVERALLS" == "1" ]; then
    COVERAGE="-Db_coverage=true -Dbuildtype=debug"
fi

meson -Db_ndebug=false -Dwerror=true -Db_sanitize=$SANITIZE $COVERAGE ..
ninja

ninja test
ninja install

if [ "$COVERALLS" == "1" ]; then
    ninja coverage-html
    BUILDDIR=$(basename $(pwd))
    coveralls --build-root $(pwd) --root $(pwd)/.. --gcov-options '\-lpr' \
        --exclude $BUILDDIR/meson-private \
        --exclude $BUILDDIR/config.h
fi

