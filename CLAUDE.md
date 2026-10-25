# CLAUDE.md

@AGENTS.md

## Claude-specific instructions

### Planning before implementation

- Before implementing a new feature or making a substantial change, inspect the relevant existing code and ARCHITECTURE.md.
- Form a clear implementation plan before modifying files.
- Identify which systems, files, replication behaviour, persistence, and extension points may be affected.
- Prefer minimal changes that fit the existing architecture over unnecessary refactoring.
- If the requested change conflicts with the existing architecture, explain the conflict before implementing it.
- For small, obvious fixes, keep planning proportionate to the size of the change.

### Enfusion MCP

- Use the enfusion-mcp server for API search (`api_search`), the offline copy of the BI wiki (`wiki_search`), and reading project files (`project_browse` / `project_read`).
- Use `wiki_search` instead of fetching the live BI wiki.
- Do not use the live Workbench tools (`wb_*`). They have proved unreliable due to timeouts and dropped connections.
- Ask the user to perform Workbench actions and provide the results when Workbench interaction is required.
- The `wb_*` tools require MCP handler scripts under `Scripts/WorkbenchGame/EnfusionMCP/`. That folder is gitignored and must never be committed.
- Before packing or publishing the addon, ensure `Scripts/WorkbenchGame/EnfusionMCP/` is removed so MCP handler scripts are not shipped with the mod.
- `game_read` / `game_browse` currently cannot read inside the game 1.8 `.pak` archives. Read base-game files through Workbench or from `_reference/` instead.