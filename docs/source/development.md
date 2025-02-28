# Faasm Development

The Faasm project is roughly separated into stuff that generates WebAssembly and
stuff that executes WebAssembly.

The most important repos

- [faasm/cpp](https://github.com/faasm/cpp) - tools for building C/C++ for use
  in Faasm.
- [faasm/python](https://github.com/faasm/python) - tools for building CPython
  and Python functions for use in Faasm.
- [faasm/faasm](https://github.com/faasm/faasm) - the Faasm runtime (independent
  of specific languages used to compile the WebAssembly)
- [faasm/faabric](https://github.com/faasm/faabric) - serverless scheduling,
  messaging and state (independent of WebAssembly).

## Initial Set-up

The Faasm development environment is containerised, as defined in the [Docker
compose config](docker-compose.yml).

We mount local checkouts of all the code into these containers, so first you'll
need to update the project submodules (may take a while):

```bash
git submodule update --init --recursive
```

To tie together the projects in the local dev environment, we mount
`dev/faasm-local` into `/usr/local/faasm` in the various containers, which you
can initialise with:

```bash
./bin/refresh_local.sh
```

## Use

Once you've set up the repo, you should first run the Faasm CLI to make sure
things work:

```bash
./bin/cli.sh faasm

# Wait for 30s while it sets up the Python environment in the background

# Check the python environment is up (should list commands)
inv -l

# Run the CMake build
inv dev.cmake
inv dev.tools
```

From there you can compile and run functions using the language-specific
containers (running the following from the local machine, not inside the Faasm
CLI container):

```bash
# C++ functions
./bin/cli.sh cpp

# Python functions
./bin/cli.sh python
```

## Tests

To run the tests, you can execute the following (note that you have to switch
between containers for the first few steps):

```bash
# --- CPP CLI ---
# Build CPP functions and lib required for the tests
inv func.local libfake

# --- Python CLI ---
# Build Python wrapper function and upload Python code
inv func func.upload-all --local

# --- Faasm CLI ---
# Build the development tools
inv dev.tools

# Run codegen (this may take a while the first time it's run)
inv codegen.local python.codegen

# Set up cgroup
./bin/cgroup.sh

# Run the tests
tests
```

## Tooling - editors, IDEs etc.

You can use custom containers that inherit from the existing CLI images if you
want to add text editors etc.

Before running the `./bin/cli.sh` script, you need to set one or more of the
following environment variables:

```bash
# Faasm
FAASM_CLI_IMAGE

# Faabric
FAABRIC_CLI_IMAGE

# CPP
CPP_CLI_IMAGE

# Python
PYTHON_CLI_IMAGE
```

The defaults for these are set in [`.env`](../.env).

## Testing

We use [Catch2](https://github.com/catchorg/Catch2) for testing and your life
will be much easier if you're familiar with their [command line
docs](https://github.com/catchorg/Catch2/blob/v2.x/docs/command-line.md).  This
means you can do things like:

```bash
# Run all the MPI tests
tests "[mpi]"

# Run a specific test
tests "Test some feature"
```

## Code style

Code style is checked as part of the CI build and uses the following
configuration:

- **C/C++** - [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html)
  with the [Mozilla C++
  style](https://firefox-source-docs.mozilla.org/code-quality/coding-style/coding_style_cpp.html)
- **Python** - [Black](https://github.com/psf/black) with an 80 char line limit
- **Markdown** - wrap to 80 characters where possible (not enforced)
- **CMake** - TBD
- **Bash** - TBD

## Networking

If you want to switch on network isolation you need to set up network
namespaces. To do this we need to ensure consistent interface naming (`eth0` for
main public interface). If your public interface is already called `eth0` then
you can skip this step.

- Edit `/etc/default/grub` and add `net.ifnames=0 biosdevname=0` to
  `GRUB_CMDLINE_LINUX_DEFAULT`
- Run `sudo update-grub`
- Restart the machine

Then, this script will set up the namespaces. From outside the container run:

```bash
source ./bin/workon.sh
inv network.create-ns 100
```

## Cgroups

To use cgroup isolation, you'll need to run:

```bash
sudo ./bin/cgroup.sh
```

## Running a local development cluster

To start the local development cluster, you can run:

```bash
inv cluster.start
```

To run external applications and benchmarks against this cluster, you may also
need to set up your config file to point at this cluster:

```bash
inv k8s.ini-file --local
```

### Making changes in your local cluster

Assuming you've changed something related to the `pool_runner` target (which is
executed by the `worker` container), you can pick up the changes with:

```bash
# Start the Faasm CLI
./bin/cli.sh faasm

# Rebuild the pool_runner target
inv dev.cc pool_runner
```

From a different terminal, restart the worker and check the logs:

```bash
inv cluster.restart-worker

docker compose logs -f
```

### Running distributed tests locally

The distributed tests check the interactions of the Faasm workers, file storage,
redis and upload containers.

First of all, you need to make sure everything else is shut down to avoid
interfering with the tests:

```bash
docker compose down
```

Make sure your local setup is built, along with the distributed tests:

```bash
# Enter CLI container
./bin/cli.sh faasm

# Build local environment
inv dev.tools

# Build dist tests
inv dev.cc dist_tests dev.cc dist_test_server
```

Outside the container, start the distributed tests server, then upload all the
functions:

```bash
# Start server
./deploy/dist-test/dev_server.sh

# Upload everything
./deploy/dist-test/upload.sh

# Run the tests
./deploy/dist-test/run.sh
```

You can then rebuild and rerun from inside the container:

```bash
# Build tests and the server
inv dev.cc dist_tests dev.cc dist_test_server

# Run
dist_tests
```

If changing the server, you need to restart from outside the container:

```bash
./deploy/dist-test/dev_server.sh restart
```

## Building outside of the container

**This is not the recommended approach to developing Faasm**, so it's not
scripted.

However, it can be useful for profiling.

You can work out what's required by looking at the following Dockerfiles:

- [`faasm/faabric-base`](https://github.com/faasm/faabric/blob/main/docker/faabric-base.dockerfile)
- [`faasm/cpp-root`](https://github.com/faasm/faabric/blob/main/docker/cpp-root.dockerfile)

The following notes were correct at the time of writing but aren't necessarily
kept up to date, so YMMV:

### LLVM and Clang

LLVM and Clang can be installed using the script from the LLVM website (we use
13 _and_ 10 at the time of writing):

```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 13
sudo ./llvm.sh 10
```

You then need to make sure all the tooling and latest C++ stdlib is installed:

```bash
sudo apt install -y g++-11 libc++-dev libc++abi-13-dev clang-tools-13 clang-format-13 clang-tidy-13
```

On Ubuntu 18.04 you need to add a specific apt repo to install g++-11:

```bash
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt install -y g++-11
```

### Conan

Follow the Conan [installation docs](https://conan.io/downloads.html).

If you've also been building using the containerised setup, it's safest to also
nuke your conan cache:

```bash
rm -rf ~/.conan
```

### CMake

You can check what version of CMake you already have. If it's above the minimum
required for Faasm you don't need to change it.

If you need to update it's best to install from source as per [the
instructions](https://cmake.org/install/).

### Python environment

You will then need to set up the Faasm Python environment:

```bash
./bin/create_venv.sh

source bin/workon.sh

inv -l
```

### Local dev files

You can pull the latest development files using:

```bash
./bin/refresh_local.sh
```

### Tidy-up

If you've been running the containerised development environment too, it's
highly likely that the permissions will be messed up and cause cryptic error
messages. To be safe, reset the permissions on everything:

```bash
sudo sh -c 'chown -R ${SUDO_USER}:${SUDO_USER} .'
```

### Building

Then you can try building one of the executables:

```bash
# Check environment is correct and build directories look sane
env | grep FAASM

# Run cmake and build
inv dev.cmake
inv dev.cc func_runner

# Check which binary is on the path
which func_runner
```

### Minio and Redis

Before running one of the executables, you must make sure that the `minio`
container is running, and reachable from outside the container.

```
# Stop anything that may be running in the background
docker compose down

docker compose up -d redis-state redis-queue minio

docker compose ps
```

To tell Faasm outside the container to use the containerised Minio, set the
following:

```bash
export S3_HOST=localhost
```

You also need Redis available locally:

```bash
sudo apt install redis-server redis-cli -y
```

### Compile and run a function

Compile a function with the CPP CLI:

```bash
./bin/cli.sh cpp

inv func demo hello
```

Then back in the Faasm root:

```bash
inv dev.tools
inv codegen demo hello
inv run demo hello
```

### Running against a dev cluster

To run your out-of-container build in a dev cluster, you need to specify the
location of the built files, which is handled by the environment variables set
in `bin/workon.sh`.

```bash
source bin/workon.sh
./deploy/local/dev_cluster.sh
```
