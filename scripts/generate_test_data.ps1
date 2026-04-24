$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$testDataRoot = Join-Path $repoRoot "test_data"

New-Item -ItemType Directory -Force -Path (Join-Path $testDataRoot "raster") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $testDataRoot "vector") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $testDataRoot "models") | Out-Null

Write-Host "已准备测试夹具目录：$testDataRoot"
Write-Host "下一步请扩展此脚本，调用仓库工具生成栅格、矢量和 ONNX 测试夹具。"
