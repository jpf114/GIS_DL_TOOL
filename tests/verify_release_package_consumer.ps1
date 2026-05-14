param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot,
    [Parameter(Mandatory = $true)]
    [string]$BuildDir
)

$ErrorActionPreference = "Stop"

$installRoot = Join-Path $RepoRoot "install"
$consumerSourceDir = Join-Path $RepoRoot "tests/package_consumer"
$consumerBuildDir = Join-Path $BuildDir "release_package_consumer"
$vcpkgRoot = $env:VCPKG_ROOT
$toolchainFile = Join-Path $vcpkgRoot "scripts/buildsystems/vcpkg.cmake"
$overlayDir = Join-Path $RepoRoot "vcpkg-overlay"
$windowsSdkDir = "C:\Program Files (x86)\Windows Kits\10\"

if (Test-Path $windowsSdkDir) {
    $env:WindowsSDKVersion = "10.0.26100.0\"
    $env:WindowsSdkDir = $windowsSdkDir
}

& cmake --install $BuildDir --config Release
if ($LASTEXITCODE -ne 0) {
    throw "Release install failed with exit code $LASTEXITCODE"
}

if (Test-Path $consumerBuildDir) {
    Remove-Item -LiteralPath $consumerBuildDir -Recurse -Force
}

$configureArgs = @(
    "-S", $consumerSourceDir,
    "-B", $consumerBuildDir,
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-T", "host=x64",
    "-DCMAKE_SYSTEM_VERSION=10.0.19045",
    "-DCMAKE_PREFIX_PATH=$installRoot",
    "-DCMAKE_GENERATOR_INSTANCE=D:/Program Files/Microsoft Visual Studio/2022/Professional",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile",
    "-DVCPKG_MANIFEST_MODE=OFF"
)

if (Test-Path $overlayDir) {
    & cmake -E env "VCPKG_OVERLAY_PORTS=$overlayDir" cmake @configureArgs
} else {
    & cmake @configureArgs
}
if ($LASTEXITCODE -ne 0) {
    throw "Package consumer configure failed with exit code $LASTEXITCODE"
}

& cmake --build $consumerBuildDir --config Release
if ($LASTEXITCODE -ne 0) {
    throw "Package consumer build failed with exit code $LASTEXITCODE"
}

$consumerExe = Join-Path $consumerBuildDir "Release/consumer_shared.exe"
if (-not (Test-Path $consumerExe)) {
    $consumerExe = Join-Path $consumerBuildDir "consumer_shared.exe"
}
if (-not (Test-Path $consumerExe)) {
    throw "Unable to locate consumer_shared executable"
}

& cmake -E env "PATH=$installRoot/bin;$env:PATH" $consumerExe
if ($LASTEXITCODE -ne 0) {
    throw "Package consumer run failed with exit code $LASTEXITCODE"
}

Write-Host "Release package consumer verification passed"
