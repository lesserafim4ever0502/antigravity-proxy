param(
    [string]$BuildDir = "build-process-name-tests",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildPath = Join-Path $Root $BuildDir

Write-Host "[测试] 配置 CreateProcess 进程名解析测试..."
cmake -S $Root -B $BuildPath -DBUILD_TESTS=ON | Out-Host

Write-Host "[测试] 构建 process_name_tests..."
cmake --build $BuildPath --config $Config --target process_name_tests | Out-Host

Write-Host "[测试] 运行 CTest..."
ctest --test-dir $BuildPath -C $Config -R process_name_tests --output-on-failure | Out-Host

Write-Host "[测试] CreateProcess 进程名解析测试通过。"
