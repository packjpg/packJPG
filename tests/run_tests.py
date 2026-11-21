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
        print("[1/6] Golden bitstream SHA-256 & round-trip matrix...")
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
        print("[2/6] Metadata discard (-d) golden check...")
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
        print("[3/6] Pipe mode (-p stdin/stdout) round-trip & error cleanliness...")
        rc, pjg_stdout, _ = run_cli(binary, ["-np", "-p", "-ver", "-"], stdin_bytes=first_jpg_bytes)
        assert rc == 0 and pjg_stdout.startswith(b"JS"), f"Pipe compress failed (rc={rc}, len={len(pjg_stdout)})"
        rc, jpg_stdout, _ = run_cli(binary, ["-np", "-p", "-ver", "-"], stdin_bytes=pjg_stdout)
        assert rc == 0 and jpg_stdout == first_jpg_bytes, "Pipe round-trip mismatch"
        rc, bad_stdout, _ = run_cli(binary, ["-np", "-p", "-"], stdin_bytes=b"JS\x19\xff\xff\xff")
        assert bad_stdout == b"", f"Expected empty stdout on failed pipe decode, got {len(bad_stdout)} bytes"
        print("  [PASS] -p/-ver binary stdin/stdout round-trip and clean failure stdout")

        # 4. Non-overwrite suffixing, directory path, long filenames, and format-string filenames
        print("[4/6] CLI file naming, directory rejection, long paths, and format-string safety...")
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
                run_cli(binary, ["-np", "-v1", ro_pjg])
            finally:
                os.chmod(ro_dir, stat.S_IRWXU)
        print("  [PASS] non-overwrite suffixing, directory path, long paths, and format-string filenames")

        # 5. Malformed / truncated JPEG & PJG negative regression suite
        print("[5/6] Malformed JPEG & PJG negative regressions...")
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
        print("[6/6] Output directory (-out), wildcards, recursive (-r), and parallel (-j4) batch...")
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
        print("  [PASS] -out lazy directory creation, */? wildcards, -r hierarchy, and -j4 parallel batch")

    print("\nAll CLI & golden bitstream tests passed!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
