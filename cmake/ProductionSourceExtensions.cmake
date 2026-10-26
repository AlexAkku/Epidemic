include_guard(GLOBAL)

set(EPIDEMIC_PRODUCTION_CXX_EXTENSIONS
    .h .hpp .inl .ipp .c .cc .cpp .cxx .ixx .cppm)

set(EPIDEMIC_PRODUCTION_ANCILLARY_EXTENSIONS
    .cmake .md .txt)

# Executable browser backends are production code and receive the same layer scan.
set(EPIDEMIC_PRODUCTION_SCRIPT_EXTENSIONS .js)

# These are validated by check_managed_architecture.py, never by C++ include regexes.
set(EPIDEMIC_PRODUCTION_MANAGED_EXTENSIONS .cs .csproj)
