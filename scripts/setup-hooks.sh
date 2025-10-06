#!/usr/bin/env bash
#
# Setup git hooks for asyncle project
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
HOOKS_DIR="$PROJECT_ROOT/.githooks"
GIT_HOOKS_DIR="$PROJECT_ROOT/.git/hooks"

echo "Setting up git hooks for asyncle..."

# Method 1: Use git config to point to .githooks directory (recommended)
if git config core.hooksPath "$HOOKS_DIR" 2>/dev/null; then
    echo "✓ Git hooks configured to use .githooks directory"
    echo "  All hooks in .githooks/ are now active"
else
    echo "Warning: Could not set core.hooksPath, falling back to symlinks"

    # Method 2: Create symlinks (fallback)
    if [ -d "$GIT_HOOKS_DIR" ]; then
        for hook in "$HOOKS_DIR"/*; do
            if [ -f "$hook" ] && [ -x "$hook" ]; then
                hook_name=$(basename "$hook")
                ln -sf "../../.githooks/$hook_name" "$GIT_HOOKS_DIR/$hook_name"
                echo "✓ Linked $hook_name"
            fi
        done
    else
        echo "Error: .git/hooks directory not found"
        exit 1
    fi
fi

echo ""
echo "Git hooks installed successfully!"
echo ""
echo "Available hooks:"
echo "  - pre-commit: Checks code formatting and runs quick build"
echo ""
echo "To skip hooks temporarily, use: git commit --no-verify"
