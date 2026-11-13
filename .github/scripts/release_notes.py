#!/usr/bin/env python3
"""Builds the GitHub release body: Groq summary + PR list + commit list.

Never fails the release - every step degrades to something still useful.
Env: GITHUB_REPOSITORY, GITHUB_TOKEN, RELEASE_SHA, RELEASE_TAG, VERSION_NUM,
BUILD_NUMBER, RELEASE_NOTES_FILE, GROQ_API_KEY (optional), GROQ_MODEL (optional).
"""
import json
import os
import re
import urllib.error
import urllib.request

REPO = os.environ.get("GITHUB_REPOSITORY", "")
TOKEN = os.environ.get("GITHUB_TOKEN", "")
SHA = os.environ.get("RELEASE_SHA", "")
TAG = os.environ.get("RELEASE_TAG", "")
OUT = os.environ.get("RELEASE_NOTES_FILE", "release_notes.md")
GROQ_KEY = os.environ.get("GROQ_API_KEY", "")
GROQ_MODEL = os.environ.get("GROQ_MODEL", "openai/gpt-oss-120b")
GROQ_URL = os.environ.get("GROQ_URL", "https://api.groq.com/openai/v1/chat/completions")

# merge commits here are plumbing ("Merge pull request #194 from gopro2027/dev"),
# so the real changelog is the non-merge commits plus the feature PR titles
PR_RE = re.compile(r"Merge pull request #(\d+)|\(#(\d+)\)")
NOISE_RE = re.compile(r"^(merge branch|merge remote|revert \"revert)", re.I)
PLUMBING_BRANCHES = {"dev", "main", "master"}
# a commit touching only these ships nothing to a device, so it stays out of
# the notes. Anything we cannot classify is kept.
NON_FIRMWARE_PREFIXES = (".github/", ".claude/", "docs/", "tutorial/", "PCB/",
                         "3d Prints/", "builds/")
MAX_PATH_CHECKS = 60  # one API call each, so give up on very large releases

SYSTEM_PROMPT = (
    "You write release notes for OAS-Man, an open-source DIY ESP32 air suspension "
    "controller (manifold firmware, wireless touchscreen controller firmware, and an "
    "Android app). Given the merged pull requests and commit subjects of a release, "
    "write 3 to 6 markdown bullets summarising what changed for the people installing "
    "this firmware. Group related commits into one bullet. Lead with anything affecting "
    "safety, valve or pressure behaviour, or OTA updating. Plain language, no marketing, "
    "one line per bullet, no headings, no preamble. Only describe what the input states - "
    "never invent a change. Leave out commit subjects you cannot interpret."
)


def note(msg):
    print("release_notes: " + str(msg), flush=True)


def http_json(url, headers, data=None, timeout=60):
    body = json.dumps(data).encode() if data is not None else None
    req = urllib.request.Request(url, data=body, headers=headers,
                                 method="POST" if body else "GET")
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8", "replace"))


def github(path):
    headers = {"Accept": "application/vnd.github+json",
               "User-Agent": "oasman-release-notes"}
    if TOKEN:
        headers["Authorization"] = "Bearer " + TOKEN
    try:
        return http_json("https://api.github.com/repos/" + REPO + path, headers)
    except Exception as err:  # noqa: BLE001 - notes must never break the release
        note("GET " + path + " failed: " + repr(err))
        return None


def previous_tag():
    latest = github("/releases/latest")
    if isinstance(latest, dict) and latest.get("tag_name"):
        return latest["tag_name"]
    note("no previous release found")
    return ""


def commit_list(prev):
    if prev:
        commits, total = [], 0
        for page in range(1, 6):  # compare caps per_page at 100
            cmp_ = github("/compare/" + prev + "..." + SHA +
                          "?per_page=100&page=" + str(page))
            if not isinstance(cmp_, dict) or "commits" not in cmp_:
                break
            total = cmp_.get("total_commits", len(cmp_["commits"]))
            commits += cmp_["commits"]
            if not cmp_["commits"] or len(commits) >= total:
                return commits, total
        if commits:
            return commits, total
        note("compare " + prev + "..." + SHA + " failed, using the recent commits instead")
    recent = github("/commits?sha=" + SHA + "&per_page=30")
    if isinstance(recent, list):
        return recent, len(recent)
    return [], 0


def touches_firmware(path):
    """True unless every file the change touches is non-firmware."""
    data = github(path)
    files = data.get("files") if isinstance(data, dict) else data
    if not isinstance(files, list) or not files:
        return True  # cannot tell, so keep it
    return any(not (f.get("filename") or "").startswith(NON_FIRMWARE_PREFIXES)
               for f in files)


def subjects(commits):
    entries = []
    for c in commits:
        if len(c.get("parents", [])) > 1:
            continue  # merge commit
        subject = (c.get("commit", {}).get("message") or "").split("\n")[0].strip()
        if subject and not NOISE_RE.match(subject):
            entries.append((c.get("sha", ""), subject))

    if len(entries) > MAX_PATH_CHECKS:
        note("too many commits to check paths, keeping all " + str(len(entries)))
        return [s for _, s in entries]

    kept = []
    for sha, subject in entries:
        if not sha or touches_firmware("/commits/" + sha):
            kept.append(subject)
        else:
            note("skipping non-firmware commit: " + subject)
    return kept


def pull_requests(commits):
    numbers = []
    for c in commits:
        for m in PR_RE.finditer(c.get("commit", {}).get("message") or ""):
            n = m.group(1) or m.group(2)
            if n not in numbers:
                numbers.append(n)
    prs = []
    for n in numbers:
        pr = github("/pulls/" + n)
        if not isinstance(pr, dict) or not pr.get("title"):
            continue
        if (pr.get("head", {}).get("ref") or "") in PLUMBING_BRANCHES:
            continue  # the dev -> main merge, its content is in the commits above
        if not touches_firmware("/pulls/" + n + "/files?per_page=100"):
            note("skipping non-firmware PR #" + n)
            continue
        prs.append((n, pr["title"].strip()))
    return prs


def groq_summary(prs, changes):
    if not GROQ_KEY:
        note("GROQ_API_KEY not set, skipping the AI summary")
        return ""
    if not prs and not changes:
        return ""
    user = "Pull requests:\n" + ("\n".join("- " + t + " (#" + n + ")" for n, t in prs) or "none") \
        + "\n\nCommits:\n" + ("\n".join("- " + s for s in changes) or "none")
    payload = {"model": GROQ_MODEL, "temperature": 0.3, "max_tokens": 800,
               "messages": [{"role": "system", "content": SYSTEM_PROMPT},
                            {"role": "user", "content": user}]}
    try:
        resp = http_json(GROQ_URL, {"Authorization": "Bearer " + GROQ_KEY,
                                    "Content-Type": "application/json",
                                    "User-Agent": "oasman-release-notes"},
                         payload, timeout=90)
        text = (resp["choices"][0]["message"].get("content") or "").strip()
    except urllib.error.HTTPError as err:
        note("groq request failed: HTTP " + str(err.code) + " " +
             err.read().decode("utf-8", "replace")[:300])
        return ""
    except Exception as err:  # noqa: BLE001
        note("groq request failed: " + repr(err))
        return ""
    if not text:
        note("groq returned no summary text")
    return text


def main():
    prev = previous_tag()
    commits, total = commit_list(prev)
    changes = subjects(commits)
    prs = pull_requests(commits)
    summary = groq_summary(prs, changes)

    if prev:
        changelog_url = ("https://github.com/" + REPO + "/compare/" + prev + "..." +
                         (TAG or SHA))
    else:
        changelog_url = "https://github.com/" + REPO + "/commits/" + (TAG or SHA)

    parts = []
    if summary:
        parts.append("## What's new\n\n" + summary + "\n\n" +
                     "_This summary is AI-generated and may contain mistakes. See the " +
                     "[full changelog on GitHub](" + changelog_url + ") for every change._")
    if prs:
        parts.append("## Pull requests\n\n" +
                     "\n".join("- " + t + " (#" + n + ")" for n, t in prs))
    if changes:
        parts.append("<details><summary>All commits (" + str(len(changes)) + ")</summary>\n\n" +
                     "\n".join("- " + s for s in changes) + "\n\n</details>")
    if total > len(commits):
        parts.append("_Only the first " + str(len(commits)) + " of " + str(total) +
                     " commits are listed._")

    footer = ["Release ID: " + os.environ.get("VERSION_NUM", "unknown"),
              "Build number: " + os.environ.get("BUILD_NUMBER", "unknown"),
              "Built from commit " + SHA]
    if prev:
        footer.append("Full changelog: " + changelog_url)
    parts.append("---\n\n" + "\n".join(footer))

    body = "\n\n".join(parts) + "\n"
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(body)
    note("wrote " + OUT)
    print(body, flush=True)


if __name__ == "__main__":
    try:
        main()
    except Exception as err:  # noqa: BLE001 - a bad body must never block a release
        note("failed, writing a minimal body instead: " + repr(err))
        with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
            fh.write("Automated build from commit " + SHA + "\n\n"
                     "Release ID: " + os.environ.get("VERSION_NUM", "unknown") + "\n"
                     "Build number: " + os.environ.get("BUILD_NUMBER", "unknown") + "\n")
