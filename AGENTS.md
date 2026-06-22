# AGENTS.md - Amarok Development Guide

## Build Commands

```bash
# Configure (from project root)
cmake -B build -DCMAKE_BUILD_TYPE=Release  # or Debug

# Configure with SQLite only (no MySQL/MariaDB required)
cmake -B build -DWITH_MYSQL=OFF -DWITH_EMBEDDED_DB=OFF -DBUILD_TESTING=OFF

# Build single component
cmake --build build --target amarok                      # full build
cmake --build build --target <target>                  # specific target

# Tests (requires gmock)
cmake -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Architecture

- **src/** - Main application: core/, core-impl/, services/, importers/, scripting/, context/, browsers/
- **tests/** - Google Mock-based test suites
- **shared/** - Shared utilities
- Entry point: Main.cpp in src/

## Important Dependencies

- Qt 6.7+, KDE Frameworks 6.5+
- GStreamer 1.10+ (audio backend)
- TagLib 1.12+ (must have ASF, MP4, OPUS support)
- MariaDB/MySQL (embedded DB optional, requires libmariadbd)
- SQLite (via Qt6::Sql) - optional storage backend

## Storage Backends

- **Embedded MySQL**, **External MySQL**, **SQLite** - selectable via `DatabaseBackend` config (0/1/2)
- SQLite stored in `~/.local/share/amarok/amarok.db` by default
- Config group: `MySQL` (for MySQL backends) and `SQLite` (for SQLite)

## Build Options

- `WITH_PLAYER=OFF` - CLI-only build
- `WITH_MYSQL=OFF` - Build with SQLite only (no MySQL/MariaDB dependencies)
- `WITH_EMBEDDED_DB=OFF` - Skip embedded MariaDBd
- `WITH_IPOD`, `WITH_LASTFM`, `WITH_GPODDER` - Optional features

## Code Style (enforced)

- 4 spaces, no tabs
- 90 char line limit
- CamelCase file names (MyClass.cpp)
- Include order: own header → Amarok → KDE → Qt → other
- Member variables: `m_` prefix
- Use `0` for null pointers (not NULL)

## Testing

- Tests use Google Mock in `tests/`
- Test config: `tests/config-amarok-test.h.cmake`
- Run all: `ctest --test-dir build`
- Run specific: `ctest --test-dir build -R <regex>`

## Debugging

```cpp
#include "debug.h"
debug() << "message";
warning() << "warn";
error() << "err";
```

## CI

Uses KDE GitLab CI templates (`.gitlab-ci.yml`).