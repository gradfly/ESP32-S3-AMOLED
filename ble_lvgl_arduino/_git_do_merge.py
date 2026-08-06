import subprocess
import os
import sys
import re
import codecs

GIT_EXE = r"c:\Users\xx.mu\.workbuddy\vendor\PortableGit\bin\git.exe"
GIT_DIR = r"E:\project\机械手\穿戴康复外骨骼\显示屏\ble_lvgl_arduino\.git\worktrees\feat-wechat-miniapp-ble-esp32-s85KC5"
GIT_WORK_TREE = r"c:\Users\xx.mu\.trae-cn\worktrees\ble_lvgl_arduino"
OUTPUT_FILE = r"c:\Users\xx.mu\.trae-cn\worktrees\ble_lvgl_arduino\_git_result.txt"

env = os.environ.copy()
env["GIT_DIR"] = GIT_DIR
env["GIT_WORK_TREE"] = GIT_WORK_TREE

results = []

def log(msg):
    print(msg)
    results.append(msg)

def run_git(args, check=False):
    cmd = [GIT_EXE] + args
    log(f"\n===== git {' '.join(args)} =====")
    try:
        result = subprocess.run(
            cmd,
            env=env,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace"
        )
        if result.stdout:
            log("STDOUT:")
            log(result.stdout)
        if result.stderr:
            log("STDERR:")
            log(result.stderr)
        log(f"RETURN CODE: {result.returncode}")
        if check and result.returncode != 0:
            raise RuntimeError(f"git {' '.join(args)} failed with code {result.returncode}")
        return result.returncode, result.stdout, result.stderr
    except Exception as e:
        log(f"EXCEPTION: {e}")
        return -1, "", str(e)

def is_in_merge():
    merge_head = os.path.join(GIT_DIR, "MERGE_HEAD")
    return os.path.exists(merge_head)

def get_conflict_files():
    code, out, err = run_git(["diff", "--name-only", "--diff-filter=U"])
    files = []
    for line in out.strip().splitlines():
        line = line.strip()
        if line:
            files.append(line)
    return files

def read_file(path):
    full_path = os.path.join(GIT_WORK_TREE, path)
    try:
        with codecs.open(full_path, "r", encoding="utf-8", errors="replace") as f:
            return f.read()
    except Exception as e:
        log(f"ERROR reading {path}: {e}")
        return None

def write_file(path, content):
    full_path = os.path.join(GIT_WORK_TREE, path)
    try:
        with codecs.open(full_path, "w", encoding="utf-8", errors="replace") as f:
            f.write(content)
        return True
    except Exception as e:
        log(f"ERROR writing {path}: {e}")
        return False

def resolve_conflict_in_content(content, file_path):
    pattern = re.compile(
        r'<<<<<<<[^\n]*\n(.*?)=======\n(.*?)>>>>>>>[^\n]*\n',
        re.DOTALL
    )

    conflicts_found = list(pattern.finditer(content))
    if not conflicts_found:
        return content, 0

    log(f"  Found {len(conflicts_found)} conflict block(s) in {file_path}")

    resolved_count = 0
    def replace_conflict(match):
        nonlocal resolved_count
        src_content = match.group(1)
        tgt_content = match.group(2)

        src_lines = src_content.splitlines(keepends=True)
        tgt_lines = tgt_content.splitlines(keepends=True)

        src_stripped = [l.rstrip() for l in src_lines]
        tgt_stripped = [l.rstrip() for l in tgt_lines]

        if src_stripped == tgt_stripped:
            log(f"    Conflict {resolved_count+1}: both sides identical - keeping either")
            resolved_count += 1
            return src_content

        merged_lines = []
        seen = set()

        def add_line(line, stripped):
            key = stripped
            if key not in seen:
                seen.add(key)
                merged_lines.append(line)

        src_only = []
        for i, line in enumerate(src_lines):
            stripped = src_stripped[i]
            if stripped in tgt_stripped:
                add_line(line, stripped)
            else:
                src_only.append((line, stripped))

        for line, stripped in src_only:
            add_line(line, stripped)

        for i, line in enumerate(tgt_lines):
            stripped = tgt_stripped[i]
            if stripped not in seen:
                add_line(line, stripped)

        merged = "".join(merged_lines)

        if not merged.endswith("\n"):
            if src_content.endswith("\n") or tgt_content.endswith("\n"):
                merged += "\n"

        log(f"    Conflict {resolved_count+1}: merged {len(src_lines)} + {len(tgt_lines)} -> {len(merged_lines)} lines")
        resolved_count += 1
        return merged

    new_content = pattern.sub(replace_conflict, content)
    return new_content, resolved_count

def resolve_file(file_path):
    log(f"\n--- Resolving conflicts in: {file_path} ---")
    content = read_file(file_path)
    if content is None:
        return False

    has_marker = ("<<<<<<<" in content and "=======" in content and ">>>>>>>" in content)
    if not has_marker:
        log(f"  No conflict markers found (already clean? skipped)")
        return True

    new_content, resolved = resolve_conflict_in_content(content, file_path)

    if "<<<<<<<" in new_content or "=======" in new_content or ">>>>>>>" in new_content:
        log(f"  WARNING: Conflict markers still exist after resolution!")
        pattern2 = re.compile(r'<<<<<<<|=======|>>>>>>>')
        remaining = len(pattern2.findall(new_content))
        log(f"  Remaining marker count: {remaining}")

    if write_file(file_path, new_content):
        log(f"  Written resolved content. {resolved} block(s) handled.")
        return True
    return False

def git_add(path):
    code, out, err = run_git(["add", "--", path])
    return code == 0

def git_commit(msg):
    code, out, err = run_git(["commit", "-m", msg])
    return code == 0

def main():
    log("=" * 60)
    log("AUTO MERGE + CONFLICT RESOLUTION SCRIPT (_git_do_merge.py)")
    log("=" * 60)

    log("\n========== STEP 0: PRE-MERGE STATUS ==========")
    run_git(["status"])
    run_git(["branch", "--show-current"])

    in_merge_state = is_in_merge()
    log(f"\nCurrently in merge state? {in_merge_state}")

    if not in_merge_state:
        log("\n========== STEP 1: EXECUTE git merge master ==========")
        code, out, err = run_git(["merge", "master", "--no-edit"])
        if code == 0:
            log("\n>>> MERGE SUCCEEDED CLEANLY - NO CONFLICTS!")
        else:
            log("\n>>> MERGE RESULTED IN CONFLICTS (will resolve below)")
    else:
        log("\n>>> Already in merge state (resolving existing conflicts)")

    log("\n========== STEP 2: STATUS AFTER MERGE ATTEMPT ==========")
    run_git(["status"])

    log("\n========== STEP 3: DETECT CONFLICT FILES ==========")
    conflict_files = get_conflict_files()
    log(f"Conflict files ({len(conflict_files)}):")
    for f in conflict_files:
        log(f"  - {f}")

    if conflict_files:
        log("\n========== STEP 4: RESOLVE CONFLICTS ==========")
        all_ok = True
        for f in conflict_files:
            ok = resolve_file(f)
            if not ok:
                all_ok = False
                log(f"  FAILED to resolve: {f}")
            else:
                git_add(f)
                log(f"  Staged: {f}")

        log("\n========== STEP 5: CHECK RESOLUTION + FINAL STATUS ==========")
        run_git(["status"])

        remaining_conflicts = get_conflict_files()
        if remaining_conflicts:
            log(f"\n>>> WARNING: {len(remaining_conflicts)} conflict(s) still unmerged!")
            for f in remaining_conflicts:
                log(f"  - {f}")
                content = read_file(f)
                if content and ("<<<<<<<" in content):
                    log("    Re-trying with simpler resolution (keeping HEAD + master additions)...")
                    new_content = re.sub(
                        r'<<<<<<<[^\n]*\n(.*?)=======\n(.*?)>>>>>>>[^\n]*\n',
                        lambda m: m.group(1) + m.group(2),
                        content,
                        flags=re.DOTALL
                    )
                    if write_file(f, new_content):
                        git_add(f)
                        log(f"    Simple merge applied and staged")
        else:
            log("\n>>> All conflicts appear resolved")

        log("\n========== STEP 6: VERIFY NO MARKERS REMAIN IN STAGED FILES ==========")
        code, diff_out, _ = run_git(["diff", "--cached", "--name-only"])
        staged = [l.strip() for l in diff_out.strip().splitlines() if l.strip()]
        for f in staged:
            content = read_file(f)
            if content and ("<<<<<<<" in content or ">>>>>>>" in content):
                log(f"  WARNING: {f} still has conflict markers in worktree!")
            else:
                log(f"  OK: {f}")

        still_unmerged = get_conflict_files()
        if not still_unmerged:
            log("\n========== STEP 7: COMMITTING MERGE ==========")
            commit_ok = git_commit("Merge master into feat-wechat-miniapp-ble-esp32-s85KC5: resolve conflicts")
            if commit_ok:
                log(">>> MERGE COMMIT CREATED SUCCESSFULLY!")
            else:
                log(">>> Commit failed (maybe nothing to commit? check status)")
                run_git(["status"])
        else:
            log(f"\n>>> Cannot commit: {len(still_unmerged)} file(s) still unmerged")
    else:
        log("\n========== STEP 4: NO CONFLICTS - FINISHING ==========")
        if is_in_merge():
            log(">>> In merge state but no unmerged paths detected - committing...")
            git_commit("Merge master into feat-wechat-miniapp-ble-esp32-s85KC5")
        else:
            log(">>> Merge already complete or no changes needed")

    log("\n========== FINAL STATUS ==========")
    run_git(["status"])
    run_git(["log", "-1", "--oneline"])

    log("\n" + "=" * 60)
    log("DONE")
    log("=" * 60)

    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        f.write("\n".join(results))

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        log(f"FATAL ERROR in main(): {e}")
        import traceback
        log(traceback.format_exc())
        with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
            f.write("\n".join(results))
        sys.exit(1)
