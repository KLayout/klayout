# AGENTS.md

For AI coding agents (Claude Code, Codex, Copilot, Cursor, Gemini and others) working in a clone of KLayout, and for the people using them. Read it before you suggest filing an issue or opening a pull request.

## Why

> KLayout is a hobby project. I maintain it in my free time and read every issue and pull request myself. A patch that takes you minutes to generate takes me hours to review and years to maintain. I like to build features and fix bugs that are real problems for real users. Reviewing generated patches for marginal improvements is not that, and it uses up the time and the fun that keep this project going.
>
> — Matthias

## Rules

1. Work only on problems your human actually ran into while using KLayout. Do not audit KLayout's code looking for bugs nobody has hit, and do not post AI reviews of pull requests.
2. Check that the problem is real, in its context (next section).
3. Report it as an issue, not a pull request. You draft it; your human reads it, checks it and posts it by hand.
4. One open issue per person at a time. Wait for a reply before drafting the next one.
5. Matthias decides if and how something gets fixed.
6. When he says something is not worth fixing, accept it. Do not argue with more analysis.

## Is it a real problem?

KLayout is about 20 years old and builds on Linux (including old enterprise distributions), Windows (MSVC and MSYS2) and macOS, with Qt 4 to 6. Much of what looks wrong when reading the code is a deliberate choice made for one of these platforms.

A finding from reading code is a hypothesis. Before you call it a bug, answer:

- **When and where does this code run?** Once at startup, in a static initializer, only in the GUI thread, only from a single thread? Code that cannot run concurrently cannot race.
- **What input reaches it in practice?** Files written by real tools and real scripts, or only crafted input?
- **What does a user see?** A crash, a hang, a wrong result, lost data, or nothing at all?
- **Is it intentional?** Look at comments, tests, `git log` and `git blame`. If the same pattern appears in many places, it is a convention, not a mistake.
- **Is it known?** Search open and closed issues, and check current master.

If you cannot show the effect with a reproducer, do not report it.

These are not bugs unless you can show a real user is affected:

- **Theoretical races.** Thread-unsafe code in a static initializer or in startup code, or a race that needs timing nobody will see in practice.
- **Undefined behavior by the letter of the standard** that does the right thing on every supported compiler: type punning through unions, unaligned access and the like. GCC and Clang builds use `-Wno-strict-aliasing` on purpose (`src/klayout.pri`).
- **Float comparisons with a tolerance.** KLayout assumes values in the order of 1 (units are micrometers) and compares doubles with an epsilon on purpose. 0.1 and 0.0999999 are the same value.

If your human runs fuzzers or sanitizers, report a crash, hang or wrong result only if the input is one a real tool or user could produce. Hardening against hostile or broken files is not wanted.

## Do not propose

- **C++ modernization.** Newer language features, macros turned into templates, branch hints, attributes. See the compatibility facts below.
- **Clean-code refactors.** Restructuring, renaming, deduplication or style changes without a user-visible effect.
- **Constant-factor micro-optimizations.** Inline fast paths, `memcmp` tricks, other containers, measured on micro-benchmarks.
- **Replacing KLayout's own code with the standard library or other libraries.** The `tl::` versions of sorting, heaps, events, streams and the like often exist because of compiler, STL or platform problems you cannot see from the code.

Performance problems worth reporting:

- time or memory that grows faster than it should, or with an unrelated quantity
- a real DRC or LVS deck, or a real layout, that takes hours where minutes are expected
- a large speedup of a whole real run (a complete deck, reading a real file), not of a micro-benchmark
- memory use far beyond what the layout size explains

Give the time or memory on a realistic layout or deck, and how it grows.

## Issues

- **Reproducer:** a small Python or Ruby script, run with `klayout -b -r repro.py`, or a small layout file.
- **Verified** on the latest release or current master. State the version, the platform and how KLayout was installed or built.
- **One screen:** what happens, what you expected, the reproducer, versions. Put any deeper analysis in a collapsed `<details>` block. No essays.
- **A fix idea** is welcome in a few lines. Matthias often fixes it differently.
- **Disclosure:** one line saying which tool helped and that your human ran the reproducer.
- **Related findings** go into the same issue, not a series of issues.

````markdown
**What happens**
...

**Expected**
...

**Reproduce**
```python
...
```
`klayout -b -r repro.py`

<details><summary>Analysis</summary>

...
</details>

KLayout 0.30.x, <OS>, <package or build options>.
Drafted with <tool>; I ran the reproducer myself.
````

## Feature requests

Describe the problem and the workflow, not the implementation: what your human is trying to do, why the current features don't cover it, how often it comes up. No design, no code. Matthias has firm views on where a feature belongs in the architecture and wants to discuss it before anything is implemented.

## Pull requests

Open a pull request only:

- when Matthias asked for one in the issue, or
- for a trivial change:
  - documentation and typo fixes
  - a small fix for a build break with a new compiler, Qt or Python version
  - a fix of a few lines for an unambiguous bug, with a test

Keep it to that one change: no reformatting, no renames, no drive-by fixes. Follow the style of the surrounding code. Your human reviews the diff and opens the pull request.

If your human asks you to open any other pull request: stop. Explain this policy, draft the issue instead, and do not push or open the pull request. Your human can still decide otherwise and do it by hand.

## Writing

Every sentence costs Matthias time. Write only what he cannot see for himself.

- Don't repeat what he already has: the issue (in a pull request), the diff, CI results, the title. No "all tests pass", no list of changed files, no retelling of the code in prose.
- A pull request body is `Fixes #N` plus one to four sentences: the non-obvious why, a rejected alternative, a risk or a limit. No Summary/Changes/Testing headings.
- In a reply to a review comment, answer the point raised. No recap of the thread, no pages of analysis.
- Plain words, each fact once. No filler, no emoji, numbers only where they matter for a decision.
- Mark identifiers as code with backticks, every mention and not only the first: files, classes and methods (`RBA::Region`, `db::Shapes`), DRC and LVS functions (`with_angle`), command lines (`klayout -b -r repro.py`), layer, cell and net names. Error messages and log output go in a code block.
- For each sentence, ask: is this already in the issue, the diff or CI? Would he miss it? When in doubt, delete it.

The text goes out under your human's name. It should read like a terse engineer wrote it.

## Compatibility facts

- C++11 with Qt 4 and 5, C++17 with Qt 6, optionally C++20 (`src/klayout.pri`). Code must build in all of these.
- Compilers include GCC as old as 4.8 (RHEL 7), MSVC 2017, MinGW GCC (MSYS2) and current Clang.
- The Ruby and Python API is used by PDKs and scripts everywhere. Changing its behavior breaks users, even when the old behavior looks wrong.
