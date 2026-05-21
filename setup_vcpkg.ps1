param(
    [string]$VcpkgTag = "256acc64012b23a13041d8705805e1f23b43a024"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$VcpkgDir = Join-Path $ProjectRoot "vcpkg"

Write-Host "=== GIS AI TOOLKIT vcpkg 初始化脚本 ===" -ForegroundColor Cyan
Write-Host "项目目录: $ProjectRoot"
Write-Host "vcpkg 目标目录: $VcpkgDir"

if (Test-Path "$VcpkgDir\vcpkg.exe") {
    Write-Host ""
    Write-Host "vcpkg 已存在于 $VcpkgDir" -ForegroundColor Green
    $version = & "$VcpkgDir\vcpkg.exe" version 2>&1 | Select-Object -First 1
    Write-Host "当前版本: $version"
    exit 0
}

if (Test-Path $VcpkgDir) {
    Write-Host ""
    Write-Host "vcpkg 目录已存在但未完成安装，正在清理..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $VcpkgDir
}

Write-Host ""
Write-Host "--- 步骤 1/3: 克隆 vcpkg ---" -ForegroundColor Yellow
& git clone https://github.com/microsoft/vcpkg.git $VcpkgDir
if ($LASTEXITCODE -ne 0) { throw "克隆 vcpkg 失败" }

Push-Location $VcpkgDir
& git checkout $VcpkgTag
Pop-Location

Write-Host ""
Write-Host "--- 步骤 2/3: 引导编译 vcpkg ---" -ForegroundColor Yellow
Push-Location $VcpkgDir
& .\bootstrap-vcpkg.bat -disableMetrics
Pop-Location
if ($LASTEXITCODE -ne 0) { throw "引导编译 vcpkg 失败" }

Write-Host ""
Write-Host "--- 步骤 3/3: 验证安装 ---" -ForegroundColor Yellow
$version = & "$VcpkgDir\vcpkg.exe" version 2>&1 | Select-Object -First 1
Write-Host "安装成功: $version" -ForegroundColor Green

Write-Host ""
Write-Host "=== 初始化完成 ===" -ForegroundColor Cyan
Write-Host "现在可以使用以下命令构建项目："
Write-Host "  cmake --preset=release" -ForegroundColor White
Write-Host "  cmake --build --preset=release" -ForegroundColor White
