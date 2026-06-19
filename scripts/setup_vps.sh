#!/bin/bash
# ============================================================================
# VPS 一键环境安装脚本 — PCL-CGRE AppImage 构建
#
# 在朋友的 VPS 上运行这一条命令即可配置好构建环境:
#   curl -O .../setup_vps.sh && bash setup_vps.sh
#
# 支持的发行版: Ubuntu 22.04+ / Debian 12+ / Arch Linux / Fedora 39+
# ============================================================================

set -euo pipefail

section() { echo -e "\n\033[1;36m==>\033[0m \033[1m$*\033[0m"; }
info()   { echo -e "  \033[34m•\033[0m $*"; }

# ── 检测发行版 ──────────────────────────────────────────────────────────────
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "${ID}"
    elif [ -f /etc/arch-release ]; then
        echo "arch"
    else
        echo "unknown"
    fi
}

DISTRO=$(detect_distro)

section "检测到发行版: ${DISTRO}"

case "${DISTRO}" in
    ubuntu|debian)
        section "安装依赖 (apt)..."
        sudo apt-get update
        sudo apt-get install -y --no-install-recommends \
            build-essential cmake pkg-config python3 \
            libgtk-4-dev libadwaita-1-dev libfontconfig-dev \
            libsoup-3.0-dev libjson-glib-dev \
            blueprint-compiler libglib2.0-dev-bin \
            patchelf file wget curl ca-certificates \
            librsvg2-bin gdk-pixbuf-query-loaders \
            adwaita-icon-theme-full fonts-dejavu-core
        ;;

    arch)
        section "安装依赖 (pacman)..."
        sudo pacman -Sy --noconfirm \
            base-devel cmake pkgconf python \
            gtk4 libadwaita fontconfig libsoup3 json-glib \
            blueprint-compiler glib2 \
            patchelf file wget curl \
            librsvg gdk-pixbuf2 \
            adwaita-icon-theme ttf-dejavu
        ;;

    fedora)
        section "安装依赖 (dnf)..."
        sudo dnf install -y \
            gcc-c++ cmake pkgconf python3 \
            gtk4-devel libadwaita-devel fontconfig-devel \
            libsoup3-devel json-glib-devel \
            blueprint-compiler glib2-devel \
            patchelf file wget curl \
            librsvg2-tools gdk-pixbuf2-devel \
            adwaita-icon-theme dejavu-sans-fonts
        ;;

    *)
        echo "未知发行版: ${DISTRO}"
        echo "请手动安装以下依赖后重新运行 build_appimage.sh:"
        echo "  - C++ 编译器 (gcc >= 13 or clang >= 17)"
        echo "  - CMake >= 3.16"
        echo "  - pkg-config"
        echo "  - GTK4 >= 4.0"
        echo "  - libadwaita >= 1.0"
        echo "  - fontconfig"
        echo "  - libsoup 3.0"
        echo "  - json-glib 1.0"
        echo "  - blueprint-compiler"
        echo "  - glib-compile-resources"
        echo "  - patchelf"
        echo "  - adwaita-icon-theme"
        exit 1
        ;;
esac

section "环境安装完成"
echo ""
echo "现在可以运行 AppImage 构建:"
echo "  ./scripts/build_appimage.sh"
echo ""
