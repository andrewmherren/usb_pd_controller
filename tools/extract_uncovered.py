#!/usr/bin/env python3
"""
extract_uncovered.py
Parse gcovr/cobertura XML (coverage.xml) and extract uncovered
line ranges with small context.

Writes a JSON Lines file (one snippet per line) suitable for
feeding to an AI assistant.

Usage:
    python3 tools/extract_uncovered.py --coverage coverage.xml \
        --root . --out uncovered-snippets.jsonl --context 4
"""
import argparse
import itertools
import json
import os
import xml.etree.ElementTree as ET


def parse_cov(xml_path):
    tree = ET.parse(xml_path)
    root = tree.getroot()
    results = {}
    for cls in root.findall(".//class"):
        filename = cls.get("filename")
        if not filename:
            continue
        for line in cls.findall("./lines/line"):
            num = line.get("number")
            hits_str = line.get("hits", "0")
            try:
                ln = int(num) if num is not None else 0
            except Exception:
                continue
            try:
                hits = int(hits_str)
            except Exception:
                hits = 0

            # include condition coverage consideration
            cond_cov = line.get("condition-coverage", "")
            if cond_cov:
                # Example formats: "0% (0/2)" or "50% (1/2)"
                percent = cond_cov.split("%")[0].strip()
                try:
                    pct_val = int(percent)
                    if pct_val == 0:
                        hits = 0
                except ValueError:
                    pass

            results.setdefault(filename, {})[ln] = hits
    return results


def contiguous_ranges(line_numbers):
    for k, g in itertools.groupby(
        enumerate(sorted(line_numbers)), lambda ix: ix[0] - ix[1]
    ):
        group = list(g)
        yield group[0][1], group[-1][1]


def read_file_lines(root, filename):
    path = os.path.join(root, filename)
    if not os.path.exists(path):
        return None
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.read().splitlines()


def make_snippets(root, covmap, context=4, max_block=50):
    # Build raw snippet ranges per file, splitting large blocks
    raw = []
    for filename, lineno_map in covmap.items():
        uncovered = [ln for ln, hits in lineno_map.items() if hits == 0]
        if not uncovered:
            continue
        for start, end in contiguous_ranges(uncovered):
            size = end - start + 1
            if size <= max_block:
                raw.append((filename, start, end))
                continue
            cur = start
            while cur <= end:
                sub_end = min(end, cur + max_block - 1)
                raw.append((filename, cur, sub_end))
                cur = sub_end + 1

    outputs = []

    def build_obj(filename, start, end, lines):
        before_i = max(1, start - context)
        after_i = min(len(lines), end + context)
        snippet_text = "\n".join(lines[start - 1 : end])
        ctx_before = "\n".join(lines[before_i - 1 : start - 1])
        ctx_after = "\n".join(lines[end:after_i])

        # try to discover a nearby function signature
        sig = ""
        search_up = 10
        top = max(0, start - 1 - search_up)
        for i in range(top, start - 1):
            line = lines[i].strip()
            if "(" in line and (
                line.endswith("{") or line.endswith(";") or ")" in line
            ):
                sig = line
                break

        base = os.path.basename(filename).replace(".cpp", "")
        return {
            "repo_root": os.path.abspath(root),
            "file_path": filename,
            "start_line": start,
            "end_line": end,
            "snippet": snippet_text,
            "context_before": ctx_before,
            "context_after": ctx_after,
            "function_signature": sig,
            "headers": [],
            "test_framework": "Unity",
            "compile_flags": "",
            "required_filename": (
                "test/test_generated_" + base + f"_{start}_{end}.cpp"
            ),
            "only_return": "file",
            "max_tokens": 1500,
        }

    for filename, start, end in raw:
        lines = read_file_lines(root, filename)
        if lines is None:
            print(f"Warning: file not found {filename}")
            continue
        outputs.append(build_obj(filename, start, end, lines))

    return outputs


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--coverage", required=True)
    p.add_argument("--root", default=".")
    p.add_argument("--out", default="uncovered-snippets.jsonl")
    p.add_argument("--context", type=int, default=4)
    p.add_argument("--max-block", type=int, default=50)
    args = p.parse_args()

    cov = parse_cov(args.coverage)
    snippets = make_snippets(
        args.root, cov, context=args.context, max_block=args.max_block
    )
    with open(args.out, "w", encoding="utf-8") as f:
        for s in snippets:
            f.write(json.dumps(s, ensure_ascii=False) + "\n")
    print(f"Wrote {len(snippets)} snippet(s) to {args.out}")


if __name__ == "__main__":
    main()
