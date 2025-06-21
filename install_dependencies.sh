#!/bin/bash

# ===========================================================================
# ShmNFrame 项目依赖安装脚本
# 用于自动检测并安装 krb5 和 tirpc 依赖包
# ===========================================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 显示帮助信息
show_help() {
    echo "依赖库安装脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  --root              使用root权限安装到系统目录"
    echo "  --user              安装到用户目录 (~/.local)"
    echo "  --prefix=PATH       指定安装前缀路径"
    echo "  --dry-run           只显示将要执行的操作，不实际安装"
    echo "  --help              显示此帮助信息"
    echo ""
    echo "环境变量:"
    echo "  INSTALL_PREFIX      安装前缀路径，默认为 ~/.local"
    echo ""
    echo "示例:"
    echo "  $0 --root           # 使用sudo安装到系统目录"
    echo "  $0 --user           # 安装到 ~/.local 目录"
    echo "  $0 --prefix=/opt/local # 安装到指定目录"
    echo "  $0 --dry-run        # 查看将要执行的操作"
}

# 检测操作系统
detect_os() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        OS_ID="${ID,,}"  # 转为小写
        OS_VERSION_ID="$VERSION_ID"
    elif [[ -f /etc/redhat-release ]]; then
        if grep -q "CentOS" /etc/redhat-release; then
            OS_ID="centos"
        elif grep -q "Red Hat" /etc/redhat-release; then
            OS_ID="rhel"
        fi
        OS_VERSION_ID=$(grep -oE '[0-9]+' /etc/redhat-release | head -1)
    elif [[ -f /etc/lsb-release ]]; then
        . /etc/lsb-release
        if [[ "$DISTRIB_ID" == "Ubuntu" ]]; then
            OS_ID="ubuntu"
            OS_VERSION_ID="$DISTRIB_RELEASE"
        fi
    else
        OS_ID="unknown"
        OS_VERSION_ID=""
    fi
    
    log_info "检测到操作系统: $OS_ID $OS_VERSION_ID"
}

# 检查root权限
check_root_permission() {
    if sudo -n true 2>/dev/null; then
        log_success "检测到sudo权限"
        return 0
    else
        log_warning "未检测到sudo权限"
        return 1
    fi
}

# 检查包是否已安装（系统级）
check_system_package() {
    local package=$1
    
    case "$OS_ID" in
        centos|rhel)
            rpm -q "$package" &>/dev/null
            ;;
        ubuntu|debian)
            dpkg -l "$package" &>/dev/null 2>&1
            ;;
        *)
            return 1
            ;;
    esac
}

# 检查库文件是否存在
check_library_exists() {
    local lib_name=$1
    local include_paths=("$@")
    
    # 检查包含文件
    for path in "${include_paths[@]:1}"; do
        if [[ -f "$path" ]]; then
            log_info "找到头文件: $path"
            return 0
        fi
    done
    
    # 检查库文件
    if ldconfig -p | grep -q "$lib_name"; then
        log_info "找到库文件: $lib_name"
        return 0
    fi
    
    return 1
}

# 系统包安装
install_system_package() {
    local packages=("$@")
    
    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "[DRY-RUN] 将要安装系统包: ${packages[*]}"
        return 0
    fi
    
    case "$OS_ID" in
        centos|rhel)
            log_info "使用yum安装: ${packages[*]}"
            sudo yum install -y "${packages[@]}"
            ;;
        ubuntu|debian)
            log_info "更新包列表..."
            sudo apt-get update
            log_info "使用apt安装: ${packages[*]}"
            sudo apt-get install -y "${packages[@]}"
            ;;
        *)
            log_error "不支持的操作系统: $OS_ID"
            return 1
            ;;
    esac
}

# 从源码编译安装
compile_from_source() {
    local package_name=$1
    local source_url=$2
    local config_args=$3
    
    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "[DRY-RUN] 将要从源码编译: $package_name"
        log_info "[DRY-RUN] 源码URL: $source_url"
        log_info "[DRY-RUN] 配置参数: $config_args"
        return 0
    fi
    
    local temp_dir=$(mktemp -d)
    local original_dir=$(pwd)
    
    log_info "开始编译 $package_name..."
    log_info "临时目录: $temp_dir"
    
    cd "$temp_dir" || {
        log_error "无法进入临时目录"
        return 1
    }
    
    # 下载源码
    log_info "下载源码..."
    if ! wget "$source_url" -O source.tar.gz; then
        log_error "下载失败: $source_url"
        cd "$original_dir"
        rm -rf "$temp_dir"
        return 1
    fi
    
    # 解压
    log_info "解压源码..."
    tar -xzf source.tar.gz
    
    # 进入源码目录（假设解压后只有一个目录）
    local src_dir=$(find . -maxdepth 1 -type d ! -name "." | head -1)
    cd "$src_dir" || {
        log_error "无法找到源码目录"
        cd "$original_dir"
        rm -rf "$temp_dir"
        return 1
    }
    
    # 配置
    log_info "配置编译选项..."
    if [[ -f configure ]]; then
        ./configure $config_args
    elif [[ -f CMakeLists.txt ]]; then
        mkdir build && cd build
        cmake .. $config_args
    else
        log_error "无法识别的构建系统"
        cd "$original_dir"
        rm -rf "$temp_dir"
        return 1
    fi
    
    # 编译
    log_info "开始编译..."
    make -j$(nproc)
    
    # 安装
    log_info "安装..."
    make install
    
    cd "$original_dir"
    rm -rf "$temp_dir"
    log_success "$package_name 编译安装完成"
}

# 安装krb5
install_krb5() {
    log_info "=== 安装 krb5 ==="
    
    # 检查是否已安装
    if check_library_exists "krb5" \
        "/usr/include/krb5.h" \
        "/usr/local/include/krb5.h" \
        "$INSTALL_PREFIX/include/krb5.h"; then
        log_success "krb5 已安装"
        return 0
    fi
    
    if [[ "$INSTALL_MODE" == "root" ]]; then
        # 系统安装
        case "$OS_ID" in
            centos|rhel)
                install_system_package krb5-devel
                ;;
            ubuntu|debian)
                install_system_package libkrb5-dev
                ;;
            *)
                log_error "不支持的操作系统: $OS_ID"
                return 1
                ;;
        esac
    else
        # 用户安装
        log_info "从源码编译安装krb5到用户目录..."
        local krb5_url="https://kerberos.org/dist/krb5/1.20/krb5-1.20.1.tar.gz"
        local config_args="--prefix=$INSTALL_PREFIX --disable-shared --enable-static"
        
        compile_from_source "krb5" "$krb5_url" "$config_args"
    fi
}

# 安装tirpc
install_tirpc() {
    log_info "=== 安装 tirpc ==="
    
    # 检查是否已安装
    if check_library_exists "tirpc" \
        "/usr/include/tirpc/rpc/rpc.h" \
        "/usr/local/include/tirpc/rpc/rpc.h" \
        "$INSTALL_PREFIX/include/tirpc/rpc/rpc.h"; then
        log_success "tirpc 已安装"
        return 0
    fi
    
    if [[ "$INSTALL_MODE" == "root" ]]; then
        # 系统安装
        case "$OS_ID" in
            centos|rhel)
                install_system_package libtirpc-devel
                ;;
            ubuntu|debian)
                install_system_package libtirpc-dev
                ;;
            *)
                log_error "不支持的操作系统: $OS_ID"
                return 1
                ;;
        esac
    else
        # 用户安装
        log_info "从源码编译安装tirpc到用户目录..."
        local tirpc_url="https://downloads.sourceforge.net/libtirpc/libtirpc-1.3.3.tar.bz2"
        local config_args="--prefix=$INSTALL_PREFIX --disable-shared --enable-static"
        
        compile_from_source "tirpc" "$tirpc_url" "$config_args"
    fi
}

# 主函数
main() {
    # 默认参数
    INSTALL_MODE=""
    INSTALL_PREFIX="${INSTALL_PREFIX:-$HOME/.local}"
    DRY_RUN="false"
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            --root)
                INSTALL_MODE="root"
                shift
                ;;
            --user)
                INSTALL_MODE="user"
                shift
                ;;
            --prefix=*)
                INSTALL_PREFIX="${1#*=}"
                INSTALL_MODE="user"
                shift
                ;;
            --dry-run)
                DRY_RUN="true"
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            *)
                log_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 如果没有指定安装模式，自动检测
    if [[ -z "$INSTALL_MODE" ]]; then
        if check_root_permission; then
            INSTALL_MODE="root"
            log_info "自动选择root模式安装"
        else
            INSTALL_MODE="user"
            log_info "自动选择用户模式安装到 $INSTALL_PREFIX"
        fi
    fi
    
    # 验证安装模式
    if [[ "$INSTALL_MODE" == "root" ]] && ! check_root_permission; then
        log_error "选择了root模式但没有sudo权限"
        log_info "请使用 --user 选项或获取sudo权限"
        exit 1
    fi
    
    log_info "=== 依赖库安装开始 ==="
    log_info "安装模式: $INSTALL_MODE"
    log_info "安装前缀: $INSTALL_PREFIX"
    log_info "是否预演: $DRY_RUN"
    
    # 检测操作系统
    detect_os
    
    # 创建安装目录
    if [[ "$INSTALL_MODE" == "user" && "$DRY_RUN" == "false" ]]; then
        mkdir -p "$INSTALL_PREFIX"/{bin,lib,include}
        log_info "创建用户安装目录: $INSTALL_PREFIX"
    fi
    
    # 安装依赖
    install_krb5
    install_tirpc
    
    log_success "=== 依赖库安装完成 ==="
    
    if [[ "$INSTALL_MODE" == "user" ]]; then
        log_info ""
        log_info "用户安装完成后，请在CMake配置时添加以下参数:"
        log_info "  -DCMAKE_PREFIX_PATH=$INSTALL_PREFIX"
        log_info ""
        log_info "或者设置环境变量:"
        log_info "  export PKG_CONFIG_PATH=$INSTALL_PREFIX/lib/pkgconfig:\$PKG_CONFIG_PATH"
        log_info "  export LD_LIBRARY_PATH=$INSTALL_PREFIX/lib:\$LD_LIBRARY_PATH"
        log_info "  export PATH=$INSTALL_PREFIX/bin:\$PATH"
    fi
}

# 运行主函数
main "$@" 