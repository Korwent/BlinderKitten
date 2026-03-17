# Fix the multiprocessor compilation setting in the project file
$projectFile = "BlinderKitten_App.vcxproj"

Write-Host "Reading project file: $projectFile"
$content = Get-Content $projectFile -Raw

Write-Host "Fixing MultiProcessorCompilation setting..."
$content = $content -replace '<MultiProcessorCompilation>false</MultiProcessorCompilation>', '<MultiProcessorCompilation>true</MultiProcessorCompilation>'

Write-Host "Writing updated project file..."
Set-Content -Path $projectFile -Value $content -NoNewline

Write-Host "Successfully fixed multiprocessor compilation setting in $projectFile"
Write-Host "The Release configuration now has MultiProcessorCompilation set to true, which matches the /FS flag."
