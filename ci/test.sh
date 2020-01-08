#! /bin/sh

usage() {
    echo "$(basename $0): build|run|all TEST_OS"
}

do_build() {
    docker build -t icecream-sundae-$TEST_OS -f ci/$TEST_OS/Dockerfile .
}

do_run() {
    docker run --rm --env CC --env CXX \
        --env SANITIZE --env COVERALLS \
        --env TRAVIS_JOB_ID="$TRAVIS_JOB_ID" \
        --env TRAVIS_BRANCH="$TRAVIS_BRANCH" \
        --env COVERALLS_REPO_TOKEN \
        --cap-add SYS_PTRACE \
        icecream-sundae-$TEST_OS \
        /bin/sh -c "../ci/test_build.sh"
}

CMD="$1"
TEST_OS="$2"

if [ -z "$CMD" ] || [ -z "$TEST_OS" ]; then
    echo "Missing argument"
    usage
    exit 1
fi

if [ -z "$SANITIZE" ]; then
    SANITIZE=none
fi
export SANITIZE


case "$CMD" in
    build)
        do_build
        exit $?
        ;;

    run)
        do_run
        exit $?
        ;;
    all)
        do_build
        if [ $? -ne 0 ]; then
            exit $?
        fi
        do_run
        exit $?
        ;;
    *)
        echo "Unknown command '$CMD'"
        usage
        exit 1
        ;;
esac

