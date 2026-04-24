$ErrorActionPreference = "Stop"

param(
    [Parameter(Mandatory = $true)]
    [string]$SourceModel
)

$repoRoot = Split-Path -Parent $PSScriptRoot
$targetDir = Join-Path $repoRoot "test_data/models"
$targetPath = Join-Path $targetDir "test_seg_model.onnx"

if (-not (Test-Path $SourceModel)) {
    Write-Error "未找到源模型文件：$SourceModel"
}

if ([System.IO.Path]::GetExtension($SourceModel).ToLowerInvariant() -ne ".onnx") {
    Write-Error "源文件不是 .onnx 模型：$SourceModel"
}

New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
Copy-Item -LiteralPath $SourceModel -Destination $targetPath -Force

Write-Host "已准备测试模型：$targetPath"
