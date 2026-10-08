"""Owning generator for a private existing SDK runtime, application and file inventory."""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import shutil
import sys
import tkinter


def sha(path):
    return hashlib.file_digest(path.open("rb"), "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--executable", type=Path, required=True)
    args = parser.parse_args()
    output, executable = args.output.resolve(), args.executable.resolve()
    if executable.parent != output or executable.name != "EpidemicAuditor.exe" or not executable.is_file():
        raise RuntimeError("invalid native bundle output identity")
    helpers = [output / "EpidemicAuditorObservation.exe", output / "EpidemicAuditorObservation.pdb",
               output / "EpidemicAuditorEtwBroker.exe", output / "EpidemicAuditorEtwBroker.pdb"]
    if not all(path.is_file() for path in helpers):
        raise RuntimeError("mandatory observation helper/PDB missing from bundle")
    source = Path(__file__).resolve().parents[1]
    base = Path(sys.base_prefix)
    runtime = output / "runtime"
    runtime.mkdir(parents=True, exist_ok=True)
    # Only generator-owned bundle files from the prior inventory may be removed.
    prior = output / "bundle_manifest.json"
    if prior.exists():
        for row in json.loads(prior.read_text())["files"]:
            target = (output / row["path"]).resolve()
            if not target.is_relative_to(output) or target == executable or target in helpers:
                if target == executable or target in helpers:
                    continue
                raise RuntimeError("prior bundle inventory escaped output")
            if target.is_file():
                target.unlink()
    copied = {}

    def copy_file(original, relative, role):
        target = output / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(original, target)
        copied[relative.as_posix()] = {"source": str(original), "source_sha256": sha(original), "role": role}

    def copy_tree(original, relative, role, exclude=()):
        for path in sorted(original.rglob("*")):
            tail = path.relative_to(original)
            if path.is_file() and not any(part in {"__pycache__", *exclude} for part in tail.parts):
                copy_file(path, relative / tail, role)

    copy_tree(base / "Lib", Path("runtime/Lib"), "python_stdlib", ("site-packages", "test", "tests"))
    copy_tree(base / "DLLs", Path("runtime/DLLs"), "python_native_modules")
    copy_tree(base / "tcl", Path("runtime/tcl"), "tcl_tk")
    binaries = [base / "python.exe", *base.glob("python*.dll"), *base.glob("vcruntime*.dll")]
    for path in binaries:
        copy_file(path, Path("runtime") / path.name, "python_runtime")
        if path.suffix.lower() == ".dll":
            copy_file(path, Path(path.name), "native_host_dependency")
    distribution = importlib.metadata.distribution("psutil")
    if not 6 <= int(distribution.version.split(".")[0]) < 8:
        raise RuntimeError("existing psutil does not satisfy declared application dependency")
    for name in distribution.files:
        original = Path(distribution.locate_file(name))
        if name.parts[0] in {"psutil", "psutil-" + distribution.version + ".dist-info"} and original.is_file() and "__pycache__" not in name.parts:
            copy_file(original, Path("runtime/Lib/site-packages") / name, "declared_dependency.psutil")
    copy_tree(source / "ep_auditor", Path("runtime/app/ep_auditor"), "auditor_source")
    copy_tree(source / 'rules', Path('runtime/app/rules'), 'auditor_rules')
    copy_file(source / "VERSION", Path("runtime/app/VERSION"), "auditor_version")
    environment = {"tcl": "tcl" + str(tkinter.TclVersion), "tk": "tk" + str(tkinter.TkVersion)}
    (runtime / "environment.json").write_text(json.dumps(environment, sort_keys=True) + "\n")
    config = output / "auditor.config.json"
    if not config.exists():
        data = json.loads((source / "auditor.config.json").read_text())
        project = source.parent
        data.update(project_root=str(project), build_dir=str(project / "build/audit"))
        data["storage"] = {"runs_dir": str(project / ".ai/auditor/runs"),
                           "database": str(project / ".ai/auditor/auditor.sqlite3")}
        config.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    files = [{"path": "EpidemicAuditor.exe", "sha256": sha(executable), "role": "native_host"},
             {"path": "runtime/environment.json", "sha256": sha(runtime / "environment.json"), "role": "private_runtime_environment"}]
    files += [{"path": path.name, "sha256": sha(path), "role": "privileged_etw_broker_installer" if "EtwBroker" in path.name
               else "external_observation_helper"} for path in helpers]
    files += [{"path": name, "sha256": sha(output / name), **value} for name, value in sorted(copied.items())]
    native_inputs = {path.relative_to(source).as_posix(): sha(path)
                     for path in (source / 'native').rglob('*') if path.is_file()
                     and (path.suffix in {'.cpp', '.h', '.asm', '.py'} or path.name == 'CMakeLists.txt')}
    prior.write_text(json.dumps({"schemaVersion": 1, "files": files, "sdk_version": sys.version,
                                "native_source_sha256": native_inputs,
                                "generator_sha256": sha(Path(__file__))}, sort_keys=True, indent=2) + "\n")
    print(json.dumps({"status": "PASS", "native_application": str(executable), "files": len(files)}))


if __name__ == "__main__":
    main()
