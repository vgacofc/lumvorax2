# Configuring

You can configure Bob Shell to match your workflow preferences.

## Getting started
You can configure Bob Shell using:

. **Project-specific settings**: Create `.bob/settings.json` in your project directory
. **User-wide settings**: Edit `~/.bob/settings.json` in your home directory
. **Command-line arguments**: Run `--sandbox` when starting Bob Shell

```bash
# Example: Start Bob Shell with sandbox mode enabled
bob --sandbox
```

## Configuration system
### How configuration works
Bob Shell uses a layered configuration system where settings from different sources are combined according to a specific precedence order:

| Priority    | Source                 | Location                             | Scope                      |
| ----------- | ---------------------- | ------------------------------------ | -------------------------- |
| 1 (Highest) | Command-line arguments | `bob --option value`                 | Current session            |
| 2           | Environment variables  | Shell or `.env` files                | System/session             |
| 3           | System settings file   | `/etc/bobshell/settings.json`        | All users, all projects    |
| 4           | Project settings file  | `.bob/settings.json`                 | Current project            |
| 5           | User settings file     | `~/.bob/settings.json`               | Current user, all projects |
| 6           | System defaults file   | `/etc/bobshell/system-defaults.json` | All users, all projects    |
| 7 (Lowest)  | Hardcoded defaults     | Built into Bob Shell                 | Always applied             |

When the same setting is defined in multiple places, the higher-priority source takes precedence.

### Settings file locations
Bob Shell looks for settings files in these locations:

* **Project settings**: `.bob/settings.json` in your project directory
* **User settings**: `~/.bob/settings.json` in your home directory
* **System settings**:

**Linux:**

```bash
/etc/bobshell/settings.json
```

**Windows:**

```bash
C:\ProgramData\bobshell\settings.json
```

**MacOS:**

```bash
/Library/Application Support/Bob Shell/settings.json
```

* **System defaults**:

**Linux:**

```bash
/etc/bobshell/system-defaults.json
```

**Windows:**

```bash
C:\ProgramData\bobshell\system-defaults.json
```

**MacOS:**

```bash
/Library/Application\ Support/Bob\ Shell/system-defaults.json
```

### Core settings categories
Bob Shell settings are organized into categories. Each category contains related settings that control specific aspects of Bob Shell's behavior.

## General settings
Control basic Bob Shell behavior and preferences.

```json
{
"general": {
"preferredEditor": "code",
"vimMode": false,
"disableAutoUpdate": false,
"disableUpdateNag": false,
"checkpointing": {
"enabled": true
}
}
}
```

| Setting                 | Type    | Default     | Description                                                        |
| ----------------------- | ------- | ----------- | ------------------------------------------------------------------ |
| `preferredEditor`       | string  | `undefined` | Editor to use when opening files                                   |
| `vimMode`               | boolean | `false`     | Enable Vim keybindings                                             |
| `disableAutoUpdate`     | boolean | `false`     | Prevent automatic updates                                          |
| `disableUpdateNag`      | boolean | `false`     | Hide update notifications                                          |
| `checkpointing.enabled` | boolean | `false`     | Enable [session checkpointing](/docs/shell/features/checkpointing) |

## UI settings
Customize Bob Shell's appearance and interface elements.

```json
{
"ui": {
"theme": "GitHub",
"hideBanner": true,
"hideTips": false,
"showLineNumbers": true
}
}
```

| Setting                               | Type    | Default     | Description                       |
| ------------------------------------- | ------- | ----------- | --------------------------------- |
| `theme`                               | string  | `undefined` | UI color theme                    |
| `customThemes`                        | object  | `{}`        | Custom theme definitions          |
| `hideWindowTitle`                     | boolean | `false`     | Hide window title bar             |
| `hideTips`                            | boolean | `false`     | Hide helpful tips                 |
| `hideBanner`                          | boolean | `false`     | Hide application banner           |
| `hideFooter`                          | boolean | `false`     | Hide footer                       |
| `showMemoryUsage`                     | boolean | `false`     | Show memory usage stats           |
| `showLineNumbers`                     | boolean | `false`     | Show line numbers in chat         |
| `showCitations`                       | boolean | `false`     | Show citations for generated text |
| `accessibility.disableLoadingPhrases` | boolean | `false`     | Disable loading phrases           |

## Context settings
Control how Bob Shell manages project context and memory.

```json
{
"context": {
"fileName": ["CONTEXT.md", "AGENTS.md"],
"discoveryMaxDirs": 200,
"includeDirectories": ["../shared-lib", "~/reference-code"],
"fileFiltering": {
"respectGitIgnore": true,
"respectBobIgnore": true
}
}
}
```

| Setting                                   | Type         | Default     | Description                       |
| ----------------------------------------- | ------------ | ----------- | --------------------------------- |
| `fileName`                                | string/array | `undefined` | Context file name(s)              |
| `importFormat`                            | string       | `undefined` | Memory import format              |
| `discoveryMaxDirs`                        | number       | `200`       | Max directories to search         |
| `includeDirectories`                      | array        | `[]`        | Additional directories to include |
| `loadFromIncludeDirectories`              | boolean      | `false`     | Load context from included dirs   |
| `fileFiltering.respectGitIgnore`          | boolean      | `true`      | Honor .gitignore files            |
| `fileFiltering.respectBobIgnore`          | boolean      | `true`      | Honor .bobignore files            |
| `fileFiltering.enableRecursiveFileSearch` | boolean      | `true`      | Enable recursive file search      |

## Tools settings
Configure how Bob Shell uses and manages tools.

```json
{
"tools": {
"sandbox": "docker",
"allowed": ["run_shell_command(git)", "run_shell_command(npm test)"],
"exclude": ["write_file"]
}
}
```

| Setting            | Type           | Default     | Description                         |
| ------------------ | -------------- | ----------- | ----------------------------------- |
| `sandbox`          | boolean/string | `undefined` | Sandbox running environment         |
| `usePty`           | boolean        | `false`     | Use node-pty for shell commands     |
| `core`             | array          | `undefined` | Restrict built-in tools (allowlist) |
| `exclude`          | array          | `undefined` | Tools to exclude from discovery     |
| `allowed`          | array          | `undefined` | Tools that bypass confirmation      |
| `discoveryCommand` | string         | `undefined` | Command for tool discovery          |
| `callCommand`      | string         | `undefined` | Command for calling tools           |

## MCP settings
Configure Model Context Protocol server connections.

```json
{
"mcpServers": {
"mainServer": {
"command": "bin/mcp_server.py"
},
"remoteServer": {
"url": "https://example.com/mcp",
"headers": {
"Authorization": "Bearer token123"
}
}
}
}
```

| Setting                    | Type   | Default     | Description                   |
| -------------------------- | ------ | ----------- | ----------------------------- |
| `mcp.serverCommand`        | string | `undefined` | Command to start MCP server   |
| `mcp.allowed`              | array  | `undefined` | Allowlist of MCP servers      |
| `mcp.excluded`             | array  | `undefined` | Denylist of MCP servers       |
| `mcpServers.<SERVER_NAME>` | object | -           | Server-specific configuration |

For each MCP server, you can configure:

* `command`: Command to run (for local servers)
* `args`: Command-line arguments
* `env`: Environment variables
* `cwd`: Working directory
* `url`: Server-Sent Events (SSE) endpoint URL (for remote servers)
* `httpUrl`: HTTP endpoint URL (for remote servers)
* `headers`: HTTP headers for requests
* `timeout`: Request timeout in milliseconds
* `trust`: Trust server and bypass confirmations
* `includeTools`: Tool names to include
* `excludeTools`: Tool names to exclude

### Command-line arguments
Pass these arguments when starting Bob Shell to override settings for that session:

```bash
# Start Bob Shell with specific settings
bob --sandbox --approval-mode auto_edit
```

| Argument                     | Description                                   | Example                            |
| ---------------------------- | --------------------------------------------- | ---------------------------------- |
| `--prompt`, `-p`             | Non-interactive prompt                        | `bob -p "Explain this code"`       |
| `--prompt-interactive`, `-i` | Interactive initial prompt                    | `bob -i "Help me debug"`           |
| `--sandbox`, `-s`            | Enable sandbox mode                           | `bob -s`                           |
| `--debug`, `-d`              | Enable debug mode                             | `bob -d`                           |
| `--yolo`                     | Auto-approve all tool calls                   | `bob --yolo`                       |
| `--approval-mode`            | Set tool approval mode                        | `bob --approval-mode=auto_edit`    |
| `--allowed-tools`            | Tools to auto-approve                         | `bob --allowed-tools="git status"` |
| `--include-directories`      | Add directories to workspace                  | `bob --include-directories=../lib` |
| `--chat-mode`                | Choose the mode for interaction               | `bob --chat-mode`                  |
| `--hide-intermediary-output` | Output only the final task completion output  | `bob --hide-intermediary-output`   |
| `--show-license`             | Show full path to license files for review    | `bob --show-license`               |
| `--accept-license`           | Accept the IBM license agreement and continue | `bob --accept-license`             |
| `--instance-id`              | Instance ID to use for this Bob Shell session | `bob --instance-id=my-instance`    |
| `--team-id`                  | Team ID to use for this Bob Shell session     | `bob --team-id=my-team`            |

## Context files
Context files (like `AGENTS.md`) provide instructions to the AI model. These files are loaded hierarchically:

. **Global context**: `~/.bob/AGENTS.md` (applies to all projects)
. **Project context**: `AGENTS.md` in project root and parent directories
. **Local context**: `AGENTS.md` in subdirectories (for component-specific instructions)

### Example context file
```markdown
# Project: My TypeScript Library

## General instructions

- Follow existing coding style
- Add JSDoc comments to all functions
- Prefer functional programming patterns
- Target TypeScript 5.0 and Node.js 20+

## Coding style

- Use 2 spaces for indentation
- Interface names should be prefixed with `I`
- Private class members should be prefixed with `_`
- Use strict equality (`===` and `!==`)
```

## Managing context
* Use `/memory refresh` to reload all context files
* Use `/memory show` to view the current context

## Sandboxing
Sandboxing provides security when running potentially unsafe operations:

```bash
# Enable sandboxing for a session
bob --sandbox
```

You can create custom sandbox environments:

. Create `.bob/sandbox.Dockerfile` in your project
. Base it on the `bobshell-sandbox` image
. Add your custom dependencies

```dockerfile
FROM bobshell-sandbox

# Add custom dependencies
RUN apt-get update && apt-get install -y python3-dev
```

Build and use your custom sandbox:

```bash
BUILD_SANDBOX=1 bob -s
```

The `create-pr` command is not compatible with Sandbox sessions.

## Usage statistics
Bob Shell collects anonymous usage statistics to improve the product. This includes:

* Tool usage patterns (names, success/failure, duration)
* API request metrics (model, duration, success)
* Session configuration information

No personal information, prompt content, or file content is collected.

To opt out, add this to your settings:

```json
{
"privacy": {
"usageStatisticsEnabled": false
}
}
```# Custom modes

You can create custom modes to tailor Bob's behavior to specific tasks or workflows. Custom modes in Bob Shell work similarly to Bob IDE modes.

### Why use custom modes in Bob Shell
* **Shell-optimized workflows**: Create modes designed specifically for terminal-based development tasks
* **Command-line safety**: Restrict modes to safe operations when working in production environments
* **Environment-specific behavior**: Configure modes that adapt to different shell environments
* **Automation-friendly**: Design modes that work seamlessly in both interactive and non-interactive sessions
* **Team standardization**: Share shell-specific modes across your team for consistent workflows

### What's included in a custom mode
Custom modes in Bob Shell use the same core structure as Bob IDE modes:

| Property             | Description                                           | Shell-Specific Considerations                                  |
| -------------------- | ----------------------------------------------------- | -------------------------------------------------------------- |
| `slug`               | Unique internal identifier                            | Used in command-line arguments:<br />`bob --chat-mode=my-mode` |
| `name`               | Display name in the UI                                | Shown in interactive mode's mode selector                      |
| `description`        | Short description shown in the mode selector          | Briefly explain your mode's purpose                            |
| `roleDefinition`     | Core identity and expertise                           | Should consider shell context and command-line workflows       |
| `groups`             | Allowed toolsets and file access                      | Command running permissions are critical in shell environments |
| `whenToUse`          | Mode selection guidance                               | Helps Bob Shell choose appropriate modes for tasks             |
| `customInstructions` | Specific behavioral guidelines, or rules for the mode | Can reference Bob Shell development patterns                   |

## Available tools
### Available tool groups
* `read`: Read files and directories
* `edit`: Modify files (can be restricted with `fileRegex`)
* `browser`: Use browser automation
* `command`: Execute terminal commands
* `mcp`: Access MCP servers

### Creating custom modes
## Configuration files
Bob Shell uses the same configuration format as Bob IDE, supporting both YAML (preferred) and JSON formats.

## Global modes
Create or edit `~/.bob/custom_modes.yaml` for modes available across all projects:

```yaml
customModes:
- slug: shell-debug
name: 🐛 Shell Debugger
roleDefinition: >-
You are a debugging specialist focused on command-line troubleshooting.
You excel at analyzing shell output, environment variables, and system logs.
whenToUse: Use for debugging shell scripts, command failures, and environment issues.
customInstructions: |-
When debugging:
- Always check environment variables first
- Examine command exit codes
- Review relevant log files
- Test commands in isolation before suggesting fixes
groups:
- read
- command
- browser
```

### Project-specific modes
Create or edit `.bob/custom_modes.yaml` in your project root:

```yaml
customModes:
- slug: deploy-helper
name: 🚀 Deployment Assistant
roleDefinition: You are a deployment specialist for this project's infrastructure.
whenToUse: Use for deployment tasks, infrastructure changes, and release management.
customInstructions: |-
Deployment guidelines:
- Always verify the target environment before running commands
- Check for running processes that might be affected
- Validate configuration files before applying changes
- Create backups before destructive operations
groups:
- read
- - edit
- fileRegex: \.(yaml|yml|sh|env)$
description: Configuration and script files only
- command
```

### Command-line mode selection
Specify a mode when starting Bob Shell:

```bash
# Start Bob Shell in a specific mode
bob --chat-mode=shell-debug

# Combine with other options
bob --chat-mode=deploy-helper --sandbox
```

### Interactive mode switching
In interactive mode, switch modes using slash commands:

```bash
# Switch to a custom mode
/mode shell-debug

# Or use the mode's slug directly
/shell-debug
```

### Shell-specific configurations
### Production safety mode
Create a safety-focused mode for production environments:

```yaml
customModes:
- slug: prod-ops
name: 🔒 Production Operations
roleDefinition: >-
You are a production operations specialist with a strong focus on safety.
You never run destructive commands without explicit confirmation.
whenToUse: Use when working with production systems or sensitive environments.
customInstructions: |-
Production safety rules:
- NEVER run destructive commands without explicit user confirmation
- Always verify the target environment before any operation
- Suggest dry-run options when available
- Check for active connections or processes before changes
- Recommend backup procedures before modifications
groups:
- read
- browser
# Note: No edit or command groups for maximum safety
```

### Script development mode
Create a mode for shell script development:

```yaml
customModes:
- slug: script-dev
name: 📜 Script Developer
roleDefinition: >-
You are a shell scripting expert specializing in bash, zsh, and POSIX-compliant scripts.
whenToUse: Use for creating, debugging, or improving shell scripts.
customInstructions: |-
Shell scripting best practices:
- Use shellcheck-compliant syntax
- Include proper error handling with set -e and set -u
- Add usage documentation at the top of scripts
- Quote variables to prevent word splitting
- Provide exit codes for different error conditions
groups:
- read
- - edit
- fileRegex: \.(sh|bash|zsh)$
description: Shell script files only
- command
```

### Command running permissions
### Restricting command access
Control which commands a mode can run by omitting the `command` group:

```yaml
customModes:
- slug: safe-reviewer
name: 👀 Safe Code Reviewer
roleDefinition: You are a code reviewer focused on analysis, not modification.
whenToUse: Use for code reviews and analysis without making changes.
groups:
- read
- browser
# No command or edit groups - read-only mode
```

### Allowing specific commands
Use Bob Shell's `allowed` tools configuration with custom modes in your settings file:

```json
{
"tools": {
"allowed": [
"run_shell_command(git status)",
"run_shell_command(git log)",
"run_shell_command(git diff)"
]
}
}
```

### Interactive vs non-interactive behavior
### Designing for both modes
Create modes that work well in both interactive and non-interactive sessions:

```yaml
customModes:
- slug: test-runner
name: 🧪 Test Runner
roleDefinition: >-
You are a testing specialist who runs and analyzes test suites.
You adapt your output based on the running context.
whenToUse: Use for running tests, analyzing test results, and debugging test failures.
customInstructions: |-
Testing guidelines:
- In interactive mode: Provide detailed explanations and suggestions
- In non-interactive mode: Focus on concise, actionable output
- Always report test results clearly
- Suggest fixes for failing tests
groups:
- read
- command
```

### Non-interactive usage
Use modes in non-interactive mode:

```bash
# Use a mode designed for automation
bob --chat-mode=test-runner -p "Run the test suite and report failures"

# Combine with output processing
bob --chat-mode=test-runner -p "Run tests" --hide-intermediary-output > results.txt
```

### Mode-specific instructions via files
### Directory-based instructions
Create mode-specific instruction files in `.bob/rules-{mode-slug}/`:

Example instruction file (`.bob/rules-shell-debug/01-environment-checks.md`):

```markdown
# Environment Debugging Checklist

When debugging shell issues, always check:

1. **Environment Variables**: PATH, SHELL, TERM, and application-specific variables
2. **Shell Configuration**: .bashrc, .zshrc, .profile files
3. **System State**: Current working directory, file permissions, disk space
4. **Command Availability**: Use `command -v <cmd>` to verify commands exist
```

### Single file instructions (fallback)
Alternatively, use a single file `.bobrules-{mode-slug}` in your workspace root.

## Configuration precedence
Mode configurations are applied in this order:

. Command-line arguments (`--chat-mode=mode-slug`)
. Project-level modes (`.bob/custom_modes.yaml`
. User-level modes (`~/.bob/custom_modes.yaml`)
. System-level modes (platform-specific locations)
. Default modes

### Sandboxing with custom modes
Combine custom modes with Bob Shell's sandbox feature for safe experimentation:

```bash
# Start a custom mode in sandbox
bob --chat-mode=script-dev --sandbox

# Use with Docker sandbox
bob --chat-mode=deploy-helper --sandbox
```

### Migrating modes from Bob IDE
## Key differences
When migrating custom modes from Bob IDE to Bob Shell:

| Aspect          | Bob IDE                       | Bob Shell                        |
| --------------- | ----------------------------- | -------------------------------- |
| UI interaction  | Visual interface with panels  | Terminal-based interface         |
| File editing    | In-editor diffs and previews  | CLI diff view or external editor |
| Command running | Integrated terminal           | Direct shell running             |
| Mode switching  | UI dropdown or slash commands | Slash commands or CLI arguments  |
| Configuration   | Settings UI + files           | Configuration files only         |

## Adaptation checklist
When adapting Bob IDE modes for Bob Shell:

* Review `roleDefinition` for shell-specific context
* Update `customInstructions` to reference command-line workflows
* Consider command running safety in `groups` configuration
* Test mode in both interactive and non-interactive sessions
* Verify file path handling works in shell context
* Test with sandbox mode if applicable

## Example migration
**Original Bob IDE mode:**

```yaml
customModes:
- slug: code-reviewer
name: 👀 Code Reviewer
roleDefinition: You are a code reviewer who provides detailed feedback.
groups:
- read
- browser
```

**Adapted for Bob Shell:**

```yaml
customModes:
- slug: code-reviewer
name: 👀 Code Reviewer
roleDefinition: >-
You are a code reviewer who provides detailed feedback.
You work efficiently in terminal environments and provide clear, actionable suggestions.
whenToUse: Use for code reviews, pull request analysis, and code quality checks.
customInstructions: |-
Code review guidelines:
- Provide feedback in a structured format suitable for terminal output
- Reference specific line numbers and file paths
- Suggest concrete improvements with examples
- Format output for easy parsing if used in non-interactive mode
groups:
- read
- command  # Added for git operations
- browser
```
# Slash commands

Create custom slash commands to automate workflows and standardize team practices.

To get started, type `/` in Bob Shell to see all available commands, or create your own by adding a markdown file to `.bob/commands/` or `~/.bob/commands/`.

### Why use slash commands?
Slash commands provide several key benefits:

* **Workflow automation**: Turn complex multi-step processes into single commands
* **Team standardization**: Share commands across your team for consistent practices
* **Context preservation**: Include project-specific context in every command
* **Quick access**: Fuzzy search and autocomplete for instant command discovery

### How slash commands work
When you type `/` in Bob Shell, a menu appears showing all available commands. These commands come from two sources:

| Command Type             | Source                                 | Purpose                                    |
| ------------------------ | -------------------------------------- | ------------------------------------------ |
| Custom workflow commands | `.bob/commands/` or `~/.bob/commands/` | User-created automation for specific tasks |
| Mode commands            | Built-in and custom modes              | Switch Bob's operational context           |

### Creating custom commands
Custom commands extend Bob's functionality by adding markdown files to specific directories:

| Location         | Scope                             | Path                                      |
| ---------------- | --------------------------------- | ----------------------------------------- |
| Project-specific | Available in current project only | `.bob/commands/` in your workspace root   |
| Global           | Available in all projects         | `~/.bob/commands/` in your home directory |

The filename becomes the command name. For example:

```
.bob/commands/
├── review.md         → /review
├── test-api.md       → /test-api
└── deploy-check.md   → /deploy-check
```

### Command name processing
When creating commands through the UI, command names are automatically processed:

* Converted to lowercase
* Spaces replaced with dashes
* Special characters removed
* Leading/trailing dashes removed

Example: "My Cool Command!" becomes `my-cool-command`

### Basic command format
Create a simple command by adding a markdown file:

```markdown
Help me review this code for security issues and suggest improvements.
```

### Advanced command with frontmatter
Add metadata using frontmatter for enhanced functionality:

```markdown
---
description: Create a new API endpoint
argument-hint: <endpoint-name> <http-method>
---
Create a new API endpoint called $1 that handles $2 requests.
Include proper error handling and documentation.
```

## Frontmatter fields
| Field           | Purpose                     | Example                             |
| --------------- | --------------------------- | ----------------------------------- |
| `description`   | Appears in the command menu | "Create a new API endpoint"         |
| `argument-hint` | Shows expected arguments    | "`<endpoint-name>` `<http-method>`" |

### Command management in Bob Shell
Bob Shell supports the same slash commands as Bob IDE. While Bob Shell does not provide a dedicated UI for managing commands, you can:

. Create command files manually in the `.bob/commands/` directory in your project or `~/.bob/commands/` in your home directory
. Edit existing command files with any text editor

### Using slash commands
Type `/` in Bob Shell to see a unified menu containing the following types of commands:

. **Unified menu**: Custom commands and mode-switching commands appear together
. **Autocomplete**: Start typing to filter commands (e.g., `/sam` shows `sample-command-name`)
. **Fuzzy search**: Find commands even with partial matches
. **Description preview**: See command descriptions in the menu
. **Visual indicators**: Mode commands are distinguished from custom commands with special icons

```
/mode code     Switch to Code mode
/mode ask      Switch to Ask mode
/review        Review code for security issues
/api-endpoint  <endpoint-name> <http-method>
```

## Argument hints
Argument hints provide instant help for slash commands, showing you what kind of information to provide when a command expects additional input.

When you type `/` to bring up the command menu, commands that expect arguments will display a light gray hint next to them. This hint tells you what kind of argument the command is expecting.

For example:

* `/mode <mode_slug>` - The hint `<mode_slug>` indicates you should provide a mode name like `code` or `debug`
* `/api-endpoint <endpoint-name> <http-method>` - Shows you need both an endpoint name and HTTP method

After selecting the command, it will be inserted into the chat input followed by a space. The hint is not inserted; it is only a visual guide to help you know what to type next. You must then manually type the argument after the command.

### Adding argument hints to custom commands
You can add argument hints to your custom commands using the `argument-hint` field in the frontmatter:

```markdown
---
description: Create a new API endpoint
argument-hint: <endpoint-name> <http-method>
---
Create a new API endpoint called $1 that handles $2 requests.
Include proper error handling and documentation.
```

This will display as `/api-endpoint <endpoint-name> <http-method>` in the command menu.

### Best practices for argument hints
* **Be specific**: Use descriptive placeholders like `<file-path>` instead of generic ones like `<arg>`
* **Show multiple arguments**: If your command needs multiple inputs, show them all: `<source> <destination>`
* **Use consistent format**: Always wrap placeholders in angle brackets: `<placeholder>`
* **Keep it concise**: Hints should be brief and clear

### Common questions about arguments
| Question                                        | Answer                                                                                                                                                          |
| ----------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| What if I don't provide the argument?           | The command might not work as expected, or it might prompt you for more information. The hint is there to help you get it right the first time.                 |
| Do all commands have hints?                     | No, only commands that are designed to take arguments will have hints. Commands that work without additional input won't show hints.                            |
| Can I use a command without replacing the hint? | The hint text (like `<mode_slug>`) needs to be replaced with actual values. Leaving the hint text will likely cause the command to fail or behave unexpectedly. |

## Best practices
## Command naming
* Use descriptive, action-oriented names
* Keep names concise but clear
* Use hyphens for multi-word commands
* Avoid generic names like `help` or `test`
* Note: Names are automatically slugified (lowercase, special characters removed)
* The `.md` extension is automatically added/removed as needed

## Command content
* Start with a clear directive
* Use structured formats (lists, sections)
* Include specific requirements
* Reference project conventions
* Keep commands focused on a single task

## Organization
* Group related commands in subdirectories
* Use consistent naming patterns
* Document complex commands
* Version control your commands
* Share team commands in the project repository

## Troubleshooting
### Commands not appearing
If your commands aren't showing up in the menu:

* **Check file location**: Ensure custom command files are in `.bob/commands/` or `~/.bob/commands/`
* **Verify file extension**: Custom commands must be `.md` files

### Command not found
When a slash command isn't found, the LLM will see:

```
The slash command '/unknown-command' was not found. Please check the command name and try again.
```

## Command conflicts
* Custom project commands override global custom commands with the same name
* Use unique names to avoid conflicts
* When creating duplicate names through the UI, numbers are appended (e.g., `new-command-1`, `new-command-2`)

### About mode commands
The slash menu includes mode-switching commands (like `/mode code`, `/mode ask`) that fundamentally change the AI's operational mode - they don't just inject text but switch the entire AI context. Custom modes you create also appear as slash commands (e.g., a mode with slug `reviewer` becomes `/reviewer`). These mode commands cannot be overridden by custom workflow commands.

### Cross-platform compatibility
Slash commands work identically across both Bob Shell and Bob IDE. This means:

* Commands created in one environment can be used in the other
* The same command syntax and behavior applies in both environments
* Team-shared commands work consistently regardless of which interface team members use

# Memory files

Break down large AGENTS.md files into smaller, reusable components using a simple import syntax.

## Overview
| Feature          | Description                                   | Benefit                        |
| ---------------- | --------------------------------------------- | ------------------------------ |
| File imports     | Include content from other files              | Organize complex context files |
| Path flexibility | Support for relative and absolute paths       | Work with any file structure   |
| Nested imports   | Files can import other files                  | Create hierarchical structures |
| Safety features  | Circular import detection and security checks | Prevent common problems        |

### Why use memory imports
* **Maintainability**: Break large context files into logical, focused components
* **Reusability**: Share common instructions across multiple projects
* **Organization**: Create a clear hierarchy of context information
* **Collaboration**: Enable team members to work on different parts of context files
* **Versioning**: Track changes to individual components more effectively

## Basic usage
## Import syntax
Use the `@` symbol followed by the path to the file you want to import:

```markdown
# Main AGENTS.md file

This is the main content.

@./components/instructions.md

More content here.

@./shared/configuration.md
```

### Supported path formats
| Path Format      | Example                      | Description                   |
| ---------------- | ---------------------------- | ----------------------------- |
| Same directory   | `@./file.md`                 | Import from current directory |
| Parent directory | `@../file.md`                | Import from parent directory  |
| Subdirectory     | `@./components/file.md`      | Import from nested directory  |
| Absolute path    | `@/absolute/path/to/file.md` | Import using full path        |

### Working with imports
### Basic import example
```markdown
# My AGENTS.md

Welcome to my project!

@./getting-started.md

## Features

@./features/overview.md
```

## Nested imports
Files can import other files, creating a hierarchical structure:

```markdown
# main.md

@./header.md
@./content.md
@./footer.md
```

Where `header.md` might contain:

```markdown
# Project Header

@./shared/title.md
```

This creates a tree structure of imports:

```
main.md
├── header.md
│    └── shared/title.md
├── content.md
└── footer.md
```

## Safety features
The memory import processor includes several safety mechanisms to prevent common issues:

### Circular import detection
The processor automatically detects and prevents circular imports. If `file-a.md` imports `file-b.md` and `file-b.md` tries to import `file-a.md`, the processor detects this circular reference and prevents it.

**Example circular import:**

```text
file-a.md
↓
file-b.md
↓
file-a.md (circular - blocked)
```

### File access security
The `validateImportPath` function ensures that imports are only allowed from specified directories, preventing access to sensitive files outside the allowed scope.

### Maximum import depth
To prevent infinite recursion, the processor has a configurable maximum import depth (default: 5 levels). Imports can be nested up to 5 levels deep.

## Error handling
The processor handles common errors gracefully:

## Missing files
If a referenced file doesn't exist, the import will fail with an error comment in the output:

```markdown
<!-- Error importing ./non-existent-file.md: File not found -->
```

### File access errors
Permission issues or other file system errors are handled with appropriate error messages:

```markdown
<!-- Error importing ./protected-file.md: Permission denied -->
```

## Advanced features
### Code region detection
The import processor uses the `marked` library to detect code blocks and inline code spans, ensuring that `@` imports inside code regions are ignored:

````markdown
# My document

This is a real import:
@./real-import.md

```markdown
# Code example
This is not a real import:
@./fake-import.md
````

````

### Import tree visualization

The processor generates an import tree that shows the hierarchy of imported files. This tree preserves the order of imports and shows the complete import chain for debugging purposes.

**Example import tree:**

```text
Memory Files
└── project: AGENTS.md
├── components/instructions.md
│    └── components/common/header.md
└── shared/configuration.md
└── shared/settings/defaults.md
````

### Comparison with `/memory` command
| Feature         | Memory Import Processor | `/memory` Command               |
| --------------- | ----------------------- | ------------------------------- |
| Structure       | Hierarchical tree       | Flat, linear document           |
| File boundaries | Seamless integration    | Marked with comments            |
| Import syntax   | `@./path/to/file.md`    | N/A (loads all AGENTS.md files) |
| Control         | Fine-grained inclusion  | Automatic directory scanning    |
| Primary use     | Modular context files   | Project-wide context            |

## API reference
## Core functions
## `processImports()`
```typescript
async function processImports(
content: string,
basePath: string,
debugMode?: boolean,
importState?: ImportState
): Promise
```

Processes import statements in AGENTS.md content.

**Parameters:**

* `content`: The content to process for imports
* `basePath`: The directory path where the current file is located
* `debugMode`: Whether to enable debug logging (default: false)
* `importState`: State tracking for circular import prevention

**Returns:** Object containing processed content and import tree

## `validateImportPath()`
```typescript
function validateImportPath(
importPath: string,
basePath: string,
allowedDirectories: string[]
): boolean
```

Validates import paths to ensure they are safe and within allowed directories.

**Parameters:**

* `importPath`: The import path to validate
* `basePath`: The base directory for resolving relative paths
* `allowedDirectories`: Array of allowed directory paths

**Returns:** Whether the import path is valid

## `findProjectRoot()`
```typescript
async function findProjectRoot(startDir: string): Promise<string>
```

Finds the project root by searching for a `.git` directory upwards from the given start directory.

**Parameters:**

* `startDir`: The directory to start searching from

**Returns:** The project root directory (or the start directory if no `.git` is found)

## Interface definitions
```typescript
interface ProcessImportsResult {
content: string;      // The processed content with imports resolved
importTree: MemoryFile; // Tree structure showing the import hierarchy
}

interface MemoryFile {
path: string;         // The file path
imports?: MemoryFile[]; // Direct imports, in the order they were imported
}
```

## Best practices
## Organization strategies
* Use descriptive file names that clearly indicate the content's purpose
* Create a logical folder structure for your imported files
* Keep imports shallow - avoid deeply nested import chains when possible
* Document your structure with comments explaining the import hierarchy
* Use relative paths when possible for better portability between environments

## Performance considerations
* Minimize the number of imports to reduce processing time
* Keep individual files focused on specific topics or components
* Test your imports regularly to ensure all referenced files exist and are accessible
* Consider caching for frequently used but rarely changed imports

## Troubleshooting
## Common issues
| Issue                    | Possible Causes                         | Solution                                 |
| ------------------------ | --------------------------------------- | ---------------------------------------- |
| Import not working       | File doesn't exist or path is incorrect | Verify file path and existence           |
| Circular import warnings | Files importing each other              | Restructure your imports to avoid cycles |
| Permission errors        | Files not readable                      | Check file permissions                   |
| Path resolution issues   | Relative path confusion                 | Try using absolute paths                 |

## Debugging techniques
Enable debug mode to see detailed logging of the import process:

```typescript
const result = await processImports(content, basePath, true);
```

This outputs detailed information about:

* Files being processed
* Import paths being resolved
* Circular references detected
* Error conditions encountered
# Custom rules

Custom rules influence how Bob Shell responds to your requests in the terminal environment, aligning output with your specific preferences and project requirements. You can control coding style, documentation approach, and decision-making processes.

### What are custom rules?
Custom rules extend Bob Shell's default behavior by defining preferences, constraints, and guidelines that direct how Bob Shell approaches tasks to match your needs when working in the terminal environment.

Common uses include:

* Coding style preferences (indentation, naming conventions)
* Documentation formats and standards
* Testing methodologies and requirements
* Project workflows and processes
* Team-specific conventions

## Rule scopes
Bob Shell supports two rule scopes that determine where your rules apply:

* **Global rules**: Apply automatically across all your projects
* **Workspace rules**: Apply only within your current project

Use global rules for personal or organization-wide standards. Use workspace rules for project-specific requirements.

## Configuration methods
Bob Shell uses the same custom rules system as IBM Bob IDE.

### File-based configuration
The simplest approach uses single files in your workspace root:

* `.bobrules` - General workspace rules
* `.bobrules-code` - Rules for Code mode
* `.bobrules-{modeSlug}` - Rules for any mode

Create a `.bobrules` file:

```bash
# In your project root
echo "Use 4 spaces for indentation" > .bobrules
```

### Directory-based configuration
For better organization, use directories:

**Workspace structure:**

```
.bob/
├── rules/              # General rules
│   └── coding-style.md
└── rules-code/         # Code mode rules
└── typescript.md
```

**Global structure:**

Linux/macOS: `~/.bob/rules/`
Windows: `%USERPROFILE%\.bob\rules\`

Create workspace rules:

```bash
mkdir -p .bob/rules
echo "# Project standards" > .bob/rules/coding-style.md
```

Create global rules:

```bash
# Linux/macOS
mkdir -p ~/.bob/rules
echo "# Global standards" > ~/.bob/rules/coding-standards.md

# Windows
mkdir %USERPROFILE%\.bob\rules
echo # Global standards > %USERPROFILE%\.bob\rules\coding-standards.md
```

## Rule priority
Bob Shell combines rules from multiple sources in this order:

. **Global rules** (`~/.bob/rules/`)
. **Workspace rules** (`.bob/rules/`)

Within each level, mode-specific rules load before general rules. Workspace rules can override global rules.

### Writing effective rules
### Be specific and actionable
**Good**: "Always use relative paths when suggesting file operations in the terminal"

**Avoid**: "Use good paths"

### Use clear structure
Organize rules by topic:

```markdown
# Shell Commands
- Prefer using standard Unix commands over specialized tools when possible
- Include explanations of what each flag does

# Script Style
- Format shell script examples with proper error handling
- Use portable shell syntax that works across bash, zsh, and other common shells

# File Operations
- Always check for file existence before operations in shell scripts
- Always use relative paths when suggesting file operations
```

### Examples of effective rules
* "Always use relative paths when suggesting file operations in the terminal"
* "Prefer using standard Unix commands over specialized tools when possible"
* "Format shell script examples with proper error handling"
* "When suggesting commands, include explanations of what each flag does"
* "Always check for file existence before operations in shell scripts"
* "Use portable shell syntax that works across bash, zsh, and other common shells"

## Advanced configuration
### Mode-specific rules
Target specific modes with dedicated directories:

| Directory       | Purpose                     |
| --------------- | --------------------------- |
| `rules/`        | General rules for all modes |
| `rules-code/`   | Code mode only              |
| `rules-plan/`   | Plan mode only              |
| `rules-{mode}/` | Any custom mode             |

### Using AGENTS.md files
For team standardization, you can use an `AGENTS.md` file in your workspace root:

```markdown
# Team Standards
- Follow company coding guidelines
- Use approved libraries only
- Document all API changes
```

Key details:

* Automatically loaded by default
* Version-control with your project
* Disable with `"bob-shell.useAgentRules": false` in settings
* Loaded after mode-specific rules but before general workspace rules

## File behavior
* **Recursive reading**: Bob Shell reads all files in rules directories, including subdirectories
* **Alphabetical order**: Files process in alphabetical order by filename
* **Automatic filtering**: Excludes cache files (`.DS_Store`, `*.bak`, `*.cache`, `*.log`, `*.tmp`, `Thumbs.db`)
* **Symbolic links**: Supported with maximum depth of 5
* **Empty files**: Silently skipped

## Team standardization
### Project-level standards
Use workspace `.bob/rules/` directories under version control:

```bash
# Add to version control
git add .bob/rules/
git commit -m "Add Bob custom rules"
```

This ensures consistent behavior across team members using Bob Shell for specific projects.

### Organization-wide standards
Distribute global rules to team members:

. Create a shared rules repository
. Team members clone to `~/.bob/rules/`
. Update periodically for consistency

## Hybrid approach
You can combine both approaches:

* Global rules for organization standards
* Workspace rules for project-specific requirements
* Workspace rules override global rules when needed

# Troubleshooting Bob Shell

Find solutions to issues you might encounter when using Bob Shell.

## Authentication
## Certificate errors
**Error:** `Unable to verify certificate`

**Cause:** You are on a corporate network with a firewall that intercepts and inspects SSL/TLS traffic. This requires a custom root CA certificate to be trusted by Node.js.

**Solution:** Set the `NODE_EXTRA_CA_CERTS` environment variable to the absolute path of your corporate root CA certificate file:

```bash
export NODE_EXTRA_CA_CERTS=/path/to/your/corporate-ca.crt
```

## IDE integration
## Connection fails
**Error:** Bob Shell cannot connect to the IDE.

**Cause:** The Bob Shell Companion extension might not be installed or running, or Bob Shell might be running outside the workspace directory.

**Solution:**

. Install the Bob Shell Companion extension in your IDE.
. Navigate to your workspace directory in the terminal.
. Start Bob Shell from the workspace directory.
. Run `/ide enable` in Bob Shell.

### Connection fails in dev container
**Error:** Bob Shell cannot connect to the IDE when running inside a dev container.

**Cause:** The Bob Shell port is not forwarded from the dev container to the host machine.

**Solution:**

. Get the Bob Shell port from the terminal inside the dev container:
```bash
echo $BOB_SHELL_CLI_IDE_SERVER_PORT
```
Example output: `42991`

. Open the Command Palette in your IDE and select **Forward a Port**.

. Add the port shown in step 1 (for example, `42991`).

. Start Bob Shell:
```bash
bob
```

. Enable IDE integration:

```bash
/ide enable
```

Or check the connection status:

```bash
/ide status
```

### Failed to connect to IDE companion extension
**Error:** `🔴 Disconnected: Failed to connect to IDE companion extension`

**Cause:** The Bob Shell Companion extension is not installed, not enabled, or not running in your IDE.

**Solution:**

. Verify the Bob Shell Companion extension is installed and enabled in your IDE.
. Open a new terminal in your IDE.
. Run `/ide enable` in Bob Shell.

### Connection lost unexpectedly
**Error:** `🔴 Disconnected: IDE connection error. The connection was lost unexpectedly`

**Cause:** The IDE connection was interrupted due to a network issue or IDE restart.

**Solution:**

. Run `/ide enable` to reconnect.
. If the issue persists, restart your IDE.

## Directory mismatch
**Error:** `🔴 Disconnected: Directory mismatch`

**Cause:** Bob Shell is running in a different directory than the workspace open in your IDE.

**Solution:**

. Navigate to the same directory that's open in your IDE.
. Restart Bob Shell from that directory.

### No workspace folder open
**Error:** `🔴 Disconnected: To use this feature, please open a workspace folder`

**Cause:** No folder or workspace is open in your IDE.

**Solution:**

. Open a folder or workspace in your IDE.
. Restart Bob Shell.

### IDE integration not supported
**Error:** `IDE integration is not supported in your current environment`

**Cause:** Bob Shell is not running from within a supported IDE's integrated terminal.

**Solution:** Run Bob Shell from within a supported IDE's integrated terminal.

## Configuration issues
### .bobignore not working
**Error:** Bob Shell ignores files you want it to access, or accesses files you want it to ignore.

**Cause:** The `.bobignore` file might have conflicting patterns, incorrect pattern order, or be in the wrong location. Changes might not have taken effect yet.

**Solution:**

. Check your `.bobignore` file for conflicting patterns.
. Ensure more specific patterns (like negations with `!`) come after general patterns.
. Restart your Bob Shell session after making changes to `.bobignore`.
. Use absolute paths if relative paths aren't working as expected.
. Verify the `.bobignore` file is in your project root directory.

### Settings not applying
**Error:** Bob Shell settings changes do not take effect.

**Cause:** The settings file might be in the wrong location, have invalid JSON syntax, or be overridden by higher-priority configuration sources. Changes might not have taken effect yet.

**Solution:**

. Check the settings file location:
* Project settings: `.bob/settings.json` in your project directory
* User settings: `~/.bob/settings.json` in your home directory

. Verify the JSON syntax is valid (use a JSON validator).

. Remember the configuration precedence order:
* Command-line arguments (highest priority)
* Environment variables
* Project settings
* User settings
* System defaults (lowest priority)

. Restart Bob Shell after changing settings files.

### Custom instructions not loading
**Error:** Custom instructions are not being applied to Bob Shell sessions.

**Cause:** Custom instruction files might be in the wrong location, have incorrect file extensions, or not be loaded into the current context.

**Solution:**

. Verify files are in the correct location:
* Workspace-wide: `.bob/rules/` in your project root
* Mode-specific: `.bob/rules-{modeSlug}/` in your project root

. Check that files have the correct extensions (`.md`, `.txt`, or `.xml`).

. Use `/memory refresh` to reload all context files.

. Use `/memory show` to verify the current context.

### Command running issues
### Command not found
**Error:** `command not found: bob`

**Cause:** Bob Shell is not correctly installed or not in your system's `PATH`.

**Solution:**

. Verify Bob Shell is installed:
```bash
which bob
```

. If not found, reinstall Bob Shell using the [installation instructions](/docs/shell/getting-started/install-and-setup).

. Check that your shell's `PATH` includes the Bob Shell installation directory.

### Shell mode not working
**Error:** Shell mode (`!` command) does not run commands.

**Cause:** You might not be typing `!` at an empty prompt, lack necessary permissions, or the command itself might be invalid.

**Solution:**

. Verify you're typing `!` at an empty prompt.
. Check that you have the necessary permissions to run shell commands.
. Try running the command directly in your terminal first to verify it works.

## Performance issues
### Slow response times
**Error:** Bob Shell responds slowly to requests.

**Cause:** Network connectivity issues, too many files loaded as context, or large binary files included in the context.

**Solution:**

. Check your network connection.
. Reduce the number of files in context by using `.bobignore`.
. Avoid including large binary files or directories like `node_modules/`.
. Consider using more specific file references with `@` instead of loading all files.

### High memory usage
**Error:** Bob Shell consumes excessive memory.

**Cause:** Too many files loaded as context, large directories not excluded, or circular imports in memory files.

**Solution:**

. Limit the number of files loaded as context.
. Use `.bobignore` to exclude large directories.
. Restart Bob Shell periodically during long sessions.
. Check for circular imports in your memory files.

### Tips for debugging Bob Shell
### Enable debug mode
To see detailed logging information, start Bob Shell with the debug flag:

```bash
bob --debug
```

Or set the debug flag in your settings:

```json
{
"general": {
"debug": true
}
}
```

### Verify your version of Bob Shell
Run the following command to see your version of Bob Shell:

Interactive session:

```bash
/about
```

Non-interactive session:

```bash
bob --version
```

## Review logs
Check Bob Shell logs for error messages:

* Logs are typically stored in `~/.bob/logs/`
* Look for recent error messages or stack traces
* Share relevant log excerpts when reporting issues
# Configuring proxy settings

Configure proxy settings for Bob Shell in enterprise environments with network restrictions

## Error messages
Connection errors or timeouts when attempting to use Bob Shell, such as:

* "Unable to connect to Bob services"
* "Network request failed"
* "Connection timeout"
* "SSL certificate verification failed"
* "ECONNREFUSED" or "ETIMEDOUT" errors

## Common causes
Your organization requires all HTTP/HTTPS traffic to flow through a proxy server rather than allowing direct connections. This is common in enterprise environments with strict network security policies.

## Solution
Configure proxy settings for Bob Shell using environment variables to route traffic through your organization's proxy server.

### Set proxy environment variables

Open your shell configuration file in a text editor:

* For Bash: `~/.bashrc` or `~/.bash_profile`
* For Zsh: `~/.zshrc`
* For Fish: `~/.config/fish/config.fish`

Add the following environment variables to your shell configuration file:

```bash
export HTTP_PROXY=http://proxy.example.com:8080
export HTTPS_PROXY=https://proxy.example.com:8080
export NO_PROXY=localhost,127.0.0.1
```

Replace `proxy.example.com:8080` with your organization's actual proxy server address and port.

The `NO_PROXY` variable specifies addresses that should bypass the proxy. Add any internal domains or IP addresses that don't require proxy access.

Save the configuration file and reload it:

```bash
# For Bash
source ~/.bashrc

# For Zsh
source ~/.zshrc

# For Fish
source ~/.config/fish/config.fish
```

Verify that the environment variables are set:

```bash
echo $HTTP_PROXY
echo $HTTPS_PROXY
```

Test Bob Shell connectivity by starting a new session.

### Configure SSL certificate validation
If your proxy uses self-signed certificates or custom certificate authorities, you might need to configure SSL certificate validation.

Add SSL configuration to your shell configuration file:

```bash
# Disable strict SSL validation (not recommended for production)
export NODE_TLS_REJECT_UNAUTHORIZED=0

# Or provide custom CA certificates (recommended)
export NODE_EXTRA_CA_CERTS=/path/to/your/ca-bundle.crt
```

Disabling SSL validation (`NODE_TLS_REJECT_UNAUTHORIZED=0`) reduces security and should only be used temporarily. Provide your organization's certificate bundle using `NODE_EXTRA_CA_CERTS`.

Obtain your organization's certificate bundle from your IT team and save it to a secure location.

Update the `NODE_EXTRA_CA_CERTS` path to point to your certificate bundle file.

Reload your shell configuration and test Bob Shell connectivity.

### Temporary proxy configuration
For temporary proxy configuration without modifying your shell configuration file, set the environment variables in your current terminal session:

```bash
export HTTP_PROXY=http://proxy.example.com:8080
export HTTPS_PROXY=https://proxy.example.com:8080
bobshell
```

These settings will only apply to the current terminal session and will be lost when you close the terminal.

# MCP

Bob Shell supports the Model Context Protocol (MCP), allowing you to extend Bob's capabilities by connecting to external services and tools. This guide explains how to configure and use MCP servers with Bob Shell.

### What is MCP?
An MCP (Model Context Protocol) server acts as a bridge between Bob and external services like databases, APIs, or custom scripts. With MCP, you can extend Bob's capabilities beyond its built-in features.

MCP servers provide Bob with:

* **Tool discovery:** Access to available tools with their descriptions and parameters
* **Tool invocation:** The ability to call specific tools with arguments and receive structured responses
* **Resource access:** The capability to read data from specific resources

### Why use MCP?
* **Extend functionality:** Add custom capabilities to Bob that aren't available out-of-the-box
* **Integrate with services:** Connect Bob to your databases, APIs, and other systems
* **Automate workflows:** Create specialized tools for your specific needs
* **Share capabilities:** Distribute useful tools across your team

### Configuring MCP servers
You can manage MCP server configurations at two levels:

. **Global configuration**: Settings in `<USER_HOME>/.bob/mcp_settings.json` apply to all workspaces
. **Project-level configuration**: Settings in `.bob/mcp.json` apply only to the current project

When a server name exists in both global and project configurations, the **project-level configuration takes precedence**.

### Edit MCP settings files
Use Bob IDE, or another text editor, to modify your MCP settings files.

Refer to the following example:

```json
{
"mcpServers": {
"server1": {
"command": "python",
"args": ["/path/to/server.py"],
"env": {
"API_KEY": "your_api_key"
},
"alwaysAllow": ["tool1", "tool2"],
"disabled": false
}
}
}
```

## Configuration properties
Each server configuration requires one of these properties:

* **`command`**: Path to the executable for Stdio transport
* **`url`**: SSE endpoint URL for remote servers
* **`httpURL`**: HTTP endpoint URL for streamable http

Optional properties include:

* **`args`**: Command-line arguments for Stdio transport
* **`headers`**: Custom HTTP headers for SSE transport
* **`env`**: Environment variables for the server process
* **`cwd`**: Working directory for Stdio transport
* **`timeout`**: Request timeout in milliseconds (default: 600,000ms; 10min)
* **`alwaysAllow`**: Tool names to approve automatically
* **`disabled`**: Set to `true` to disable the server

## Transport types
MCP supports two ways to communicate with servers:

## STDIO transport
STDIO transport runs servers locally on your machine:

* Uses standard input/output streams for communication
* Provides lower latency and better security
* Runs as a child process on your machine

Example configuration:

```json
{
"mcpServers": {
"local-server": {
"command": "node",
"args": ["server.js"],
"cwd": "/path/to/project/Bob",
"env": {
"API_KEY": "your_api_key"
},
"alwaysAllow": ["tool1", "tool2"]
}
}
}
```

## SSE transport
SSE transport connects to remote servers over HTTP/HTTPS:

* Uses Server-Sent Events protocol
* Works with servers hosted on different machines
* Requires network access

Example configuration:

```json
{
"mcpServers": {
"remote-server": {
"url": "https://your-server-url.com/mcp",
"headers": {
"Authorization": "Bearer your-token"
},
"alwaysAllow": ["tool3"]
}
}
}
```

### Platform-specific examples
## Windows configuration
```json
{
"mcpServers": {
"puppeteer": {
"command": "cmd",
"args": [
"/c",
"npx",
"-y",
"@modelcontextprotocol/server-puppeteer"
]
}
}
}
```

### macOS and Linux configuration
```json
{
"mcpServers": {
"puppeteer": {
"command": "npx",
"args": [
"-y",
"@modelcontextprotocol/server-puppeteer"
]
}
}
}
```

### Runtime version manager configuration
When using version managers like [asdf](https://asdf-vm.com/) or [mise](https://mise.jdx.dev/):

```json
{
"mcpServers": {
"mcp-batchit": {
"command": "mise",
"args": [
"x",
"--",
"node",
"/Users/myself/workspace/mcp-batchit/build/index.js"
],
"alwaysAllow": [
"search",
"batch_execute"
]
}
}
}
```
# Telemetry data

Learn about the telemetry data Bob Shell can collect, how it's used to improve the product, and how to enable or disable data collection.

IBM Bob collects telemetry data by default. You can opt out to disable telemetry data collection at any time. When enabled:

* Actions are recorded that you take in IBM Bob.
* It helps identify which features are the most useful to users.
* You can enable or disable the collection of this data at any time.

Telemetry data helps IBM learn how to improve Bob. Your cooperation is greatly appreciated.

## Collected data
The table below summarizes the categories of usage data that IBM Bob can collect.

| Event                | Description                                                                              |
| -------------------- | ---------------------------------------------------------------------------------------- |
| Tasks                | Records the action of a task being opened, completed, or reopened.                       |
| Inline completion    | Records the generation, acceptance, or rejection of inline code completion.              |
| DevSecOps scans      | Records the initiation, successful completion, or failure of a scheduled scan.           |
| Task                 | Records confirmation, clarification, or approval of tasks.                               |
| Conversation message | Records the action of a message being sent from you to Bob or Bob to you.                |
| Tool                 | Records the use of tool invocations, such as `execute_command`, `read_file`, and others. |

### How to enable or disable telemetry data collection

With a Bob Shell session open, type `/settings`.

Navigate down and select **Enable Usage Metrics**.

Press <kbd>enter</kbd> to toggle between true (Enabled) and false (Disabled). You can press <kbd>tab</kbd> to save settings globally
or for a user.
# Tools

Learn how Bob Shell uses specialized tools to read files, edit code, run commands, and interact with your development environment from the command line.

## Tool workflow
When you describe what you want to accomplish in natural language, Bob Shell will:

. Select the appropriate tool based on your request.
. Present the tool with its parameters for your review.
. Run the approved tool and show you the results.
. Continue this process until your task is complete.

## Tool categories
Bob Shell's tools are organized into categories based on their primary function. Understanding these categories helps you know what Bob Shell can do and how it accomplishes tasks.

## Read tools
Access file content and understand code structure without making changes.

| Tool                         | Purpose                                                     | Example use                                         |
| :--------------------------- | :---------------------------------------------------------- | :-------------------------------------------------- |
| `read_file`                  | Read the contents of one or more files                      | View configuration files, examine source code       |
| `search_files`               | Search for patterns across multiple files using regex       | Find all TODO comments, locate function definitions |
| `list_files`                 | List files and directories in a specified path              | Explore project structure, find specific file types |
| `list_code_definition_names` | Extract class, function, and method names from source files | Get an overview of code organization                |

**When Bob Shell uses read tools:** When you ask Bob Shell to review code, find specific patterns, or understand project structure.

## Write tools
Create new files or modify existing code with precision.

| Tool             | Purpose                                                  | Example use                                         |
| :--------------- | :------------------------------------------------------- | :-------------------------------------------------- |
| `write_to_file`  | Create a new file or completely rewrite an existing file | Generate new components, create configuration files |
| `apply_diff`     | Make targeted changes to specific sections of a file     | Update function logic, fix bugs, refactor code      |
| `insert_content` | Add new lines at a specific location in a file           | Add imports, insert new functions                   |

**When Bob Shell uses write tools:** When you ask Bob Shell to create files, implement features, fix bugs, or refactor code.

## Command tools
Run commands and perform system operations in your terminal.

| Tool              | Purpose                            | Example use                                     |
| :---------------- | :--------------------------------- | :---------------------------------------------- |
| `execute_command` | Run CLI commands in your workspace | Install dependencies, run tests, build projects |

**When Bob Shell uses command tools:** When you ask Bob Shell to run commands, install packages, run scripts, or complete system operations.

## MCP tools
Extend Bob Shell's capabilities through Model Context Protocol servers.

| Tool           | Purpose                                        | Example use                                                   |
| :------------- | :--------------------------------------------- | :------------------------------------------------------------ |
| `use_mcp_tool` | Access tools provided by connected MCP servers | Query databases, interact with APIs, access external services |

**When Bob Shell uses MCP tools:** When you've configured MCP servers and need to access their specialized capabilities.

## Mode tools
Switch between different Bob Shell modes for specialized tasks.

| Tool          | Purpose                                            | Example use                                                               |
| :------------ | :------------------------------------------------- | :------------------------------------------------------------------------ |
| `switch_mode` | Change to a different mode (Code, Plan, Ask, etc.) | Switch to Plan mode for architecture design, Code mode for implementation |

**When Bob Shell uses mode tools:** When the current task would benefit from a different mode's specialized capabilities.

## Question tools
Gather additional information needed to complete tasks.

| Tool                    | Purpose                                              | Example use                                                              |
| :---------------------- | :--------------------------------------------------- | :----------------------------------------------------------------------- |
| `ask_followup_question` | Request clarification or additional details from you | Ask about preferred implementation approach, request missing information |

# Keyboard shortcuts

Find all available shortcuts by function, with platform-specific variations where applicable.

## Quick reference
| Category                                    | Shortcut                                                                                    | Purpose                       |
| ------------------------------------------- | ------------------------------------------------------------------------------------------- | ----------------------------- |
| [Navigation](#navigation-shortcuts)         | <kbd>Ctrl+A</kbd> , <kbd>Ctrl+E</kbd> , <kbd>↑</kbd> <kbd>↓</kbd> <kbd>←</kbd> <kbd>→</kbd> | Move through text and history |
| [Editing](#editing-shortcuts)               | <kbd>Ctrl+K</kbd> , <kbd>Ctrl+U</kbd> , <kbd>Ctrl+W</kbd>                                   | Modify and edit text          |
| [Application control](#application-control) | <kbd>Ctrl+C</kbd> , <kbd>Ctrl+D</kbd> , <kbd>Ctrl+L</kbd>                                   | Control Bob Shell's behavior  |
| [Special features](#special-features)       | <kbd>Ctrl+Y</kbd> , <kbd>Ctrl+T</kbd> , <kbd>Ctrl+G</kbd>                                   | Access advanced functionality |

## Navigation shortcuts
Efficiently move through text and command history:

## Cursor movement
| Shortcut                                   | macOS Alternative             | Description                      |
| ------------------------------------------ | ----------------------------- | -------------------------------- |
| <kbd>Ctrl+A</kbd> / <kbd>Home</kbd>        |                               | Move cursor to beginning of line |
| <kbd>Ctrl+E</kbd> / <kbd>End</kbd>         |                               | Move cursor to end of line       |
| <kbd>Ctrl+B</kbd> / <kbd>Left Arrow</kbd>  |                               | Move cursor one character left   |
| <kbd>Ctrl+F</kbd> / <kbd>Right Arrow</kbd> |                               | Move cursor one character right  |
| <kbd>Ctrl+Left Arrow</kbd>                 | <kbd>Option+Left Arrow</kbd>  | Move cursor one word left        |
| <kbd>Ctrl+Right Arrow</kbd>                | <kbd>Option+Right Arrow</kbd> | Move cursor one word right       |

## History navigation
| Shortcut              | Alternative       | Description                         |
| --------------------- | ----------------- | ----------------------------------- |
| <kbd>Up Arrow</kbd>   | <kbd>Ctrl+P</kbd> | Navigate up through input history   |
| <kbd>Down Arrow</kbd> | <kbd>Ctrl+N</kbd> | Navigate down through input history |

## Editing shortcuts
Efficiently edit and modify text:

## Text deletion
| Shortcut                                      | Description                             |
| --------------------------------------------- | --------------------------------------- |
| <kbd>Ctrl+H</kbd> / <kbd>Backspace</kbd>      | Delete character to left of cursor      |
| <kbd>Ctrl+D</kbd> / <kbd>Delete</kbd>         | Delete character to right of cursor     |
| <kbd>Ctrl+W</kbd> / <kbd>Ctrl+Backspace</kbd> | Delete word to left of cursor           |
| <kbd>Ctrl+Delete</kbd>                        | Delete word to right of cursor          |
| <kbd>Ctrl+U</kbd>                             | Delete from cursor to beginning of line |
| <kbd>Ctrl+K</kbd>                             | Delete from cursor to end of line       |

## Text manipulation
| Shortcut                                 | Description                             |
| ---------------------------------------- | --------------------------------------- |
| <kbd>\\</kbd> + <kbd>Enter</kbd>         | Insert a newline (when at end of line)  |
| <kbd>Ctrl+V</kbd>                        | Paste clipboard content (text or image) |
| <kbd>Ctrl+X</kbd> / <kbd>Alt+Enter</kbd> | Open current input in external editor   |
| <kbd>Tab</kbd>                           | Autocomplete current suggestion         |
| <kbd>Esc</kbd> (double press)            | Clear the input prompt                  |

## Application control
Control Bob Shell's core functionality:

## Session management
| Shortcut          | Description                                                                 |
| ----------------- | --------------------------------------------------------------------------- |
| <kbd>Ctrl+C</kbd> | Cancel ongoing request and clear input<br />Press twice to exit application |
| <kbd>Ctrl+D</kbd> | Exit application (when input is empty)<br />Press twice to confirm          |
| <kbd>Ctrl+L</kbd> | Clear the screen                                                            |
| <kbd>Esc</kbd>    | Close dialogs and suggestions                                               |

## Mode toggles
| Shortcut                    | Description                                                         |
| --------------------------- | ------------------------------------------------------------------- |
| <kbd>!</kbd> (empty prompt) | Toggle shell mode                                                   |
| <kbd>Ctrl+Y</kbd>           | Toggle auto-approval (YOLO mode) for all tool calls                 |
| <kbd>Ctrl+O</kbd>           | Toggle debug console display                                        |
| <kbd>Ctrl+S</kbd>           | Toggle response truncation<br />Allow long responses to print fully |
| <kbd>Ctrl+T</kbd>           | Toggle tool descriptions display                                    |
| <kbd>Tab</kbd>              | Cycle through available modes                                       |

## Special features
Access specialized functionality:

### Suggestions and selections
| Shortcut                          | Description                       |
| --------------------------------- | --------------------------------- |
| <kbd>Tab</kbd> / <kbd>Enter</kbd> | Accept selected suggestion        |
| <kbd>Up Arrow</kbd>               | Navigate up through suggestions   |
| <kbd>Down Arrow</kbd>             | Navigate down through suggestions |

### Radio button selection
| Shortcut                             | Description                                   |
| ------------------------------------ | --------------------------------------------- |
| <kbd>Up Arrow</kbd> / <kbd>k</kbd>   | Move selection up                             |
| <kbd>Down Arrow</kbd> / <kbd>j</kbd> | Move selection down                           |
| <kbd>Enter</kbd>                     | Confirm selection                             |
| <kbd>1-9</kbd>                       | Select item by number                         |
| Multi-digit numbers                  | Press digits in quick succession for items >9 |

## IDE integration
| Shortcut          | Description                    |
| ----------------- | ------------------------------ |
| <kbd>Ctrl+G</kbd> | View context received from IDE |

### Platform-specific notes
## macOS users
On macOS, many shortcuts use the <kbd>Option</kbd> key (also called <kbd>Alt</kbd> or <kbd>⌥</kbd>) instead of <kbd>Ctrl</kbd> for word-based operations:

* Use <kbd>Option+Left/Right</kbd> instead of <kbd>Ctrl+Left/Right</kbd> for word movement
* Use <kbd>Option+Backspace</kbd> instead of <kbd>Ctrl+W</kbd> for deleting the previous word

## Terminal compatibility
Some shortcuts may be intercepted by your terminal emulator before reaching Bob Shell:

* If <kbd>Ctrl+C</kbd> doesn't work as expected, try <kbd>Ctrl+C</kbd> twice
* If <kbd>Ctrl+Left/Right</kbd> doesn't work, check your terminal's keyboard settings

### Keyboard shortcut tips
* **Learn incrementally:** Start with basic navigation (<kbd>Ctrl+A</kbd> , <kbd>Ctrl+E</kbd> , <kbd>↑</kbd> <kbd>↓</kbd> 

# Ignoring files

Control which files Bob Shell can access by creating a `.bobignore` file in your project.

## Overview
| Feature          | Description                                                    | Benefit                        |
| ---------------- | -------------------------------------------------------------- | ------------------------------ |
| Path exclusion   | Prevent Bob Shell from accessing specific files or directories | Protect sensitive information  |
| Pattern matching | Use glob patterns to match multiple files                      | Efficiently exclude file types |
| Negation support | Override exclusions for specific files                         | Fine-grained control           |
| Project-specific | Each project can have its own exclusion rules                  | Customized to project needs    |

### Why use .bobignore?
* **Privacy protection:** Keep sensitive files like API keys and credentials private
* **Performance improvement:** Exclude large binary files or directories that slow down operations
* **Noise reduction:** Remove irrelevant files from Bob Shell's context
* **Focus control:** Direct Bob Shell's attention to the most relevant parts of your codebase

### How .bobignore works
When you add paths to your `.bobignore` file, Bob Shell tools that respect this file will automatically exclude matching files and directories from their operations. For example, when using the `read_many_files` command, any paths in your `.bobignore` file will be skipped.

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│                 │     │                 │     │                 │
│  Your project   │────▶│  .bobignore     │────▶│  Files Bob Shell │
│  files          │     │  filter         │     │  can access     │
│                 │     │                 │     │                 │
└─────────────────┘     └─────────────────┘     └─────────────────┘
```

Changes to your `.bobignore` file require restarting your Bob Shell session to take effect.

## Pattern syntax
The `.bobignore` file follows the same pattern syntax as `.gitignore`:

| Pattern         | Description                          | Example                    |
| --------------- | ------------------------------------ | -------------------------- |
| `file.txt`      | Matches a specific file              | `apikeys.txt`              |
| `*.ext`         | Matches all files with the extension | `*.log`                    |
| `/dir/`         | Matches a directory                  | `/node_modules/`           |
| `/path/to/file` | Anchors the path to the root         | `/src/config/secrets.json` |
| `!pattern`      | Negates a previous pattern           | `!important.md`            |
| `#`             | Comment line                         | `# API credentials`        |

## Getting started
### Creating a .bobignore file
. Create a file named `.bobignore` in the root of your project directory
. Add patterns for files and directories you want to exclude
. Save the file and restart your Bob Shell session

```bash
# From your project root
touch .bobignore
echo "# Files to ignore" > .bobignore
echo "secrets/" >> .bobignore
```

### Common exclusion patterns
Here are some common patterns you might want to add to your `.bobignore` file:

```sh
# Sensitive information
.env
secrets/
*password*
*credential*
*apikey*

# Large directories
node_modules/
.git/
dist/
build/

# Binary and media files
*.zip
*.tar.gz
*.mp4
*.jpg
*.png

# Log files
*.log
logs/
```

## Pattern examples
### Excluding specific directories
To exclude entire directories and their contents:

```sh
# Exclude the packages directory and all its contents
/packages/

# Exclude all node_modules directories anywhere in the project
**/node_modules/

# Exclude the dist directory but only at the root level
/dist/
```

## Using wildcards
Wildcards let you match multiple files with similar patterns:

```sh
# Exclude all markdown files
*.md

# Exclude all JavaScript files in the src directory
/src/**/*.js

# Exclude all files with "test" in their name
*test*
```

## Using negation
You can override exclusions for specific files:

```sh
# Exclude all markdown files
*.md

# But include README.md
!README.md

# And include all markdown files in the docs directory
!docs/*.md
```

## Advanced usage
## Combining patterns
You can create sophisticated exclusion rules by combining patterns:

```sh
# Exclude all JavaScript files
*.js

# But include all files in the src directory
!src/**/*.js

# Except for test files in the src directory
src/**/*.test.js
```

### Debugging your .bobignore file
If Bob Shell seems to be ignoring files you want it to access, or accessing files you want it to ignore:

. Check your `.bobignore` file for conflicting patterns
. Remember that more specific patterns (like negations) should come after general patterns
. Restart your Bob Shell session after making changes
. Use absolute paths if relative paths aren't working as expected

## Best practices
* **Start simple:** Begin with a few essential patterns and add more as needed
* **Comment your patterns:** Add comments to explain why certain files are excluded
* **Be specific:** Use precise patterns to avoid accidentally excluding important files
* **Consider security:** Always exclude files containing sensitive information
* **Maintain regularly:** Update your `.bobignore` file as your project evolves<kbd>←</kbd> <kbd>→</kbd>)
* **Practice deletion shortcuts:** <kbd>Ctrl+U</kbd> and <kbd>Ctrl+K</kbd> are particularly useful
* **Use history navigation:** <kbd>Up/Down</kbd> arrows save time when repeating commands
* **Try shell mode:** Type <kbd>!</kbd> at an empty prompt to quickly run shell commands
* **Customize your terminal:** Some terminals allow remapping keys for better compatibility

# Checkpointing

Create automatic snapshots of your project before applying changes.

### Why use checkpointing?
* **Safety net:** Experiment with code changes without fear of breaking your project
* **Easy rollback:** Restore your project to a previous state with a single command
* **Conversation preservation:** Return to the exact conversation context when a change was made
* **Non-intrusive:** Works alongside your existing Git workflow without interference

### How checkpointing works
When you approve a file-modifying operation (such as `write_file` or `replace`), Bob Shell automatically:

. **Creates a Git snapshot** in a shadow repository (`~/.bob/history/<project_hash>`)
> **Note:** This shadow repository is separate from your project's Git repository and won't interfere with your normal Git workflow.

. **Saves your conversation history** up to that point

. **Records the tool call** that was about to run

This three-part checkpoint allows you to:

* Revert all files to their previous state
* Resume the conversation from where you were
* Review and potentially re-run the original tool call

All checkpoint data is stored locally on your machine:

* Git snapshots in `~/.bob/history/<project_hash>`
* Conversation history and tool calls in `~/.bob/tmp/<project_hash>/checkpoints`

## Enabling checkpointing
Checkpointing is disabled by default. You can enable it in two ways:

### Settings file (persistent)
To enable checkpointing for all sessions:

. Edit your `settings.json` file
. Add the following configuration:

```json
{
"general": {
"checkpointing": {
"enabled": true
}
}
}
```

## Managing checkpoints
### Viewing available checkpoints
To see all saved checkpoints for your current project:

```bash
/restore
```

Bob Shell will display a list of checkpoint files with names that include:

* Timestamp
* Modified file name
* Tool that was used

Example: `2025-06-22T10-00-00_000Z-my-file.txt-write_file`

### Restoring a checkpoint
To restore your project to a specific checkpoint:

```bash
/restore <checkpoint_file>
```

For example:

```bash
/restore 2025-06-22T10-00-00_000Z-my-file.txt-write_file
```

This will:

. Revert your files to their previous state
. Restore the conversation history
. Re-propose the original tool call

## Best practices
* **Enable for critical work:** Always use checkpointing when working on important code
* **Test before relying:** Try the restore functionality on a non-critical project first
* **Regular commits:** Continue making regular Git commits for meaningful changes
* **Checkpoint cleanup:** Periodically remove old checkpoints you no longer need
* **Combine with version control:** Use checkpointing alongside your normal Git workflow for maximum safety

## Limitations
* Checkpointing only tracks files that are already under version control or have been modified by Bob Shell
* External changes made outside of Bob Shell won't be included in checkpoints
* Very large projects may experience slightly slower checkpoint creation times

# Instance command

Switch between IBM instances and teams using the /instance command.

Type `/instance` to open an interactive selection dialog where you can view and select from your available instances and teams.

### Why use the instance command?
* **Multi-instance management**: Switch between different IBM instances.
* **Team selection**: Enterprise users can select specific teams within instances.
* **Budget visibility**: View budget limits and current usage for each instance or team.
* **Persistent configuration**: Your selection persists across sessions.

### How the instance command works
When you type `/instance`, an interactive table appears showing your available instances and teams. Navigate through options using keyboard controls and select the instance or team you want to use.

### Switching instances or teams

Type `/instance` to open the selection dialog.

Navigate through available options using the arrow keys.

| Key       | Action                                    |
| --------- | ----------------------------------------- |
| `↑` / `↓` | Navigate through options                  |
| `Enter`   | Select highlighted option                 |
| `Esc`     | Cancel (disabled during first-time setup) |

The highlighted option is marked with `●`, and your current selection is marked with `★`.

Press `Enter` to select the highlighted instance or team.

A success message displays your selection. Authentication refreshes automatically, and your selection persists across sessions.

During first-time setup, you must select an instance before you can use Bob Shell. The `Esc` key is disabled until you complete the initial configuration.

### Understanding the instance table
The instance selection table displays different information depending on your plan type:

| Column              | Description                                                     |
| ------------------- | --------------------------------------------------------------- |
| Selection indicator | `●` for highlighted option, `★` for current selection, or empty |
| INSTANCE            | IBM instance name                                               |
| PLAN                | Subscription plan type                                          |
| TEAM                | Team name (enterprise users only)                               |
| BUDGET              | Budget limit: number, `∞` (unlimited), or `n/a`                 |
| USAGE               | Current usage with 2 decimal places                             |

**Enterprise plan:**

Enterprise users see additional team information:

* TEAM column shows available teams
* Select from multiple teams per instance
* Budget and usage displayed per team
* Format: `Instance Name (Plan) : Team Name`

**Individual plan:**

Individual plan users work with instances only:

* No TEAM column
* Select instances without team options
* Budget and usage displayed per instance
* Format: `Instance Name (Plan)`

## Troubleshooting

**Cannot cancel during first-time setup:**

If you cannot press `Esc` to cancel:

* This is expected behavior during first-time setup.
* You must select an instance before you can use Bob Shell.
* After initial configuration, `Esc` will be available for future selections.

**Instance not appearing:**

If an expected instance is not showing in the list:

* Verify you have access to the instance through your IBM account.
* Check that your authentication is current.
* Contact your administrator if you believe you should have access.

**Selection not persisting:**

If your instance selection does not persist across sessions:

* Ensure Bob Shell has write permissions to its configuration directory.
* Check that your configuration files are not being reset by other processes.
* Try selecting the instance again and verify the success message appears.

# Sandboxing

Isolate Bob Shell operations in a secure sandbox environment to protect your host system.

Sandboxing isolates potentially dangerous operations, such as shell commands or file modifications, from your host system.

### Why use sandboxing?
Sandboxing provides several key benefits:

* **Isolation**: Limits file system access to your project directory only
* **Consistency**: Ensures reproducible environments across different systems
* **Safety**: Reduces risk when experimenting with untrusted code or commands

## Sandboxing methods
Choose from the following options to create a sandbox environment:

### macOS Seatbelt (macOS only)
You can use the default `sandbox-exec` utility.

**Default profile**: `permissive-open` - restricts writes outside your project directory while allowing most other operations.

### Container-based (Docker/Podman)
Cross-platform sandboxing with complete process isolation using Docker or Podman containers.

Container-based sandboxing requires building the sandbox image locally or using a published image from your organization's registry.

## Configuration
You can enable sandboxing through command flags, environment variables, or configuration files.

## Command flags
Enable sandboxing for a single command using the `-s` or `--sandbox` flag. Use command flags for one-time testing or when you need sandboxing for a specific command without affecting your default workflow.

```bash
bob -s "analyze this shell script for potential security issues before execution"
```

## Environment variables
Set the `BOB_SHELL_SANDBOX` environment variable. Use environment variables when you want sandboxing enabled for an entire terminal session or specific project without modifying configuration files.

```bash
export BOB_SHELL_SANDBOX=true
bob "analyze this shell script for potential security issues before execution"
```

You can also specify the sandbox type:

```bash
export BOB_SHELL_SANDBOX=docker  # or podman, or sandbox-exec
```

## Settings file
Add the `sandbox` option to the `tools` object in your `settings.json`. Use the settings file for persistent, project-wide sandboxing that applies to all team members and sessions.

```json
{
"tools": {
"sandbox": true
}
}
```

You can also use a specific sandbox type:

```json
{
"tools": {
"sandbox": "docker"
}
}
```

**Configuration precedence** (highest to lowest):

. Command flag (`-s` or `--sandbox`)
. Environment variable (`BOB_SHELL_SANDBOX`)
. Settings file (`settings.json`)

### macOS Seatbelt profiles
Control the level of restriction using the `SEATBELT_PROFILE` environment variable:

| Profile                     | Network   | Write access         | Use case               | When to use                                                                                                                                                                       |
| --------------------------- | --------- | -------------------- | ---------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `permissive-open` (default) | Allowed   | Project only         | General development    | Everyday development work where you need network access and reasonable security without significant restrictions.                                                                 |
| `permissive-closed`         | Blocked   | Project only         | Offline work           | Working offline or in environments where network access should be blocked, but you still need full project directory access.                                                      |
| `permissive-proxied`        | Via proxy | Project only         | Corporate environments | Corporate environments where all network traffic must route through a proxy server. Best for enterprise settings with strict network policies.                                    |
| `restrictive-open`          | Allowed   | Strict limits        | High security          | High security with network access, but strict file system limitations beyond the project directory. Ideal for working with potentially malicious code that needs internet access. |
| `restrictive-closed`        | Blocked   | Maximum restrictions | Maximum security       | Maximum security. Use when working with untrusted or potentially dangerous code that requires complete isolation from your network and file system.                               |

Example:

```bash
export SEATBELT_PROFILE=restrictive-open
bob -s "analyze this shell script for potential security issues before execution"
```

### Custom sandbox flags
For container-based sandboxing, inject custom flags into the `docker` or `podman` command using the `SANDBOX_FLAGS` environment variable. Use this when you need more control over container resources, security settings, or volume mounts, best for advanced users who need to customize memory limits, CPU allocation, or SELinux configurations for specific workloads.

**Single flag example:**

```bash
export SANDBOX_FLAGS="--security-opt label=disable"
bob -s "analyze this shell script for potential security issues before execution"
```

**Multiple flags example:**

```bash
export SANDBOX_FLAGS="--memory=4g --cpus=2"
bob -s "analyze this shell script for potential security issues before execution"
```

This is useful for:

* Disabling SELinux labeling on Podman.
* Setting resource limits.
* Configuring network settings.
* Adding custom volume mounts.

### Linux UID/GID handling
Bob Shell automatically handles user permissions on Linux to ensure files created in the sandbox have the correct ownership. Use this to control file ownership mapping between the container and host system, best for Linux environments where you need to ensure files created in the sandbox have correct permissions or when troubleshooting permission issues.

Override this behavior if needed:

```bash
# Force host UID/GID mapping
export SANDBOX_SET_UID_GID=true
```

```bash
# Disable UID/GID mapping
export SANDBOX_SET_UID_GID=false
```

## Limitations
While sandboxing significantly improves security, it has some important limitations to be aware of:

## General:
* **Not a complete security solution**: Sandboxing reduces but doesn't eliminate all risks. Always review Bob's actions before approval.
* **Performance overhead**: Container-based sandboxing has minimal overhead after the initial build, but operations may be slightly slower than native execution.
* **GUI applications**: Graphical applications typically won't work inside sandboxes due to display isolation.

## File system:
* **Project directory only**: By default, only your current project directory is accessible inside the sandbox.
* **Symbolic links**: Symlinks pointing outside the project directory may not work correctly.
* **File permissions**: Permission issues may occur with files created inside the sandbox, especially on Linux.

## Network:
* **Restricted profiles**: Some Seatbelt profiles (`permissive-closed`, `restrictive-closed`) block all network access.
* **Localhost services**: Services running on your host machine may not be accessible from inside container sandboxes without additional configuration.
* **VPN connections**: Container sandboxes may not have access to VPN-connected resources.

### Tools and commands:
* **Missing tools**: The sandbox environment may not have all tools installed that are available on your host system. You can add tools via custom Dockerfile or `sandbox.bashrc`.
* **System commands**: Commands that require system-level access (e.g., `sudo`, system service management) won't work in sandboxes.
* **Hardware access**: Direct hardware access (USB devices, GPUs, etc.) is typically not available in sandboxes.

## macOS Seatbelt:
* **macOS only**: Seatbelt sandboxing only works on macOS systems.
* **Less isolation**: Provides less isolation than container-based approaches.
* **Profile restrictions**: Some operations may be blocked depending on the chosen profile.


