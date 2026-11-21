#!/usr/bin/env python3
"""Cross-platform CLI, golden bitstream, and malformed-input regression suite for packJPG."""

import base64
import hashlib
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile
import zlib


def make_env():
    env = os.environ.copy()
    env.setdefault("ASAN_OPTIONS", "detect_leaks=1:halt_on_error=1:abort_on_error=0:exitcode=77")
    env.setdefault("UBSAN_OPTIONS", "halt_on_error=1:print_stacktrace=1:exitcode=78")
    return env


ENV = make_env()


def run_cli(binary, args, stdin_bytes=None):
    proc = subprocess.run(
        [binary] + args,
        input=stdin_bytes,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=ENV,
        timeout=30,
    )
    err_text = proc.stderr.decode("utf-8", errors="replace")
    if (
        proc.returncode in (77, 78)
        or proc.returncode < 0
        or "ERROR: AddressSanitizer" in err_text
        or "ERROR: LeakSanitizer" in err_text
        or "runtime error:" in err_text
    ):
        out_text = proc.stdout.decode("utf-8", errors="replace")
        raise AssertionError(
            f"Sanitizer/crash failure (rc={proc.returncode}) for {args}:\n"
            f"STDOUT:\n{out_text}\nSTDERR:\n{err_text}"
        )
    return proc.returncode, proc.stdout, proc.stderr


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <path-to-packJPG-binary>", file=sys.stderr)
        return 2

    binary = os.path.abspath(sys.argv[1])
    if not os.path.exists(binary):
        raise FileNotFoundError(f"Binary not found: {binary}")

    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    corpus_path = os.path.join(repo_root, "tests", "golden_corpus.json")
    with open(corpus_path, "r", encoding="utf-8") as f:
        corpus = json.load(f)

    with tempfile.TemporaryDirectory(prefix="packjpg_suite_") as tmp:
        # 1. Golden v2.5k bitstream compatibility & byte-identical round-trip across all 10 JPEG modes
        print("[1/5] Golden bitstream SHA-256 & round-trip matrix...")
        first_jpg_bytes = None
        for name, entry in sorted(corpus["cases"].items()):
            jpg_bytes = zlib.decompress(base64.b64decode(entry["jpg_zb64"]))
            if first_jpg_bytes is None:
                first_jpg_bytes = jpg_bytes

            jpg_file = os.path.join(tmp, f"{name}.jpg")
            pjg_file = os.path.join(tmp, f"{name}.pjg")
            rt_pjg = os.path.join(tmp, f"{name}_rt.pjg")
            rt_jpg = os.path.join(tmp, f"{name}_rt.jpg")

            with open(jpg_file, "wb") as f:
                f.write(jpg_bytes)

            rc, out, err = run_cli(binary, ["-np", "-ver", "-o", jpg_file])
            if rc != 0 or not os.path.exists(pjg_file):
                raise AssertionError(f"Compression failed for {name} (rc={rc}): {out!r} {err!r}")

            with open(pjg_file, "rb") as f:
                pjg_bytes = f.read()
            got_sha = hashlib.sha256(pjg_bytes).hexdigest()
            if got_sha != entry["pjg_sha256"]:
                raise AssertionError(
                    f"Golden PJG SHA-256 mismatch for {name}:\n"
                    f"  expected: {entry['pjg_sha256']}\n"
                    f"  actual:   {got_sha}"
                )

            shutil.copy2(pjg_file, rt_pjg)
            rc, out, err = run_cli(binary, ["-np", "-o", rt_pjg])
            if rc != 0 or not os.path.exists(rt_jpg):
                raise AssertionError(f"Decompression failed for {name} (rc={rc}): {out!r} {err!r}")

            with open(rt_jpg, "rb") as f:
                restored = f.read()
            if restored != jpg_bytes:
                raise AssertionError(f"Round-trip JPEG mismatch for {name}")
            print(f"  [PASS] {name} (jpg={len(jpg_bytes)}B -> pjg={len(pjg_bytes)}B)")

        # 2. Metadata stripping (-d) check
        print("[2/5] Metadata discard (-d) golden check...")
        meta_jpg = zlib.decompress(base64.b64decode(corpus["cases"]["metadata_and_trailing_garbage"]["jpg_zb64"]))
        d_in = os.path.join(tmp, "discard_meta.jpg")
        d_pjg = os.path.join(tmp, "discard_meta.pjg")
        d_rt_pjg = os.path.join(tmp, "discard_meta_rt.pjg")
        d_rt_jpg = os.path.join(tmp, "discard_meta_rt.jpg")
        with open(d_in, "wb") as f:
            f.write(meta_jpg)
        rc, _, _ = run_cli(binary, ["-np", "-d", "-o", d_in])
        assert rc == 0 and os.path.exists(d_pjg)
        with open(d_pjg, "rb") as f:
            d_sha = hashlib.sha256(f.read()).hexdigest()
        if d_sha != corpus["discard_meta_pjg_sha256"]:
            raise AssertionError(f"-d golden hash mismatch: {d_sha} != {corpus['discard_meta_pjg_sha256']}")
        shutil.copy2(d_pjg, d_rt_pjg)
        rc, _, _ = run_cli(binary, ["-np", "-o", d_rt_pjg])
        assert rc == 0 and os.path.exists(d_rt_jpg)
        with open(d_rt_jpg, "rb") as f:
            rebuilt = f.read()
        assert rebuilt.startswith(b"\xff\xd8")
        assert b"Exif" not in rebuilt and b"JFIF" not in rebuilt
        assert b"TRAILING_GARBAGE_BYTES_0123456789" in rebuilt
        print("  [PASS] -d metadata discard & header rebuild")

        # 3. Pipe mode (-p: stdin -> stdout) compress & decompress round-trip
        print("[3/5] Pipe mode (-p stdin/stdout) round-trip...")
        rc, pjg_stdout, _ = run_cli(binary, ["-np", "-p", "-"], stdin_bytes=first_jpg_bytes)
        assert rc == 0 and pjg_stdout.startswith(b"JS"), f"Pipe compress failed (rc={rc}, len={len(pjg_stdout)})"
        rc, jpg_stdout, _ = run_cli(binary, ["-np", "-p", "-"], stdin_bytes=pjg_stdout)
        assert rc == 0 and jpg_stdout == first_jpg_bytes, "Pipe round-trip mismatch"
        print("  [PASS] -p binary stdin/stdout round-trip")

        # 4. Non-overwrite suffixing, long filenames, and format-string filenames
        print("[4/5] CLI file naming, long paths, and format-string safety...")
        no_ow_dir = os.path.join(tmp, "no_overwrite")
        os.makedirs(no_ow_dir)
        base_jpg = os.path.join(no_ow_dir, "sample.jpg")
        base_pjg = os.path.join(no_ow_dir, "sample.pjg")
        alt_pjg = os.path.join(no_ow_dir, "sample_.pjg")
        with open(base_jpg, "wb") as f:
            f.write(first_jpg_bytes)
        with open(base_pjg, "wb") as f:
            f.write(b"SENTINEL")
        rc, _, _ = run_cli(binary, ["-np", base_jpg])
        assert rc == 0 and os.path.exists(alt_pjg), "Expected sample_.pjg when -o is not set"
        with open(base_pjg, "rb") as f:
            assert f.read() == b"SENTINEL", "Existing sample.pjg was overwritten without -o!"

        long_missing = os.path.join(tmp, "missing_" + ("x" * 180) + ".jpg")
        _, out, err = run_cli(binary, ["-np", "-v1", long_missing])
        combined = (out + err).decode("utf-8", errors="replace")
        assert "missing_" + ("x" * 180) in combined

        fmt_missing = os.path.join(tmp, "fmt_%s_%n_%x_%p.jpg")
        _, out, err = run_cli(binary, ["-np", "-v1", fmt_missing])
        assert "%s_%n_%x_%p" in (out + err).decode("utf-8", errors="replace")

        if os.name != "nt" and hasattr(os, "geteuid") and os.geteuid() != 0:
            ro_dir = os.path.join(tmp, "readonly")
            os.makedirs(ro_dir)
            ro_pjg = os.path.join(ro_dir, "locked.pjg")
            shutil.copy2(alt_pjg, ro_pjg)
            os.chmod(ro_dir, stat.S_IRUSR | stat.S_IXUSR)
            try:
                run_cli(binary, ["-np", "-v1", ro_pjg])
            finally:
                os.chmod(ro_dir, stat.S_IRWXU)
        print("  [PASS] non-overwrite suffixing, long paths, and format-string filenames")

        # 5. Malformed / truncated JPEG & PJG negative regression suite
        print("[5/5] Malformed JPEG & PJG negative regressions...")
        malformed_inputs = {
            "empty.jpg": b"",
            "soi_only.jpg": b"\xff\xd8",
            "trunc_app0.jpg": b"\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01",
            "sof_qt64.jpg": b"\xff\xd8\xff\xc0\x00\x0b\x08\x00\x08\x00\x08\x01\x01\x11\x40\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "sof_short.jpg": b"\xff\xd8\xff\xc0\x00\x05\x08\x00\x08\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "sof_zero_dim.jpg": b"\xff\xd8\xff\xc0\x00\x0b\x08\x00\x00\x00\x08\x01\x01\x11\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "dri_short.jpg": b"\xff\xd8\xff\xdd\x00\x03\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "dqt_short.jpg": b"\xff\xd8\xff\xdb\x00\x10" + b"\x00" * 14 + b"\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "dht_short.jpg": b"\xff\xd8\xff\xc4\x00\x10" + b"\x00" * 14 + b"\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "sos_short.jpg": b"\xff\xd8\xff\xc0\x00\x0b\x08\x00\x08\x00\x08\x01\x01\x11\x00\xff\xda\x00\x04\x01\x00\x00\xff\xd9",
            "rst_in_hdr.jpg": b"\xff\xd8\xff\xd0\x00\x04\x00\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "pjg_magic_only.pjg": b"JS",
            "pjg_trunc_hdr.pjg": b"JS\x00\x01\x02",
            "pjg_bad_ver.pjg": b"JS\x18",
            "pjg_bad_hcode.pjg": b"JS\x05",
            "pjg_bad_custom_settings.pjg": b"JS\x00\xff\xff\xff\xff\x00\x00\x00\x00\x19\x00\x00\x00\x00",
            "pjg_corrupt_ari.pjg": b"JS\x19" + b"\xff" * 64,
        }
        neg_dir = os.path.join(tmp, "negative")
        os.makedirs(neg_dir)
        for fname, payload in malformed_inputs.items():
            fpath = os.path.join(neg_dir, fname)
            with open(fpath, "wb") as f:
                f.write(payload)
            run_cli(binary, ["-np", "-v1", fpath])
        print(f"  [PASS] {len(malformed_inputs)} malformed inputs handled cleanly")

    print("\nAll CLI & golden bitstream tests passed!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
