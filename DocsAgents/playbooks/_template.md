---
type: Reference
title: Playbook template
description: Copy this file to start a new playbook; keep the section structure.
tags: [meta, template]
---

<!--
Copy to DocsAgents/playbooks/<verb-noun>.md and change the frontmatter to:

  type: Playbook
  title: <imperative, e.g. "Add a new .ssc simfile tag">
  description: <one sentence>
  tags: [...]

Keep it short. A playbook is a checklist plus the traps, not an essay.
Delete this comment.
-->

# Goal

One sentence: what this procedure accomplishes.

# When to use

The trigger. When *not* to use it / what it does not cover.

# Files always touched

A table: path → what changes there. This is the part that saves the most
time — the next agent should not have to grep for it.

# Steps

1. Numbered, concrete, in order. Reference exact symbols / macros / line
   anchors where stable.
2. …

# Gotchas

Bulleted. The things that caused a correction loop last time. Cache
version bumps, CI validators, ordering constraints, platform specifics.

# Verification

- What must be green before pushing (build target, smoke test, specific
  unit test).
- What the maintainer should spot-check in async review (per `AGENTS.md`
  §4 — commit + push, don't wait). Note if it's higher-risk (§4) so the
  commit gets split finely.

# History

- `YYYY-MM-DD` — created / what changed and why.
