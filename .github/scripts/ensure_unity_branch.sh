#!/usr/bin/env bash
# Ensures a same-named branch exists on the .NET bindings repo, branching from BRANCH_FROM if missing.
# We do this so there is a branch to edit whilst performing a PR, such that we can unblock ourselves.
# Required env: REPO, BRANCH_TO_MAKE, BRANCH_FROM, GH_TOKEN, GITHUB_STEP_SUMMARY
set -euo pipefail

BRANCH_URL="https://github.com/$REPO/tree/$BRANCH_TO_MAKE"

if gh api "repos/$REPO/git/ref/heads/$BRANCH_TO_MAKE" >/dev/null 2>&1; then
  echo "Branch $BRANCH_TO_MAKE already exists on $REPO, skipping creation."
  STATUS="Using existing branch"
else
  SHA=$(gh api "repos/$REPO/git/ref/heads/$BRANCH_FROM" --jq '.object.sha')
  gh api "repos/$REPO/git/refs" \
    --method POST \
    --field ref="refs/heads/$BRANCH_TO_MAKE" \
    --field sha="$SHA"
  STATUS="Created new branch from \`$BRANCH_FROM\`"
fi

{
  echo "### .NET Bindings Branch"
  echo ""
  echo "$STATUS: [\`$BRANCH_TO_MAKE\`]($BRANCH_URL)"
  echo ""
  echo "Push any required binding changes to this branch — re-run this workflow from the Actions tab to pick them up."
} >> "$GITHUB_STEP_SUMMARY"
