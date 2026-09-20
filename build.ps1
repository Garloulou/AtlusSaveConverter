# Build and launch script for AtlusSaveConverter

param(
    [switch]$Run = $false,
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

Write-Host "=============================================" -ForegroundColor Cyan
Write-Host " Building AtlusSaveConverter ($Config)" -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan

$buildDir = "build"
if (!(Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

$cmakeExe = "C:\Program Files\CMake\bin\cmake.exe"
if (!(Test-Path $cmakeExe)) {
    $cmakeExe = "cmake.exe"
}

Write-Host "`n[1/2] Configuring CMake..." -ForegroundColor Yellow
& $cmakeExe -S . -B $buildDir -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed." -ForegroundColor Red
    exit 1
}

Write-Host "`n[2/2] Building AtlusSaveConverter ($Config)..." -ForegroundColor Yellow
& $cmakeExe --build $buildDir --config $Config
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed." -ForegroundColor Red
    exit 1
}

$exePath = "$buildDir\$Config\AtlusSaveConverter.exe"
Write-Host "`nCompilation réussie !" -ForegroundColor Green
Write-Host "Exécutable disponible: $exePath" -ForegroundColor Green

if ($Run -or $args -contains "-Run") {
    Write-Host "`nLancement de AtlusSaveConverter..." -ForegroundColor Cyan
    Start-Process -FilePath $exePath
}
