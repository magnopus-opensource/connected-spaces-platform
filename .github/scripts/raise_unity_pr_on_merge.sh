#!/usr/bin/env bash
# Opens a PR on the .NET bindings repo if BRANCH has commits ahead of BASE. Should be triggered on a successful merge of a PR.
# Only raises a PR if the working branch actually updated.
# Required env: REPO, BRANCH, BASE, SOURCE_PR_URL, GH_TOKEN
set -euo pipefail

AHEAD=$(gh api "repos/$REPO/compare/$BASE...$BRANCH" --jq '.ahead_by')
if [ "$AHEAD" = "0" ]; then
  echo "No commits ahead of $BASE on $BRANCH; skipping PR."
  exit 0
fi

gh pr create --repo "$REPO" --head "$BRANCH" --base "$BASE" \
  --title "Breaking CSP Changes: $BRANCH" \
  --body "Auto-raised from merged CSP PR: $SOURCE_PR_URL"
