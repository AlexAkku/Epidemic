include_guard(GLOBAL)

set(EPIDEMIC_PRODUCTION_CXX_EXTENSIONS
    .h .hpp .inl .ipp .c .cc .cpp .cxx .ixx .cppm)

set(EPIDEMIC_PRODUCTION_ANCILLARY_EXTENSIONS
    .cmake .md .txt)

# Executable browser backends are production code and receive the same layer scan.
set(EPIDEMIC_PRODUCTION_SCRIPT_EXTENSIONS .js)
