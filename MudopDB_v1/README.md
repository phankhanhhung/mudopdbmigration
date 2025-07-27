# MudopDB_v1

Small experimental C++ project demonstrating a tiny database-style app with a modular layout.

## Repo layout

- `include/` : public headers (api, helper)
- `src/` : implementation and `main.cpp`
- `src/api` : driver/connection/statement implementations
- `src/helper` : small helpers
- `Dockerfile.debian` : Debian-based multi-stage Dockerfile to build and run the project
- `Dockerfile` : Ubuntu-based multi-stage Dockerfile

## Build (local, out-of-source)

```bash
# from repo root
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --parallel

# run
./src/mudopdb
```

## Run locally: app vs tests

- Run the app (executes `main.cpp`):
```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --parallel
./src/mudopdb
```

- Run tests (uses GoogleTest; does NOT run `main.cpp`):
```bash
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
cmake --build . --parallel
ctest --output-on-failure
```

- Run a single test by name (example):
```bash
ctest -R UtilityTrim -V
```

## Run in Docker (Debian)

This repo ships a multi-stage Dockerfile (`Dockerfile.debian`) with three stages:
- `build`: compiles everything with CMake (tests enabled)
- `tests`: runs unit tests via CTest (container exits non‑zero if tests fail)
- `runtime`: a slim image that only contains the app binary

### Build and run the app (runtime image)

```bash
# Build the runtime image (uses the build stage under the hood)
docker build -t mudopdb:runtime -f Dockerfile.debian .

# Run the app interactively
docker run --rm -it mudopdb:runtime

# Non-interactive: provide connection string via stdin and then commands
echo "connection-string" | docker run --rm -i mudopdb:runtime

# Or pass the connection string as an argument (main supports argv[1])
# and feed commands via stdin (quit to exit)
printf "quit\n" | docker run --rm -i mudopdb:runtime mem://test
# Here-doc variant
docker run --rm -i mudopdb:runtime mem://test << 'EOF'
quit
EOF
```

What happens:
- Docker builds sources in the build stage, then copies `/workspace/build/src/mudopdb` into a small Debian base.
- The container entrypoint is the app (`/app/mudopdb`).

### Build and run tests (tests stage)

```bash
# Build an image that runs the unit tests as its entrypoint
docker build --target tests -t mudopdb:tests -f Dockerfile.debian .

# Execute tests (ctest --output-on-failure runs inside the container)
docker run --rm mudopdb:tests

# Run a single test (args are appended to ctest entrypoint)
docker run --rm mudopdb:tests -R App.QuitImmediately -V
```

Notes:
- Tests are built with `-DBUILD_TESTS=ON` in the Docker build stage.
- The tests container exits with code 0 on success; any failing test makes the container exit non‑zero.
- `.dockerignore` excludes local build artifacts to keep images smaller and cache stable.

## Notes & next steps
- The project uses CMake and builds a static helper library plus an executable.
- For development, prefer out-of-source builds and keep `build/` excluded from Docker context (`.dockerignore` included).
- I can add CI (GitHub Actions) to run builds on push and run unit tests.
