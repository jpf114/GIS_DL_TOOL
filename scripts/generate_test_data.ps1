$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$testDataRoot = Join-Path $repoRoot "test_data"

New-Item -ItemType Directory -Force -Path (Join-Path $testDataRoot "raster") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $testDataRoot "vector") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $testDataRoot "models") | Out-Null

$toolCandidates = @(
    (Join-Path $repoRoot "build/dev-windows/bin/Debug/generate_test_data.exe"),
    (Join-Path $repoRoot "build/dev-windows/bin/Release/generate_test_data.exe"),
    (Join-Path $repoRoot "build/release-windows/bin/Release/generate_test_data.exe"),
    (Join-Path $repoRoot "build/release-windows/bin/generate_test_data.exe")
)

$toolPath = $toolCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $toolPath) {
    Write-Host "Prepared test fixture directories: $testDataRoot"
    Write-Host "generate_test_data.exe was not found."
    Write-Host "Build it first, for example:"
    Write-Host "  cmake --preset=dev-windows"
    Write-Host "  cmake --build --preset=dev --target generate_test_data"
    exit 1
}

Push-Location $repoRoot
try {
    & $toolPath
} finally {
    Pop-Location
}

$modelPath = Join-Path $testDataRoot "models/test_seg_model.onnx"
if (-not (Test-Path $modelPath)) {
    Write-Warning "Raster and vector fixtures were generated, but the ONNX test model is still missing: $modelPath"
    Write-Host "AI integration tests still require test_seg_model.onnx to be prepared separately."
} else {
    Write-Host "All test fixtures are ready: $testDataRoot"
}
