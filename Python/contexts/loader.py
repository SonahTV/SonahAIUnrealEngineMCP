"""Keyword-routed UE5 cheatsheet loader.

Maps a free-text topic query to one or more curated markdown cheatsheets
shipped alongside this module. No live scraping — all content is local.
"""

from __future__ import annotations

import os
import re
from typing import Dict, List, Tuple

_CONTEXT_DIR = os.path.dirname(os.path.abspath(__file__))

# Topic -> filename + list of regex patterns that route to it.
# Order matters: first match wins on ambiguous queries.
_ROUTES: List[Tuple[str, str, List[str]]] = [
    (
        "animation",
        "animation.md",
        [
            r"\banim(ation)?\b",
            r"\bstate\s*machine\b",
            r"\btransition\b",
            r"\bblend\s*space\b",
            r"\banim\s*bp\b",
            r"\banim\s*graph\b",
            r"\bmontage\b",
            r"\bnotify\b",
            r"\bskeletal\s*mesh\b",
        ],
    ),
    (
        "blueprint",
        "blueprint.md",
        [
            r"\bblueprint\b",
            r"\bbp\b",
            r"\bk2node\b",
            r"\buedgraph\b",
            r"\bevent\s*graph\b",
            r"\bconstruction\s*script\b",
        ],
    ),
    (
        "replication",
        "replication.md",
        [
            r"\breplica(tion|te)\b",
            r"\bmultiplayer\b",
            r"\bnetwork\b",
            r"\brpc\b",
            r"\bserver\b",
            r"\bclient\b",
            r"\bdedicated\s*server\b",
            r"\brepnotify\b",
        ],
    ),
    (
        "actor",
        "actor.md",
        [
            r"\bactor\b",
            r"\bpawn\b",
            r"\bcharacter\b",
            r"\bcomponent\b",
            r"\btick\b",
            r"\bspawn\b",
            r"\bbeginplay\b",
        ],
    ),
    (
        "assets",
        "assets.md",
        [
            r"\basset\b",
            r"\bcontent\s*browser\b",
            r"\bpackage\b",
            r"\bimport\b",
            r"\bumg\b",
            r"\bwidget\b",
            r"\bmaterial\b",
        ],
    ),
    (
        "slate",
        "slate.md",
        [
            r"\bslate\b",
            r"\bsompound\s*widget\b",
            r"\bsbutton\b",
            r"\bstextblock\b",
            r"\beditor\s*ui\b",
            r"\btoolmenu\b",
        ],
    ),
]


def list_topics() -> List[str]:
    return [topic for topic, _, _ in _ROUTES]


def _read(filename: str) -> str:
    path = os.path.join(_CONTEXT_DIR, filename)
    if not os.path.exists(path):
        return f"# {filename} not found at {path}"
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def get_context(query: str) -> Dict[str, object]:
    """Return the most relevant cheatsheet(s) for a topic query.

    If the query matches multiple topics, returns all matches concatenated.
    If nothing matches, returns the topic list so the caller can pick.
    """
    if not query or not query.strip():
        return {
            "topics": list_topics(),
            "message": "Provide a topic query. Available topics listed above.",
        }

    q = query.lower()
    matched: List[str] = []
    for topic, filename, patterns in _ROUTES:
        for pat in patterns:
            if re.search(pat, q, re.IGNORECASE):
                matched.append(topic)
                break

    if not matched:
        return {
            "query": query,
            "matched": [],
            "topics": list_topics(),
            "message": "No cheatsheet matched. Try one of the topics above as a substring.",
        }

    sections = []
    for topic in matched:
        filename = next(f for t, f, _ in _ROUTES if t == topic)
        sections.append(f"## Topic: {topic}\n\n{_read(filename)}")

    return {
        "query": query,
        "matched": matched,
        "content": "\n\n---\n\n".join(sections),
    }
