Write-Host "Running P2Pal Tests..."


Start-Process -FilePath .\P2Pal.exe
Start-Process -FilePath .\P2Pal.exe


Start-Sleep -Seconds 5

Write-Host "Sending Test Message..."
$udpClient = New-Object System.Net.Sockets.UdpClient
$udpClient.Connect("127.0.0.1", 45454)
$bytes = [System.Text.Encoding]::UTF8.GetBytes("Test Message")
$udpClient.Send($bytes, $bytes.Length)


Start-Sleep -Seconds 5


if (Get-Content P2Pal.log | Select-String "Test Message") {
    Write-Host "Test Passed"
} else {
    Write-Host "Test Failed"
}
