# Git & Cursor workflow (hrvflower-esp32)

## Never add Cursor as co-author

Cursor may append to **any** `git commit` / `git commit --amend` run from the Agent terminal:

```text
Co-authored-by: Cursor <cursoragent@cursor.com>
```

`--no-verify` does **not** reliably prevent this.

### Prevention

1. **Cursor Settings → Agents → Attribution** — turn **off**:
   - Commit Attribution
   - PR Attribution
2. Restart Cursor completely.
3. Optional CLI: `~/.cursor/cli-config.json` with attribution flags false; run `cursor /update-cli-config` if using CLI.
4. Enterprise: team admin may force attribution — overrides local config.

### Rewrite a commit without trailers

```bash
cd /path/to/hrvflower-esp32

TREE=$(git rev-parse HEAD^{tree})
PARENT=$(git rev-parse HEAD^)   # omit -p for root commit

NEW=$(GIT_AUTHOR_NAME='tangyumei' \
      GIT_AUTHOR_EMAIL='tangyumei@espressif.com' \
      GIT_AUTHOR_DATE="$(git log -1 --format=%aI)" \
      GIT_COMMITTER_NAME='tangyumei' \
      GIT_COMMITTER_EMAIL='tangyumei@espressif.com' \
      git commit-tree "$TREE" -p "$PARENT" -F - <<'EOF'
feat: your subject line

Body paragraph explaining why.

Author: tangyumei <tangyumei@espressif.com>
EOF
)
git reset --hard "$NEW"
git log -1 --format='%B'   # verify: no Co-authored-by line
```

For the **first** commit (no parent), drop `-p "$PARENT"`.

### Push after rewrite

```bash
git push --force-with-lease origin main
```

Never `git push --force` to `main` without user request; prefer `--force-with-lease`.

---

## Commit author identity (this repo)

Use the **same** identity for git author, committer, and the optional body footer:

| Field | Value |
|-------|--------|
| `user.name` / author name | `tangyumei` |
| `user.email` | `tangyumei@espressif.com` |
| Body footer (last line) | `Author: tangyumei <tangyumei@espressif.com>` |

Rules:

- Capital **`Author:`** only — never `author:`.
- Do **not** mix `Tang Yumei`, `yumei.tang@espressif.com`, or other spellings in commit messages.
- Footer is one blank line before `Author:`; no `Co-authored-by` or Cursor trailers.
- Set `GIT_AUTHOR_*` and `GIT_COMMITTER_*` to the same values in every `commit-tree` call.

Example message:

```text
feat(display): restore HRV UI from NVS on boot

Explain why the change is needed.

Author: tangyumei <tangyumei@espressif.com>
```

---

## Branch naming

| Branch | Role |
|--------|------|
| `main` | Release / default on GitHub; match `origin/main` locally unless merging |
| `feat/<topic>` | Feature work; push this branch, open PR into `main` |

**Topic** = short kebab-case slug, e.g. `low-power-imu`, `wifi-provisioning`.

Example: `feat/low-power-imu` → remote `origin/feat/low-power-imu`.

```bash
git checkout main && git pull origin main
git checkout -b feat/my-topic
# ... commits ...
git push -u origin feat/my-topic
```

---

## Commit message format

Subject (Conventional Commits + scope when helpful):

```text
feat(atoms3r): add deep sleep with BMI270 motion wake
fix(wifi): correct provisioning UI queue init
docs: update FIRMWARE_CN low-power section
```

Body: why, not a file list. Last line:

```text
Author: tangyumei <tangyumei@espressif.com>
```

---

## Pre-commit cleanup (required)

Before **every** `git add` / commit the user requests:

1. **Remove redundancy** — unused `#include`, dead helpers, duplicate `REQUIRES` entries, one-line wrappers only called once, unread queue/event payloads.
2. **Keep patterns** — LVGL only under `display_lock()` from dedicated tasks (see `wifi_connect.cpp` / `hrv_user_btn.cpp`); button callbacks must not block (use `iot_button` + queue only).
3. **Align config/docs** — board `sdkconfig.defaults.board` matches Kconfig; pin/README notes match M5 docs when touching hardware.
4. **Verify build** — `idf.py build` or `./scripts/build_board.sh m5_AtomS3R boards/m5stack` succeeds after cleanup.
5. **Version** — bump root `CMakeLists.txt` `PROJECT_VER` for user-visible firmware changes in the same commit (or call out if skipping).

---

## When to commit

- Only when the user **explicitly** asks to commit, amend, or push.
- Run the **pre-commit cleanup** checklist above first.
- Draft message: 1-line subject (`feat:` / `fix:` / `docs:`), short body on **why**, then the `Author:` footer above.
- Use HEREDOC for `git commit -m "$(cat <<'EOF' ... EOF)"`.

---

## GitHub (`gh`)

Agent environment may not be logged in:

```bash
gh auth login
gh repo view tangyumei3535/hrvflower-esp32
```

Create empty repo (no README/license on GitHub), then:

```bash
git push -u origin main
```

---

## Recreate remote repo (clean Contributors)

If `cursoragent` appeared on GitHub Insights from old commits:

1. Delete repo on GitHub (or `gh repo delete ... --yes`).
2. Create **empty** repo `hrvflower-esp32`.
3. Push local `main` (history already cleaned via `commit-tree`).
4. Contributors should show only `tangyumei3535` after cache updates.

---

## Optional: strip trailer hook (local only)

`.git/hooks/prepare-commit-msg` is not version-controlled:

```bash
cat > .git/hooks/prepare-commit-msg <<'EOF'
#!/bin/bash
sed -i '' '/cursoragent@cursor\.com/d' "$1" 2>/dev/null || sed -i '/cursoragent@cursor\.com/d' "$1"
EOF
chmod +x .git/hooks/prepare-commit-msg
```

Re-clone requires re-installing the hook.

---

## Amend into previous commit

User may ask to fold changes into the last commit:

1. Stage files.
2. Prefer **`commit-tree`** with parent = `HEAD^` and tree = `git rev-parse HEAD^{tree}` after staging…  
   **Note**: staged changes need to be committed to index first — use `git write-tree` after `git add` for amended tree, or amend via `commit-tree` on `git write-tree` output.

Simpler when working tree matches desired tree:

```bash
git add -A
TREE=$(git write-tree)
PARENT=$(git rev-parse HEAD^)
# ... commit-tree with -p "$PARENT" and same message as before
```

If user allows normal amend and Attribution is off, `git commit --amend` may still add trailer — verify with `git log -1 --format=%B`.
