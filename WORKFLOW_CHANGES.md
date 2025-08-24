# Workflow Changes

## Current Workflow: `.github/workflows/build.yml`
A barebones workflow that:
- Builds on Ubuntu, macOS, and Windows
- Uses DuckDB v1.3.1
- Sets up vcpkg for dependencies
- Runs `make release-snowflake`

## Backup Workflow: `.github/workflows/ExtensionTemplate.yml.backup`
The original comprehensive workflow with:
- Template testing functionality
- Advanced build configurations
- Test running
- Multiple DuckDB version support

## To Revert to Original Workflow:
```bash
mv .github/workflows/ExtensionTemplate.yml.backup .github/workflows/ExtensionTemplate.yml
rm .github/workflows/build.yml
```

## Functionality Retained:
- ✅ All submodules (duckdb, extension-ci-tools, arrow-adbc)
- ✅ vcpkg dependency management
- ✅ DuckDB v1.3.1 (ossivalis)
- ✅ Snowflake extension build
- ✅ Cross-platform support (Linux, macOS, Windows)
- ✅ Fast builds with ninja (Linux/macOS) and MSBuild (Windows) 