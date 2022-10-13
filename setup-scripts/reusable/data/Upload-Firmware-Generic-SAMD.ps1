$firmwareName = $args[0]
$toolsExist = Test-Path -PathType Leaf ~/bin/bossac.exe
if (!$toolsExist) {
    ../Build-knxprod.ps1
}

if ($toolsExist) {
    Write-Host Suche COM-Port fuer SAMD...
    $portList = get-pnpdevice -class Ports
    if ($portList) {
        foreach($device in $portList) {
            if ($device.Present -and $device.InstanceId.StartsWith("USB\")) {
                $isCom = $device.Name -match "COM(\d{1,3})"
                if($isCom)
                {
                    $port = $Matches[0]
                    Write-Host Gefunden $port
                    break
                }
            }
        }
        if($port)
        {
            Write-Host Verwende $port
            $serial = new-Object System.IO.Ports.SerialPort $port,1200,None,8,1
            $serial.Open()
            $serial.Close()
            Start-Sleep -s 1
            ~/bin/bossac --info --write --verify --reset --erase data/$firmwareName
            timeout /T 20 
        } else {
            Write-Host
            Write-Host "Kein Port gefunden!"
            Write-Host
            Write-Host "Versuche bitte die alternative Setup-Methode: Den SAMD in den Bootloader-Modus versetzen"
            Write-Host "Dazu am Gerät die Reset-Taste zwei mal schnell hintereinander drücken. Die LED wird dann"
            Write-Host "Pulsieren. Jetzt befindet sich das Gerät im Bootloader-Modus."
            Write-Host "Anschließend das Skript erneut starten."
            timeout /T 60     
        }
    }
}