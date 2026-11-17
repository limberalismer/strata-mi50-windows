param(
    [Parameter(Mandatory=$true)][string]$ModelDir,
    [string]$PackDir,
    [string]$Draft,
    [string]$RuntimeArchive,
    [switch]$DownloadModel,
    [switch]$SkipModelHash,
    [int]$Port = 8081,
    [int]$Gpu = 0
)
$ErrorActionPreference = 'Stop'
$taskRoot = $PSScriptRoot
$taskTemp = Join-Path $taskRoot 'local/setup-tmp'
New-Item -ItemType Directory -Path $taskTemp -Force | Out-Null
$taskOldTemp = @{}
foreach ($taskName in @('TEMP', 'TMP', 'TMPDIR')) {
    $taskOldTemp[$taskName] = [Environment]::GetEnvironmentVariable($taskName, 'Process')
    [Environment]::SetEnvironmentVariable($taskName, $taskTemp, 'Process')
}
try {
$taskPython = Join-Path $taskRoot '.venv/Scripts/python.exe'
if (-not (Test-Path -LiteralPath $taskPython)) {
    py -3 -m venv (Join-Path $taskRoot '.venv')
    if ($LASTEXITCODE -ne 0) { throw 'Install Python 3.11+ x64, including the py launcher.' }
}
& $taskPython -m pip install --disable-pip-version-check --cache-dir (Join-Path $taskRoot 'local/pip-cache') --index-url https://pypi.org/simple --timeout 30 --retries 2 -r (Join-Path $taskRoot 'requirements.txt')
if ($LASTEXITCODE -ne 0) { throw 'Python dependency installation failed.' }
$taskArgs = @((Join-Path $taskRoot 'scripts/setup.py'), '--model-dir', $ModelDir, '--fetch-runtime', '--fetch-draft', '--port', "$Port", '--gpu', "$Gpu")
if ($PackDir) { $taskArgs += @('--pack-dir', $PackDir) }
if ($Draft) { $taskArgs += @('--draft', $Draft) }
if ($RuntimeArchive) { $taskArgs += @('--runtime-archive', $RuntimeArchive) }
if ($DownloadModel) { $taskArgs += '--download-model' }
if ($SkipModelHash) { $taskArgs += '--skip-model-hash' }
& $taskPython @taskArgs
if ($LASTEXITCODE -ne 0) { throw 'Preparation failed. Fix the reported prerequisite and rerun.' }
& $taskPython (Join-Path $taskRoot 'scripts/doctor.py')
if ($LASTEXITCODE -ne 0) { throw 'Installation checks failed.' }
} finally {
    foreach ($taskName in @('TEMP', 'TMP', 'TMPDIR')) {
        [Environment]::SetEnvironmentVariable($taskName, $taskOldTemp[$taskName], 'Process')
    }
}
