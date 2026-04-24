# ChatBot Shark — project context

This file holds durable context for the repository and the key decisions that shape it. It is updated whenever a structural, refactoring, documentation, build, test or publication decision is made.

Created: 2026-04-23.

## Initial setup

The repository hosts three versions of the educational ChatBot Shark project that together trace its evolution:

1. `Shark_2_0_CLI` — CLI version, all data kept in memory via classes and smart pointers.
2. `Shark_3_0_Postgres` — CLI version with a PostgreSQL-backed server.
3. `Shark_3_0_Qt` — desktop version on Qt + PostgreSQL.

Each version already ships its own `README.md`.

## Global goal

Publish the whole set as a GitHub portfolio project aimed at an international engineering audience (recruiters, hiring managers, reviewers).

Expected outcomes:

1. Refactor each subproject where needed.
2. Align formatting with modern C++ / CMake / Qt best practices.
3. Improve documentation at repo-root level and per subproject.
4. Add the engineering artefacts that raise the bar for a portfolio repo: build and run instructions, architectural notes, diagrams, configuration examples, tests, CI, a license, a `.gitignore`, and where useful demo screenshots and a roadmap.
5. Preserve the educational value — the evolution from in-memory CLI to PostgreSQL and then to Qt desktop is part of the point.

## Current layout

```text
.
├── .gitignore
├── LICENSE
├── PROJECT_CONTEXT.md
├── README.md
├── Shark_2_0_CLI/
│   └── README.md
├── Shark_3_0_Postgres/
│   ├── README.md
│   └── connect_db.conf.example
└── Shark_3_0_Qt/
    ├── README.md
    └── connect_db.conf.example
```

## Working principles

1. Do not mix unrelated refactoring with documentation changes if that would hurt review.
2. Keep the evolution of the project visible as an asset; do not try to fold the three versions into one faceless codebase.
3. Before a large refactor, record the goal and boundaries in this file first.
4. After any architectural or organizational decision, add a journal entry here.
5. For a public GitHub repository, avoid secrets, real passwords, private endpoints and personal configuration data.
6. **Language policy.** All public-facing artefacts in this repo are written in English: root and per-version READMEs, code comments, commit messages, user-visible UI strings (CLI prompts, Qt labels, log lines, validation messages) and this context file. No `.ru.md` duplicates. Historical Russian commit messages are left as-is — no history rewrite just for language.

## Execution rules

1. Any change is made only after explicit owner approval.
2. Every change requires a plan with the exact list of files touched.
3. Only the approved plan is executed.
4. Silent or out-of-plan changes are forbidden.
5. Opportunistic refactoring during execution is forbidden unless explicitly in the plan.
6. Deep refactors require their own separate approval.
7. Structure, architecture, libraries, classes, methods and patterns cannot be changed without a separate approval.

## Pre-sketched work directions

1. Repository root:
   - rewrite the root `README.md` as a project showcase;
   - add navigation to each version;
   - describe the architectural evolution;
   - add requirements, quick start, platform status, screenshots/diagrams;
   - verify `.gitignore`, license and the absence of secrets.

2. `Shark_2_0_CLI`:
   - align C++ code and CMake to one style;
   - improve the CLI README structure;
   - surface the learning highlights: classes, pointers / smart pointers, in-memory model, TCP client-server.

3. `Shark_3_0_Postgres`:
   - review the PostgreSQL access layer;
   - separate a config example from real connection data;
   - document the schema, initialization and startup.

4. `Shark_3_0_Qt`:
   - review the Qt client, server and DTO layout;
   - improve the macOS/Linux build documentation;
   - curate screenshots and user-scenario descriptions.

## Decision journal

### 2026-04-23 (iteration 1)

- Created `PROJECT_CONTEXT.md` at the repository root as the durable recovery point for context.
- Captured the initial setup: the repo holds three versions of ChatBot Shark, each demonstrating the next stage of project evolution.
- Decided to treat the repository as an educational portfolio piece for employers, with the architectural evolution being part of its value.
- Adopted the rule: any change goes through explicit owner approval and only within an approved plan.
- Adopted the constraint: deep refactoring, opportunistic refactoring and changes to structure, architecture, libraries, classes, methods and patterns are forbidden without a dedicated approval.

### 2026-04-23 (iteration 2) — Phase 0 done

Phase 0 prepared the public repo for further work: purge the secret from the working tree and from history, align directory names, set up basic infrastructure. No application-code changes.

Decisions made:

- The AWS RDS instance `shark-database.czmyiskayc7p.eu-north-1.rds.amazonaws.com` was deleted by the owner. The password in old commits no longer represents an active threat, but it was still scrubbed from history for the sake of a clean publication.
- `main` history was rewritten via `git filter-repo`, followed by `git push --force-with-lease origin main`. A backup branch `backup/pre-phase0` was cut from the pre-Phase-0 state and pushed to `origin` as an emergency anchor. The owner removes it manually once confident everything is in order.
- `dist/`, `build_macos/`, `build_linux_static/`, Qt IDE caches (`CMakeLists.txt.user`, `build.ninja`, `compile_commands.json`, `Testing/`) and `.DS_Store` removed from both the working tree and the entire history.
- Added a root `.gitignore` covering build directories, IDE caches, OS clutter, secrets (`connect_db.conf` with an `*.example` exception) and compiled binaries.
- Added a root `LICENSE` (MIT, 2025-2026, Yan Batytskiy).
- Subproject directories renamed for a single style:
  - `Shark2_0_Cli/` → `Shark_2_0_CLI/`,
  - `shark3_0_Postgres/` → `Shark_3_0_Postgres/`,
  - `Shark_3_0_Qt/` → `Shark_3_0_Qt/`.
  Performed with `git mv`, per-file history preserved.
- Real `connect_db.conf` files were replaced by `connect_db.conf.example` templates in `Shark_3_0_Postgres/` and `Shark_3_0_Qt/`. The affected READMEs got a short note about copying the template.
- Removed the `add_custom_command` from `Shark_3_0_Qt/CMakeLists.txt` that used to copy `connect_db.conf` next to the `server` binary. The user now places the config by hand per the README instructions.
- As a separate commit before Phase 0, the accumulated patches in `Shark_2_0_CLI/src/` were landed: guard against an empty `addressInternet`, removal of a default DDNS endpoint, dropped redundant `const` on return-by-value, initialization of `bool result = false`, newline at EOF.
- A local build of `Shark_2_0_CLI` via `cmake --build build_macos --parallel` succeeded after all the changes (core + client + server).

Leak verification:

- `git grep -n KWeJkI9549` in the current tree — empty.
- `git log main -S 'KWeJkI9549' --oneline` — empty.
- `git ls-files` contains no paths under `dist/`, `build_*/`, `.DS_Store`, or `connect_db.conf` (only `connect_db.conf.example` remains).

### 2026-04-23 (iteration 3) — Phase 1 done

Phase 1 turned the root `README.md` from a stub into a showcase. Subproject documentation was not touched.

Decisions made:

- README language — Russian for this iteration (superseded in iteration 4, see below).
- Contacts — email only: `YanBatytskiy@gmail.com`. No LinkedIn/TG.
- Badges — minimal: C++20/23, CMake ≥ 3.16, MIT. No CI badges yet.
- The architectural evolution is rendered via a mermaid block (GitHub renders it natively).
- Screenshots — three PNGs from `Shark_3_0_Qt/Screens/`: `1_login_form_MacOS.png`, `5_chat_list.png`, `7_user_profile.png`.
- Class diagrams are not duplicated at root — just a link to `Shark_2_0_CLI/Classes.png`.
- Quick start — one `<details>` block per version, no duplication of per-version READMEs.

Changed files:

- `README.md` — rewritten from a one-liner into ~130 lines of showcase.
- `PROJECT_CONTEXT.md` — this journal entry.

### 2026-04-23 (iteration 4) — English-only policy

After shipping the Russian root README, the owner flagged that the target audience is international: the repo must be English-only. The previous recommendation to default to RU was a mistake.

Decisions made:

- Locked-in language policy: English everywhere public (documentation, commits, code comments, UI strings). See the `Working principles` section above.
- This context file is also rewritten in English — it is no longer treated as internal-only.
- No `.ru.md` shadow files will be created.
- Historical Russian commit messages (everything up to and including the Phase 1 commit) are left intact — we do not rewrite history just to translate commit subjects.
- UI strings translation extends to the application itself: CLI prompts in Shark_2_0_CLI / Shark_3_0_Postgres, Qt labels and messages in Shark_3_0_Qt.

Scope already executed (and committed) in this iteration:

- `README.md` re-translated to English (commit `docs(root): translate README to English`).

Scope still in progress (tracked via the session TODO list, committed incrementally):

- `PROJECT_CONTEXT.md` — this rewrite.
- Per-version READMEs: `Shark_2_0_CLI/README.md`, `Shark_3_0_Postgres/README.md`, `Shark_3_0_Qt/README.md`.
- C++ source comments across the three subprojects (~127 files with Cyrillic text, ~2.1k lines).
- CLI `std::cout` prompts in Shark_2_0_CLI and Shark_3_0_Postgres.
- Qt UI strings — 10 `.ui` XML files plus programmatic strings in `.cpp`.
- Comments in shell build scripts (10 files).

Next iteration: Phase 2 — documentation and build hygiene for `Shark_2_0_CLI` (README, CMake style, learning highlights), no logic changes.

### 2026-04-23 (iteration 5) — English-only pivot completed

Completing the English-only pivot started in iteration 4:

- All per-version READMEs translated (`Shark_2_0_CLI`, `Shark_3_0_Postgres`, `Shark_3_0_Qt`).
- All C++ source comments and user-visible strings translated across the three subprojects: 33 files in Shark_2_0_CLI, 37 in Shark_3_0_Postgres, 57 in Shark_3_0_Qt, plus 10 Qt `.ui` XML files.
- Shell build-script echoes and error messages translated across all 10 scripts.
- Final verification: `grep -rln --include='*.md' --include='*.cpp' --include='*.h' --include='*.ui' --include='*.sh' -P '[А-Яа-яЁё]' .` returns empty.
- Local build check: `Shark_2_0_CLI` and `Shark_3_0_Postgres` (with libpq explicit paths) compile cleanly. `Shark_3_0_Qt` was not built locally — Qt 6 is not installed in this environment, but all 10 `.ui` files still parse as valid XML and sampled `.cpp` files pass `clang -fsyntax-only`.
- For the PG subproject, `find_package(PostgreSQL REQUIRED)` does not auto-locate Homebrew's keg-only `libpq` on macOS. Noted as a pre-existing CMake quirk to address later (Phase 5). Not blocking.
- Stray translated implementation artefacts found along the way: Cyrillic alphabet arrays in `core/system/system_function.h` of the Cli and PG subprojects were encoded as `\u00XX` UCNs so the bytes are preserved and no raw Cyrillic remains in source. A handful of NBSP (`0xA0`) characters that had leaked into Qt files were replaced with regular spaces.

### 2026-04-23 (iteration 6) — Phase 2 done (Shark_2_0_CLI docs + CMake)

Phase 2 objective: turn `Shark_2_0_CLI` into a proper learning showcase via README restructure and targeted CMake cleanup. No logic or code-style changes — that is Phase 3 by separate plan.

Changes:

- `Shark_2_0_CLI/README.md` — rewritten around reviewer experience. Sections now: elevator pitch, "What this version demonstrates" (8 learning highlights), mermaid architecture diagram, **code tour** table mapping topics to entry-point files, requirements, clean quick-start block, 7-step demo flow, known limitations with forward links to the PG and Qt successors, license. Removed the previous empty "Configure" / "Scripts" sub-headings.
- `Shark_2_0_CLI/CMakeLists.txt` — three targeted changes:
  1. `project(Shark2_0) → project(Shark2_0 LANGUAGES CXX)` — consistent with the other subprojects, prevents the unnecessary C compiler probe.
  2. Unconditional `set(CMAKE_BUILD_TYPE Debug)` → `if(NOT CMAKE_BUILD_TYPE) set(... Debug) endif()`. Previously `-DCMAKE_BUILD_TYPE=Release` was silently ignored. Verified: with guard, default is still Debug (`-O0 -g`), explicit Release gives `-O3 -DNDEBUG`.
  3. Section comments translated to English (the last remaining Russian in the build configuration).
  No other changes — `GLOB_RECURSE CONFIGURE_DEPENDS`, SHARED core library, target link wiring, include paths all kept as is.
- Release build of `Shark_2_0_CLI` verified locally.

Noticed and deliberately deferred to Phase 3 or later:

- `src/client/CMakeLists.txt` and `src/dto/CMakeLists.txt` declare targets (`shared_lib`, `dto_lib`) that are never pulled in by the root CMakeLists (no `add_subdirectory`). They are dead CMake configuration. Removing them is not in Phase 2 scope.
- The default target layout uses globbing (`GLOB_RECURSE`) which is simple but not strictly idiomatic for modern CMake. Switching to explicit source lists is a judgment call for Phase 3.

Next iteration: Phase 3 — targeted code-level refactor in `Shark_2_0_CLI` (scope to be agreed in a separate plan).

### 2026-04-23 (iteration 7) — Phase 4 done (Shark_3_0_Postgres docs + CMake)

Phase 4 objective: apply the Phase 2 treatment to the PostgreSQL-backed subproject. Docs restructure + CMake cleanup; no logic or code-style changes — that stays a Phase 5 item.

Changes:

- `Shark_3_0_Postgres/README.md` — rewritten around reviewer experience. Sections: elevator pitch (with forward/back links to 2.0 and Qt), "What this version demonstrates" (8 PG-focused learning highlights including libpq, schema bootstrap, JSON config, SQL-specific exceptions, client-side SQLite scaffolding), mermaid architecture diagram with the PG arrow, code tour with PG-specific entry points, **Database schema** table documenting the six tables (`users`, `users_passhash`, `chats`, `participants`, `messages`, `message_status`), requirements, quick start with `connect_db.conf.example`, **Build quirks on macOS** subsection, 7-step demo flow anchored on persistence-survives-restart, known limitations. Removed the empty "Configure" / "Scripts" sub-headings from the previous structure.
- `Shark_3_0_Postgres/CMakeLists.txt` — two targeted changes:
  1. Added a Homebrew-aware `PostgreSQL_ROOT` fallback on APPLE. Homebrew ships `libpq` as keg-only, so `find_package(PostgreSQL)` used to miss it without `-DPostgreSQL_ROOT=...`. The new block probes `/opt/homebrew/opt/libpq` and `/usr/local/opt/libpq` and only sets the hint when neither a cache nor an env override is present. Linux is unchanged (guarded by `if(APPLE)`). This closes the pre-existing CMake quirk flagged in iteration 5. Verified: `cmake -B build -S . -DCMAKE_BUILD_TYPE=Release` now finds `/opt/homebrew/opt/libpq/lib/libpq.dylib` without any flags, and client/server/libcore targets link cleanly.
  2. Translated the one remaining Russian section comment (`# ===== core (с PostgreSQL) =====`) to English.
  Everything else (GLOB_RECURSE, target wiring, include paths, INTERFACE-vs-STATIC dto fallback, server-side `CONFIG_DIR` definition) kept as is.

Noticed and deliberately deferred (same pattern as Shark_2_0_CLI):

- `Shark_3_0_Postgres/src/client/CMakeLists.txt` and `src/server/CMakeLists.txt` are full standalone alternative configs (the server one declares its own `project(Shark2_0)` with CXX 17 and four `add_subdirectory` calls) that the root CMakeLists never pulls in. Dead configuration; removal belongs to Phase 5 or a separate cleanup plan.
- `client_sql_lite.cpp` links in libsqlite3 and defines `sqlite3_open_v2` wrappers but is not wired into the main flow. Kept as scaffolding per the original intent.

Next iteration: Phase 6 — documentation and build hygiene for `Shark_3_0_Qt` (README restructure, Qt-specific learning highlights, screenshots curation), no logic changes. A targeted code-refactor pass for `Shark_2_0_CLI` (Phase 3) and one for `Shark_3_0_Postgres` (Phase 5) remain on the roadmap but require their own plans.

### 2026-04-23 (iteration 8) — Phase 3 done (Shark_2_0_CLI polish)

Phase 3 objective: close out `Shark_2_0_CLI` completely before moving on to Qt. Low-risk polish across the subproject — no logic changes, no signature changes, no architectural work. All scope was scoped in advance and ratified via plan + ExitPlanMode.

Changes (landed as 4 + 1 commits):

- **A. Dead config and dev-only scratch files removed.** 4 nested `CMakeLists.txt` under `src/client`, `src/core`, `src/dto`, `src/server` — none were referenced via `add_subdirectory` from the root; `src/server/CMakeLists.txt` even redeclared its own `project(Shark2_0)` with CXX 17 and pointed at a non-existent `shared/` directory. Also removed `combine_code_MacOS.sh` (author's dev utility that concatenated all sources into `for_chat.txt` for LLM pasting), the empty `for_chat.txt`, and `scripts/scripts.txt` (personal command notes — `lsof`, `kill -9`). Post-removal Release build still passes. Reviewer-visible noise dropped.
- **B. EOF newlines and one tab.** Appended trailing newline to 18 cpp/h files that were missing one. Replaced the single TAB character at `src/client/menu/2_1_new_chat_menu.cpp:168` (`else \tif` → `else if`) with spaces. After the pass, `grep -rP '\t' Shark_2_0_CLI/src` is empty.
- **C. `[[maybe_unused]]` on intentional placeholders.** 12 unique parameter positions annotated across 8 files: both `UnknownException` / `BadWeakException` forwarding constructors, `MessageContent::setMessageContent`, `chooseOneParticipant(chat_ptr)`, `userChatDeleteAll(clientSession)`, `Chat::createNewMessageId(isServerStatus)`, `ChatSystem::eraseUser(user)`, `addMessageToChatInit(serverSession)`, and four methods in `ServerSession` (`routingRequestsFromClient`, `processingGetIndexes`, `processingReceivedQueue`, `createNewMessageChatSrv`). The original estimate was 57 — that was a multi-TU duplicate count; the unique positions were 12. Annotation on the definition only, no signatures or bodies touched. Verified: build under `-Wall -Wextra -Wpedantic` now reports **0 warnings from project code** (`picosha2.h` is vendored and left untouched).
- **D. Commented-out debug scaffolding deleted.** Removed three `std::cout` DEBUG lines after an unconditional return in `Chat::getUnreadMessageCount` (unreachable) and five `[DEBUG] std::cerr` blocks in `serialize.cpp` along with their `// debug check` header markers, plus a commented-out alternate direction-parse block. No observable behaviour change.
- **E. This journal entry.**

With Phase 3 in, `Shark_2_0_CLI` is now closed across all three axes: documentation (Phase 2), code hygiene (Phase 3), language (iteration 5). No known outstanding items.

Next iteration: whatever the owner picks next — Phase 6 (Qt docs) is the default roadmap step, but a code-refactor pass for `Shark_3_0_Postgres` (Phase 5) or the same closing treatment for `Shark_3_0_Qt` are equally valid. All require their own plans.

### 2026-04-23 (iteration 10) — Intermediate hyphen naming pass

An intermediate pass replaced dots in versioned project names with hyphenated directory names. This was later superseded by iteration 11, where the owner's underscore-based naming system was restored.

Changes:

- CLI subproject directory renamed to `Shark-2_0-Cli/`
- PostgreSQL subproject directory renamed to `Shark-3_0-Postgres/`
- Qt subproject directory renamed to `Shark-3_0-Qt/`
- Root and per-version READMEs, plus this context file, were updated to use the hyphenated names consistently at that time.

### 2026-04-23 (iteration 11) — Owner naming restored

The previous hyphenated directory names were not the owner's preferred naming system. The repository was returned to the underscore-based pattern requested by the owner.

Changes:

- CLI subproject directory renamed to `Shark_2_0_CLI/`
- PostgreSQL subproject directory renamed to `Shark_3_0_Postgres/`
- Qt subproject directory renamed to `Shark_3_0_Qt/`
- Root and per-version `README.md` files were rewritten so each implementation now has its own visual block and its own accurate discovery description.
