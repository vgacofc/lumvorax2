voici la cle api sur https://arcprize.org/platform/user :  b6ebc9c5-a296-4578-be69-0cc16c9455d4  


> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# ARC-AGI-3 Quickstart

> ARC-AGI-3 is an Interactive Reasoning Benchmark designed to measure an AI Agent's ability to generalize in novel, unseen environments.

<div style={{ display: 'flex', alignItems: 'flex-start', gap: '1rem' }}>
  <div style={{ flex: 1 }}>
    <p>
      Traditionally, to measure AI, static benchmarks have been the yardstick.
      These work well for evaluating LLMs and AI reasoning systems. However, to evaluate frontier AI agent systems, we
      need new tools that measure:
    </p>

    <ul>
      <li>Exploration</li>
      <li>Percept → Plan → Action</li>
      <li>Memory</li>
      <li>Goal Acquisition</li>
      <li>Alignment</li>
    </ul>

    <p>
      By building agents that can play ARC-AGI-3, you're directly contributing
      to the frontier of AI research. <br /><br /> Learn more about{' '}
      <a href="https://arcprize.org/arc-agi/3">ARC-AGI-3</a>.
    </p>
  </div>

  <div style={{ flex: 1, textAlign: 'center' }}>
    <img src="https://mintcdn.com/arcprizefoundation/sx3SsV7kmM_q56IF/images/Ls20Human.gif?s=61025c7aeb245af080aba9e735a6f1cf" alt="Human playing LS20" width="512" height="512" data-path="images/Ls20Human.gif" />

    <p>
      Can you build an agent to beat{' '}
      <a href="https://arcprize.org/tasks/ls20">this game</a>?
    </p>
  </div>
</div>

## Play your first ARC-AGI-3 environment

### 1. Install the [ARC-AGI Toolkit](https://github.com/arcprize/arc-agi)

```bash theme={null}
uv init
uv add arc-agi
# or
pip install arc-agi
```

### 2. Set your `ARC_API_KEY`

Optionally set your `ARC_API_KEY`. If no key is provided, an anonymous key will be used. However, registering for an API key will give you access to public games at release. [Get an ARC\_API\_KEY](/api-keys)

```bash theme={null}
export ARC_API_KEY="your-api-key-here"
# or
echo 'ARC_API_KEY=your-api-key-here' > .env
```

### 3. Play your first game

Create a file called `my-play.py`:

```python theme={null}
import arc_agi
from arcengine import GameAction

arc = arc_agi.Arcade()
env = arc.make("ls20", render_mode="terminal")

# Take a few actions
for _ in range(10):
    env.step(GameAction.ACTION1)

print(arc.get_scorecard())
```

Run it:

```bash theme={null}
python play.py
```

You should see the game render in your terminal and a scorecard with your results.

🎉 Congratulations! You just played your first ARC-AGI-3 environment programatically.

Do you feel the AGI yet?

## Next Steps

After running your first environment:

1. **Make it fast** - Use `env = arc.make("ls20")` without `render_mode` to hit +2K FPS
2. **Try a different game** - Run `env = arc.make("ft09", render_mode="terminal")` to play a another game. See a list of games available at [arcprize.org/tasks](https://arcprize.org/tasks) or via the [ARC-AGI Toolkit](/toolkit/list-games)
3. **Use an agent** - Explore [agent templates](/llm_agents) or [create your own agent](/agents-quickstart).
4. **Explore the ARC-AGI Toolkit** - [The ARC-AGI Toolkit](./toolkit/overview) allows quick and easy integration with ARC-AGI Environments.


> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# API Keys

> How to get and use your ARC-AGI-3 API key

## Why Get an API Key?

Registering for an API key allows you to:

* **Track your progress** across games and sessions
* **Access the full list of games** when launch goes out

## How to Get Your API Key

1. Go to [arcprize.org/platform](https://arcprize.org/platform)

2. Register by logging in with either **Google** or **GitHub**

3. Click on your **user profile** in the top right corner

4. In your user profile, find the **API Keys** section

5. Create a new key. This is your `ARC_AGI_API` key

Once you have your key, set it in your enviornment or `.env` and you'll have access to the entire set of public games once available on the platform.

## Using Your API Key

Set your API key as an environment variable:

```bash theme={null}
export ARC_API_KEY="your-api-key-here"
```

Or add it to a `.env` file in your project:

```bash theme={null}
echo 'ARC_API_KEY=your-api-key-here' > .env
```

The toolkit will automatically load your key from the environment when you create an `Arcade` instance:

```python theme={null}
import arc_agi

# Automatically uses ARC_API_KEY from environment
arc = arc_agi.Arcade()

# Or pass the API key explicitly
arc = arc_agi.Arcade(arc_api_key="your-api-key-here")
```
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Game Schema

> Structure and format of ARC-AGI-3 game environments

ARC-AGI-3 games are turn-based environments where agents interact with 2D grids through a standardized action interface. Each game maintains state through discrete action-response cycles.

* Agents receive 1-N frames of JSON objects with the game state and metadata.
* Agents respond with an [action](/actions) to interact with the game.

## Grid Structure

* **Dimensions:** Maximum 64x64 grid size
* **Cell Values:** Integer values 0-15 representing different states/colors
* **Coordinate System:** (0,0) at top-left, (x,y) format

## Game ID Format

Game IDs are formatted as `<game_name>`-`<version>`.

`game_names` are stable, but `version` may change as games update.

## Game Available Actions

Each game provides an explicit set of available actions. The actions available vary per game and are stated explicitly so your agent knows what it can do.

To learn about the standardized action interface, see the [Actions](/actions) page.

To see how to retrieve a game's available actions programmatically, see [List Available Actions](/toolkit/list-actions).

> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Available Games

> List of ARC-AGI-3 games and how to discover them

ARC-AGI-3 consists of a series of public games that are playable by both humans and AI agents.

## Discovering Games

To see a list of available games:

* Browse games at [arcprize.org/tasks](https://arcprize.org/tasks)
* Use the ARC-AGI Toolkit to [list games programmatically](/toolkit/list-games)

By default, three games are available to anonymous users after launch. An API key is <u>required</u> to access the remaining public games.

[Get a free API key](/api-keys) to unlock them.

## Example Games

* [ls20](https://arcprize.org/tasks/ls20) - Agent reasoning
* [ft09](https://arcprize.org/tasks/ft09) - Elementary Logic
* [vc33](https://arcprize.org/tasks/vc33) - Orchestration


> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Actions

> Input interface for ARC-AGI-3 games

All games implement a standardized action interface with seven core actions:

| Action    | Description                                                                                   |
| --------- | --------------------------------------------------------------------------------------------- |
| `RESET`   | Initialize or restarts the game/level state                                                   |
| `ACTION1` | Simple action - varies by game (semantically mapped to up)                                    |
| `ACTION2` | Simple action - varies by game (semantically mapped to down)                                  |
| `ACTION3` | Simple action - varies by game (semantically mapped to left)                                  |
| `ACTION4` | Simple action - varies by game (semantically mapped to right)                                 |
| `ACTION5` | Simple action - varies by game (e.g., interact, select, rotate, attach/detach, execute, etc.) |
| `ACTION6` | Complex action requiring x,y coordinates (0-63 range)                                         |
| `ACTION7` | Simple action - Undo (e.g., interact, select)                                                 |

### Human Player Keybindings

When playing games manually in the ARC-AGI-3 UI, you can use these keyboard shortcuts instead of clicking action buttons:

| Control Scheme     | ACTION1 | ACTION2 | ACTION3 | ACTION4 | ACTION5 | ACTION6     | ACTION7    |
| ------------------ | ------- | ------- | ------- | ------- | ------- | ----------- | ---------- |
| **WASD + Space**   | `W`     | `S`     | `A`     | `D`     | `Space` | Mouse Click | CTRL/CMD+Z |
| **Arrow Keys + F** | `↑`     | `↓`     | `←`     | `→`     | `F`     | Mouse Click | CTRL/CMD+Z |

All control schemes support mouse clicking for ACTION6 (coordinate-based actions). Choose whichever scheme feels most comfortable for your playstyle.

### Game-over state

When a game reaches a game-over state, the only valid action is `RESET`. Sending any other action (e.g., `ACTION1` through `ACTION7`) to a game in this state returns a `400 Bad Request` error.

If you encounter a `400` error during gameplay, check whether the game has ended and issue a `RESET` to start a new game.

<Note>A `400` error indicates the game is in a game-over state. A `500` error, by contrast, indicates a server-side issue.</Note>

### Available actions

Each game explicitly defines the set of available actions that can be used within that game. This approach ensures clarity for both human and AI participants by making it clear which actions are permitted, thereby reducing confusion. In the human-facing UI, available actions are visually highlighted or dismissed to provide the same affordance.

For each action taken, the metadata of the returned frame will indicate which actions are available. Agents may use this information to narrow the action space and develop effective strategies for completing the game.

Note: Action 6 does not provide explicit X/Y coordinates for active areas. If Action 6 is available, only its availability will be indicated, without specifying which coordinates are active.
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Recordings & Replays

> Viewing your agent's gameplay

Recordings let you view and share your agent's gameplay sessions.

## When Recordings Are Available

| Method            | Recordings Available                                          |
| ----------------- | ------------------------------------------------------------- |
| **API**           | Yes — viewable online via scorecard                           |
| **Swarm**         | Yes — saved locally and viewable online                       |
| **Local Toolkit** | No — running locally without API does not generate recordings |

## Online Replays

For games played via the API, you can view recordings online through your scorecard:

`https://arcprize.org/scorecards/<scorecard_id>`

Here is an example [recording](https://arcprize.org/replay/1d251d20-9043-4ace-9f9d-09822f5438d8).

## Local Recording Files

When running a [swarm](/swarms), agent gameplay is recorded by default and stored in the `recordings/` directory with GUID-based filenames:

```
ls20-6cbb1acf0530.random.100.a1b2c3d4-e5f6-7890-abcd-ef1234567890.recording.jsonl
```

The filename format is: `{game_id}.{agent_type}.{max_actions}.{guid}.recording.jsonl`

## Recording File Format

### JSONL Format

Recordings are stored in JSONL format with timestamped entries:

```json theme={null}
{"timestamp": "2024-01-15T10:30:45.123456+00:00", "data": {"game_id": "ls20-016295f7601e", "frame": [...], "state": "NOT_FINISHED", "score": 5, "action_input": {"id": 0, "data": {"game_id": "ls20-016295f7601e"}, "reasoning": "..."}, "guid": "...", "full_reset": false}}
{"timestamp": "2024-01-15T10:30:46.234567+00:00", "data": {"game_id": "ls20-016295f7601e", "frame": [...], "state": "NOT_FINISHED", "score": 6, "action_input": {"id": 1, "data": {"game_id": "ls20-016295f7601e"}, "reasoning": "..."}, "guid": "...", "full_reset": false}}
```
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Agents Quickstart

> Build AI agents for ARC-AGI-3 environments

Get started with the [ARC-AGI-3 Agents repo](https://github.com/arcprize/ARC-AGI-3-Agents).

Watch the [Agent Quickstart tutorial video](https://www.youtube.com/watch?v=xEVg9dcJMkw).

## Step 1: Clone the Repo

```bash theme={null}
git clone https://github.com/arcprize/ARC-AGI-3-Agents.git
cd ARC-AGI-3-Agents
```

Make sure you have your `ARC_API_KEY` populated in your environment variables. See [API Keys](/api-keys) for setup instructions.

## Step 2: Run an Agent

Run the random agent on the `ls20` game:

```bash theme={null}
uv run main.py --agent=random --game=ls20
```

The [replay](/recordings) of your agent is available at the end of the run.

## Next Steps

* [Create Your Own Agent](/create-agent) — Build a custom agent
* [LLM Agents](/llm_agents) — Build agents powered by LLMs
* [Partner Templates](/partner_templates/agentops) — Start from partner-provided templates

> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Benchmarking Tooling (BETA)

> Run repeatable agent evaluations.

Currently in beta, our Benchmarking Agent will be the standard way to measure AI performance across model providers.

## ARC Harness `arcagi3`

This is a developer harness for building and benchmarking agentic research workflows on the **ARC-AGI-3** corpus of environments.

## When to use it

* Compare model versions or prompt strategies on the same game set.
* Detect regressions after code or prompt changes.
* Generate official scorecards and replays for sharing.
* Experimenting with multiple custom agentic architectures.

## Quickstart

### Prerequisites

* **Python**: `3.9+`
* **uv**: recommended package manager. Install from [uv.pm](https://github.com/astral-sh/uv) or `curl -LsSf https://astral.sh/uv/install.sh | sh`
* **ARC-AGI-3 API key**: required to talk to the ARC server. Sign up for a key [here](https://arcprize.org/platform)

### Install

Clone the repository:

```bash theme={null}
git clone git@github.com:arcprize/arc-agi-3-benchmarking.git
cd arc-agi-3-benchmarking
```

From repo root:

```bash theme={null}
uv venv
uv sync
```

This creates a virtual environment (if needed) and installs the project and dependencies in editable mode.

Alternatively, without `uv`:

```bash theme={null}
pip install -e .
```

### Setting up your environment

Set the ARC API key and your provider keys. You can put them in a `.env` file (see [`.env.example`](https://github.com/arcprize/arc-agi-3-benchmarking/blob/main/.env.example)) or export them in your shell.

Provider key links:

* [OpenAI](https://platform.openai.com/account/api-keys)
* [Anthropic](https://console.anthropic.com/account/api-keys)
* [Google Gemini](https://console.cloud.google.com/apis/credentials)
* [OpenRouter](https://openrouter.ai/api-keys)
* [Fireworks](https://app.fireworks.ai/account/api-keys)
* [Groq](https://groq.com/account/api-keys)
* [DeepSeek](https://console.deepseek.com/account/api-keys)
* [Hugging Face](https://huggingface.co/settings/tokens)

Check configuration:

```bash theme={null}
uv run python -m arcagi3.runner --check
```

### Select your game

```bash theme={null}
uv run python -m arcagi3.runner --list-games
```

### Pick your model

```bash theme={null}
uv run python -m arcagi3.runner --list-models
```

### Benchmark

```bash theme={null}
uv run python -m arcagi3.runner \
  --game_id ls20 \
  --config gpt-5-2-openrouter \
  --max_actions 3
```

### Scorecards

When you run a benchmark, a scorecard is saved on the ARC server. If you are logged in, you can view them at [arcprize.org/scorecards](https://arcprize.org/scorecards).

## Learn More

The [benchmarking README](https://github.com/arcprize/arc-agi-3-benchmarking#readme) has more information than what is published here. Be sure to also reference how to [create your own agent](https://github.com/arcprize/arc-agi-3-benchmarking/blob/main/docs/create_agent.md) to start experimenting with new agentic architectures.
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# ARC Prize 2026

> Local dev starter kit for the ARC Prize 2026 Kaggle competition.

The [ARC-AGI-3 Kaggle Starter](https://github.com/arcprize/ARC-AGI-3-Kaggle-Starter) is a local dev kit for the [ARC Prize 2026 - ARC-AGI-3](https://www.kaggle.com/competitions/arc-prize-2026-arc-agi-3) Kaggle competition. You edit one Python file on your laptop, see it play the real game environments locally, and push it to Kaggle as a submission with a single command.

Repository: [github.com/arcprize/ARC-AGI-3-Kaggle-Starter](https://github.com/arcprize/ARC-AGI-3-Kaggle-Starter)

## Prerequisites

* **Python 3.12** (the competition's `arc-agi` package requires it)
  * macOS: `brew install python@3.12`
  * Ubuntu: `sudo apt install python3.12 python3.12-venv`
  * Windows: install from [python.org](https://www.python.org/downloads/)
* **A Kaggle account** with the competition rules accepted ([accept here](https://www.kaggle.com/competitions/arc-prize-2026-arc-agi-3/rules)).
* **A Kaggle API token** created at [Kaggle Settings → Create New Token](https://www.kaggle.com/settings). You'll drop it into the project (no `~/.kaggle/kaggle.json` required).

No GPU is required for the starter agent.

## Quick start

```bash theme={null}
# 1. Clone the starter
git clone https://github.com/arcprize/ARC-AGI-3-Kaggle-Starter.git
cd ARC-AGI-3-Kaggle-Starter

# 2. Drop your Kaggle API token into the project-local .kaggle/ folder
#    (NOT your home directory)
mkdir -p .kaggle && echo "KGAT_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" > .kaggle/access_token
chmod 600 .kaggle/access_token

# 3. One-time setup: venv, dependencies, framework
make setup

# 4. Edit agent/my_agent.py. This is the only file you change.

# 5. Run it locally against every game (seconds)
make play-local

# 6. Push it to Kaggle as a submission notebook
make submit

# 7. Watch the run
make status
```

When `make status` shows `complete`, open the notebook on [kaggle.com](https://www.kaggle.com), find your kernel, click **Submit to Competition** in the top right, and pick `submission.parquet` from the Output File dropdown.

Steps 4-7 are the iteration loop; clicking *Submit to Competition* is the deliberate moment you spend a daily submission.

## The one file you edit: `agent/my_agent.py`

This is the only file you normally touch. It defines a `MyAgent` class with two methods:

```python theme={null}
class MyAgent(Agent):
    def is_done(self, frames, latest_frame) -> bool:
        """Return True when your agent wants to stop playing."""
        ...

    def choose_action(self, frames, latest_frame) -> GameAction:
        """Look at the game state and return the next action."""
        ...
```

The starter picks random actions, a baseline that proves your pipeline works end-to-end. Replace the body of `choose_action` with your strategy. Kaggle plumbing, submission file format, and game orchestration are all handled for you.

## What happens when you run `make submit`

The competition is a **code competition**: you submit a notebook, Kaggle runs it twice.

1. **Phase A: Save & Run All.** Kaggle runs your notebook in their real environment and validates that the code executes without errors. `make status` will report `complete`.
2. **Phase B: Competition Rerun.** Triggered when you click **Submit to Competition** on the kernel page. Your agent plays the hidden game set and your leaderboard score appears.

<Note>
  Before your first `make submit`, open `notebooks/kernel-metadata.json` and replace `REPLACE_WITH_YOUR_USERNAME` with your Kaggle handle. The Makefile will refuse to push until you do.
</Note>

### Choosing an accelerator

The notebook is generated with a **T4 GPU** by default (matches Kaggle's sample submission). To change it, open `scripts/build_notebook.py` and edit one line near the top:

```python theme={null}
ACCELERATOR = "t4"     # one of: cpu, t4, p100, rtx6000
```

Re-run `make submit`. Both the notebook metadata and `notebooks/kernel-metadata.json` get updated automatically.

| Value       | Hardware                           | When to use                                               |
| ----------- | ---------------------------------- | --------------------------------------------------------- |
| `"cpu"`     | No GPU                             | The random starter, or any non-ML agent                   |
| `"t4"`      | Nvidia T4 ×2                       | **Default.** Small models, fast iteration                 |
| `"p100"`    | Nvidia P100                        | Single big-memory GPU                                     |
| `"rtx6000"` | Nvidia RTX 6000 (`g4-standard-48`) | Heavy ML; **ARC-AGI-3 exclusive**, burns GPU quota faster |

RTX 6000 is reserved for ARC-AGI-3 notebooks. Don't use it for early iteration. All accelerated Kaggle sessions have internet disabled, which is already the default in this kit.

## All the commands

| Command                     | What it does                                                                 |
| --------------------------- | ---------------------------------------------------------------------------- |
| `make setup`                | One-time install: Python venv, `arc-agi`, `kaggle` CLI, clones the framework |
| `make play-local`           | Runs your agent against every game in the dataset, locally                   |
| `make play-local GAME=ls20` | Same, but only one game (faster while debugging)                             |
| `make verify-local`         | 30-second smoke test on two games                                            |
| `make list-games`           | Print every game id available                                                |
| `make pull-sample`          | Download the official sample agent for reference                             |
| `make notebook`             | Build the Kaggle notebook from your agent (no push)                          |
| `make submit`               | Build the notebook **and** push it to Kaggle                                 |
| `make status`               | Check the status of your most recent Kaggle run                              |
| `make clean`                | Remove the venv, downloads, and generated notebook                           |

## Why this setup instead of editing in the Kaggle notebook?

1. **Iteration speed.** Editing in your IDE then running `make play-local` gives you a real-game-engine feedback loop in seconds. Kaggle's editor loop is *minutes* per change.
2. **No environment surprises.** The local `arc-agi` PyPI package hosts the same game engine the Kaggle gateway runs. If it works locally, it works on Kaggle.
3. **Your code stays in git.** Notebooks are awful for diffs and code review. Your real work lives in `agent/my_agent.py`; the notebook is an auto-generated deployment artifact.

## Project layout

```
.
├── agent/
│   └── my_agent.py             ★ The file you edit
├── scripts/
│   ├── play_local.py           Runs your agent against real games
│   ├── build_notebook.py       Packages your agent into a Kaggle notebook
│   └── slim_framework.py       Trims framework deps so install is light
├── notebooks/
│   ├── kernel-metadata.json    Edit once: your Kaggle username
│   └── submission.ipynb        Auto-generated, never edit by hand
├── vendor/                     Cloned framework (gitignored)
├── .venv/                      Python 3.12 venv (gitignored)
├── .kaggle/                    Your project-local Kaggle token (gitignored)
└── Makefile
```

## Troubleshooting

**`make setup` fails: `python3.12: command not found`.** Install Python 3.12; the `arc-agi` package requires it. macOS: `brew install python@3.12`.

**`make submit` says "edit kernel-metadata.json".** You haven't replaced `REPLACE_WITH_YOUR_USERNAME` in `notebooks/kernel-metadata.json` yet.

**`make submit` says `401 Unauthorized`.** Your Kaggle token is missing or invalid. Generate a fresh one from your [Kaggle Settings page](https://www.kaggle.com/settings) and overwrite `.kaggle/access_token`.

**`make play-local` says "Could not create environment".** Your machine couldn't reach the ARC-AGI API to download the game source on first run. Check your internet, then try again. Once downloaded, games are cached in `environment_files/` and you're fully offline.

**My local score is 0.0.** That's expected for the random starter agent. Your job is to make it non-zero.

## Where to go next

* Read the rest of the [ARC-AGI-3 docs](/) to understand the benchmark.
* Run `make pull-sample` to study Kaggle's reference agent.
* Join the competition's [discussion forum](https://www.kaggle.com/competitions/arc-prize-2026-arc-agi-3/discussion) for community Q\&A.
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# ARC-AGI Toolkit Quickstart

> Getting started with the ARC-AGI Toolkit

The ARC-AGI Toolkit is an open-source Python SDK for ARC-AGI-3 environments, geared towards researchers looking to make progress on ARC-AGI-3.

The Toolkit enables:

* **Local development** — Run your agents locally without needing the API, built on top of the [ARC-AGI-3 game engine](https://github.com/arcprize/ARCEngine)
* **Customization** — Edit existing games and create new ones
* **Flexibility** — Interact with environments locally or via API

## QuickStart

### 1. Install the Toolkit

```bash theme={null}
uv add arc-agi
# or
pip install arc-agi
```

### 2. Set your API key (optional)

```bash theme={null}
export ARC_API_KEY="your-api-key-here"
```

If no key is provided, an anonymous key will be used. See [API Keys](/api-keys) for more details.

### 3. Play a game

```python theme={null}
import arc_agi
from arcengine import GameAction

arc = arc_agi.Arcade()
env = arc.make("ls20", render_mode="terminal")

# See available actions
print(env.action_space)

# Take an action
obs = env.step(GameAction.ACTION1)

# Check your scorecard
print(arc.get_scorecard())
```

## Next Steps

* [Minimal Example](./minimal) — A complete script example
* [List Games](./list-games) — Discover available games
* [Local vs Online](/local-vs-online) — Choose how to run games

## Changelog

## \[0.9.3] - 2026-03-09

### Added

* `OperationMode.COMPETITION` method, see [Documentation](./competition_mode)
* Official Scoring
  * Average for an individual games is now weighted by the level index (1 indxed)
  * Score for an individual level is now squared.  A score of `0.5` now becomes `0.25`

### Fixed

* Continued fixes for 404 Scorecard not found

## \[0.9.2] - 2026-02-26

### Added

* `listen_and_serve` method, see [Documentation](./listen_and_serve)

### Fixed

* 404 Scorecard not found about 50% of the time when in `ONLINE` mode
* Game source being downloaded even if local copy already exists

## \[0.9.1] - 2026-01-29

Initial Release
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Competition Mode

> Running in Competition Mode

## Overview

This mode is **REQUIRED** to show up on the Unverified leaderboard and forces the following behavior.

* Environments must be interacted with via the API
* Scoring is against all available environments, even if you choose not to interact with them
* Only *Level Resets* are premitted, *Game Resets* are not allowed and become *Level Resets*
* Can only interact (call `make`) a single time for each environment
* Can only open a single Scorecard
* Cannot get scoring of an inflight scorecard, `get_scorecard` does not work

**Note:** The Kaggle Compeition is forced into this mode.

### Example

```
from arc_agi import Arcade, OperationMode

arc = Arcade(operation_mode=OperationMode.COMPETITION)
```

You can also set the ENVVAR of `OPERATION_MODE=COMPETITION`.
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Listen And Serve

> Running the REST Endpoints to interact with Environments.

## Overview

Start a blocking Flask server that exposes the REST API. Uses `arc_agi.server.create_app()` under the hood.  This conforms to the [Rest API](https://docs.arcprize.org/rest_overview) to allow local execution for interactions with languages other than Python or with this Toolkit running in `ONLINE` mode.

<Note>
  This is how Kaggle runs your solution when you set `competition_mode=True`. Use `listen_and_serve` to set up your own server locally and replicate the Kaggle environment for development and testing.
</Note>

**Parameters:**

* `host` (`str`, optional): Bind address. Default `"0.0.0.0"` to accept connections from any interface.
* `port` (`int`, optional): Port to listen on. Default `8001`.
* `competition_mode` (`bool`, optional): If `True`, enable competition mode. Default `False`.
* `save_all_recordings` (`bool`, optional): If `True`, save recordings for all runs. Default `False`.
* `add_cookie` (`Callable[[Response, str], Response]`, optional): Callback to inject a cookie into API responses. Receives `(response, api_key)`; must return the modified response. Use for session stickiness (e.g. ALB app cookies).
* `scorecard_timeout` (`int`, optional): Idle timeout in seconds before scorecards are auto-closed. If set, starts a background cleanup loop.
* `on_scorecard_close` (`Callable[[EnvironmentScorecard], None]`, optional): Callback invoked when a scorecard is closed (manually or by timeout).
* `extra_api_routes` (`Callable[[Arcade, Flask], None]`, optional): Callback to register custom routes. Receives `(arcade, app)`.
* `renderer` (`Callable[[int, FrameDataRaw], None]`, optional): Callback invoked for each frame during gameplay. Receives `(step_index, frame_data)`. Use for logging, visualization, or custom display.
* `**kwargs`: Passed through to `Flask.run()` (e.g. `debug=True`, `threaded=True`).

**Example (basic):**

```python theme={null}
arc = Arcade()
arc.listen_and_serve(port=8001)
```

**Example (with `add_cookie` for session stickiness):**

```python theme={null}
from flask import Response

def add_session_cookie(resp: Response, api_key: str) -> Response:
    resp.set_cookie("APPLICATION_COOKIE", api_key, path="/", httponly=True)
    return resp

arc.listen_and_serve(add_cookie=add_session_cookie)
```

**Example (with `on_scorecard_close`):**

```python theme={null}
def on_close(scorecard):
    print(f"Scorecard closed: {scorecard.score}")

arc.listen_and_serve(on_scorecard_close=on_close)
```

**Example (with `extra_api_routes`):**

```python theme={null}
def register_custom(arcade, app):
    @app.route("/custom")
    def custom():
        return {"environments": len(arcade.available_environments)}

arc.listen_and_serve(extra_api_routes=register_custom)
```

**Example (with `renderer` for logging):**

```python theme={null}
def log_frame(step: int, frame_data):
    print(f"Step {step}: state={frame_data.state}, levels_completed={frame_data.levels_completed}")

arc.listen_and_serve(renderer=log_frame)
```
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# List Games

> How to list available games using the ARC-AGI Toolkit

## Overview

The ARC-AGI Toolkit provides access to a variety of interactive puzzle environments. Before playing, you can discover what games are available using the `get_environments()` method.

## Listing All Available Games

Use `get_environments()` to retrieve all games you have access to:

```python theme={null}
import arc_agi

arc = arc_agi.Arcade()
games = arc.get_environments()

for game in games:
    print(f"{game.game_id}: {game.title}")
```

This returns both local games (from your `environment_files` directory) and remote games available via the API.

## Working with Game Information

Each game in the list is an `EnvironmentInfo` object with useful properties:

```python theme={null}
games = arc.get_environments()

for game in games:
    print(f"ID: {game.game_id}")
    print(f"Title: {game.title}")
    print(f"Tags: {game.tags}")
    print("---")
```

## Selecting a Game to Play

Once you've found a game you want to play, use its `game_id` with the `make()` method:

```python theme={null}
# List games and pick one
games = arc.get_environments()
print(f"Found {len(games)} games available")

# Play the first game
if games:
    game_id = games[0].game_id
    env = arc.make(game_id, render_mode="terminal")
```

## Local vs Remote Games

The games returned depend on your operation mode:

| Mode               | Games Returned       |
| ------------------ | -------------------- |
| `NORMAL` (default) | Local + Remote games |
| `ONLINE`           | Remote games only    |
| `OFFLINE`          | Local games only     |

```python theme={null}
from arc_agi import Arcade, OperationMode

# List both local and remote games (default)
arc = Arcade(operation_mode=OperationMode.NORMAL)
all_games = arc.get_environments()

# List only local games
arc = Arcade(operation_mode=OperationMode.OFFLINE)
local_games = arc.get_environments()

# List only remote games
arc = Arcade(operation_mode=OperationMode.ONLINE)
remote_games = arc.get_environments()
```

## Next Steps

* [Play a game using the REPL](./repl)
* [Build a game-playing script](./minimal)
* [Learn about game actions](/actions)
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Get Scorecard

> How to retrieve scorecard results using the ARC-AGI Toolkit

## Overview

After playing games, you can retrieve your scorecard to see aggregated performance results. The Toolkit automatically manages a default scorecard, or you can create and manage your own.

## Getting the Default Scorecard

The Toolkit automatically creates a scorecard when you start playing. Retrieve it with `get_scorecard()`:

```python theme={null}
import arc_agi
from arcengine import GameAction

arc = arc_agi.Arcade()
env = arc.make("ls20", render_mode="terminal")

# Play the game
env.step(GameAction.ACTION1)

# Get your scorecard
scorecard = arc.get_scorecard()
if scorecard:
    print(f"Score: {scorecard.score}")
    print(f"Games played: {len(scorecard.games)}")
```

## Scorecard Properties

The scorecard contains your aggregated results:

```python theme={null}
scorecard = arc.get_scorecard()

if scorecard:
    print(f"Score: {scorecard.score}")
    print(f"Games: {scorecard.games}")
```

## See Full Scorecard

To view the complete scorecard with all fields, use `model_dump_json()`:

```python theme={null}
scorecard = arc.get_scorecard()

if scorecard:
    print(scorecard.model_dump_json(indent=2))
```

## Next Steps

* [Create Scorecard](/toolkit/create-scorecard) — Create a custom scorecard with tags
* [Close Scorecard](/toolkit/close-scorecard) — Finalize and close a scorecard
* [Learn about scoring methodology](/methodology)
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Create Scorecard

> How to create a custom scorecard using the ARC-AGI Toolkit

## Overview

While the Toolkit automatically manages a default scorecard, you can create your own custom scorecards with tags and metadata to organize your experiments.

## Creating a Custom Scorecard

Use `create_scorecard()` to create a scorecard with custom fields:

```python theme={null}
import arc_agi

arc = arc_agi.Arcade()

# Create a scorecard with custom fields
scorecard_id = arc.create_scorecard(
    source_url="https://github.com/my/repo",
    tags=["experiment", "v1"],
    opaque={"custom_field": "any data you want"}
)

print(f"Created scorecard: {scorecard_id}")
```

## Using Your Scorecard

Pass the scorecard ID when creating environments:

```python theme={null}
# Use your custom scorecard when making environments
env = arc.make("ls20", scorecard_id=scorecard_id, render_mode="terminal")
```

All game runs will now be tracked under your custom scorecard.

## Scorecard Fields

| Field        | Description                                             |
| ------------ | ------------------------------------------------------- |
| `tags`       | Array of strings to categorize and filter scorecards    |
| `source_url` | Optional URL field (e.g., link to your code repository) |
| `opaque`     | Optional field for arbitrary data                       |

## Next Steps

* [Get Scorecard](/toolkit/get-scorecard) — Retrieve scorecard results
* [Close Scorecard](/toolkit/close-scorecard) — Finalize and close a scorecard
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Close Scorecard

> How to close and finalize a scorecard using the ARC-AGI Toolkit

## Overview

When you're done with a scorecard, close it to finalize the results. This is important for proper tracking and leaderboard submission.

## Closing the Default Scorecard

If you're using the default scorecard, simply call `close_scorecard()`:

```python theme={null}
import arc_agi

arc = arc_agi.Arcade()
env = arc.make("ls20", render_mode="terminal")

# ... play games ...

# Close and get final results
final_scorecard = arc.close_scorecard()

if final_scorecard:
    print(f"Final score: {final_scorecard.score}")
    print(final_scorecard.model_dump_json(indent=2))
```

After closing, a new default scorecard will be created on the next `make()` call.

## Closing a Specific Scorecard

If you created a custom scorecard, pass its ID:

```python theme={null}
# Close a specific scorecard
final_scorecard = arc.close_scorecard(scorecard_id="your-scorecard-id")
```

## Why Close Scorecards?

* Scorecards auto-close after 15 minutes of inactivity
* Closing ensures your results are finalized
* Stopping your program with Ctrl-C without closing may prevent you from seeing results

## Next Steps

* [Get Scorecard](/toolkit/get-scorecard) — Retrieve scorecard results
* [Create Scorecard](/toolkit/create-scorecard) — Create a custom scorecard
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Create ARC-AGI-3 Environment

> Building and adding a game to ARC-AGI-3 Environments

This example walks through a simple game where randomly generated sprites appear and the player must click to remove them all to win.

[See the full code](#complete-code)

## Game File Structure

Each game adheres to the following structure:

* **Imports**
  <br />Import ARCEngine classes and any standard library modules needed.
* **Sprite Definitions**
  <br />Define sprite templates in a dictionary. Sprites are objects with a name, rectangular pixel array, and optional tags.
* **Level Definitions**
  <br />Define levels as a list of `Level` objects, each containing sprite placements and configuration data.
* **Constants**
  <br />Define game-wide constants for colors, grid size, and gameplay parameters.
* **Game Class**
  <br />Create a class extending `ARCBaseGame` that implements game logic. The class name must match the 4-character game ID with the first letter capitalized.

## Directory Structure

To start building your own environment, create the following directory structure:

```
ARC-AGI/
└── environment_files/
    └── ab12/               # use any game ID you want
        └── v1/             # use any version identifier you want
            ├── ab12.py     # must match game ID above
            └── metadata.json
```

### Metadata File

Create `metadata.json`:

```json theme={null}
{
  "game_id": "ab12-v1",
  "local_dir": "environment_files\\ab12\\v1"
}
```

| Field              | Description                                              |
| ------------------ | -------------------------------------------------------- |
| `game_id`          | Unique identifier in format `{4-char game ID}-{version}` |
| `default_fps`      | Frames per second for playback (optional)                |
| `baseline_actions` | Array of average action counts per level (optional)      |
| `tags`             | Optional tags for categorization                         |
| `local_dir`        | Relative path to game directory                          |

## Imports and Constants

Start with the imports and constants:

```python theme={null}
import random

from arcengine import (
    ARCBaseGame,
    Camera,
    GameAction,
    Level,
    Sprite,
)

BACKGROUND_COLOR = 0  
PADDING_COLOR = 4     

# Available colors for sprites (excluding all black -> white colors for visibility)
SPRITE_COLORS = [6, 7, 8, 9, 10, 11, 12, 13, 14, 15]

# Parameters for Procedural Generation
MIN_SIZE = 1
MAX_SIZE = 4
```

## Sprite and Level Definitions

Define the sprites and levels for your game.\
In this example, the base sprite template that will be cloned and modified during generation:

```python theme={null}
# Base sprite template - single pixel will be cloned, scaled, and recolored
sprites = {
    "sprite-1": Sprite(
        pixels=[
            [9],
        ],
        name="sprite-1",
        visible=True,
        collidable=True,
    ),
}

# Create levels array with all level definitions
levels = [
    # Level 1
    Level(
        sprites=[],
        grid_size=(8, 8),
    ),
    Level(
        sprites=[],
        grid_size=(16, 16),
    ),
    Level(
        sprites=[],
        grid_size=(24, 24),
    ),
    Level(
        sprites=[],
        grid_size=(32, 32),
    ),
    Level(
        sprites=[],
        grid_size=(64, 64),
    ),
]
```

## Game Class Definition

The game class extends `ARCBaseGame` and accepts a seed parameter:

```python theme={null}
class Ab12(ARCBaseGame):
    """Click-to-remove game with seeded random sprite generation."""

    def __init__(self, seed: int = 0) -> None:
        self._rng = random.Random(seed)
    
        # Create camera with background and padding colors
        camera = Camera(
            background=BACKGROUND_COLOR,
            letter_box=PADDING_COLOR,
            width=8,
            height=8,
        )

        # Initialize base game
        super().__init__(
            game_id="ab12",
            levels=levels,
            camera=camera,
        )
```

## Gameplay Logic and Mechanics

This game generates sprites by cloning from the sprite dictionary and applying random properties:

```python theme={null}
    def generate_sprites(self) -> None:
           """Generate a random set of sprites based on the seed."""
           # Determine number of sprites
           cell_count = self.current_level.grid_size[0] * self.current_level.grid_size[1]
           sprite_count = cell_count // 64  
           for idx in range(sprite_count):
               scale = self._rng.randint(MIN_SIZE, MAX_SIZE)
               color = self._rng.choice(SPRITE_COLORS)
               x = self._rng.randint(0, self.current_level.grid_size[0] - 1)
               y = self._rng.randint(0, self.current_level.grid_size[1] - 1)
               # Create the sprite setting color, scale, and position then add it to the level
               sprite = sprites[f"sprite-1"].clone().color_remap(None, color).set_scale(scale).set_position(x, y)
               self.current_level.add_sprite(sprite)
```

### Generation Pattern

The method chains operations on the cloned sprite:

1. **`.clone()`** - Creates independent copy of template
2. **`.color_remap(None, color)`** - Changes all pixels to random color
3. **`.set_scale(scale)`** - Applies the random scale
4. **`.set_position(x, y)`** - Places at random position in the grid

These can be used in level definitions directly for static levels.

## Level Initialization

The `on_set_level()` method is called when a level loads:

```python theme={null}
    def on_set_level(self, level: Level) -> None:
        """Called when the level is set."""
        # Generate sprites based on seed
        self.generate_sprites()
```

This method triggers sprite generation each time the level is set or reset.

## Win Condition

Define when the player wins:

```python theme={null}
    def _check_win(self) -> bool:
        """Check if all targets have been removed."""
        return len(self.current_level._sprites) == 0
```

## Game Loop

The `step()` method contains the main game logic.

### Action Flow

1. **Check action type**: `GameAction.ACTION6` is the click action
2. **Get coordinates**: Extract `x`, `y` from `action.data`
3. **Convert coordinates**: `display_to_grid()` handles camera scaling
4. **Find sprite**: Check if click hit any sprite
5. **Remove sprite**: Update level state
6. **Check win**: Advance to the next level or win the game on the last level if condition is met
7. **Complete action**: Always call `self.complete_action()`

```python theme={null}
    def step(self) -> None:
        """Process game logic for each step."""

        # 1. Check action type - GameAction.ACTION6 is the click action
        if self.action.id == GameAction.ACTION6:
            # 2. Get coordinates - Extract x, y from action.data
            x = self.action.data.get("x", 0)
            y = self.action.data.get("y", 0)

            # 3. Convert coordinates - display_to_grid() handles camera scaling
            coords = self.camera.display_to_grid(x, y)

            if coords:
                grid_x, grid_y = coords

                # 4. Find sprite - Check if click hit any sprite
                clicked_sprite = self.current_level.get_sprite_at(grid_x, grid_y)
                if clicked_sprite:
                    # 5. Remove sprite - Update level state
                    self.current_level.remove_sprite(clicked_sprite)

                    # 6. Check win - Advance to next level if condition is met
                    if self._check_win():
                        self.next_level()

        # 7. Complete action - Always call self.complete_action()
        self.complete_action()
```

## Testing the Game

Run the game using the ARC-AGI-3 client:

```python theme={null}
import arc_agi
from arcengine import GameAction

# Default: looks for games in "environment_files" directory
arc = arc_agi.Arcade()
env = arc.make("ab12-v1", seed=0, render_mode="terminal")

# Or specify a custom directory
arc = arc_agi.Arcade(environments_dir="./my_games")
env = arc.make("ab12-v1", seed=0, render_mode="terminal")

# Perform clicks (ACTION6 with x, y coordinates)
env.step(GameAction.ACTION6, data={"x": 32, "y": 32})
```

If you'd like to see a 100-step run on your game, play the game with the [Sample Agent](./toolkit/minimal).

## Complete Code

<Accordion title="Full ab12.py source code">
  ```python theme={null}
  import random

  from arcengine import (
      ARCBaseGame,
      Camera,
      GameAction,
      Level,
      Sprite,
  )

  # Create sprites dictionary with all sprite definitions
  sprites = {
      "sprite-1": Sprite(
          pixels=[
              [9],
          ],
          name="sprite-1",
          visible=True,
          collidable=True,
      ),
  }

  # Create levels array with all level definitions
  levels = [
      # Level 1
      Level(
          sprites=[],
          grid_size=(8, 8),
      ),
      Level(
          sprites=[],
          grid_size=(16, 16),
      ),
      Level(
          sprites=[],
          grid_size=(24, 24),
      ),
      Level(
          sprites=[],
          grid_size=(32, 32),
      ),
      Level(
          sprites=[],
          grid_size=(64, 64),
      ),
  ]

  BACKGROUND_COLOR = 0
  PADDING_COLOR = 4

  # Available colors for sprites (excluding all black -> white colors for visibility)
  SPRITE_COLORS = [6, 7, 8, 9, 10, 11, 12, 13, 14, 15]

  # Parameters for Procedural Generation
  MIN_SIZE = 1
  MAX_SIZE = 4


  class Ab12(ARCBaseGame):
      """Click-to-remove puzzle game with seeded random sprite generation."""

      def __init__(self, seed: int = 0) -> None:
          self._rng = random.Random(seed)
          # Create camera with background and padding colors
          camera = Camera(
              background=BACKGROUND_COLOR,
              letter_box=PADDING_COLOR,
              width=8,
              height=8,
          )

          super().__init__(
              game_id="ab12",
              levels=levels,
              camera=camera,
          )

      def generate_sprites(self) -> None:
             """Generate a random set of sprites based on the seed."""
             # Determine number of sprites
             cell_count = self.current_level.grid_size[0] * self.current_level.grid_size[1]
             sprite_count = cell_count // 64  
             for idx in range(sprite_count):
                 scale = self._rng.randint(MIN_SIZE, MAX_SIZE)
                 color = self._rng.choice(SPRITE_COLORS)
                 x = self._rng.randint(0, self.current_level.grid_size[0] - 1)
                 y = self._rng.randint(0, self.current_level.grid_size[1] - 1)
                 # Create the sprite setting color, scale, and position then add it to the level
                 sprite = sprites[f"sprite-1"].clone().color_remap(None, color).set_scale(scale).set_position(x, y)
                 self.current_level.add_sprite(sprite)

      def on_set_level(self, level: Level) -> None:
          """Called when the level is set."""
          # Generate sprites based on seed
          self.generate_sprites()

      def _check_win(self) -> bool:
          """Check if all targets have been removed."""
          return len(self.current_level._sprites) == 0

      def step(self) -> None:
          """Process game logic for each step."""
          # Handle click action (ACTION6)
          if self.action.id == GameAction.ACTION6:
              x = self.action.data.get("x", 0)
              y = self.action.data.get("y", 0)

              # Convert display coordinates to grid coordinates
              coords = self.camera.display_to_grid(x, y)

              if coords:
                  grid_x, grid_y = coords

                  # Find and remove the clicked sprite from the level
                  clicked_sprite = self.current_level.get_sprite_at(grid_x, grid_y)
                  if clicked_sprite:
                      self.current_level.remove_sprite(clicked_sprite)

                      # Check win condition
                      if self._check_win():
                          self.next_level()

          self.complete_action()
  ```
</Accordion>

## Further Reading

For more information, refer to the **ARC Engine Documentation**.
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Edit Game

> A guide to modifying games in ARC-AGI-3 Environments

## Project Setup

### Environment Configuration

Game files are stored in an environments directory. The default is `environment_files` in the project root.

You can configure this in your `.env` file:

```dotenv theme={null}
environments_dir = my_environments
```

Or specify it directly when initializing the client:

```python theme={null}
arc = arc_agi.Arcade(environments_dir="./my_environments")
```

### Directory Structure

The environments follow this directory structure:

```
ARC-AGI/
└── environment_files/
    └── ls20/
        └── v1/
            ├── ls20.py           # Main game file
            └── metadata.json     # Game metadata
```

### Creating a New Version

Copy your existing version folder to create a new version:

<CodeGroup>
  ```bash Mac/Linux theme={null}
  cp -r environment_files/ls20/v1 environment_files/ls20/v2
  ```

  ```bash Windows theme={null}
  xcopy environment_files\ls20\v1 environment_files\ls20\v2\ /E /I
  ```
</CodeGroup>

Your directory now looks like this:

```
ARC-AGI/
└── environment_files/
    └── ls20/
        ├── v1/
        │   ├── ls20.py
        │   └── metadata.json
        └── v2/                   # your new version
            ├── ls20.py
            └── metadata.json
```

Then update `metadata.json` with the new version ([more info](/add_game#metadata-file)):

```json theme={null}
{
  "game_id": "ls20-v2",
  "default_fps": 5,
  "local_dir": "environment_files\\ls20\\v2"
}
```

Test the new version:

```python theme={null}
import arc_agi

arc = arc_agi.Arcade()
env = arc.make("ls20-v2", render_mode="terminal")
```

***

## Editing the Game File

The main game logic resides in `game-id.py`. This file contains:

| Name      | Type                 | Description                                            |
| --------- | -------------------- | ------------------------------------------------------ |
| `sprites` | `dict[str, Sprite]`  | Sprite templates with pixel arrays and properties      |
| `levels`  | `list[Level]`        | Level objects with sprite placements and configuration |
| `GameId`  | `class(ARCBaseGame)` | Game class implementing gameplay mechanics and logic   |

```python theme={null}
# Typical structure of game-id.py

from arcengine import ARCBaseGame, Camera, GameAction, Level, Sprite

# Sprite definitions
sprites = {
    "sprite-1": Sprite(pixels=[...], name="sprite-1", ...),
    "sprite-2": Sprite(pixels=[...], name="sprite-2", ...),
    # ...
}

# Level definitions
levels = [
    Level(sprites=[sprites["sprite-1"].clone().set_position(0, 0)...], grid_size=(64, 64), data={...}),
    Level(sprites=[sprites["sprite-2"].clone().set_position(0, 0)...], grid_size=(64, 64), data={...}),
    # ...
]

# Game class
class Game-id(ARCBaseGame):
    def __init__(self) -> None:
        # Initialize camera, UI, game state
        ...
    
    def on_set_level(self, level: Level) -> None:
        # Called when a level loads - setup level-specific state
        ...
    
    def step(self) -> None:
        # Main game logic - handle actions, collisions, win/lose conditions
        ...
        self.complete_action()
```

***

## Editing Existing Sprites

### Modifying Sprite Pixels

To change an existing sprite's appearance, edit its pixel array directly in the `sprites` dictionary:

This sprite in `ls20.py` now uses colors 8 and 10 instead of 9 and 12.

```python theme={null}
sprites = {
    "pca": Sprite(
        pixels=[
            [10, 10, 10, 10, 10],
            [10, 10, 10, 10, 10],
            [8, 8, 8, 8, 8],
            [8, 8, 8, 8, 8],
            [8, 8, 8, 8, 8],
        ],
        name="pca",
        visible=True,
        collidable=True,
        tags=["caf"],
    ),
}
```

ARCEngine uses a 16-color palette (0-15) plus -1 for transparent and -2 for transparent and collidable.

### Editing Sprites in Level Definitions

When sprites are placed in levels, you can modify them inline:

```python theme={null}
Level(
    sprites=[
        # Edit position
        sprites["pca"].clone().set_position(29, 35),
        
        # Edit colors
        sprites["zba"].clone().set_position(15, 16).color_remap(None, 12),
        
        # Edit rotation
        sprites["kdy"].clone().set_position(49, 45).set_rotation(90),
    ],
    # ...
)
```

This is now the new version of ls20 level 1:

<Frame>
  <img src="https://mintcdn.com/arcprizefoundation/sx3SsV7kmM_q56IF/images/ls20-v2.png?fit=max&auto=format&n=sx3SsV7kmM_q56IF&q=85&s=5513d5289c384ff4a0feacc6414d4036" alt="level variant" width="1442" height="1236" data-path="images/ls20-v2.png" />
</Frame>

***

## Additional Information

### Level Data

The `data` dictionary stores level-specific configuration. This will vary for every game.

```python theme={null}
data={
    "Amount": 30,
    "Values": [5, 0, 2],
    "level_flag": False
    "names": ["name-1", "name-2"],
}
```

level data is accessed in the game class:

```python theme={null}
self.amount = self.current_level.get_data("Amount")
self.flag = self.current_level.get_data("level_flag")
```

***

## Other Techniques

### Dynamic Sprite Addition/Removal

```python theme={null}
# Add sprite during gameplay
new_sprite = sprites["sprite-name"].clone().set_position(x, y)
self.current_level.add_sprite(new_sprite)

# Remove sprite
self.current_level.remove_sprite(some_sprite)
```

### Querying Sprites

```python theme={null}
# Get sprites by tag
players = self.current_level.get_sprites_by_tag("Player")

# Get sprite at position
sprite = self.current_level.get_sprite_at(x, y)

# Get sprites by name
ABC_sprites = self.current_level.get_sprites_by_name("ABC")
```

### Animation Pattern

For multi-frame effects, delay `complete_action()`:

```python theme={null}
def step(self) -> None:
    if self.animating:
        self.animation_frame += 1
        if self.animation_frame >= self.animation_length:
            self.animating = False
            self.complete_action()
        return  # Don't complete action yet
    
    # Normal game logic...
    self.complete_action() 
```

Note the game loop keeps calling `step()` until `complete_action()` is called.

## Further Reading

For more detailed information refer to the [ARC Engine](https://github.com/arcprize/ARCEngine).
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Render Games

> How to render ARC-AGI-3 games

You can render games in several ways:

## Terminal Rendering (bounded)

Text-based rendering with frame rate limiting:

```python theme={null}
env = arc.make("ls20", render_mode="terminal")
```

## Terminal Rendering (unbounded)

Text-based rendering without frame rate limiting:

```python theme={null}
env = arc.make("ls20", render_mode="terminal-fast")
```

## Human Rendering

Matplotlib visualization with frame rate limiting:

```python theme={null}
env = arc.make("ls20", render_mode="human")
```

## Custom Renderer

Provide your own rendering function:

```python theme={null}
from arcengine import FrameDataRaw

def my_renderer(steps: int, frame_data: FrameDataRaw) -> None:
    print(f"Step {steps}: {frame_data.state.name}")

env = arc.make("ls20", renderer=my_renderer)
```
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# Arcade

> ARC-AGI 3 Client for Interactive Environments

The `Arcade` class is the main entry point for interacting with ARC-AGI-3 environments. It handles configuration, environment discovery, and scorecard management.

## Constructor Parameters

The `Arcade` constructor accepts the following parameters. All parameters can be overridden by environment variables, with constructor arguments taking precedence.

```python theme={null}
from arc_agi import Arcade, OperationMode
import logging

arc = Arcade(
    operation_mode=OperationMode.OFFLINE,
    arc_api_key="your-api-key",
    environments_dir="./my_games",
    recordings_dir="./my_recordings",
    arc_base_url="https://three.arcprize.org",
    logger=logging.getLogger("my_agent")
)
```

<Note>
  All parameters are optional. See details below for each option.
</Note>

### `operation_mode`

Controls where games are loaded from. See [Local vs Online](/local-vs-online) for guidance on which mode to use.

| Mode                    | Description                                             |
| ----------------------- | ------------------------------------------------------- |
| `OperationMode.NORMAL`  | Load both local and remote games (default)              |
| `OperationMode.OFFLINE` | Load local games only — fast, no rate limits            |
| `OperationMode.ONLINE`  | Load remote games only — enables scorecards and replays |

```python theme={null}
from arc_agi import Arcade, OperationMode

# Default: both local and remote games
arc = Arcade()

# Local only (recommended for development)
arc = Arcade(operation_mode=OperationMode.OFFLINE)

# Remote only (for scorecards and replays)
arc = Arcade(operation_mode=OperationMode.ONLINE)
```

You can also set this via environment variable:

```bash theme={null}
export OPERATION_MODE=OFFLINE
```

### `arc_api_key`

API key for the ARC API. Required for `ONLINE` mode. If empty and not in offline mode, an anonymous key will be automatically fetched.

```python theme={null}
# Explicitly set API key
arc = Arcade(arc_api_key="your-api-key")

# Or use environment variable (recommended)
# export ARC_API_KEY="your-api-key"
arc = Arcade()
```

See [API Keys](/api-keys) for setup instructions.

### `environments_dir`

Directory to scan for local game files (`metadata.json`). Default: `"environment_files"`.

```python theme={null}
arc = Arcade(environments_dir="./my_games")
```

Environment variable: `ENVIRONMENTS_DIR`

### `recordings_dir`

Directory to save game recordings in JSONL format. Default: `"recordings"`.

```python theme={null}
arc = Arcade(recordings_dir="./my_recordings")
```

Environment variable: `RECORDINGS_DIR`

### `arc_base_url`

Base URL for the ARC API. Default: `"https://three.arcprize.org"`.

```python theme={null}
arc = Arcade(arc_base_url="https://custom-endpoint.example.com")
```

Environment variable: `ARC_BASE_URL`

### `logger`

Optional logger instance. If not provided, a default logger logging to STDOUT is created.

```python theme={null}
import logging

my_logger = logging.getLogger("my_agent")
arc = Arcade(logger=my_logger)
```

## Methods

### `make()`

Create and initialize an environment wrapper for a specific game.

**Signature:** `make(game_id, seed=0, scorecard_id=None, save_recording=False, render_mode=None, renderer=None)`

**Parameters:**

* `game_id` (`str`): Game identifier in format `'ls20'` or `'ls20-1234abcd'`. The first 4 characters are the game\_id, everything after `'-'` is the version.
* `seed` (`int`, optional): Random seed for the game. Defaults to `0`.
* `scorecard_id` (`str`, optional): Scorecard ID for tracking runs. If `None` is provided (the default), the system will create and maintain a single default scorecard that is automatically reused across all `make()` calls. This allows you to track multiple games in the same scorecard without explicitly managing scorecard IDs.
* `save_recording` (`bool`, optional): Whether to save recordings to JSONL file. Defaults to `False`.
* `render_mode` (`str`, optional): Render mode string (`"human"`, `"terminal"`, `"terminal-fast"`). If provided, creates a renderer automatically.
* `renderer` (`Callable[[int, FrameDataRaw], None]`, optional): Custom renderer function. If both `render_mode` and `renderer` are provided, `renderer` takes precedence.

**Returns:**

* `EnvironmentWrapper` or `None`: Returns an `EnvironmentWrapper` instance if successful, `None` otherwise.

**Example:**

```python theme={null}
env = arc.make("ls20", render_mode="terminal")
env = arc.make("ls20-1234abcd", seed=42, save_recording=True)
```

### `get_environments()`

Get the list of available environments (both local and remote).

**Returns:**

* `list[EnvironmentInfo]`: List of `EnvironmentInfo` objects representing available environments.

**Example:**

```python theme={null}
envs = arc.get_environments()
for env in envs:
    print(f"{env.game_id}: {env.title}")
```

### `create_scorecard()`

Create a new scorecard for tracking game runs.

**Signature:** `create_scorecard(source_url=None, tags=None, opaque=None)`

**Parameters:**

* `source_url` (`str`, optional): Optional source URL for the scorecard.
* `tags` (`list[str]`, optional): Optional list of tags for the scorecard. Defaults to `["wrapper"]`.
* `opaque` (`Any`, optional): Optional opaque data for the scorecard.

**Returns:**

* `str`: The ID of the newly created scorecard.

**Example:**

```python theme={null}
scorecard_id = arc.create_scorecard(
    source_url="https://github.com/my/repo",
    tags=["experiment", "v1"]
)
```

### `open_scorecard()`

Alias for `create_scorecard()`. Opens a new scorecard.

**Signature:** `open_scorecard(source_url=None, tags=None, opaque=None)`

**Parameters:** Same as `create_scorecard()`.

**Returns:**

* `str`: The ID of the newly created scorecard.

### `get_scorecard()`

Get a scorecard by ID, converted to `EnvironmentScorecard`.

**Signature:** `get_scorecard(scorecard_id=None)`

**Parameters:**

* `scorecard_id` (`str`, optional): Scorecard ID. If `None` is provided (the default), returns the default scorecard that the system is currently using (the same one created automatically when `make()` is called with `scorecard_id=None`).

**Returns:**

* `EnvironmentScorecard` or `None`: Scorecard object if found, `None` otherwise.

**Example:**

```python theme={null}
scorecard = arc.get_scorecard()
if scorecard:
    print(f"Score: {scorecard.score}")
    print(f"Games played: {len(scorecard.games)}")
```

### `close_scorecard()`

Close a scorecard and return the final scorecard data.

**Signature:** `close_scorecard(scorecard_id=None)`

**Parameters:**

* `scorecard_id` (`str`, optional): Scorecard ID. If `None` is provided (the default), closes the default scorecard that the system is currently using (the same one created automatically when `make()` is called with `scorecard_id=None`). After closing, the default scorecard is cleared and a new one will be created on the next `make()` call.

**Returns:**

* `EnvironmentScorecard` or `None`: Final scorecard object if found, `None` otherwise.

**Example:**

```python theme={null}
final_scorecard = arc.close_scorecard()
if final_scorecard:
    print(f"Final score: {final_scorecard.score}")
```
> ## Documentation Index
> Fetch the complete documentation index at: https://docs.arcprize.org/llms.txt
> Use this file to discover all available pages before exploring further.

# EnvironmentWrapper

> Generic Python Interface for Interacting with ARC-AGI 3 Environments

The `EnvironmentWrapper` class provides a common interface for interacting with environments, whether they are local (`LocalEnvironmentWrapper`) or remote (`RemoteEnvironmentWrapper`).

## Properties

### `observation_space`

Get the observation space (last response data).

**Returns:**

* `FrameDataRaw` or `None`: The `FrameDataRaw` object from the last response, or `None` if no response has been set yet.

**Example:**

```python theme={null}
obs = env.observation_space
if obs:
    print(f"Game state: {obs.state}")
    print(f"Levels completed: {obs.levels_completed}")
```

### `action_space`

Get the action space (available actions).

**Returns:**

* `list[GameAction]`: A list of `GameAction` objects representing available actions. Returns an empty list if no response has been set yet.

**Example:**

```python theme={null}
actions = env.action_space
print(f"Available actions: {[a.name for a in actions]}")
```

### `info`

Get the environment information.

**Returns:**

* `EnvironmentInfo`: The `EnvironmentInfo` object for this environment.

**Example:**

```python theme={null}
info = env.info
print(f"Game ID: {info.game_id}")
print(f"Title: {info.title}")
print(f"Tags: {info.tags}")
```

## Methods

### `reset()`

Reset the environment and return the initial frame data.

**Returns:**

* `FrameDataRaw` or `None`: `FrameDataRaw` object with initial game state, or `None` if reset failed.

**Example:**

```python theme={null}
obs = env.reset()
if obs:
    print("Environment reset successfully")
```

### `step()`

Perform a step in the environment.

**Signature:** `step(action, data=None, reasoning=None)`

**Parameters:**

* `action` (`GameAction`): The game action to perform (e.g., `GameAction.ACTION1`, `GameAction.ACTION2`).
* `data` (`dict[str, Any]`, optional): Optional action data dictionary. For complex actions, should contain `"x"` and `"y"` coordinates.
* `reasoning` (`dict[str, Any]`, optional): Optional reasoning dictionary to include in recordings.

**Returns:**

* `FrameDataRaw` or `None`: `FrameDataRaw` object with updated game state, or `None` if step failed.

**Example:**

```python theme={null}
from arcengine import GameAction

# Simple action
obs = env.step(GameAction.ACTION1)

# Complex action with coordinates
obs = env.step(
    GameAction.ACTION6,
    data={"x": 32, "y": 32},
    reasoning={"thought": "clicking center of screen"}
)

# Check game state after step
if obs and obs.state == GameState.WIN:
    print("Game won!")
```
