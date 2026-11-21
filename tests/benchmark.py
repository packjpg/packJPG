#!/usr/bin/env python3
"""Repeatable encode/decode throughput and peak-RSS benchmark for packJPG."""

import argparse
import hashlib
import math
import os
import re
import shutil
import statistics
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def _build_synthetic_rgb(width: int, height: int) -> bytes:
    """Deterministic multi-frequency + edge + micro-texture RGB image."""
    buf = bytearray(width * height * 3)
    # Precompute 1D tables so generating multi-megapixel images in Python is fast
    sin_x1 = [int(60.0 * math.sin(x * 0.031)) for x in range(width)]
    cos_x2 = [int(35.0 * math.cos(x * 0.113)) for x in range(width)]
    sin_y1 = [int(60.0 * math.cos(y * 0.027)) for y in range(height)]
    sin_y2 = [int(35.0 * math.sin(y * 0.097)) for y in range(height)]

    idx = 0
    for y in range(height):
        sy1 = sin_y1[y]
        sy2 = sin_y2[y]
        band = 28 if ((y // 24) & 1) else -28
        row_seed = (y * 2654435761) & 0xFFFFFFFF
        for x in range(width):
            # Fast deterministic hash for fine photographic grain
            h = ((x * 2246822519) ^ row_seed) & 0xFFFFFFFF
            h = ((h ^ (h >> 15)) * 3266489917) & 0xFFFFFFFF
            grain = ((h >> 24) & 0x1F) - 16
            edge = band if ((x // 32) & 1) else -band

            r = 128 + sin_x1[x] + sy1 + (edge >> 1) + grain
            g = 124 + cos_x2[x] + sy2 - (edge >> 2) + (grain >> 1)
            b = 132 - (sin_x1[x] >> 1) + cos_x2[x] + sy1 // 2 - grain

            buf[idx] = 0 if r < 0 else (255 if r > 255 else r)
            buf[idx + 1] = 0 if g < 0 else (255 if g > 255 else g)
            buf[idx + 2] = 0 if b < 0 else (255 if b > 255 else b)
            idx += 3
    return bytes(buf)


def generate_corpus(out_dir: Path) -> list:
    """Create 4 representative JPEGs (thumbnail, baseline, progressive, 4:4:4 hi-res)."""
    from PIL import Image

    specs = [
        ("thumb_64x64_base", 64, 64, {"quality": 88, "subsampling": 2, "progressive": False, "optimize": True}, 40),
        ("web_1024x768_base", 1024, 768, {"quality": 90, "subsampling": 2, "progressive": False, "optimize": True}, 7),
        ("prog_1280x960_420", 1280, 960, {"quality": 90, "subsampling": 2, "progressive": True, "optimize": True}, 5),
        ("photo_2048x1536_444", 2048, 1536, {"quality": 93, "subsampling": 0, "progressive": False, "optimize": True}, 3),
    ]

    cases = []
    for name, w, h, save_kwargs, default_iters in specs:
        jpg_path = out_dir / f"{name}.jpg"
        rgb = _build_synthetic_rgb(w, h)
        img = Image.frombytes("RGB", (w, h), rgb)
        img.save(jpg_path, format="JPEG", **save_kwargs)
        cases.append((name, jpg_path, jpg_path.stat().st_size, default_iters))
    return cases


def build_rss_wrapper(work_dir: Path):
    cc = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if not cc or os.name == "nt":
        return None
    src = work_dir / "rss_wrap.c"
    exe = work_dir / "rss_wrap"
    src.write_text(
        "#include <stdio.h>\n"
        "#include <sys/resource.h>\n"
        "#include <sys/wait.h>\n"
        "#include <unistd.h>\n"
        "int main(int argc, char** argv) {\n"
        "    if (argc < 2) return 2;\n"
        "    pid_t pid = fork();\n"
        "    if (pid == 0) {\n"
        "        freopen(\"/dev/null\", \"w\", stdout);\n"
        "        freopen(\"/dev/null\", \"w\", stderr);\n"
        "        execv(argv[1], &argv[1]);\n"
        "        _exit(127);\n"
        "    }\n"
        "    int status = 0;\n"
        "    struct rusage ru;\n"
        "    if (wait4(pid, &status, 0, &ru) < 0) return 2;\n"
        "    printf(\"%ld\\n\", (long)ru.ru_maxrss);\n"
        "    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;\n"
        "}\n"
    )
    res = subprocess.run([cc, "-O2", str(src), "-o", str(exe)], capture_output=True)
    return exe if res.returncode == 0 else None


def measure_peak_rss_kb(cmd: list, rss_wrap=None) -> int:
    if rss_wrap is not None:
        proc = subprocess.run([str(rss_wrap), *cmd], capture_output=True, text=True, check=True)
        kb = int(proc.stdout.strip())
        return kb // 1024 if sys.platform == "darwin" else kb
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    return 0


def bench_binary(binary: Path, cases: list, work_dir: Path, scale_iters: float = 1.0, rss_wrap=None) -> dict:
    results = {}
    for name, src_jpg, jpg_size, base_iters in cases:
        iters = max(3, int(round(base_iters * scale_iters)))
        case_dir = work_dir / binary.name / name
        case_dir.mkdir(parents=True, exist_ok=True)
        jpg_file = case_dir / "input.jpg"
        pjg_file = case_dir / "input.pjg"
        shutil.copyfile(src_jpg, jpg_file)

        # Warmup + peak RSS measurement on encode
        enc_rss_kb = measure_peak_rss_kb([str(binary), "-np", "-o", str(jpg_file)], rss_wrap)
        pjg_bytes = pjg_file.read_bytes()
        pjg_sha256 = hashlib.sha256(pjg_bytes).hexdigest()

        # Verify bit-exact round-trip
        rt_dir = case_dir / "rt"
        rt_dir.mkdir(exist_ok=True)
        rt_pjg = rt_dir / "rt.pjg"
        rt_jpg = rt_dir / "rt.jpg"
        rt_pjg.write_bytes(pjg_bytes)
        dec_rss_kb = measure_peak_rss_kb([str(binary), "-np", "-o", str(rt_pjg)], rss_wrap)
        if rt_jpg.read_bytes() != src_jpg.read_bytes():
            raise RuntimeError(f"Round-trip mismatch for {binary} on {name}")

        enc_times = []
        for _ in range(iters):
            t0 = time.perf_counter()
            subprocess.run(
                [str(binary), "-np", "-o", str(jpg_file)],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=True,
            )
            enc_times.append(time.perf_counter() - t0)

        dec_times = []
        for _ in range(iters):
            t0 = time.perf_counter()
            subprocess.run(
                [str(binary), "-np", "-o", str(rt_pjg)],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=True,
            )
            dec_times.append(time.perf_counter() - t0)

        enc_med = statistics.median(enc_times)
        dec_med = statistics.median(dec_times)
        mb = jpg_size / (1024.0 * 1024.0)
        results[name] = {
            "jpg_bytes": jpg_size,
            "pjg_bytes": len(pjg_bytes),
            "pjg_sha256": pjg_sha256,
            "enc_ms": enc_med * 1000.0,
            "dec_ms": dec_med * 1000.0,
            "enc_mbs": mb / enc_med,
            "dec_mbs": mb / dec_med,
            "enc_rss_kb": enc_rss_kb,
            "dec_rss_kb": dec_rss_kb,
        }
    return results


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("binaries", nargs="+", type=Path, help="packJPG binary/binaries to benchmark")
    ap.add_argument("--scale", type=float, default=1.0, help="Iteration count multiplier")
    args = ap.parse_args()

    bins = [b.resolve() for b in args.binaries]
    for b in bins:
        if not b.is_file():
            print(f"Binary not found: {b}", file=sys.stderr)
            return 1

    with tempfile.TemporaryDirectory(prefix="packjpg_bench_") as tmp:
        tmp_path = Path(tmp)
        corpus_dir = tmp_path / "corpus"
        corpus_dir.mkdir()
        print("Generating deterministic benchmark corpus...")
        cases = generate_corpus(corpus_dir)
        for name, _, sz, iters in cases:
            print(f"  {name:24s}: {sz / 1024.0:8.1f} KB")

        rss_wrap = build_rss_wrapper(tmp_path)
        all_res = {}
        for idx, b in enumerate(bins):
            label = f"{b.name}#{idx}" if b.name in all_res else b.name
            print(f"\nBenchmarking {label} ({b})...")
            res = bench_binary(b, cases, tmp_path / f"run_{idx}", args.scale, rss_wrap)
            all_res[label] = res
            for name, _, _, _ in cases:
                r = res[name]
                print(
                    f"  {name:24s} | enc {r['enc_ms']:7.2f} ms ({r['enc_mbs']:6.2f} MB/s, RSS {r['enc_rss_kb']:6d} KB)"
                    f" | dec {r['dec_ms']:7.2f} ms ({r['dec_mbs']:6.2f} MB/s, RSS {r['dec_rss_kb']:6d} KB)"
                    f" | pjg {r['pjg_bytes'] / 1024.0:7.1f} KB"
                )

        if len(all_res) >= 2:
            labels = list(all_res.keys())
            base_lbl = labels[0]
            base = all_res[base_lbl]
            for cand_lbl in labels[1:]:
                cand = all_res[cand_lbl]
                print(f"\nComparison ({cand_lbl} vs {base_lbl}):")
                for name, _, _, _ in cases:
                    rb = base[name]
                    rc = cand[name]
                    same_hash = "IDENTICAL" if rb["pjg_sha256"] == rc["pjg_sha256"] else "DIFFERS!"
                    enc_sp = rb["enc_ms"] / rc["enc_ms"]
                    dec_sp = rb["dec_ms"] / rc["dec_ms"]
                    rss_delta = (
                        f"{(rc['enc_rss_kb'] - rb['enc_rss_kb']) / rb['enc_rss_kb'] * 100.0:+.1f}%"
                        if rb["enc_rss_kb"] > 0
                        else "n/a"
                    )
                    print(
                        f"  {name:24s} | enc {enc_sp:5.2f}x | dec {dec_sp:5.2f}x"
                        f" | enc RSS {rss_delta:>7s} | bitstream: {same_hash}"
                    )
                    if rb["pjg_sha256"] != rc["pjg_sha256"]:
                        print("ERROR: PJG bitstream diverged!", file=sys.stderr)
                        return 2

    return 0


if __name__ == "__main__":
    sys.exit(main())
