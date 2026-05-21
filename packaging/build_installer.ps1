param(
    [string]$BuildType = "Release",
    [string]$Generator = "NSIS",
    [switch]$SkipBuild,
    [switch]$SkipInstall,
    [switch]$StandaloneNSIS
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

Write-Host "=== GIS AI TOOLKIT 打包脚本 ===" -ForegroundColor Cyan
Write-Host "项目根目录: $ProjectRoot"
Write-Host "构建类型: $BuildType"
Write-Host "打包格式: $Generator"

if (-not $SkipBuild) {
    Write-Host ""
    Write-Host "--- 步骤 1/4: 配置 CMake ---" -ForegroundColor Yellow
    $ConfigureArgs = @(
        "-S", $ProjectRoot,
        "-B", "$ProjectRoot\build\release",
        "-G", "Visual Studio 17 2022",
        "-A", "x64",
        "-DGIS_AI_BUILD_GUI=ON",
        "-DGIS_AI_BUILD_TESTS=OFF",
        "-DGIS_AI_BUILD_EXAMPLES=OFF",
        "-DGIS_AI_BUILD_DOCS=OFF",
        "-DCMAKE_BUILD_TYPE=$BuildType"
    )
    & cmake @ConfigureArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake 配置失败" }

    Write-Host ""
    Write-Host "--- 步骤 2/4: 构建 Release ---" -ForegroundColor Yellow
    & cmake --build "$ProjectRoot\build\release" --config $BuildType --parallel
    if ($LASTEXITCODE -ne 0) { throw "构建失败" }
}

if (-not $SkipInstall) {
    Write-Host ""
    Write-Host "--- 步骤 3/4: 安装到 install 目录 ---" -ForegroundColor Yellow
    & cmake --install "$ProjectRoot\build\release" --config $BuildType
    if ($LASTEXITCODE -ne 0) { throw "安装失败" }
}

if ($StandaloneNSIS) {
    Write-Host ""
    Write-Host "--- 步骤 4/4: 使用独立 NSIS 脚本打包 ---" -ForegroundColor Yellow
    $NSISPath = "C:\Program Files (x86)\NSIS\makensis.exe"
    if (-not (Test-Path $NSISPath)) {
        $NSISPath = (Get-Command makensis -ErrorAction SilentlyContinue).Source
    }
    if (-not $NSISPath) {
        throw "未找到 NSIS (makensis.exe)，请安装 NSIS: https://nsis.sourceforge.io/Download"
    }
    Push-Location "$ProjectRoot\packaging\nsis"
    & $NSISPath installer.nsi
    Pop-Location
    if ($LASTEXITCODE -ne 0) { throw "NSIS 打包失败" }
    Write-Host ""
    Write-Host "安装包已生成: $ProjectRoot\packaging\nsis\GIS_AI_TOOLKIT-*-setup.exe" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "--- 步骤 4/4: 使用 CPack 打包 ---" -ForegroundColor Yellow
    Push-Location "$ProjectRoot\build\release"
    & cpack -G $Generator -C $BuildType
    Pop-Location
    if ($LASTEXITCODE -ne 0) { throw "CPack 打包失败" }
    Write-Host ""
    Write-Host "安装包已生成: $ProjectRoot\build\release\_CPack_Packages\" -ForegroundColor Green
}

Write-Host ""
Write-Host "=== 打包完成 ===" -ForegroundColor Cyan
