param([string]$FilePath)
$cert = Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -like "*MediSaveDev*" } | Select-Object -First 1
if ($cert) {
    if (Test-Path $FilePath) {
        Set-AuthenticodeSignature -FilePath $FilePath -Certificate $cert | Out-Null
    }
    if (Test-Path "$FilePath.exe") {
        Set-AuthenticodeSignature -FilePath "$FilePath.exe" -Certificate $cert | Out-Null
    }
}
