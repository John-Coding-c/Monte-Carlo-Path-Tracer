# Build the raytracer with g++ (MSYS2 / MinGW-w64).
#
#   .\build.ps1              # build
#   .\build.ps1 -Run         # build, then render the default scene
#   .\build.ps1 -Debug       # unoptimized build with sanitizer-friendly flags

param(
    [switch]$Run,
    [switch]$Debug
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path

# Prefer g++ on PATH, fall back to the usual MSYS2 location.
$gxx = (Get-Command g++ -ErrorAction SilentlyContinue).Source
if (-not $gxx) { $gxx = 'C:\msys64\ucrt64\bin\g++.exe' }
if (-not (Test-Path $gxx)) {
    throw "g++ not found. Install MSYS2 (pacman -S mingw-w64-ucrt-x86_64-gcc) or put g++ on PATH."
}

$flags = if ($Debug) {
    @('-std=c++17', '-O0', '-g', '-Wall', '-Wextra', '-pthread')
} else {
    @('-std=c++17', '-O3', '-march=native', '-Wall', '-Wextra', '-pthread')
}

New-Item -ItemType Directory -Force -Path (Join-Path $root 'build') | Out-Null
$out = Join-Path $root 'build\raytracer.exe'

Write-Host "building $out"
& $gxx @flags (Join-Path $root 'src\main.cpp') -o $out
if ($LASTEXITCODE -ne 0) { throw "build failed ($LASTEXITCODE)" }
Write-Host "ok"

if ($Run) {
    & $out --scene spheres --width 800 --samples 100 --out (Join-Path $root 'out\render.png')
}
