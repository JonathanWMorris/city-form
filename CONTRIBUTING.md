# Contributing to City Form

City Form is at the beginning of development, and contributions from
programmers, designers, simulation researchers, technical artists, testers, and
writers are welcome.

The most valuable early contributions are small, well-explained changes that
strengthen the v0.1 loop or make the project easier for the next contributor to
understand.

## Start with the Project Direction

Before proposing implementation work, read:

- [Project Values](docs/project-values.md)
- [v0.1 Product Scope](docs/product-scope.md)
- [Domain Model](docs/domain-model.md)
- [Architecture](docs/architecture.md)
- [Simulation Foundation](docs/simulation-foundation.md)
- [Traffic Model](docs/traffic-model.md)
- [Architecture Decision Records](docs/decisions/README.md)
- [Roadmap](docs/roadmap.md)

For substantial features or architectural changes, open a GitHub issue before
writing a large patch. Explain the user need, intended behavior, alternatives,
and effect on v0.1 scope. Early discussion reduces duplicated work and makes
important decisions visible.

Small fixes and documentation improvements do not need a lengthy proposal.

## Development Setup

You will need:

- Git LFS
- Unreal Engine 5.8
- A C++ toolchain supported by that Unreal version on your platform

Clone and prepare the repository:

```sh
git lfs install
git clone https://github.com/JonathanWMorris/city-form.git
cd city-form
git lfs pull
```

Open `CityForm/CityForm.uproject`. Allow Unreal to generate local project files
and build the C++ module if prompted.

macOS on Apple Silicon is currently the verified environment. Windows
contributors are welcome, but should describe their engine, compiler, GPU API,
and test results because Windows has not yet been verified by the maintainer.

### Optional Unreal editor automation

The project includes an optional, editor-only Unreal Model Context Protocol
(MCP) configuration for contributors using compatible development tools. It is
not required to build, run, or contribute to City Form, and it is never a
runtime dependency of the game or simulation.

See [Unreal MCP Workflow](docs/unreal-mcp.md) for setup, security,
troubleshooting, and automation-test instructions.

## Contribution Workflow

1. Start from an up-to-date `main` branch.
2. Create a focused branch for one change.
3. Keep commits understandable and avoid unrelated formatting or asset churn.
4. Add or update tests and documentation with the behavior they cover.
5. Build and run the relevant Unreal tests when possible.
6. Review the staged file list for generated directories before committing.
7. Open a pull request describing what changed, why, how it was tested, and what
   remains incomplete.

Generated `Binaries`, `DerivedDataCache`, `Intermediate`, `Saved`, IDE
workspaces, and personal developer content must not be committed. Binary Unreal
assets and configured large source formats belong in Git LFS.

## Architecture Expectations

- `CitySimulation` is authoritative; `CityForm` presents and manipulates it.
- Simulation code must not depend on cameras, meshes, UI, loaded levels, or
  presentation classes.
- Persistent simulated entities use compact records and stable typed IDs, not
  one Actor per entity.
- Player tools submit explicit commands instead of mutating simulation
  containers directly.
- Visible vehicles and pedestrians represent simulation records but do not own
  their authoritative state.
- Simulation time and random state must be explicit where deterministic
  behavior is promised.
- Platform-specific code must be isolated and justified.
- New dependencies, plugins, and experimental engine features require prior
  discussion and a clear portability and maintenance case.

Avoid speculative frameworks. Introduce the smallest abstraction that solves
the current milestone while preserving the documented boundaries.

## C++ and Unreal Practices

- Follow Unreal naming and style conventions in engine-facing code.
- Prefer explicit ownership and data flow in the simulation core.
- Keep headers focused and dependencies minimal.
- Use standard C++ or platform-neutral Unreal APIs where practical.
- Use Blueprints for visual assembly and configuration, not as the sole owner of
  authoritative simulation rules.
- Include enough context in validation failures and logs to identify the
  affected record and invariant.

Automated formatting is not yet configured. Match the surrounding code and
avoid reformatting unrelated files.

## Testing

Authoritative simulation changes should include automated tests appropriate to
their behavior. Important early cases include:

- Deterministic results for controlled seeds and command sequences
- Valid road-node and road-segment references
- Rejection or reporting of dangling IDs
- Non-negative capacities and assignments within capacity
- Valid trip origins, destinations, and routes
- Simulation advancement without a gameplay viewport

For performance-sensitive work, provide the scenario, hardware context, build
configuration, before-and-after measurements, and any quality tradeoff.

If a relevant test cannot be run locally, state that clearly in the pull
request rather than implying success.

Contributors using the optional Unreal MCP workflow can discover and run tests
from a live editor as described in [Unreal MCP Workflow](docs/unreal-mcp.md).

### Verified macOS commands

From the repository root, set the path to your Unreal Engine 5.8 installation:

```sh
CITY_FORM_UE_ROOT="/Users/Shared/Epic Games/UE_5.8"
```

Build the editor target:

```sh
"$CITY_FORM_UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh" \
  CityFormEditor Mac Development \
  -Project="$PWD/CityForm/CityForm.uproject" \
  -WaitMutex
```

Close the Unreal editor before using the command-line build and headless-test
workflow. When the editor is open, UnrealBuildTool may emit a numbered
hot-reload module while a separate commandlet loads an older base module.
Use the MCP workflow below when tests need to run against the open editor.

Compile the game target without its Xcode post-build/deploy step:

```sh
"$CITY_FORM_UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh" \
  CityForm Mac Development \
  -Project="$PWD/CityForm/CityForm.uproject" \
  -WaitMutex -NoLink
```

The compile-only form is temporary while
[the macOS PostBuildSync failure](https://github.com/JonathanWMorris/city-form/issues/4)
is investigated.

Run the complete simulation test suite without a viewport:

```sh
"$CITY_FORM_UE_ROOT/Engine/Binaries/Mac/UnrealEditor-Cmd" \
  "$PWD/CityForm/CityForm.uproject" \
  -NullRHI -unattended -nop4 -nosplash \
  -ExecCmds="Automation RunTests CityForm.Simulation; Quit" \
  -TestExit="Automation Test Queue Empty" -log
```

The command must report matching tests with successful results. An exit code of
zero alone is not enough; review the automation result lines in the Unreal log.

## Documentation

Repository Markdown is the source of truth. Update the relevant document when a
change affects:

- Player-visible scope or behavior
- Architectural responsibilities or dependency direction
- Setup, build, or testing instructions
- Platform compatibility
- Roadmap exit criteria
- Accepted architecture decisions

Documentation should distinguish current behavior from planned behavior.

## Reporting Problems

A useful issue includes:

- What you expected and what happened
- Reproduction steps or a minimal scenario
- Unreal Engine version and build configuration
- Operating system, architecture, and relevant hardware
- Logs, screenshots, or recordings when they clarify the problem
- Whether the issue reproduces from a clean checkout

Remove private information and credentials from logs before posting them.

## Working Together

Be respectful, specific, and patient. Critique ideas and implementations rather
than people. Assume that contributors have different backgrounds and levels of
familiarity with Unreal, C++, simulation, and city planning.

Maintainers may decline work that conflicts with project values, v0.1 scope, or
architectural boundaries. The goal is to explain those decisions clearly and
leave a useful public record.

By contributing, you agree that your contribution may be distributed under the
repository's [MIT License](LICENSE).
