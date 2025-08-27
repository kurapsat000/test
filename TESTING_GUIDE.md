# Fast Multi-OS Testing Guide for DuckDB Snowflake Extension

This guide provides multiple testing approaches from fastest to most comprehensive, allowing you to test across all operating systems efficiently.

## 🚀 Quick Testing Methods (Fastest → Slowest)

### 1. ⚡ Super-Fast Local Validation (5 seconds)

Tests syntax and configuration without building anything:

```bash
./scripts/validate-deps.sh
```

**What it tests:**

- ✅ CMake syntax validation
- ✅ vcpkg.json JSON syntax
- ✅ Dependency resolution order
- ✅ OS-specific compatibility checks
- ✅ Common configuration issues

**Use when:** Making small changes to CMakeLists.txt or vcpkg.json

---

### 2. 🐳 Local GitHub Actions Testing (2-5 minutes)

Uses `act` to run GitHub Actions locally with Docker:

```bash
./scripts/test-local.sh
```

**Interactive menu options:**

1. **Test basic dependencies** (2 min) - Tests protobuf, gRPC, OpenSSL
2. **Test Arrow dependencies** (5 min) - Tests complete Arrow chain
3. **Test both** (7 min) - Full local test suite
4. **List workflows** - See available tests

**What it tests:**

- ✅ Real dependency installation
- ✅ CMake configuration with vcpkg
- ✅ Cross-compilation scenarios
- ✅ Linking validation
- 🔄 Simulates Linux environment locally

**Use when:** Testing significant changes before pushing to GitHub

---

### 3. ☁️ GitHub Actions Multi-OS Testing (5-10 minutes)

Trigger comprehensive testing across all platforms:

```bash
# Push to trigger the test workflow
git push origin Andy-main-full

# Or manually trigger via GitHub web interface:
# Go to Actions → "Fast Multi-OS Build Test" → "Run workflow"
```

**What it tests:**

- ✅ **Linux (Ubuntu)**: Native builds with apt packages
- ✅ **macOS**: Homebrew integration and ARM64 compatibility
- ✅ **Windows**: MSVC toolchain and chocolatey packages
- ✅ Complete Arrow[flight] dependency chain
- ✅ Cross-platform vcpkg integration
- ✅ Real build environment simulation

**Use when:** Final validation before merging or for cross-platform issues

---

## 🎯 Testing Strategy by Change Type

### Configuration Changes (CMakeLists.txt, vcpkg.json)

```bash
# 1. Quick syntax check (5 sec)
./scripts/validate-deps.sh

# 2. If OK, test locally (2 min)
./scripts/test-local.sh  # Choose option 1

# 3. If major changes, test all platforms
git push  # Triggers multi-OS workflow
```

### Dependency Changes (Adding/removing packages)

```bash
# 1. Validate JSON syntax
./scripts/validate-deps.sh

# 2. Test complete dependency chain locally
./scripts/test-local.sh  # Choose option 3

# 3. Test all platforms
git push  # Essential for dependency changes
```

### Build System Changes

```bash
# Always test all platforms immediately
git push
```

---

## 🛠️ Setup Instructions

### Prerequisites

```bash
# Install testing tools
brew install act jq  # macOS
# or
sudo apt-get install jq  # Linux
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash

# Ensure Docker is running
docker info
```

### First-time Setup

```bash
# 1. Clone with submodules
git clone --recurse-submodules <your-repo>

# 2. Make scripts executable (already done)
chmod +x scripts/*.sh

# 3. Test the setup
./scripts/validate-deps.sh
```

---

## 📊 Test Matrix Coverage

### Local Testing (`act`)

| OS                  | Dependency Test | Arrow Test | Full Build |
| ------------------- | --------------- | ---------- | ---------- |
| Linux (simulated)   | ✅              | ✅         | ❌         |
| macOS (simulated)   | ✅              | ✅         | ❌         |
| Windows (simulated) | ✅              | ❌         | ❌         |

### GitHub Actions Testing

| OS             | Dependency Test | Arrow Test | Full Build |
| -------------- | --------------- | ---------- | ---------- |
| Ubuntu 22.04   | ✅              | ✅         | ❌         |
| macOS Latest   | ✅              | ✅         | ❌         |
| Windows Latest | ✅              | ❌         | ❌         |

**Note:** Full builds are handled by the main distribution pipeline, these tests focus on dependency resolution.

---

## 🐛 Troubleshooting

### act Issues

```bash
# If act fails to start
docker system prune -a  # Clean Docker
act --list  # Verify workflows are detected

# If containers are slow
# Edit .actrc to use smaller images (already configured)
```

### Local vcpkg Issues

```bash
# If vcpkg installations fail locally
rm -rf vcpkg  # Start fresh
./scripts/test-local.sh  # Will re-clone vcpkg
```

### GitHub Actions Timing Out

```bash
# The test workflow is optimized for speed:
# - Minimal dependency installation
# - Aggressive disk space cleanup
# - Parallel job execution
# - Windows Arrow testing disabled (known to be slow)
```

---

## 💡 Pro Tips

### Fast Iteration Workflow

1. **Edit code** → `./scripts/validate-deps.sh` (5 sec)
2. **Edit config** → `./scripts/test-local.sh` option 1 (2 min)
3. **Major changes** → `git push` (5-10 min)

### Debugging Failed Tests

```bash
# Local logs are saved automatically
ls test-*.log  # Check act output

# GitHub Actions logs available in web interface
# Go to Actions → Failed workflow → View logs
```

### Parallel Development

```bash
# Work on features while tests run
git checkout -b feature-branch
# Make changes...
./scripts/validate-deps.sh  # Quick validation

# Meanwhile, main branch tests run in background
```

This testing setup gives you **5-second feedback** for syntax issues and **2-minute feedback** for dependency problems, dramatically speeding up development while ensuring cross-platform compatibility.
