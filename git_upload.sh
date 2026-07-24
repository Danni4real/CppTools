#!/bin/bash
set -e

# TODO： build project

# TODO： test project

./gen_version_header.sh

git pull
git add -u
git commit -m "$@"
git push origin HEAD:refs/for/`git symbolic-ref -q --short HEAD`
