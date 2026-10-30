#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <Windows.h>
#include <shellapi.h>
#include <filesystem>
#include <string>

namespace {
int fail(const char *message) {
    MessageBoxA(nullptr, message, "Epidemic Auditor startup failed", MB_OK | MB_ICONERROR);
    return 1;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    wchar_t image[32768]{};

    const DWORD length = GetModuleFileNameW(nullptr, image, 32768);
    if (length == 0 || length >= 32768)
        return fail("Cannot determine application image path");
    const auto directory = std::filesystem::path(image).parent_path();
    const auto runtime = directory / L"runtime";
    if (!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS) ||
        !AddDllDirectory(directory.c_str()) || !AddDllDirectory((runtime / L"DLLs").c_str())) {
        return fail("Cannot admit private application DLL directories");
    }

    int count = 0;
    wchar_t **arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!arguments)
        return fail("Cannot decode application arguments");
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    config.parse_argv = 0;
    config.write_bytecode = 0;
    config.module_search_paths_set = 1;
    PyStatus status = PyConfig_SetString(&config, &config.home, runtime.c_str());
    if (!PyStatus_Exception(status))
        status = PyConfig_SetString(&config, &config.executable, image);
    for (const auto &path :
         {runtime / L"Lib", runtime / L"DLLs", runtime / L"Lib/site-packages", runtime / L"app"}) {
        if (!PyStatus_Exception(status))
            status = PyWideStringList_Append(&config.module_search_paths, path.c_str());
    }

    if (!PyStatus_Exception(status))
        status = PyConfig_SetArgv(&config, count, arguments);
    LocalFree(static_cast<HLOCAL>(arguments));
    if (!PyStatus_Exception(status))
        status = Py_InitializeFromConfig(&config);
    std::string error = PyStatus_Exception(status)
                            ? (status.err_msg ? status.err_msg : "Python initialization failed")
                            : "";
    PyConfig_Clear(&config);
    if (!error.empty())
        return fail(error.c_str());
    const int result = PyRun_SimpleString("from ep_auditor.entrypoint import main\n"
                                          "raise SystemExit(main())\n");
    const int finalized = Py_FinalizeEx();
    return result == 0 && finalized == 0 ? 0 : 1;
}
