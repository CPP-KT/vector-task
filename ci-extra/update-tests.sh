#!/usr/bin/env bash
set -euo pipefail

chown "$(id -u):$(id -g)" .

git config --global user.name "John Doe"
git config --global user.email johndoe@example.com

git remote add upstream "https://github.com/$UPSTREAM_REPO.git"
git fetch upstream master

if git merge-base --is-ancestor upstream/master @; then
  echo 'Tests are already up-to-date.'
  echo 'TESTS_UPDATED=0' >>"$GITHUB_ENV"
else
  echo 'Updating tests...'
  git rebase upstream/master
  echo 'Tests updated.'
  echo 'TESTS_UPDATED=1' >>"$GITHUB_ENV"
fi
