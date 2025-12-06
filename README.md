# SafeRun

A signer-based runtime policy enforcement system for macOS. SafeRun controls what a program can do at runtime based on **who signed it**, not just whether it's signed.

## Overview

When you run `saferun ./prog`, it:

1. Verifies the digital signature on `./prog` using OpenSSL
2. Extracts the signer identity (e.g., `CN=Oracle Software,O=Oracle`)
3. Loads a YAML policy profile for that signer
4. Launches the program with a shim library that intercepts system calls
5. Evaluates each syscall against the signer's policy and allows or blocks it

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                          saferun                                │
│  (orchestrator: signature verification, policy lookup, launch)  │
└─────────────────┬───────────────────────────────────────────────┘
                  │
        ┌─────────▼─────────┐
        │    controller     │
        │ (Python: policy   │◄──── policy.yaml
        │  evaluation)      │
        └─────────▲─────────┘
                  │ Unix socket (request/response)
        ┌─────────▼─────────┐
        │   preload.dylib   │
        │ (C shim: syscall  │
        │  interception)    │
        └─────────▲─────────┘
                  │ DYLD_INSERT_LIBRARIES
        ┌─────────▼─────────┐
        │  target program   │
        └───────────────────┘
```

## Installation

```bash
git clone <repository-url>
cd SafeRun

# Build the shim
cd shim && make && cd ..

# Make scripts executable
chmod +x saferun controller scripts/*.sh
```

## Quick Start

### 1. Generate a signer identity

```bash
./scripts/generate_signer.sh "CN=Oracle Software/O=Oracle" oracle
```

This creates `oracle-key.pem` (private key) and `oracle-cert.pem` (certificate).

### 2. Create a signed bundle

```bash
./scripts/sign_bundle.sh your_program.cpp oracle
```

This compiles, signs, and bundles the program into `demo/bundle_oracle/`.

### 3. Configure the policy

Edit `policy.yaml` to define what the signer can do:

```yaml
signers:
  "CN=Oracle Software,O=Oracle":
    allowed_paths:
      - "/tmp/**"
      - "/Users/**"
    denied_paths:
      - "/etc/**"
      - "~/.ssh/**"
    allow_network: false
    allowed_executables:
      - "/bin/ls"
      - "/usr/bin/cat"
    denied_executables:
      - "/bin/sh"
      - "/bin/bash"
```

### 4. Run the program

```bash
cd demo/bundle_oracle
../../saferun ./your_program
```

## Policy Configuration

Policies are defined per-signer in `policy.yaml`:

| Field | Type | Description |
|-------|------|-------------|
| `allowed_paths` | list | Glob patterns for permitted file access |
| `denied_paths` | list | Glob patterns for blocked file access (checked first) |
| `allow_network` | bool | Whether outbound network connections are permitted |
| `allowed_executables` | list | Programs that can be executed via execve |
| `denied_executables` | list | Programs blocked from execution (checked first) |

Unknown signers fall back to the `unknown_signer` policy.

## Intercepted System Calls

| Syscall | Policy Control | Behavior on Deny |
|---------|----------------|------------------|
| `open()` / `openat()` | `allowed_paths`, `denied_paths` | Returns -1, errno=EACCES |
| `connect()` | `allow_network` | Returns -1, errno=EACCES |
| `execve()` | `allowed_executables`, `denied_executables` | Returns -1, errno=EACCES |

## File Structure

```
SafeRun/
├── saferun              # Main orchestrator script
├── controller           # Policy evaluation daemon
├── policy.yaml          # Signer policies
├── shim/
│   ├── preload.c        # Syscall interposition library
│   └── Makefile
├── py/
│   └── signature_check.py  # Signature verification module
├── scripts/
│   ├── generate_signer.sh  # Create signing identity
│   └── sign_bundle.sh      # Sign and bundle a program
├── examples/
│   └── *.cpp            # Test programs
└── demo/
    └── bundle_*/        # Signed program bundles
```

## Testing

### Test file access control

```bash
# oracle_tool.cpp tries to read /etc/hosts (should be blocked)
cd demo/bundle_oracle
../../saferun ./oracle_tool
```

### Test executable control

```bash
# spawn_test.cpp tries to run /bin/sh (should be blocked) and /bin/ls (should be allowed)
./scripts/sign_bundle.sh spawn_test.cpp oracle
cd demo/bundle_oracle
../../saferun ./spawn_test
```

### Test different signer policies

```bash
# Create Microsoft signer (has allow_network: true)
./scripts/generate_signer.sh "CN=Microsoft Corporation/O=Microsoft" microsoft
./scripts/sign_bundle.sh oracle_tool.cpp microsoft
cd demo/bundle_microsoft
../../saferun ./oracle_tool
# Network access should be ALLOWED (unlike Oracle)
```