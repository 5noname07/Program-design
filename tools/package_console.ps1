param(
    [string]$BuildDir = 'build-console',
    [string]$OutputDir = 'dist-console'
)
$ErrorActionPreference = 'Stop'
# 终端版只复制C程序和TXT数据，不调用任何Qt部署工具。
New-Item -ItemType Directory -Force -Path $OutputDir,"$OutputDir/data" | Out-Null
Copy-Item -LiteralPath "$BuildDir/canteen.exe" -Destination "$OutputDir/canteen.exe"
if (-not (Test-Path -LiteralPath "$OutputDir/data/canteen.txt")) {
    Copy-Item -LiteralPath 'data/canteen.txt' -Destination "$OutputDir/data/canteen.txt"
}
# 发布包中的启动器固定工作目录，双击或从其他目录运行均能找到存档。
$launcher = "@echo off`r`ncd /d `"%~dp0`"`r`n`"%~dp0canteen.exe`" `"%~dp0data\canteen.txt`"`r`npause`r`n"
[System.IO.File]::WriteAllText((Join-Path (Resolve-Path $OutputDir) 'Start-Console.cmd'), $launcher, [System.Text.Encoding]::ASCII)
Write-Output "终端版已准备好：$OutputDir/canteen.exe"
