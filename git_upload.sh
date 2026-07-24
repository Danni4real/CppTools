#!/bin/bash
set -e

./gen_version_header.sh

git pull
git add -u
git commit -m "$@"
git push origin HEAD:refs/for/`git symbolic-ref -q --short HEAD`
