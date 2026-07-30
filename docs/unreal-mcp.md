# Unreal MCP Contributor Workflow

City Form includes optional editor-only support for Unreal Engine's Model
Context Protocol (MCP) plugin. It lets a compatible local development client
inspect the running editor and invoke explicitly exposed editor tools.

MCP is contributor tooling only. It is not required to build or run City Form,
does not participate in the authoritative simulation, and must not become a
packaged-game dependency.

## Requirements

- Unreal Engine 5.8
- A development client that supports remote HTTP MCP servers
- The City Form repository opened as a trusted workspace in that client

The project descriptor enables these plugins for editor targets:

- `ModelContextProtocol`
- `EditorToolset`
- `AutomationTestToolset`

`ModelingToolsEditorMode` remains enabled by the original Unreal project.

## Connect a Local Client

The repository's `.codex/config.toml` configures an optional server named
`unreal-mcp`:

```toml
[mcp_servers.unreal-mcp]
url = "http://127.0.0.1:8000/mcp"
enabled = true
required = false
startup_timeout_sec = 3
tool_timeout_sec = 60
default_tools_approval_mode = "prompt"
```

The server is deliberately optional so development clients can start while the
Unreal editor is closed. After cloning:

1. Open `CityForm/CityForm.uproject` in Unreal Engine 5.8.
2. Confirm the three MCP-related plugins are enabled if Unreal asks to update
   the project.
3. Start the MCP server from Unreal's MCP editor controls if it is not already
   configured to start automatically.
4. Restart or reconnect the development client after trusting the repository.
5. List the available MCP toolsets. A working connection should expose editor
   and automation-test toolsets.

The exact client interface may differ, but the endpoint remains
`http://127.0.0.1:8000/mcp`.

## Verify the Connection

Use a read-only editor operation first:

1. List the available toolsets.
2. Enable or inspect `EditorToolset`.
3. Request the current editor selection.

An empty actor selection is a successful response when nothing is selected.
A connection error, timeout, or missing toolset is not.

## Run Automation Tests

With the editor and MCP server running:

1. Inspect or enable `AutomationTestToolset`.
2. Discover tests.
3. Filter the test list to the relevant namespace, such as
   `CityForm.Simulation.Foundation`.
4. Run the filtered tests.
5. Retrieve the completed results and include them in the pull request's test
   notes.

MCP test execution complements command-line testing; it does not replace a
normal build. If no matching tests exist yet, discovery should still complete
without an editor crash or protocol error.

## Security

The configured server listens on the loopback interface and currently has no
authentication layer. Treat it as local developer access to the open editor:

- Do not change the endpoint to a public or LAN-facing address.
- Do not tunnel or proxy it to an untrusted network.
- Keep tool approvals enabled for operations that can change project state.
- Review proposed editor mutations before approving them.
- Stop the server when it is not needed on an untrusted machine or network.
- Never place credentials or tokens in repository MCP configuration.

## Troubleshooting

### The client reports `No such file or directory`

Restart the development client and verify its own MCP/runtime installation.
This error can occur before it attempts to contact Unreal and does not
necessarily mean the Unreal plugin is misconfigured.

### The client cannot connect

- Confirm Unreal Engine is open with the City Form project.
- Confirm the MCP server is running on port `8000`.
- Confirm no other process is using that port.
- Restart the MCP server, then reconnect the client.
- Verify the URL is exactly `http://127.0.0.1:8000/mcp`.

### The server connects but expected tools are absent

- Confirm `EditorToolset` and `AutomationTestToolset` are enabled.
- Ask the server to list toolsets again after enabling a plugin.
- Restart Unreal after changing plugin state.
- Rebuild editor modules if Unreal reports an incompatible module.

### Tests are missing or stale

Force automation-test rediscovery, then list tests using the full namespace.
Recompile the editor target after adding or renaming C++ automation tests.

### Unreal fails during a macOS post-build step

Record the full command and log in the relevant GitHub issue. A failure in an
Xcode-generated post-build action can be distinct from a C++ compiler error;
do not report the module as failing to compile when compilation and linking
already succeeded.

## Maintenance Rules

- Keep MCP plugins restricted to editor targets.
- Do not reference MCP modules from `CityForm` or `CitySimulation`.
- Keep the server optional in contributor configuration.
- Document any added toolset, permission, or network exposure before enabling
  it for the project.
- Verify macOS Apple Silicon and Windows compatibility before making MCP part
  of a required workflow.
