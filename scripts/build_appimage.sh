#!/bin/bash
# ============================================================================
# PCL-CGRE AppImage 构建脚本
#
# 用法:
#   ./scripts/build_appimage.sh              # 构建并生成 AppImage
#   ./scripts/build_appimage.sh --no-docker  # 在 VPS 上原生运行（需先安装依赖）
#
# 输出: build/dist/PCL-CGRE-<version>-x86_64.AppImage
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
VERSION="${VERSION:-0.1.0}"
ARCH="${ARCH:-x86_64}"
BUILD_DIR="${PROJECT_DIR}/build/appimage-build"
APPDIR="${BUILD_DIR}/AppDir"
DIST_DIR="${PROJECT_DIR}/build/dist"

# ── 工具函数 ────────────────────────────────────────────────────────────────

section() { echo -e "\n\033[1;36m==>\033[0m \033[1m$*\033[0m"; }
info()   { echo -e "  \033[34m•\033[0m $*"; }
warn()   { echo -e "  \033[33m⚠\033[0m $*"; }
err()    { echo -e "  \033[31m✗\033[0m $*"; exit 1; }

# 检查命令是否存在
need_cmd() {
    command -v "$1" >/dev/null 2>&1 || err "缺少命令: $1 — 请先安装"
}

# 复制共享库及其依赖（排除系统核心库）
copy_libs() {
    local binary="$1"
    local dest="${2:-${APPDIR}/usr/lib}"
    local exclude_pattern="${3:-}"

    mkdir -p "${dest}"

    # 不应打包的系统库（这些由目标系统提供）
    local system_libs="linux-vdso linux-gate ld-linux libc.so libc- libm.so libm- libpthread libdl.so libdl- librt.so librt- libresolv libnss_ libutil.so libutil- libcrypt.so libcrypt-"

    # 收集所有需要的库
    local all_libs
    all_libs=$(ldd "${binary}" 2>/dev/null | grep '=>' | awk '{print $3}' | grep -v '^$' | sort -u)

    # 递归收集 .so 依赖
    local processed=""
    local queue="${all_libs}"
    local bundled=()

    while [ -n "${queue}" ]; do
        local current="${queue}"
        queue=""

        for lib in ${current}; do
            [ -z "${lib}" ] && continue
            [[ " ${processed} " =~ " ${lib} " ]] && continue
            processed="${processed} ${lib}"

            # 跳过系统库
            local libname
            libname=$(basename "${lib}" | sed 's/\.[0-9.]*$//')
            local skip=false
            for syslib in ${system_libs}; do
                [[ "${libname}" =~ ^${syslib} ]] && skip=true && break
            done
            ${skip} && continue

            bundled+=("${lib}")

            # 递归找这个库的依赖
            local deps
            deps=$(ldd "${lib}" 2>/dev/null | grep '=>' | awk '{print $3}' | grep -v '^$' || true)
            for dep in ${deps}; do
                [ -z "${dep}" ] && continue
                [[ " ${processed} " =~ " ${dep} " ]] && continue
                queue="${queue} ${dep}"
            done
        done
    done

    # 复制库文件（保留符号链接）
    info "复制 ${#bundled[@]} 个共享库..."
    for lib in "${bundled[@]}"; do
        local base
        base=$(basename "${lib}")

        # 如果是符号链接，复制链接目标并重建链接
        if [ -L "${lib}" ]; then
            local target
            target=$(readlink -f "${lib}")
            cp -n "${target}" "${dest}/" 2>/dev/null || true
            # 重建符号链接
            local linkname="${dest}/${base}"
            [ -L "${linkname}" ] && rm -f "${linkname}"
            ln -sf "$(basename "${target}")" "${linkname}" 2>/dev/null || cp "${target}" "${linkname}"
        else
            cp -n "${lib}" "${dest}/" 2>/dev/null || true
        fi
    done

    echo "${#bundled[@]}"
}

# ── 主流程 ──────────────────────────────────────────────────────────────────

section "PCL-CGRE AppImage 构建器"
info "项目目录: ${PROJECT_DIR}"
info "版本: ${VERSION}"

# 1. 检查构建依赖
section "检查依赖..."
need_cmd cmake
need_cmd g++
need_cmd pkg-config
need_cmd python3
need_cmd file
need_cmd strip
need_cmd patchelf

# 检查库依赖
pkg-config --exists gtk4           || err "gtk4 未安装"
pkg-config --exists libadwaita-1   || err "libadwaita-1 未安装"
pkg-config --exists fontconfig     || err "fontconfig 未安装"
pkg-config --exists libsoup-3.0    || err "libsoup-3.0 未安装"
pkg-config --exists json-glib-1.0  || err "json-glib-1.0 未安装"

need_cmd blueprint-compiler
need_cmd glib-compile-resources
need_cmd gdk-pixbuf-query-loaders
need_cmd glib-compile-schemas

info "所有构建依赖已满足"

# 2. 构建项目
section "编译项目 (Release)..."
cmake -B "${BUILD_DIR}/cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    "${PROJECT_DIR}"

cmake --build "${BUILD_DIR}/cmake" --parallel "$(nproc)"

info "编译完成"

# 3. 设置 AppDir 结构
section "创建 AppDir 结构..."

rm -rf "${APPDIR}"
mkdir -p "${APPDIR}/usr/bin"
mkdir -p "${APPDIR}/usr/lib"
mkdir -p "${APPDIR}/usr/share/applications"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/256x256/apps"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/scalable/apps"
mkdir -p "${APPDIR}/usr/share/glib-2.0/schemas"
mkdir -p "${APPDIR}/usr/share/libadwaita-1"
mkdir -p "${APPDIR}/usr/lib/gtk-4.0"
mkdir -p "${APPDIR}/usr/lib/gdk-pixbuf-2.0"
# fontconfig: 不打包，使用宿主配置

# 复制二进制和资源
info "复制二进制文件..."
cp "${BUILD_DIR}/cmake/pcl-cgre"           "${APPDIR}/usr/bin/"
cp "${BUILD_DIR}/cmake/pcl-cgre.gresource" "${APPDIR}/usr/bin/"

info "复制资源文件..."
cp -r "${BUILD_DIR}/cmake/resources"       "${APPDIR}/usr/bin/"

# 裁切字体：保留 Regular/Medium/Semibold/Bold（已子集化到 GB 2312）
info "裁切字体（保留 Regular/Medium/Semibold/Bold 字重）..."
FONTS_DIR="${APPDIR}/usr/bin/resources/fonts"
# 先删掉所有 SC 字体（源目录可能有多余字重）
rm -f "${FONTS_DIR}"/HarmonyOS_SansSC_*.ttf
# 从源码目录重新复制需要的 SC 字重
for weight in Regular Medium Semibold Bold; do
    src="${PROJECT_DIR}/resources/fonts/HarmonyOS_SansSC_${weight}.ttf"
    if [ -f "${src}" ]; then
        cp "${src}" "${FONTS_DIR}/"
    fi
done
# 拉丁字体全保留（每个仅 153KB）
info "字体已裁切（$(du -sh "${FONTS_DIR}" | cut -f1)）"
cp -r "${BUILD_DIR}/cmake/data"            "${APPDIR}/usr/bin/"

# 4. 复制共享库
section "收集运行时共享库..."

lib_count=$(copy_libs "${APPDIR}/usr/bin/pcl-cgre")

# 额外检查 libadwaita 是否被正确复制（它可能通过 dlopen 加载）
for extra_lib in \
    /usr/lib/libadwaita-1.so.0 \
    /usr/lib/libgtk-4.so.1 \
    /usr/lib/libgdk_pixbuf-2.0.so.0 \
    /usr/lib/libsoup-3.0.so.0 \
    /usr/lib/libjson-glib-1.0.so.0 \
    /usr/lib/libfontconfig.so.1 \
    /usr/lib/libepoxy.so.0; do
    if [ -f "${extra_lib}" ]; then
        cp -n "${extra_lib}" "${APPDIR}/usr/lib/" 2>/dev/null || true
    fi
done

info "已复制 ${lib_count} 个库到 AppDir/usr/lib/"

# 5. GTK4 运行时设置
section "设置 GTK4 运行时..."

# 5a. GTK4 模块 (immodules, media, printbackends)
GTK4_MODULES="/usr/lib/gtk-4.0/4.0.0"
if [ -d "${GTK4_MODULES}" ]; then
    mkdir -p "${APPDIR}/usr/lib/gtk-4.0/4.0.0"
    cp -r "${GTK4_MODULES}/"* "${APPDIR}/usr/lib/gtk-4.0/4.0.0/" 2>/dev/null || true
    # 修正模块中库的 RPATH
    find "${APPDIR}/usr/lib/gtk-4.0/4.0.0" -name '*.so' | while read mod; do
        patchelf --set-rpath '$ORIGIN/../../..' "${mod}" 2>/dev/null || true
    done
    info "GTK4 模块已复制"
else
    warn "未找到 GTK4 模块目录: ${GTK4_MODULES}"
fi

# 5b. GDK Pixbuf 加载器
PIXBUF_LOADERS="/usr/lib/gdk-pixbuf-2.0/2.10.0"
if [ -d "${PIXBUF_LOADERS}" ]; then
    mkdir -p "${APPDIR}/usr/lib/gdk-pixbuf-2.0/2.10.0"
    cp -r "${PIXBUF_LOADERS}/"* "${APPDIR}/usr/lib/gdk-pixbuf-2.0/2.10.0/" 2>/dev/null || true
    # 修正 RPATH
    find "${APPDIR}/usr/lib/gdk-pixbuf-2.0/2.10.0" -name '*.so' | while read mod; do
        patchelf --set-rpath '$ORIGIN/../../..' "${mod}" 2>/dev/null || true
    done
    info "Pixbuf 加载器已复制"
fi

# 5c. 生成 pixbuf loaders.cache
info "生成 pixbuf loaders.cache..."
gdk-pixbuf-query-loaders \
    "${APPDIR}/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders/"*.so \
    > "${APPDIR}/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache" 2>/dev/null || true

# 5d. Adwaita 图标 — 仅保留 GTK4 控件内部引用的 actions + ui
info "复制 Adwaita 图标（仅 actions + ui）..."
ADWAITA_SYMBOLIC="/usr/share/icons/Adwaita/symbolic"
if [ -d "${ADWAITA_SYMBOLIC}" ]; then
    for sub in actions ui; do
        if [ -d "${ADWAITA_SYMBOLIC}/${sub}" ]; then
            mkdir -p "${APPDIR}/usr/share/icons/Adwaita/symbolic/${sub}"
            cp -r "${ADWAITA_SYMBOLIC}/${sub}/"* "${APPDIR}/usr/share/icons/Adwaita/symbolic/${sub}/"
        fi
    done
    # 必须有 index.theme，否则 GTK 不认这个图标主题
    cp "${ADWAITA_SYMBOLIC}/../index.theme" "${APPDIR}/usr/share/icons/Adwaita/" 2>/dev/null || true
    info "Adwaita 图标已精简（$(du -sh "${APPDIR}/usr/share/icons/Adwaita" | cut -f1)）"
else
    warn "未找到 Adwaita 图标主题"
fi

# 5e. libadwaita 样式表
info "复制 libadwaita 样式..."
LIBADWAITA_SHARE="/usr/share/libadwaita-1"
if [ -d "${LIBADWAITA_SHARE}" ]; then
    cp -r "${LIBADWAITA_SHARE}/"* "${APPDIR}/usr/share/libadwaita-1/" 2>/dev/null || true
fi

# 5f. GLib GSettings 模式
info "编译 GSettings 模式..."
GLIB_SCHEMAS="/usr/share/glib-2.0/schemas"
if [ -d "${GLIB_SCHEMAS}" ]; then
    cp "${GLIB_SCHEMAS}/org.gtk.gtk4.Settings.gschema.xml" \
       "${APPDIR}/usr/share/glib-2.0/schemas/" 2>/dev/null || true
    cp "${GLIB_SCHEMAS}/org.gnome.desktop.interface.gschema.xml" \
       "${APPDIR}/usr/share/glib-2.0/schemas/" 2>/dev/null || true
    cp "${GLIB_SCHEMAS}/org.gnome.desktop.thumbnail-cache.gschema.xml" \
       "${APPDIR}/usr/share/glib-2.0/schemas/" 2>/dev/null || true
fi
glib-compile-schemas "${APPDIR}/usr/share/glib-2.0/schemas/" 2>/dev/null || true

# 5g. Fontconfig — 不打包配置和系统字体
#     让 AppImage 使用宿主机的 /etc/fonts 和系统字体。
#     应用自带的 HarmonyOS Sans 字体由 FontHelper.cpp 通过
#     FcConfigAppFontAddFile() 注册，无需 fontconfig 配置干预。
info "fontconfig: 使用宿主系统配置（不打包字体）"

# 6. 修正 RPATH
section "修正二进制 RPATH..."
patchelf --set-rpath '$ORIGIN/../lib' "${APPDIR}/usr/bin/pcl-cgre"
info "RPATH 已设置: \$ORIGIN/../lib"

# 7. 创建 AppRun
section "创建 AppRun 启动脚本..."
cat > "${APPDIR}/AppRun" << 'APPRUN'
#!/bin/bash
# PCL-CGRE AppRun — 设置运行时环境并启动应用

HERE="$(dirname "$(readlink -f "$0")")"
APPDIR="${HERE}"

# ── 库路径 ────────────────────────────────────────────────────────────────
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH:-}"

# ── GTK4 运行时 ────────────────────────────────────────────────────────────
# GTK 模块路径（immodules, media, printbackends）
export GTK_PATH="${APPDIR}/usr/lib/gtk-4.0"
export GTK4_PATH="${APPDIR}/usr/lib/gtk-4.0"

# GDK Pixbuf 加载器
export GDK_PIXBUF_MODULE_FILE="${APPDIR}/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
export GDK_PIXBUF_MODULEDIR="${APPDIR}/usr/lib/gdk-pixbuf-2.0/2.10.0/loaders"

# ── GLib ──────────────────────────────────────────────────────────────────
export GSETTINGS_SCHEMA_DIR="${APPDIR}/usr/share/glib-2.0/schemas"

# ── XDG 数据路径（图标、主题、样式）───────────────────────────────────────
export XDG_DATA_DIRS="${APPDIR}/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

# ── Fontconfig ─────────────────────────────────────────────────────────────
# 使用宿主系统配置（/etc/fonts），不覆盖
# HarmonyOS Sans 字体由应用代码自行注册

# ── GDK 后端 ───────────────────────────────────────────────────────────────
# 优先 Wayland，退回 X11
if [ -z "${GDK_BACKEND:-}" ]; then
    if [ -n "${WAYLAND_DISPLAY:-}" ]; then
        export GDK_BACKEND=wayland
    elif [ -n "${DISPLAY:-}" ]; then
        export GDK_BACKEND=x11
    fi
fi

# ── 启动应用 ──────────────────────────────────────────────────────────────
exec "${APPDIR}/usr/bin/pcl-cgre" "$@"
APPRUN
chmod +x "${APPDIR}/AppRun"
info "AppRun 已创建"

# 8. 创建 .desktop 文件
section "创建 .desktop 文件..."
cat > "${APPDIR}/usr/share/applications/pcl-cgre.desktop" << DESKTOP
[Desktop Entry]
Name=PCL-CGRE
Name[zh_CN]=Plain Craft Launcher 社区移植版
Comment=A GTK4 + libadwaita Minecraft launcher
Comment[zh_CN]=基于 GTK4 + libadwaita 的 Minecraft 启动器
Exec=pcl-cgre
Icon=pcl-cgre
Type=Application
Categories=Game;
Terminal=false
StartupNotify=true
DESKTOP
ln -sf usr/share/applications/pcl-cgre.desktop "${APPDIR}/pcl-cgre.desktop"

# 9. 创建图标
section "创建图标..."
# 从 SVG 图标生成 PNG（如果项目有的话）
ICON_SVG="${PROJECT_DIR}/resources/icons/pcl-cgre/scalable/apps/pcl-cgre.svg"
if [ -f "${ICON_SVG}" ] && command -v rsvg-convert >/dev/null 2>&1; then
    rsvg-convert -w 256 -h 256 "${ICON_SVG}" -o "${APPDIR}/usr/share/icons/hicolor/256x256/apps/pcl-cgre.png"
    info "图标已从 SVG 生成"
elif [ -f "${ICON_SVG}" ]; then
    cp "${ICON_SVG}" "${APPDIR}/usr/share/icons/hicolor/scalable/apps/pcl-cgre.svg"
    info "SVG 图标已复制（无 rsvg-convert，跳过 PNG 生成）"
else
    warn "未找到项目图标 — 使用占位图标"
    # 创建一个最小的 1x1 透明 PNG 作为占位
    python3 -c "
import struct, zlib
def create_png(path):
    sig = b'\\x89PNG\\r\\n\\x1a\\n'
    def chunk(ctype, data):
        c = ctype + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)
    ihdr = chunk(b'IHDR', struct.pack('>IIBBBBB', 256, 256, 8, 2, 0, 0, 0))
    raw = b''
    for y in range(256):
        raw += b'\\x00' + b'\\x00\\x00\\x00\\x00' * 256
    idat = chunk(b'IDAT', zlib.compress(raw))
    iend = chunk(b'IEND', b'')
    with open(path, 'wb') as f:
        f.write(sig + ihdr + idat + iend)
create_png('${APPDIR}/usr/share/icons/hicolor/256x256/apps/pcl-cgre.png')
"
fi
ln -sf usr/share/icons/hicolor/256x256/apps/pcl-cgre.png "${APPDIR}/pcl-cgre.png" 2>/dev/null || true
ln -sf usr/share/icons/hicolor/256x256/apps/pcl-cgre.png "${APPDIR}/.DirIcon" 2>/dev/null || true

# 10. 清理符号表（减小体积）
section "剥离调试符号..."
find "${APPDIR}/usr/lib" -name '*.so*' -type f -exec strip --strip-unneeded {} \; 2>/dev/null || true
strip --strip-unneeded "${APPDIR}/usr/bin/pcl-cgre" 2>/dev/null || true
info "符号已剥离"

# 11. 获取 appimagetool
section "下载 appimagetool..."
APPIMAGETOOL="${BUILD_DIR}/appimagetool-${ARCH}.AppImage"
APPIMAGETOOL_URL="https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-${ARCH}.AppImage"

if [ ! -f "${APPIMAGETOOL}" ]; then
    wget -q --show-progress -O "${APPIMAGETOOL}" "${APPIMAGETOOL_URL}" \
        || curl -SL --progress-bar -o "${APPIMAGETOOL}" "${APPIMAGETOOL_URL}"
    chmod +x "${APPIMAGETOOL}"
    info "appimagetool 已下载"
else
    info "appimagetool 已存在，跳过下载"
fi

# 12. 打包 AppImage
section "生成 AppImage..."
mkdir -p "${DIST_DIR}"

export ARCH="${ARCH}"
export VERSION="${VERSION}"

APPIMAGE_NAME="PCL-CGRE-${VERSION}-${ARCH}.AppImage"

"${APPIMAGETOOL}" "${APPDIR}" "${DIST_DIR}/${APPIMAGE_NAME}" \
    --no-appstream 2>&1 || {
        warn "appimagetool 直接运行失败，尝试提取模式..."
        "${APPIMAGETOOL}" --appimage-extract-and-run "${APPDIR}" "${DIST_DIR}/${APPIMAGE_NAME}" --no-appstream
    }

chmod +x "${DIST_DIR}/${APPIMAGE_NAME}"

# 13. 验证
section "构建完成"
echo ""
echo "  📦 AppImage: ${DIST_DIR}/${APPIMAGE_NAME}"
echo "  📏 大小:     $(du -h "${DIST_DIR}/${APPIMAGE_NAME}" | cut -f1)"
echo ""
echo "  可以在任何 glibc ≥ 2.35 的 Linux 系统上运行："
echo "    ./${APPIMAGE_NAME}"
echo ""
echo "  或者解压后调试："
echo "    ./${APPIMAGE_NAME} --appimage-extract"
echo ""
