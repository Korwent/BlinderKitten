@echo off
echo Fixing BlinderKitten build configuration...
powershell -Command "(Get-Content BlinderKitten_App.vcxproj -Raw) -replace '<MultiProcessorCompilation>false</MultiProcessorCompilation>','<MultiProcessorCompilation>true</MultiProcessorCompilation>' | Set-Content BlinderKitten_App.vcxproj -NoNewline"
echo Done! MultiProcessorCompilation is now enabled for Release builds.
