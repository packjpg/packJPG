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
import threading
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
        or "WARNING: ThreadSanitizer" in err_text
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
        print("[1/8] Golden bitstream SHA-256 & round-trip matrix...")
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
        print("[2/8] Metadata discard (-d) golden check...")
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

        # 3. Pipe mode (-p: stdin -> stdout) compress, -ver, decompress, and clean error stdout
        print("[3/8] Pipe mode (-p stdin/stdout) round-trip & error cleanliness...")
        rc, pjg_stdout, _ = run_cli(binary, ["-np", "-p", "-ver", "-"], stdin_bytes=first_jpg_bytes)
        assert rc == 0 and pjg_stdout.startswith(b"JS"), f"Pipe compress failed (rc={rc}, len={len(pjg_stdout)})"
        rc, jpg_stdout, _ = run_cli(binary, ["-np", "-p", "-ver", "-"], stdin_bytes=pjg_stdout)
        assert rc == 0 and jpg_stdout == first_jpg_bytes, "Pipe round-trip mismatch"
        rc, bad_stdout, _ = run_cli(binary, ["-np", "-p", "-"], stdin_bytes=b"JS\x19\xff\xff\xff")
        assert bad_stdout == b"", f"Expected empty stdout on failed pipe decode, got {len(bad_stdout)} bytes"
        print("  [PASS] -p/-ver binary stdin/stdout round-trip and clean failure stdout")

        # 4. Non-overwrite suffixing, directory path, long filenames, and format-string filenames
        print("[4/8] CLI file naming, directory rejection, long paths, and format-string safety...")
        no_ow_dir = os.path.join(tmp, "no_overwrite")
        os.makedirs(no_ow_dir)
        # Passing a directory path must not crash with uncaught std::length_error
        run_cli(binary, ["-np", "-v1", no_ow_dir])

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
                rc_ro, out_ro, err_ro = run_cli(binary, ["-np", "-v1", ro_pjg])
                ro_msg = (out_ro + err_ro).decode("utf-8", errors="replace")
                assert rc_ro != 0, "Expected non-zero exit code when output directory is read-only"
                assert "locked.jpg" in ro_msg, f"Expected target path in error output, got: {ro_msg}"
                assert ".packjpg_tmp_" not in ro_msg, f"Internal staging temp name leaked in error: {ro_msg}"
                assert os.listdir(ro_dir) == ["locked.pjg"], f"Unexpected files in read-only dir: {os.listdir(ro_dir)}"
            finally:
                os.chmod(ro_dir, stat.S_IRWXU)
        print("  [PASS] non-overwrite suffixing, directory path, long paths, and format-string filenames")

        # 5. Malformed / truncated JPEG & PJG negative regression suite
        print("[5/8] Malformed JPEG & PJG negative regressions...")
        malformed_inputs = {
            "empty.jpg": b"",
            "soi_only.jpg": b"\xff\xd8",
            "trunc_app0.jpg": b"\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01",
            "sof_qt64.jpg": b"\xff\xd8\xff\xc0\x00\x0b\x08\x00\x08\x00\x08\x01\x01\x11\x40\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "sof_short.jpg": b"\xff\xd8\xff\xc0\x00\x05\x08\x00\x08\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "sof_zero_dim.jpg": b"\xff\xd8\xff\xc0\x00\x0b\x08\x00\x00\x00\x08\x01\x01\x11\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "sof_bad_sf15.jpg": b"\xff\xd8\xff\xdb\x00\x43\x00" + b"\x10" * 64 + b"\xff\xc0\x00\x0b\x08\x00\x08\x00\x08\x01\x01\xf1\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "sof_dup_jid.jpg": b"\xff\xd8\xff\xdb\x00\x43\x00" + b"\x10" * 64 + b"\xff\xc0\x00\x0e\x08\x00\x08\x00\x08\x02\x01\x11\x00\x01\x11\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "sof_duplicate_marker.jpg": b"\xff\xd8\xff\xdb\x00\x43\x00" + b"\x10" * 64 + b"\xff\xc0\x00\x0b\x08\x00\x08\x00\x08\x01\x01\x11\x00\xff\xc0\x00\x0b\x08\x00\x08\x00\x08\x01\x01\x11\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "sof_65535_oom.jpg": b"\xff\xd8\xff\xdb\x00\x43\x00" + b"\x10" * 64 + b"\xff\xc0\x00\x14\x08\xff\xff\xff\xff\x04\x01\x44\x00\x02\x44\x00\x03\x44\x00\x04\x44\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "dht_kraft_overflow.jpg": b"\xff\xd8\xff\xdb\x00\x43\x00" + b"\x10" * 64 + b"\xff\xc0\x00\x0b\x08\x00\x08\x00\x08\x01\x01\x11\x00\xff\xc4\x00\x23\x00\x10" + b"\x00" * 15 + bytes(range(16)) + b"\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "sos_dup_cmp.jpg": b"\xff\xd8\xff\xdb\x00\x43\x00" + b"\x10" * 64 + b"\xff\xc0\x00\x0e\x08\x00\x08\x00\x08\x02\x01\x11\x00\x02\x11\x00\xff\xda\x00\x0a\x02\x01\x00\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "dri_short.jpg": b"\xff\xd8\xff\xdd\x00\x03\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "dqt_short.jpg": b"\xff\xd8\xff\xdb\x00\x10" + b"\x00" * 14 + b"\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "dht_short.jpg": b"\xff\xd8\xff\xc4\x00\x10" + b"\x00" * 14 + b"\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "sos_short.jpg": b"\xff\xd8\xff\xc0\x00\x0b\x08\x00\x08\x00\x08\x01\x01\x11\x00\xff\xda\x00\x04\x01\x00\x00\xff\xd9",
            "rst_in_hdr.jpg": b"\xff\xd8\xff\xd0\x00\x04\x00\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "dqt_zero_entry.jpg": b"\xff\xd8\xff\xdb\x00\x43\x00\x10\x10\x10\x10\x10\x00" + b"\x10" * 58 + b"\xff\xc0\x00\x0b\x08\x00\x08\x00\x08\x01\x01\x11\x00\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00\x00\xff\xd9",
            "pjg_magic_only.pjg": b"JS",
            "pjg_trunc_hdr.pjg": b"JS\x00\x01\x02",
            "pjg_bad_ver.pjg": b"JS\x18",
            "pjg_bad_hcode.pjg": b"JS\x05",
            "pjg_bad_custom_settings.pjg": b"JS\x00\xff\xff\xff\xff\x00\x00\x00\x00\x19\x00\x00\x00\x00",
            "pjg_corrupt_ari.pjg": b"JS\x19" + b"\xff" * 64,
            "pjg_zero_bomb.pjg": b"JS\x19" + b"\x00" * 32,
            "pjg_eof_spin.pjg": b"JS\x19\x80",
        }
        neg_dir = os.path.join(tmp, "negative")
        os.makedirs(neg_dir)
        for fname, payload in malformed_inputs.items():
            fpath = os.path.join(neg_dir, fname)
            with open(fpath, "wb") as f:
                f.write(payload)
            rc_neg, _, _ = run_cli(binary, ["-np", "-v1", fpath])
            assert rc_neg != 0, f"Expected non-zero exit code for malformed {fname}"

        # Differential fuzzing regressions: non-injective sequential AC 0x30 symbol & mixed padbits
        raw_440 = zlib.decompress(base64.b64decode(corpus["cases"]["ycbcr_440_16x32"]["jpg_zb64"]))
        mut_0x30 = raw_440.replace(b"\x12\x21\x31\x41", b"\x12\x21\x30\x41", 1)
        assert mut_0x30 != raw_440
        mut_0x30_path = os.path.join(neg_dir, "seq_ac_0x30.jpg")
        with open(mut_0x30_path, "wb") as f:
            f.write(mut_0x30)
        rc_0x30, _, _ = run_cli(binary, ["-np", mut_0x30_path])
        assert rc_0x30 != 0 and not os.path.exists(os.path.join(neg_dir, "seq_ac_0x30.pjg"))

        raw_gray = bytearray(zlib.decompress(base64.b64decode(corpus["cases"]["gray_15x17"]["jpg_zb64"])))
        assert raw_gray[-2:] == b"\xff\xd9"
        if raw_gray[-3] == 0x00 and raw_gray[-4] == 0xFF:
            raw_gray[-4] ^= 0x01
        else:
            raw_gray[-3] ^= 0x01
        mixed_pad_path = os.path.join(neg_dir, "mixed_padbits.jpg")
        with open(mixed_pad_path, "wb") as f:
            f.write(raw_gray)
        rc_pad_strict, _, _ = run_cli(binary, ["-np", mixed_pad_path])
        assert rc_pad_strict != 0, "Expected strict mode to reject mixed padbits"
        rc_pad_tol, _, _ = run_cli(binary, ["-np", "-p", mixed_pad_path])
        assert rc_pad_tol == 0, "Expected -p mode to accept mixed padbits with warning"

        # Exact byte-aligned Huffman scan (4-bit DC 0 + 4-bit AC EOB = 8 bits, 0 padbits -> padbit stays -1)
        byte_aligned_jpg = (
            b"\xff\xd8"
            b"\xff\xdb\x00\x43\x00" + (b"\x10" * 64) +
            b"\xff\xc0\x00\x0b\x08\x00\x08\x00\x08\x01\x01\x11\x00"
            b"\xff\xc4\x00\x14\x00\x00\x00\x00\x01" + (b"\x00" * 12) + b"\x00"
            b"\xff\xc4\x00\x14\x10\x00\x00\x00\x01" + (b"\x00" * 12) + b"\x00"
            b"\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00"
            b"\x00"
            b"\xff\xd9"
        )
        aligned_jpg_path = os.path.join(neg_dir, "byte_aligned_scan.jpg")
        aligned_pjg_path = os.path.join(neg_dir, "byte_aligned_scan.pjg")
        with open(aligned_jpg_path, "wb") as f:
            f.write(byte_aligned_jpg)
        rc_aligned, _, _ = run_cli(binary, ["-np", "-ver", "-o", aligned_jpg_path])
        assert rc_aligned == 0 and os.path.isfile(aligned_pjg_path), "Byte-aligned scan compress failed"
        os.remove(aligned_jpg_path)
        rc_aligned_dec, _, _ = run_cli(binary, ["-np", "-o", aligned_pjg_path])
        assert rc_aligned_dec == 0, "Byte-aligned scan decompress failed"
        with open(aligned_jpg_path, "rb") as f:
            assert f.read() == byte_aligned_jpg, "Byte-aligned scan round-trip mismatch"
        print(f"  [PASS] {len(malformed_inputs) + 3} malformed, padbit, & fuzz-discovered edge cases handled cleanly")

        # 6. Output directory (-out), wildcards (*.jpg / ?), recursive (-r), and parallel batch (-j4)
        print("[6/8] Output directory (-out), wildcards, recursive (-r), and parallel (-j4) batch...")
        batch_in = os.path.join(tmp, "batch_in")
        sub_a = os.path.join(batch_in, "album_a")
        sub_b = os.path.join(batch_in, "album_b", "nested")
        os.makedirs(sub_a)
        os.makedirs(sub_b)

        expected_jpgs = {}
        case_items = sorted(corpus["cases"].items())
        for idx, (cname, centry) in enumerate(case_items[:6]):
            raw_jpg = zlib.decompress(base64.b64decode(centry["jpg_zb64"]))
            target_dir = sub_a if (idx % 2 == 0) else sub_b
            rel_path = os.path.relpath(os.path.join(target_dir, f"{cname}.jpg"), batch_in)
            full_jpg = os.path.join(batch_in, rel_path)
            with open(full_jpg, "wb") as f:
                f.write(raw_jpg)
            expected_jpgs[rel_path] = (raw_jpg, centry["pjg_sha256"])

        # Unexpanded wildcard pattern passed directly in argv (simulating Windows cmd.exe)
        glob_out = os.path.join(tmp, "glob_out")
        glob_pattern = os.path.join(sub_a, "*.jpg")
        rc, _, _ = run_cli(binary, ["-np", "-out", glob_out, glob_pattern])
        assert rc == 0, f"Wildcard -out failed with rc={rc}"
        assert len([f for f in os.listdir(glob_out) if f.endswith(".pjg")]) == 3

        # Single-char '?' wildcard pattern + -out=<dir> syntax
        q_out = os.path.join(tmp, "q_out")
        first_sub_a_name = os.path.basename(next(k for k in expected_jpgs if k.startswith("album_a")))
        q_pattern = os.path.join(sub_a, "?" + first_sub_a_name[1:])
        rc, _, _ = run_cli(binary, ["-np", f"-out={q_out}", q_pattern])
        assert rc == 0 and len(os.listdir(q_out)) == 1

        # Recursive directory traversal (-r) + parallel workers (-j4) + -ver + -out hierarchy preservation
        rec_pjg_out = os.path.join(tmp, "rec_pjg")
        rec_jpg_out = os.path.join(tmp, "rec_jpg")
        rc, _, _ = run_cli(binary, ["-np", "-r", "-j4", "-ver", "-out", rec_pjg_out, batch_in])
        assert rc == 0, f"Parallel recursive compress failed (rc={rc})"
        for rel_jpg, (_, exp_sha) in expected_jpgs.items():
            rel_pjg = os.path.splitext(rel_jpg)[0] + ".pjg"
            pjg_path = os.path.join(rec_pjg_out, rel_pjg)
            assert os.path.isfile(pjg_path), f"Missing recursive output {pjg_path}"
            with open(pjg_path, "rb") as f:
                assert hashlib.sha256(f.read()).hexdigest() == exp_sha

        rc, _, _ = run_cli(binary, ["-np", "-r", "-j4", "-out", rec_jpg_out, rec_pjg_out])
        assert rc == 0, f"Parallel recursive decompress failed (rc={rc})"
        for rel_jpg, (orig_bytes, _) in expected_jpgs.items():
            restored_path = os.path.join(rec_jpg_out, rel_jpg)
            assert os.path.isfile(restored_path), f"Missing restored JPEG {restored_path}"
            with open(restored_path, "rb") as f:
                assert f.read() == orig_bytes

        # Collision-free non-overwrite reservation across threads when flattening duplicate basenames into one -out dir
        dup_a = os.path.join(tmp, "dup_a")
        dup_b = os.path.join(tmp, "dup_b")
        dup_out = os.path.join(tmp, "dup_out")
        os.makedirs(dup_a)
        os.makedirs(dup_b)
        with open(os.path.join(dup_a, "same.jpg"), "wb") as f:
            f.write(first_jpg_bytes)
        with open(os.path.join(dup_b, "same.jpg"), "wb") as f:
            f.write(first_jpg_bytes)
        rc, _, _ = run_cli(
            binary,
            ["-np", "-j2", "-out", dup_out, os.path.join(dup_a, "same.jpg"), os.path.join(dup_b, "same.jpg")],
        )
        assert rc == 0
        assert os.path.isfile(os.path.join(dup_out, "same.pjg"))
        assert os.path.isfile(os.path.join(dup_out, "same_.pjg"))

        # Error handling when -out points to (or beneath) an existing regular file or is missing its argument
        rc_bad1, _, _ = run_cli(binary, ["-np", "-out", os.path.join(dup_a, "same.jpg"), os.path.join(dup_a, "same.jpg")])
        rc_bad2, _, _ = run_cli(binary, ["-np", "-out"])
        rc_bad3, _, _ = run_cli(binary, ["-np", "-out", os.path.join(dup_a, "same.jpg", "sub"), os.path.join(dup_a, "same.jpg")])
        assert rc_bad1 != 0 and rc_bad2 != 0 and rc_bad3 != 0

        # Lazy -out creation: no-input help runs and invalid/missing inputs must NOT leave empty directories behind
        lazy_no_args = os.path.join(tmp, "lazy_no_args")
        lazy_bad_in = os.path.join(tmp, "lazy_bad_in")
        bad_txt = os.path.join(tmp, "not_an_image.txt")
        with open(bad_txt, "wb") as f:
            f.write(b"plain text")
        rc_l1, _, _ = run_cli(binary, ["-np", "-out", lazy_no_args])
        rc_l2, _, _ = run_cli(binary, ["-np", "-out", lazy_bad_in, os.path.join(tmp, "does_not_exist.jpg"), bad_txt])
        assert rc_l1 != 0 and rc_l2 != 0
        assert not os.path.exists(lazy_no_args), "Empty -out directory should not be created when no inputs are given"
        assert not os.path.exists(lazy_bad_in), "Empty -out directory should not be created when all inputs fail check_file"

        # 7. CLI safety regressions: misnamed .pjg -o self-clobber, read-only target, -j2 -o collision, -r -out nested idempotency, strict flags
        misnamed_pjg = os.path.join(tmp, "misnamed_jpeg.pjg")
        with open(misnamed_pjg, "wb") as f:
            f.write(first_jpg_bytes)
        rc_self, _, _ = run_cli(binary, ["-np", "-o", misnamed_pjg])
        assert rc_self != 0, "Expected packJPG -o on JPEG misnamed .pjg to refuse self-clobber"
        assert os.path.isfile(misnamed_pjg), "Self-clobber check must never delete input file!"
        with open(misnamed_pjg, "rb") as f:
            assert f.read() == first_jpg_bytes, "Misnamed input file bytes must remain 100% intact!"

        # Without -o, misnamed_jpeg.pjg safely writes misnamed_jpeg_.pjg
        rc_self_safe, _, _ = run_cli(binary, ["-np", misnamed_pjg])
        assert rc_self_safe == 0 and os.path.isfile(os.path.join(tmp, "misnamed_jpeg_.pjg"))

        # Pre-existing read-only target file must NOT be unlinked if fopen("wb") fails with EACCES
        if os.name != "nt" and hasattr(os, "geteuid") and os.geteuid() != 0:
            ro_dir = os.path.join(tmp, "ro_test")
            os.makedirs(ro_dir)
            ro_jpg = os.path.join(ro_dir, "keep.jpg")
            ro_pjg = os.path.join(ro_dir, "keep.pjg")
            with open(ro_jpg, "wb") as f:
                f.write(first_jpg_bytes)
            with open(ro_pjg, "wb") as f:
                f.write(b"PRECIOUS_DATA")
            os.chmod(ro_pjg, 0o444)
            try:
                rc_ro, _, _ = run_cli(binary, ["-np", "-o", ro_jpg])
                assert rc_ro != 0
                assert os.path.isfile(ro_pjg), "Failed fopen on read-only target must NEVER unlink pre-existing file!"
                with open(ro_pjg, "rb") as f:
                    assert f.read() == b"PRECIOUS_DATA"
            finally:
                os.chmod(ro_pjg, 0o644)

        # Colliding -o outputs in same batch (same.jpg + same.jpeg -> same.pjg) must keep first output valid & fail second cleanly
        coll_dir = os.path.join(tmp, "coll_batch")
        os.makedirs(coll_dir)
        c_jpg1 = os.path.join(coll_dir, "photo.jpg")
        c_jpg2 = os.path.join(coll_dir, "photo.jpeg")
        with open(c_jpg1, "wb") as f:
            f.write(first_jpg_bytes)
        with open(c_jpg2, "wb") as f:
            f.write(first_jpg_bytes)
        rc_coll, _, _ = run_cli(binary, ["-np", "-j2", "-o", c_jpg1, c_jpg2])
        assert rc_coll != 0, "Expected batch output path collision under -o to report an error"
        assert os.path.isfile(os.path.join(coll_dir, "photo.pjg"))

        # Recursive -r with -out nested inside input directory must NOT re-ingest its own -out directory on a second run
        idem_in = os.path.join(tmp, "idem_tree")
        idem_out = os.path.join(idem_in, "out_sub")
        os.makedirs(idem_in)
        with open(os.path.join(idem_in, "one.jpg"), "wb") as f:
            f.write(first_jpg_bytes)
        rc_id1, _, _ = run_cli(binary, ["-np", "-r", "-o", "-out", idem_out, idem_in])
        rc_id2, _, _ = run_cli(binary, ["-np", "-r", "-o", "-out", idem_out, idem_in])
        assert rc_id1 == 0 and rc_id2 == 0
        assert os.path.isfile(os.path.join(idem_out, "one.pjg"))
        assert not os.path.exists(os.path.join(idem_out, "out_sub")), "-r must skip -out subdirectory"

        # Strict flag parsing: filename starting with -v1_ must be processed as a file, and huge -j99999999999 clamps to 64
        dash_dir = os.path.join(tmp, "dash_flags")
        os.makedirs(dash_dir)
        dash_file = "-v1_sample.jpg"
        with open(os.path.join(dash_dir, dash_file), "wb") as f:
            f.write(first_jpg_bytes)
        dash_proc = subprocess.run(
            [os.path.abspath(binary), "-np", "-j99999999999", dash_file],
            cwd=dash_dir,
            env=ENV,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=30,
        )
        assert dash_proc.returncode == 0 and os.path.isfile(os.path.join(dash_dir, "-v1_sample.pjg"))

        # 8. Second-pass CLI regressions: help flags (#10), UTF-8 filenames (#10), atomic output preservation on warning/error/-ver (#9), hard-link & dangling-symlink guards (#11)
        help_flags = ["-h", "--help", "-?"] + (["/?"] if os.name == "nt" else [])
        for hflag in help_flags:
            rc_h, out_h, _ = run_cli(binary, [hflag])
            assert rc_h == 0 and b"Usage:" in out_h, f"Expected exit 0 and Usage text for {hflag}, got rc={rc_h}"

        # UTF-8 Unicode path round-trip (direct + -r -out)
        uni_dir = os.path.join(tmp, "unicode_\u03b1\u03b2\u03b3_\u732b")
        uni_out = os.path.join(tmp, "unicode_out_\u732b")
        os.makedirs(uni_dir)
        uni_jpg = os.path.join(uni_dir, "photo_\u03c0_\u732b.jpg")
        with open(uni_jpg, "wb") as f:
            f.write(first_jpg_bytes)
        rc_u1, _, _ = run_cli(binary, ["-np", "-ver", "-r", "-out", uni_out, uni_dir])
        assert rc_u1 == 0
        uni_pjg = os.path.join(uni_out, "photo_\u03c0_\u732b.pjg")
        assert os.path.isfile(uni_pjg), "Missing UTF-8 output .pjg"
        uni_rt_dir = os.path.join(tmp, "unicode_rt_\u732b")
        rc_u2, _, _ = run_cli(binary, ["-np", "-o", "-out", uni_rt_dir, uni_pjg])
        assert rc_u2 == 0
        with open(os.path.join(uni_rt_dir, "photo_\u03c0_\u732b.jpg"), "rb") as f:
            assert f.read() == first_jpg_bytes

        # Finding #9: pre-existing writable output file MUST remain intact when -o conversion skips on warning, fails decode, or fails -ver!
        atomic_dir = os.path.join(tmp, "atomic_cli")
        os.makedirs(atomic_dir)

        warn_in = os.path.join(atomic_dir, "keep_on_warn.jpg")
        warn_target = os.path.join(atomic_dir, "keep_on_warn.pjg")
        shutil.copy2(mixed_pad_path, warn_in)
        with open(warn_target, "wb") as f:
            f.write(b"PRECIOUS_PJG_BEFORE_WARNING")
        rc_aw, _, _ = run_cli(binary, ["-np", "-o", warn_in])
        assert rc_aw != 0
        with open(warn_target, "rb") as f:
            assert f.read() == b"PRECIOUS_PJG_BEFORE_WARNING", "Pre-existing output destroyed on warning skip!"

        err_in = os.path.join(atomic_dir, "keep_on_err.jpg")
        err_target = os.path.join(atomic_dir, "keep_on_err.pjg")
        with open(err_in, "wb") as f:
            f.write(b"\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01")
        with open(err_target, "wb") as f:
            f.write(b"PRECIOUS_PJG_BEFORE_ERROR")
        rc_ae, _, _ = run_cli(binary, ["-np", "-o", err_in])
        assert rc_ae != 0
        with open(err_target, "rb") as f:
            assert f.read() == b"PRECIOUS_PJG_BEFORE_ERROR", "Pre-existing output destroyed on decode error!"

        ver_in = os.path.join(atomic_dir, "keep_on_ver_fail.jpg")
        ver_target = os.path.join(atomic_dir, "keep_on_ver_fail.pjg")
        shutil.copy2(mixed_pad_path, ver_in)
        with open(ver_target, "wb") as f:
            f.write(b"PRECIOUS_PJG_BEFORE_VER_FAIL")
        rc_av, _, _ = run_cli(binary, ["-np", "-p", "-ver", "-o", ver_in])
        assert rc_av != 0
        with open(ver_target, "rb") as f:
            assert f.read() == b"PRECIOUS_PJG_BEFORE_VER_FAIL", "Pre-existing output destroyed on -ver mismatch!"
        assert not [n for n in os.listdir(atomic_dir) if ".packjpg_tmp_" in n], "Leftover staging file found!"

        # Finding #11: hard-linked target -> input refused under -o; dangling symlink preserved without -o
        hl_dir = os.path.join(tmp, "hardlink_guard")
        os.makedirs(hl_dir)
        hl_jpg = os.path.join(hl_dir, "linked.jpg")
        hl_pjg = os.path.join(hl_dir, "linked.pjg")
        with open(hl_jpg, "wb") as f:
            f.write(first_jpg_bytes)
        try:
            os.link(hl_jpg, hl_pjg)
            has_hardlink = True
        except OSError:
            has_hardlink = False
        if has_hardlink:
            rc_hl, _, _ = run_cli(binary, ["-np", "-o", hl_jpg])
            assert rc_hl != 0, "Expected -o to refuse overwriting target hard-linked to input"
            with open(hl_jpg, "rb") as f:
                assert f.read() == first_jpg_bytes

            # Batch input hard-link check: b_alias.dat is hard-linked to batch_item.pjg
            b_jpg = os.path.join(hl_dir, "batch_item.jpg")
            b_pjg = os.path.join(hl_dir, "batch_item.pjg")
            b_alias = os.path.join(hl_dir, "b_alias.pjg")
            with open(b_jpg, "wb") as f:
                f.write(first_jpg_bytes)
            with open(b_pjg, "wb") as f:
                f.write(pjg_stdout)
            os.link(b_pjg, b_alias)
            rc_bhl, _, _ = run_cli(binary, ["-np", "-o", b_jpg, b_alias])
            assert rc_bhl != 0, "Expected BatchPathGuard to block overwriting hard-linked batch input"
            with open(b_alias, "rb") as f:
                assert f.read() == pjg_stdout

        if os.name != "nt":
            sym_dir = os.path.join(tmp, "dangling_sym")
            os.makedirs(sym_dir)
            sym_jpg = os.path.join(sym_dir, "dangle.jpg")
            sym_pjg = os.path.join(sym_dir, "dangle.pjg")
            with open(sym_jpg, "wb") as f:
                f.write(first_jpg_bytes)
            os.symlink(os.path.join(sym_dir, "nonexistent_target"), sym_pjg)
            rc_sym, _, _ = run_cli(binary, ["-np", sym_jpg])
            assert rc_sym == 0
            assert os.path.islink(sym_pjg), "Dangling symlink was clobbered without -o!"
            assert os.path.isfile(os.path.join(sym_dir, "dangle_.pjg")), "Expected dangle_.pjg alongside dangling symlink"

            # Pass-3 Bug #1: -o writes through existing symlink and preserves 0600 permissions on target
            wt_dir = os.path.join(tmp, "writethrough_sym")
            wt_real_dir = os.path.join(wt_dir, "real_dest")
            os.makedirs(wt_real_dir)
            wt_jpg = os.path.join(wt_dir, "photo.jpg")
            wt_real_pjg = os.path.join(wt_real_dir, "actual.pjg")
            wt_link_pjg = os.path.join(wt_dir, "photo.pjg")
            with open(wt_jpg, "wb") as f:
                f.write(first_jpg_bytes)
            with open(wt_real_pjg, "wb") as f:
                f.write(b"OLD_SECRET")
            os.chmod(wt_real_pjg, 0o600)
            os.symlink(wt_real_pjg, wt_link_pjg)
            rc_wt, _, _ = run_cli(binary, ["-np", "-o", wt_jpg])
            assert rc_wt == 0, "Expected -o symlink write-through to succeed"
            assert os.path.islink(wt_link_pjg), "Symlink was replaced by regular file under -o!"
            assert (os.stat(wt_real_pjg).st_mode & 0o777) == 0o600, "0600 file mode was not preserved under -o!"
            with open(wt_real_pjg, "rb") as f:
                assert f.read() == pjg_stdout, "Linked target file did not receive valid .pjg bytes!"

            # Pass-3 Bug #2: directory-symlink -j2 batch & concurrent two-process non-overwrite safety
            alias_base = os.path.join(tmp, "alias_race")
            real_sub = os.path.join(alias_base, "real")
            alias_sub = os.path.join(alias_base, "alias")
            os.makedirs(real_sub)
            os.symlink(real_sub, alias_sub)
            with open(os.path.join(real_sub, "same.jpg"), "wb") as f:
                f.write(first_jpg_bytes)
            with open(os.path.join(real_sub, "same.jpeg"), "wb") as f:
                f.write(first_jpg_bytes)
            rc_ab, _, _ = run_cli(binary, ["-np", "-j2", os.path.join(real_sub, "same.jpg"), os.path.join(alias_sub, "same.jpeg")])
            assert rc_ab == 0
            assert os.path.isfile(os.path.join(real_sub, "same.pjg")) and os.path.isfile(os.path.join(real_sub, "same_.pjg")), (
                "Directory-aliased -j2 non-overwrite batch lost one of same.pjg / same_.pjg!"
            )

        # Pass-3 Bug #2 (cross-platform): two concurrent packJPG processes without -o must never clobber each other
        proc_race_dir = os.path.join(tmp, "proc_race")
        os.makedirs(proc_race_dir)
        pr_a = os.path.join(proc_race_dir, "race.jpg")
        pr_b = os.path.join(proc_race_dir, "race.jpeg")
        with open(pr_a, "wb") as f:
            f.write(first_jpg_bytes)
        with open(pr_b, "wb") as f:
            f.write(first_jpg_bytes)
        p1 = subprocess.Popen([binary, "-np", pr_a], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        p2 = subprocess.Popen([binary, "-np", pr_b], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        _, _ = p1.communicate(timeout=30)
        _, _ = p2.communicate(timeout=30)
        assert p1.returncode == 0 and p2.returncode == 0
        for expected_out in ("race.pjg", "race_.pjg"):
            ep = os.path.join(proc_race_dir, expected_out)
            assert os.path.isfile(ep), f"Concurrent non-overwrite processes lost {expected_out}!"
            with open(ep, "rb") as f:
                assert f.read() == pjg_stdout

        # Pass-3 Bug #8 (cross-platform): UTF-8 multibyte '?' wildcard matches 1 Unicode code point ('bé.jpg')
        u8_glob_dir = os.path.join(tmp, "u8_glob")
        os.makedirs(u8_glob_dir)
        u8_accent = os.path.join(u8_glob_dir, "b\u00e9.jpg")
        u8_two = os.path.join(u8_glob_dir, "b\u00e9x.jpg")
        with open(u8_accent, "wb") as f:
            f.write(first_jpg_bytes)
        with open(u8_two, "wb") as f:
            f.write(first_jpg_bytes)
        rc_u8g, _, _ = run_cli(binary, ["-np", os.path.join(u8_glob_dir, "b?.jpg")])
        assert rc_u8g == 0, "UTF-8 '?' wildcard failed to match multibyte 'bé.jpg'"
        assert os.path.isfile(os.path.join(u8_glob_dir, "b\u00e9.pjg"))
        assert not os.path.exists(os.path.join(u8_glob_dir, "b\u00e9x.pjg"))

        print("  [PASS] -out lazy directory creation, */? wildcards, -r hierarchy, -j4 parallel batch, and third-pass CLI safety guards")

        # 9. Fourth-pass CLI regressions:
        #    - Bug #2: -ver on >4,096-block (640x480 = 4,800 blocks) JPEG & PJG, plus -d -ver on metadata-bearing JPEG & PJG
        #    - Bug #6 & #7: 300-file -j4 parallel batch with mixed invalid item (O(1) BatchPathGuard, zero leftover .packjpg_tmp_*)
        #    - Bug #8 & #9: -ver -o through /dev/null & POSIX FIFO symlinks, plus symlinked-dir '../' relative symlink
        print("[7/8] Testing fourth-pass CLI regressions (-ver >4096 blocks, -d -ver, special targets, and 300-file -j4 batch)...")
        p4_dir = os.path.join(tmp, "fourth_pass")
        os.makedirs(p4_dir)

        # Build a valid 640x480 (80x60 = 4,800 blocks > 4,096) grayscale baseline JPEG with APP0 + COM metadata
        vga_w, vga_h = 640, 480
        vga_blocks = (vga_w // 8) * (vga_h // 8)
        vga_jpg_bytes = (
            b"\xff\xd8"
            + b"\xff\xe0\x00\x10JFIF\x00\x01\x01\x01\x00\x48\x00\x48\x00\x00"
            + b"\xff\xfe\x00\x0cpackJPG-p4"
            + b"\xff\xdb\x00\x43\x00" + (b"\x10" * 64)
            + bytes([0xFF, 0xC0, 0x00, 0x0B, 0x08, (vga_h >> 8) & 0xFF, vga_h & 0xFF, (vga_w >> 8) & 0xFF, vga_w & 0xFF, 0x01, 0x01, 0x11, 0x00])
            + b"\xff\xc4\x00\x14\x00\x01" + (b"\x00" * 16)
            + b"\xff\xc4\x00\x14\x10\x01" + (b"\x00" * 16)
            + b"\xff\xda\x00\x08\x01\x01\x00\x00\x3f\x00"
            + (b"\x00" * ((vga_blocks * 2) // 8))
            + b"\xff\xd9"
        )
        vga_jpg = os.path.join(p4_dir, "vga_640x480.jpg")
        vga_pjg = os.path.join(p4_dir, "vga_640x480.pjg")
        with open(vga_jpg, "wb") as f:
            f.write(vga_jpg_bytes)

        rc_vga_c, _, _ = run_cli(binary, ["-np", "-ver", "-o", vga_jpg])
        assert rc_vga_c == 0 and os.path.isfile(vga_pjg), "Failed -ver compression on 640x480 (>4,096 blocks) JPEG!"
        vga_rt_dir = os.path.join(p4_dir, "vga_rt")
        rc_vga_d, _, _ = run_cli(binary, ["-np", "-ver", "-o", "-out", vga_rt_dir, vga_pjg])
        assert rc_vga_d == 0, "Failed -ver decompression on 640x480 (>4,096 blocks) PJG!"
        with open(os.path.join(vga_rt_dir, "vga_640x480.jpg"), "rb") as f:
            assert f.read() == vga_jpg_bytes

        # -d -ver on metadata-bearing JPEG and metadata-bearing PJG
        dver_dir = os.path.join(p4_dir, "dver")
        rc_dv_c, _, _ = run_cli(binary, ["-np", "-d", "-ver", "-o", "-out", dver_dir, vga_jpg])
        assert rc_dv_c == 0 and os.path.isfile(os.path.join(dver_dir, "vga_640x480.pjg")), "Failed -d -ver on metadata JPEG!"
        dver_jpg_dir = os.path.join(p4_dir, "dver_from_pjg")
        rc_dv_d, _, _ = run_cli(binary, ["-np", "-d", "-ver", "-o", "-out", dver_jpg_dir, vga_pjg])
        assert rc_dv_d == 0, "Failed -d -ver on metadata PJG!"
        with open(os.path.join(dver_jpg_dir, "vga_640x480.jpg"), "rb") as f:
            stripped_bytes = f.read()
        assert b"JFIF" not in stripped_bytes and b"packJPG-p4" not in stripped_bytes

        if os.name != "nt":
            # Bug #8: -ver -o through /dev/null symlink reports non-zero compression ratio and succeeds!
            devnull_dir = os.path.join(p4_dir, "devnull_ver")
            os.makedirs(devnull_dir)
            dn_jpg = os.path.join(devnull_dir, "null_ver.jpg")
            dn_pjg = os.path.join(devnull_dir, "null_ver.pjg")
            with open(dn_jpg, "wb") as f:
                f.write(vga_jpg_bytes)
            os.symlink("/dev/null", dn_pjg)
            rc_dn, dn_out, _ = run_cli(binary, ["-np", "-v0", "-ver", "-o", dn_jpg])
            assert rc_dn == 0, "Failed -ver -o through /dev/null symlink!"
            assert b"0.00%" not in dn_out, f"Expected non-zero compression ratio on /dev/null, got {dn_out!r}"

            # Bug #8: -ver -o through POSIX FIFO symlink succeeds without deadlocking open(O_RDONLY)!
            fifo_dir = os.path.join(p4_dir, "fifo_ver")
            os.makedirs(fifo_dir)
            real_fifo = os.path.join(fifo_dir, "sink.fifo")
            os.mkfifo(real_fifo, 0o600)
            fifo_jpg = os.path.join(fifo_dir, "streamed.jpg")
            fifo_pjg = os.path.join(fifo_dir, "streamed.pjg")
            with open(fifo_jpg, "wb") as f:
                f.write(first_jpg_bytes)
            os.symlink(real_fifo, fifo_pjg)
            fifo_received = bytearray()

            def _drain_fifo():
                with open(real_fifo, "rb") as rf:
                    fifo_received.extend(rf.read())

            t_fifo = threading.Thread(target=_drain_fifo)
            t_fifo.start()
            rc_ff, _, _ = run_cli(binary, ["-np", "-ver", "-o", fifo_jpg])
            t_fifo.join(timeout=10)
            assert rc_ff == 0 and bytes(fifo_received) == pjg_stdout, "Failed -ver -o through FIFO symlink!"

        # Bugs #6 & #7: 300-file -j4 batch with 1 corrupt input mixed in (O(1) BatchPathGuard + clean staging cleanup)
        scale_in = os.path.join(p4_dir, "scale_in")
        scale_out = os.path.join(p4_dir, "scale_out")
        os.makedirs(scale_in)
        for idx in range(300):
            with open(os.path.join(scale_in, f"img_{idx:04d}.jpg"), "wb") as f:
                f.write(first_jpg_bytes)
        with open(os.path.join(scale_in, "corrupt_mid.jpg"), "wb") as f:
            f.write(b"\xff\xd8\xff\xc0\x00\x0b\x08")
        rc_sc, _, _ = run_cli(binary, ["-np", "-j4", "-r", "-out", scale_out, scale_in])
        assert rc_sc != 0, "Expected non-zero exit when batch includes 1 corrupt file"
        produced = [n for n in os.listdir(scale_out) if n.endswith(".pjg")]
        leftovers = [n for n in os.listdir(scale_out) if ".packjpg_tmp_" in n]
        assert len(produced) == 300, f"Expected 300 valid outputs in -j4 batch, got {len(produced)}"
        assert not leftovers, f"Found leftover staging files in -j4 batch: {leftovers}"
        print("  [PASS] Fourth-pass CLI regressions (-ver >4096 blocks, -d -ver, special stream targets, and 300-file -j4 batch)")

        # 10. Combinatorial CLI filesystem & concurrency property matrix:
        #     Verifies all 4 universal postconditions across (input_kind) x (target_state) x (overwrite) x (verify) + (-j2 pairs):
        #       P1. Input file bytes are never modified or unlinked.
        #       P2. Zero leftover .packjpg_tmp_* staging files anywhere in the sandbox.
        #       P3. On rc != 0, pre-existing target file/dir/symlink (and linked target + mode) is 100% untouched.
        #       P4. Without -o, pre-existing targets/symlinks step aside to stem_.ext; with -o, symlinks write through & preserve mode.
        print("[8/8] Running combinatorial CLI filesystem & concurrency property matrix...")
        matrix_root = os.path.join(tmp, "prop_matrix")
        os.makedirs(matrix_root)
        can_test_ro = (os.name == "nt") or (hasattr(os, "geteuid") and os.geteuid() != 0)

        target_states = ["absent", "existing_0600", "existing_dir", "hardlink_to_input"]
        if can_test_ro:
            target_states.append("readonly_0400")
        if os.name != "nt":
            target_states.extend([
                "symlink_existing_0600",
                "symlink_dangling",
                "symlink_rel_parent",
                "symlink_loop",
                "dev_null",
            ])

        core_inputs = [
            ("valid_jpg", "item.jpg", first_jpg_bytes, "pjg", pjg_stdout, True),
            ("valid_pjg", "item.pjg", pjg_stdout, "jpg", first_jpg_bytes, True),
            ("warn_jpg", "warn.jpg", bytes(raw_gray), "pjg", None, False),
            ("corrupt_jpg", "bad.jpg", b"\xff\xd8\xff\xc0\x00\x0b\x08", "pjg", None, False),
        ]
        extra_inputs = [
            ("unicode_jpg", "\u00fc\u00f1\u00ee\u00e7\u00f8d\u00e9_\u96ea.jpg", first_jpg_bytes, "pjg", pjg_stdout, True),
            ("long_jpg", os.path.join("L" * 60, "M" * 60, "N" * 60, "deep_item.jpg"), first_jpg_bytes, "pjg", pjg_stdout, True),
        ]

        def assert_no_staging_files(root_dir: str, ctx_desc: str) -> None:
            for cur_root, dirs, files in os.walk(root_dir):
                for name in dirs + files:
                    if ".packjpg_tmp_" in name:
                        raise AssertionError(f"Leftover staging entry {name!r} in {ctx_desc}")

        cells_checked = 0
        cell_seq = 0
        all_runs = []
        for spec in core_inputs:
            for tstate in target_states:
                for ovw in (False, True):
                    for ver in (False, True):
                        all_runs.append((spec, tstate, ovw, ver))
        extra_tstates = ["absent", "existing_0600", "existing_dir", "hardlink_to_input"]
        if os.name != "nt":
            extra_tstates.extend(["symlink_existing_0600", "symlink_dangling"])
        for spec in extra_inputs:
            for tstate in extra_tstates:
                for ovw in (False, True):
                    all_runs.append((spec, tstate, ovw, True))

        for (ikind, rel_in, in_bytes, out_ext, expected_out_bytes, input_valid), tstate, ovw, ver in all_runs:
            cell_seq += 1
            cell_dir = os.path.join(matrix_root, f"c{cell_seq:04d}")
            os.makedirs(cell_dir)
            ctx_desc = f"{ikind}/{tstate}/ovw={ovw}/ver={ver}"
            chmod_cleanup = []
            try:
                if tstate == "symlink_rel_parent":
                    real_parent = os.path.join(cell_dir, "real_p")
                    sib_dir = os.path.join(cell_dir, "sib_p")
                    sym_parent = os.path.join(cell_dir, "sym_p")
                    os.makedirs(real_parent)
                    os.makedirs(sib_dir)
                    os.symlink(real_parent, sym_parent)
                    in_path = os.path.join(sym_parent, rel_in)
                else:
                    in_path = os.path.join(cell_dir, rel_in)

                os.makedirs(os.path.dirname(in_path), exist_ok=True)
                with open(in_path, "wb") as f:
                    f.write(in_bytes)

                stem_no_ext = os.path.splitext(in_path)[0]
                target_path = f"{stem_no_ext}.{out_ext}"
                alt_path = f"{stem_no_ext}_.{out_ext}"
                linked_real_path = None
                sentinel_bytes = None

                if tstate == "existing_0600":
                    sentinel_bytes = b"SENTINEL_0600"
                    with open(target_path, "wb") as f:
                        f.write(sentinel_bytes)
                    if os.name != "nt":
                        os.chmod(target_path, 0o600)
                elif tstate == "readonly_0400":
                    sentinel_bytes = b"SENTINEL_0400"
                    with open(target_path, "wb") as f:
                        f.write(sentinel_bytes)
                    os.chmod(target_path, stat.S_IREAD if os.name == "nt" else 0o400)
                    chmod_cleanup.append(target_path)
                elif tstate == "existing_dir":
                    os.makedirs(target_path)
                    with open(os.path.join(target_path, "keep.txt"), "wb") as f:
                        f.write(b"KEEP")
                elif tstate == "hardlink_to_input":
                    try:
                        os.link(in_path, target_path)
                    except OSError:
                        continue
                    sentinel_bytes = in_bytes
                elif tstate == "symlink_existing_0600":
                    real_sub = os.path.join(cell_dir, "linked_sub")
                    os.makedirs(real_sub)
                    linked_real_path = os.path.join(real_sub, "linked_target.bin")
                    sentinel_bytes = b"LINKED_0600"
                    with open(linked_real_path, "wb") as f:
                        f.write(sentinel_bytes)
                    os.chmod(linked_real_path, 0o600)
                    os.symlink(linked_real_path, target_path)
                elif tstate == "symlink_dangling":
                    real_sub = os.path.join(cell_dir, "dangling_sub")
                    os.makedirs(real_sub)
                    linked_real_path = os.path.join(real_sub, "created_target.bin")
                    os.symlink(linked_real_path, target_path)
                elif tstate == "symlink_rel_parent":
                    linked_real_path = os.path.join(cell_dir, "sib_p", "rel_target.bin")
                    sentinel_bytes = b"REL_0600"
                    with open(linked_real_path, "wb") as f:
                        f.write(sentinel_bytes)
                    os.chmod(linked_real_path, 0o600)
                    real_in_dir = os.path.join(cell_dir, "real_p")
                    real_target_link = os.path.join(real_in_dir, os.path.basename(target_path))
                    os.symlink(os.path.join("..", "sib_p", "rel_target.bin"), real_target_link)
                elif tstate == "symlink_loop":
                    peer_link = f"{stem_no_ext}.loop_peer"
                    os.symlink(peer_link, target_path)
                    os.symlink(target_path, peer_link)
                elif tstate == "dev_null":
                    os.symlink("/dev/null", target_path)

                flags = ["-np"]
                if ovw:
                    flags.append("-o")
                if ver:
                    flags.append("-ver")
                flags.append(in_path)

                target_blocks_ovw = tstate in ("existing_dir", "readonly_0400", "hardlink_to_input", "symlink_loop")
                expect_ok = input_valid and not (ovw and target_blocks_ovw)

                rc_m, out_m, err_m = run_cli(binary, flags)

                # Postcondition P1: input file bytes are never modified or deleted!
                assert os.path.isfile(in_path), f"Input unlinked in {ctx_desc}"
                with open(in_path, "rb") as f:
                    assert f.read() == in_bytes, f"Input mutated in {ctx_desc}"

                # Postcondition P2: zero leftover .packjpg_tmp_* staging files!
                assert_no_staging_files(cell_dir, ctx_desc)

                if expect_ok:
                    assert rc_m == 0, f"Expected rc==0 in {ctx_desc}, got rc={rc_m}: {out_m!r} {err_m!r}"
                else:
                    assert rc_m != 0, f"Expected rc!=0 in {ctx_desc}, got rc=0"

                if rc_m != 0:
                    # Postcondition P3: atomic preservation of pre-existing target state on failure!
                    assert not os.path.exists(alt_path), f"Unexpected {alt_path} on failure in {ctx_desc}"
                    if tstate == "absent":
                        assert not os.path.exists(target_path) and not os.path.islink(target_path), f"Partial target created in {ctx_desc}"
                    elif tstate in ("existing_0600", "readonly_0400", "hardlink_to_input"):
                        with open(target_path, "rb") as f:
                            assert f.read() == sentinel_bytes, f"Pre-existing target clobbered on failure in {ctx_desc}"
                    elif tstate == "existing_dir":
                        assert os.path.isdir(target_path) and os.path.isfile(os.path.join(target_path, "keep.txt")), ctx_desc
                    elif tstate in ("symlink_existing_0600", "symlink_rel_parent"):
                        assert os.path.islink(target_path), f"Symlink lost on failure in {ctx_desc}"
                        with open(linked_real_path, "rb") as f:
                            assert f.read() == sentinel_bytes, f"Linked target clobbered on failure in {ctx_desc}"
                        assert (os.stat(linked_real_path).st_mode & 0o777) == 0o600, ctx_desc
                    elif tstate == "symlink_dangling":
                        assert os.path.islink(target_path) and not os.path.exists(linked_real_path), ctx_desc
                    elif tstate == "symlink_loop":
                        assert os.path.islink(target_path), ctx_desc
                else:
                    # Postcondition P4: exact output & symlink preservation / write-through contract on success!
                    if not ovw:
                        if tstate == "absent":
                            with open(target_path, "rb") as f:
                                assert f.read() == expected_out_bytes, ctx_desc
                        else:
                            with open(alt_path, "rb") as f:
                                assert f.read() == expected_out_bytes, f"Expected {alt_path} in {ctx_desc}"
                            if tstate in ("existing_0600", "readonly_0400", "hardlink_to_input"):
                                with open(target_path, "rb") as f:
                                    assert f.read() == sentinel_bytes, ctx_desc
                            elif tstate == "existing_dir":
                                assert os.path.isdir(target_path), ctx_desc
                            elif tstate in ("symlink_existing_0600", "symlink_rel_parent"):
                                assert os.path.islink(target_path), ctx_desc
                                with open(linked_real_path, "rb") as f:
                                    assert f.read() == sentinel_bytes, ctx_desc
                            elif tstate == "symlink_dangling":
                                assert os.path.islink(target_path) and not os.path.exists(linked_real_path), ctx_desc
                            elif tstate == "symlink_loop":
                                assert os.path.islink(target_path), ctx_desc
                    else:
                        if tstate in ("absent", "existing_0600"):
                            with open(target_path, "rb") as f:
                                assert f.read() == expected_out_bytes, ctx_desc
                            if tstate == "existing_0600" and os.name != "nt":
                                assert (os.stat(target_path).st_mode & 0o777) == 0o600, f"0600 mode lost in {ctx_desc}"
                        elif tstate in ("symlink_existing_0600", "symlink_rel_parent"):
                            assert os.path.islink(target_path), f"Symlink replaced under -o in {ctx_desc}"
                            with open(linked_real_path, "rb") as f:
                                assert f.read() == expected_out_bytes, ctx_desc
                            assert (os.stat(linked_real_path).st_mode & 0o777) == 0o600, f"0600 mode lost in {ctx_desc}"
                        elif tstate == "symlink_dangling":
                            assert os.path.islink(target_path), f"Dangling symlink replaced under -o in {ctx_desc}"
                            with open(linked_real_path, "rb") as f:
                                assert f.read() == expected_out_bytes, ctx_desc
                        elif tstate == "dev_null":
                            assert os.path.islink(target_path), ctx_desc

                cells_checked += 1
            finally:
                for ro_p in chmod_cleanup:
                    try:
                        os.chmod(ro_p, stat.S_IWRITE | stat.S_IREAD if os.name == "nt" else 0o600)
                    except OSError:
                        pass
                shutil.rmtree(cell_dir, ignore_errors=True)

        # Parallel -j2 multi-item property checks (mixed valid + corrupt with -ver, and stem collision without -o)
        j2_dir = os.path.join(matrix_root, "j2_pairs")
        os.makedirs(j2_dir)
        j2_valid = os.path.join(j2_dir, "ok.jpg")
        j2_bad = os.path.join(j2_dir, "corrupt.jpg")
        j2_bad_existing = os.path.join(j2_dir, "corrupt.pjg")
        with open(j2_valid, "wb") as f:
            f.write(first_jpg_bytes)
        with open(j2_bad, "wb") as f:
            f.write(b"\xff\xd8\xff\xc0\x00\x0b\x08")
        with open(j2_bad_existing, "wb") as f:
            f.write(b"KEEP_CORRUPT_TARGET")
        rc_j2m, _, _ = run_cli(binary, ["-np", "-j2", "-o", "-ver", j2_valid, j2_bad])
        assert rc_j2m != 0
        with open(os.path.join(j2_dir, "ok.pjg"), "rb") as f:
            assert f.read() == pjg_stdout
        with open(j2_bad_existing, "rb") as f:
            assert f.read() == b"KEEP_CORRUPT_TARGET"
        assert_no_staging_files(j2_dir, "j2_mixed_pair")

        # Verbosity -v2 stage-by-stage reporting on both valid -ver round-trip and corrupt input
        rc_v2_ok, out_v2_ok, _ = run_cli(binary, ["-np", "-v2", "-ver", "-o", j2_valid])
        rc_v2_err, out_v2_err, _ = run_cli(binary, ["-np", "-v2", j2_bad])
        assert rc_v2_ok == 0 and b"Verifying output stream" in out_v2_ok
        assert rc_v2_err != 0 and b"ERROR" in out_v2_err
        print(f"  [PASS] Combinatorial property matrix ({cells_checked} filesystem/flag cells + -j2/-v2 verified)")

    print("\nAll CLI & golden bitstream tests passed!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
