# SHARK_3_0_Qt

![C++23](https://img.shields.io/badge/C%2B%2B-23-1f6feb)
![UI](https://img.shields.io/badge/UI-Qt%206-41cd52)
![Storage](https://img.shields.io/badge/Storage-PostgreSQL-336791)
![Platforms](https://img.shields.io/badge/Platforms-macOS%20%7C%20Linux-555)

> The desktop ChatBot Shark implementation: Qt 6 client, PostgreSQL-backed server, richer interaction flow, logging and connection-state monitoring.

Back to the repository overview: [README.md](../README.md)

---

## [01] Snapshot

| Aspect | Current implementation |
| --- | --- |
| Purpose | move the project from terminal interaction to desktop UI without hiding the backend/server side |
| UI | Qt 6 widgets application |
| Storage | PostgreSQL |
| Discovery | `localhost -> LAN` only in the current Qt transport layer |
| Main visual | desktop UI workflow |

## [02] What This Version Adds

- Qt 6 desktop client instead of CLI menus.
- Logging to file plus UI-side log controls.
- Connection monitor thread with visible state changes.
- User profile flows, contact search, block/unblock and ban/unban operations.
- Richer chat and participant-picking UX.
- Transaction-oriented PostgreSQL work with client-side acknowledgement.

## [03] Discovery Logic

The current Qt transport does **not** use Internet/DDNS lookup. It only tries:

1. `localhost`
2. local network discovery

That behavior is implemented in:

- [src/client/tcp_transport/session_types.h](src/client/tcp_transport/session_types.h)
- [src/client/tcp_transport/tcp_transport.cpp](src/client/tcp_transport/tcp_transport.cpp)

## [04] UI Surface

- Login and registration are implemented as separate Qt forms.
- The desktop client includes logs, chat list, contact list and user profile flows.
- Connection-state monitoring and log visibility are part of the actual user workflow.

## [05] Project Layout

```text
Shark_3_0_Qt/
├── CMakeLists.txt
├── config/
├── README.md
├── Screens/
├── Shark_UI/
├── scripts/
└── src/
```

## [06] Build And Run

Create the runtime config:

```bash
cp config/connect_db.local-docker.example config/connect_db.conf
```

The local template uses `"sslmode": "disable"` because the development Docker/desktop PostgreSQL setup does not expose TLS by default.

Optional local PostgreSQL backend:

```bash
task docker:up
task docker:delete   # full reset of the local PostgreSQL container and volume
```

### macOS

```bash
cp config/connect_db.local-docker.example config/connect_db.conf
bash scripts/build_macos.sh
./build_macos/server &
./build_macos/Shark_UI/Shark_ui
```

### Linux

```bash
bash scripts/build_linux.sh
./dist/linux/server/run_server.sh &
./dist/linux/client/run_client.sh
```

## [07] Operational Notes

- The server requires `config/connect_db.conf`.
- Database recreation is exposed from the login UI for demo/test use.
- Logging and connection state are part of the user-visible workflow.
- VS Code tasks can bootstrap `config/connect_db.conf` from the local Docker template and block launch while the file is missing or invalid.

### Local Editor Tooling

- [`.clang-format`](.clang-format) defines the local formatting style
- [`.vscode/settings.json`](.vscode/settings.json) points IntelliSense to `build_macos/compile_commands.json`
- [`.vscode/tasks.json`](.vscode/tasks.json) contains build and format tasks
- [`.vscode/launch.json`](.vscode/launch.json) contains launch profiles for `server` and `Shark_ui`
- `docker db up` / `docker db down` manage a local PostgreSQL container through `config/docker-compose.yml`
- `docker db delete` removes the local PostgreSQL container, network, and volume data

### Taskfile workflow

- [Taskfile.yaml](Taskfile.yaml) is the project entrypoint for build, format, run and Docker orchestration
- `task build:macos`
- `task format`, `task format:check`
- `task run:server`, `task run:ui`, `task run:client`
- `task config:bootstrap`
- `task docker:up`, `task docker:down`, `task docker:delete`, `task docker:logs`
- `config/.env.local-docker.example` defines the local Docker compose pattern
- `config/connect_db.local-docker.example` is the tracked DB config template

## [08] License

Released under the MIT License - see [LICENSE](../LICENSE).
