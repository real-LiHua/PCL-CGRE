#!/usr/bin/env python3
"""
Set up a Windows GTK4 sysroot from MSYS2 packages for cross-compilation.

Downloads the MSYS2 mingw64 package database, resolves dependencies for the
target packages (gtk4, libadwaita, libsoup3, json-glib, fontconfig), and
extracts them into a sysroot directory.

Usage:
    python3 setup_msys2_sysroot.py [--sysroot /path/to/sysroot] [--jobs N]
"""

import argparse
import os
import subprocess
import sys
import tarfile
import tempfile
import json
from pathlib import Path
from urllib.request import urlopen, urlretrieve
from concurrent.futures import ThreadPoolExecutor, as_completed

# ── Configuration ──────────────────────────────────────────────────────────────

MSYS2_MIRROR = os.environ.get("MSYS2_MIRROR", "https://repo.msys2.org/mingw/mingw64")
DB_URL = f"{MSYS2_MIRROR}/mingw64.db"

# Top-level packages we need (MSYS2 mingw-w64-x86_64-* naming)
TOP_PACKAGES = [
    "mingw-w64-x86_64-gtk4",
    "mingw-w64-x86_64-libadwaita",
    "mingw-w64-x86_64-libsoup3",
    "mingw-w64-x86_64-json-glib",
    "mingw-w64-x86_64-fontconfig",
]

# Additional toolchain / runtime good-to-haves
EXTRA_PACKAGES = [
    "mingw-w64-x86_64-gcc-libs",      # libgcc_s_seh-1.dll, libstdc++-6.dll
    "mingw-w64-x86_64-winpthreads",   # libwinpthread-1.dll
    "mingw-w64-x86_64-gettext",       # libintl-8.dll
    "mingw-w64-x86_64-libiconv",      # libiconv-2.dll
    "mingw-w64-x86_64-zlib",          # zlib1.dll
    "mingw-w64-x86_64-libpng",        # libpng16-16.dll
    "mingw-w64-x86_64-bzip2",         # libbz2-1.dll
    "mingw-w64-x86_64-xz",            # liblzma-5.dll
    "mingw-w64-x86_64-expat",         # libexpat-1.dll
    "mingw-w64-x86_64-pcre2",         # libpcre2-8-0.dll
    "mingw-w64-x86_64-libffi",        # libffi-8.dll
    "mingw-w64-x86_64-gdk-pixbuf2",   # gdk-pixbuf loaders
    "mingw-w64-x86_64-harfbuzz",
    "mingw-w64-x86_64-freetype",
    "mingw-w64-x86_64-cairo",
    "mingw-w64-x86_64-pango",
    "mingw-w64-x86_64-glib2",
    "mingw-w64-x86_64-gsettings-desktop-schemas",
    "mingw-w64-x86_64-shared-mime-info",
    "mingw-w64-x86_64-libjpeg-turbo",
    "mingw-w64-x86_64-libtiff",
    "mingw-w64-x86_64-librsvg",
    "mingw-w64-x86_64-libxml2",
    "mingw-w64-x86_64-graphite2",
    "mingw-w64-x86_64-fribidi",
    "mingw-w64-x86_64-pixman",
    "mingw-w64-x86_64-sqlite3",
]


def download_file(url, dest):
    """Download a file with progress indication."""
    print(f"  DOWNLOAD: {os.path.basename(url)}")
    try:
        urlretrieve(url, dest)
        return True
    except Exception as e:
        print(f"  ERROR downloading {url}: {e}", file=sys.stderr)
        return False


def fetch_package_db(db_url, cache_dir):
    """Download and parse the MSYS2 package database.

    The database is a gzip-compressed tar archive where each file is a
    directory named <pkgname>-<version> containing a `desc` file.
    """
    print(f"Fetching package database from {db_url} ...")
    db_path = cache_dir / "mingw64.db"
    if not db_path.exists():
        if not download_file(db_url, db_path):
            sys.exit(1)

    # Also fetch the .db.tar.gz variant (some mirrors differ)
    db_tar_path = cache_dir / "mingw64.db.tar.gz"
    if not db_tar_path.exists():
        db_tar_gz_url = db_url.replace(".db", ".db.tar.gz")
        try:
            urlretrieve(db_tar_gz_url, db_tar_path)
        except Exception:
            pass  # OK if this fails, .db might be a tar.gz already

    packages = {}  # name -> {version, depends, desc}

    # Try the .db file first (it's usually a tar archive)
    for path in [db_path, db_tar_path]:
        if not path.exists():
            continue
        try:
            with tarfile.open(path, "r:*") as tf:
                # The tar contains directories like "gtk4-4.18.2-1/"
                # each with a "desc" file inside
                for member in tf.getmembers():
                    parts = member.name.strip("/").split("/")
                    if len(parts) == 2 and parts[1] == "desc":
                        pkg_dir = parts[0]
                        # Parse pkgname-version-release
                        # But version can contain dashes, so extract carefully
                        desc_file = tf.extractfile(member)
                        if desc_file is None:
                            continue
                        info = parse_desc(desc_file.read().decode("utf-8", errors="replace"))
                        if info and "NAME" in info:
                            name = info["NAME"]
                            packages[name] = info
            if packages:
                break
        except Exception as e:
            print(f"  Could not read {path}: {e}")
            continue

    print(f"  Parsed {len(packages)} packages from database")
    return packages


def parse_desc(text):
    """Parse a pacman/alpm desc file.

    Format:
        %NAME%
        pkgname

        %VERSION%
        1.2.3-4

        %DEPENDS%
        dep1
        dep2

        %DESC%
        description here
    """
    info = {}
    current_key = None
    current_values = []

    for line in text.splitlines():
        line = line.strip()
        if not line:
            if current_key and current_values:
                if current_key in ("DEPENDS", "PROVIDES", "CONFLICTS",
                                   "REPLACES", "GROUPS", "LICENSE",
                                   "OPTDEPENDS", "MAKEDEPENDS", "CHECKDEPENDS"):
                    info[current_key] = current_values
                elif len(current_values) == 1:
                    info[current_key] = current_values[0]
                else:
                    info[current_key] = current_values
                current_values = []
            continue

        if line.startswith("%") and line.endswith("%"):
            current_key = line[1:-1]
            current_values = []
        elif current_key:
            current_values.append(line)

    return info


def resolve_dependencies(packages, top_names):
    """Recursively resolve all dependencies for the given package names.

    Returns an ordered list of package dicts (deps first).
    """
    resolved = []
    seen = set()

    def resolve(name):
        """Resolve a single dependency name (without version constraint)."""
        # Strip version constraints like ">=4.0", ">=1.0-2", etc.
        base = name.split(">=")[0].split("<=")[0].split("=")[0].split(">")[0].split("<")[0].strip()
        if base in seen:
            return
        seen.add(base)

        if base not in packages:
            print(f"  WARNING: Package not in database: {base} (from {name})")
            return

        pkg = packages[base]
        deps = pkg.get("DEPENDS", [])

        # Resolve dependencies first
        for dep in deps:
            resolve(dep)

        resolved.append(pkg)

    for name in top_names:
        resolve(name)

    return resolved


def download_and_extract(pkg_list, sysroot, cache_dir, mirror, jobs=4):
    """Download and extract packages into the sysroot."""
    print(f"\nDownloading and extracting {len(pkg_list)} packages "
          f"(sysroot: {sysroot}) ...\n")

    os.makedirs(sysroot, exist_ok=True)

    def process_pkg(pkg):
        name = pkg["NAME"]
        version = pkg["VERSION"]
        arch = pkg.get("ARCH", "any")
        if arch == "any":
            pkg_url_dir = mirror.replace("/mingw64", "/any")
        else:
            pkg_url_dir = mirror

        filename = f"{name}-{version}-{arch}.pkg.tar.zst"
        url = f"{pkg_url_dir}/{filename}"
        cache_path = cache_dir / filename

        # Also try .tar.xz if .tar.zst doesn't exist
        if not cache_path.exists():
            if not download_file(url, cache_path):
                # Try xz variant
                filename_xz = filename.replace(".zst", ".xz")
                url_xz = f"{pkg_url_dir}/{filename_xz}"
                cache_path_xz = cache_dir / filename_xz
                if download_file(url_xz, cache_path_xz):
                    cache_path = cache_path_xz
                else:
                    return (name, "DOWNLOAD_FAILED")

        # Extract
        try:
            # Use system tar which supports zstd if available,
            # otherwise try python tarfile with zstd
            result = subprocess.run(
                ["tar", "-xf", str(cache_path), "-C", str(sysroot),
                 "--warning=none"],
                capture_output=True, text=True
            )
            if result.returncode != 0:
                # Try with zstd explicitly
                result2 = subprocess.run(
                    ["zstd", "-d", "-c", str(cache_path)],
                    capture_output=True
                )
                if result2.returncode == 0:
                    subprocess.run(
                        ["tar", "-xf", "-", "-C", str(sysroot)],
                        input=result2.stdout
                    )
                else:
                    return (name, f"EXTRACT_FAILED: {result.stderr[:200]}")
            return (name, "OK")
        except Exception as e:
            return (name, f"ERROR: {e}")

    results = []
    with ThreadPoolExecutor(max_workers=jobs) as executor:
        futures = {executor.submit(process_pkg, pkg): pkg for pkg in pkg_list}
        for i, future in enumerate(as_completed(futures), 1):
            name, status = future.result()
            if status != "OK":
                print(f"  [{i}/{len(pkg_list)}] {name}: {status}")
            else:
                print(f"  [{i}/{len(pkg_list)}] {name}: OK")
            results.append((name, status))

    failed = [(n, s) for n, s in results if s != "OK"]
    if failed:
        print(f"\n⚠ {len(failed)} packages failed:")
        for n, s in failed:
            print(f"  - {n}: {s}")

    return results


def generate_pkg_config_wrappers(sysroot):
    """Fix .pc files for cross-compilation sysroot."""
    pkgconfig_dir = sysroot / "mingw64" / "lib" / "pkgconfig"
    if not pkgconfig_dir.exists():
        print(f"  pkgconfig dir not found at {pkgconfig_dir}")
        return

    print("\nFixing pkg-config files ...")
    for pc_file in pkgconfig_dir.glob("*.pc"):
        content = pc_file.read_text()
        # Replace absolute MSYS2 paths with sysroot-relative ones
        new_content = content.replace("prefix=/mingw64", f"prefix={sysroot}/mingw64")
        new_content = new_content.replace("prefix=/ucrt64", f"prefix={sysroot}/ucrt64")
        if new_content != content:
            pc_file.write_text(new_content)
            print(f"  Fixed: {pc_file.name}")


def main():
    parser = argparse.ArgumentParser(
        description="Set up MSYS2 Windows GTK sysroot for cross-compilation")
    parser.add_argument("--sysroot", default="/home/admin/Devel/PCL-GTK/build/mingw-sysroot",
                        help="Target sysroot directory")
    parser.add_argument("--cache", default=None,
                        help="Package cache directory")
    parser.add_argument("--jobs", type=int, default=8,
                        help="Parallel download jobs")
    args = parser.parse_args()

    sysroot = Path(args.sysroot)
    cache_dir = Path(args.cache) if args.cache else Path(args.sysroot) / ".cache"
    os.makedirs(cache_dir, exist_ok=True)

    # 1. Fetch package database
    packages = fetch_package_db(DB_URL, cache_dir)

    if not packages:
        print("ERROR: Could not fetch or parse package database", file=sys.stderr)
        sys.exit(1)

    # 2. Resolve dependencies
    all_names = TOP_PACKAGES + EXTRA_PACKAGES
    print(f"\nResolving dependencies for {len(all_names)} top-level packages ...")
    resolved = resolve_dependencies(packages, all_names)
    print(f"  Total packages to download: {len(resolved)}")

    # 3. Download and extract
    results = download_and_extract(resolved, sysroot, cache_dir,
                                    MSYS2_MIRROR, args.jobs)

    # 4. Fix pkg-config files
    generate_pkg_config_wrappers(sysroot)

    # Summary
    print(f"\n{'='*60}")
    print(f"Sysroot set up at: {sysroot}")
    print(f"MinGW prefix:    {sysroot}/mingw64")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
