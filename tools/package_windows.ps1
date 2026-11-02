param(
    [string]$QtBin = 'D:/msys64/mingw64/bin',
    [string]$BuildDir = 'build-qt',
    [string]$OutputDir = 'dist'
)
$ErrorActionPreference = 'Stop'
# 在项目根目录执行；仅复制产物，不删除目录，不覆盖已有用户存档。
New-Item -ItemType Directory -Force $OutputDir | Out-Null
Copy-Item -LiteralPath "$BuildDir/canteen_gui.exe" -Destination "$OutputDir/canteen_gui.exe"
New-Item -ItemType Directory -Force "$OutputDir/data" | Out-Null
if (-not (Test-Path -LiteralPath "$OutputDir/data/canteen.txt")) {
    Copy-Item -LiteralPath 'data/canteen.txt' -Destination "$OutputDir/data/canteen.txt"
}
& "$QtBin/windeployqt6.exe" --release --no-translations "$OutputDir/canteen_gui.exe"
if ($LASTEXITCODE -ne 0) { throw 'Qt部署失败' }

# windeployqt在MSYS2环境中不一定复制全部非Qt依赖，递归检查PE导入表。
$queue = [System.Collections.Generic.Queue[string]]::new()
$visited = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
Get-ChildItem -LiteralPath $OutputDir -Recurse -File | Where-Object { $_.Extension -in '.dll','.exe' } | ForEach-Object { $queue.Enqueue($_.FullName) }
while ($queue.Count -gt 0) {
    $binary = $queue.Dequeue()
    if (-not $visited.Add($binary)) { continue }
    $imports = & "$QtBin/objdump.exe" -p $binary
    if ($LASTEXITCODE -ne 0) { throw "依赖检查失败：$binary" }
    foreach ($line in $imports) {
        if ($line -match 'DLL Name:\s*(\S+)') {
            $dependency = $Matches[1]
            $source = Join-Path $QtBin $dependency
            $target = Join-Path $OutputDir $dependency
            # 系统DLL由Windows提供；只复制工具链目录中存在的运行库。
            if ((Test-Path -LiteralPath $source) -and -not (Test-Path -LiteralPath $target)) {
                Copy-Item -LiteralPath $source -Destination $target
                $queue.Enqueue((Get-Item -LiteralPath $target).FullName)
            }
        }
    }
}
Write-Output "发布目录已准备好：$OutputDir/canteen_gui.exe"
