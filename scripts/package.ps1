param(
    [string]$Python = '.build/venv/Scripts/python.exe',
    [string]$Frontend = '.build/windows/Meridian.exe',
    [switch]$SkipEngineBuild
)
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)
$workspace = (Get-Location).Path
$pythonExe = (Get-Command $Python -ErrorAction Stop).Source
if (-not (Test-Path -LiteralPath $Frontend)) { throw 'Build the C++ frontend first; see README.' }

& $pythonExe -m unittest discover -s tests -v
if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
if (-not $SkipEngineBuild) {
    & $pythonExe -m PyInstaller --noconfirm --distpath .build/frozen --workpath .build/pyinstaller scripts/engine.spec
    if ($LASTEXITCODE -ne 0) { throw 'Engine build failed.' }
}
& ./.build/frozen/engine/meridian-engine.exe --self-test
if ($LASTEXITCODE -ne 0) { throw 'Frozen engine self-test failed.' }

$releaseRoot = Join-Path $workspace 'release'
$stage = Join-Path $releaseRoot 'Meridian'
$sourceStage = Join-Path $releaseRoot 'Meridian-source'
# Only clear the two known generated staging directories, after validating their absolute parents.
foreach ($target in @($stage, $sourceStage)) {
    $resolved = [IO.Path]::GetFullPath($target)
    if ((Split-Path -Parent $resolved) -ne $releaseRoot) { throw 'Unsafe staging path.' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
    New-Item -ItemType Directory -Path $resolved | Out-Null
}
Copy-Item -LiteralPath $Frontend -Destination (Join-Path $stage 'Meridian.exe')
Copy-Item -Recurse -LiteralPath '.build/frozen/engine' -Destination $stage
foreach ($dir in @('data','assets')) { Copy-Item -Recurse -LiteralPath $dir -Destination $stage }
$uiReport = Join-Path $workspace '.build/ui-self-test.json'
$uiProcess = Start-Process -FilePath (Join-Path $stage 'Meridian.exe') -ArgumentList @('--self-test', ('"' + $uiReport + '"')) -WindowStyle Hidden -Wait -PassThru
if ($uiProcess.ExitCode -ne 0) { throw 'Native desktop self-test failed.' }
foreach ($file in @('README.md','README.zh-CN.md','README.en.md','LICENSE','THIRD_PARTY_NOTICES.md')) { Copy-Item -LiteralPath $file -Destination $stage }
Copy-Item -Recurse -LiteralPath 'docs' -Destination $stage
New-Item -ItemType Directory -Force (Join-Path $stage 'licenses') | Out-Null
Copy-Item -LiteralPath 'third_party/nlohmann-LICENSE' -Destination (Join-Path $stage 'licenses')
& $pythonExe scripts/collect_licenses.py --output (Join-Path $stage 'licenses') --sources .build/upstream-source
if ($LASTEXITCODE -ne 0) { throw 'Could not collect dependency notices/sources.' }
foreach ($notice in @('mingw-COPYRIGHT.txt','gcc-COPYRIGHT.txt')) {
    if (Test-Path -LiteralPath "third_party/$notice") { Copy-Item -LiteralPath "third_party/$notice" -Destination (Join-Path $stage 'licenses') }
}

$allowed = @('src','backend','data','assets','scripts','tests','docs','third_party','.github',
    '.gitignore','.gitattributes','.clang-format','CMakeLists.txt','LICENSE','README.md','README.zh-CN.md','README.en.md',
    'THIRD_PARTY_NOTICES.md','CONTRIBUTING.md','SECURITY.md','requirements.txt','requirements-build.txt',
    'requirements-lock.txt','ios_location_tool.py','diagnose_system.py','start_tool.bat','set_apple_park.bat','run_wsl_ubuntu.sh')
foreach ($item in $allowed) {
    $source = Join-Path $workspace $item
    if (Test-Path -LiteralPath $source -PathType Container) {
        Get-ChildItem -LiteralPath $source -Recurse -File | Where-Object {
            $_.FullName -notmatch '[\\/]__pycache__[\\/]' -and $_.Extension -notin @('.pyc','.pyo','.log')
        } | ForEach-Object {
            $relative = $_.FullName.Substring($workspace.Length + 1)
            $destination = Join-Path $sourceStage $relative
            New-Item -ItemType Directory -Force (Split-Path -Parent $destination) | Out-Null
            Copy-Item -LiteralPath $_.FullName -Destination $destination
        }
    } else { Copy-Item -LiteralPath $source -Destination $sourceStage }
}
foreach ($auditTarget in @($stage, $sourceStage)) {
    & $pythonExe scripts/audit_release.py $auditTarget
    if ($LASTEXITCODE -ne 0) { throw 'Privacy audit failed. Release archives were not produced.' }
}
& $pythonExe scripts/audit_frozen.py (Join-Path $stage 'engine/meridian-engine.exe')
if ($LASTEXITCODE -ne 0) { throw 'Compressed Python code privacy audit failed.' }
# zipfile includes dotfiles (Compress-Archive omits them on some PowerShell versions).
& $pythonExe scripts/zip_release.py
if ($LASTEXITCODE -ne 0) { throw 'Archive creation failed.' }
& $pythonExe scripts/smoke_release.py
if ($LASTEXITCODE -ne 0) { throw 'Portable release smoke test failed.' }
Get-ChildItem -LiteralPath $releaseRoot -Filter '*.zip' | ForEach-Object {
    $digest = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$digest  $($_.Name)"
} | Set-Content -LiteralPath (Join-Path $releaseRoot 'SHA256SUMS.txt') -Encoding ascii
Write-Host 'Ready: release/Meridian/Meridian.exe and release/*.zip'
