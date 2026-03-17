import re

project_file = 'BlinderKitten_App.vcxproj'

with open(project_file, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace the MultiProcessorCompilation setting from false to true
content = content.replace(
    '<MultiProcessorCompilation>false</MultiProcessorCompilation>',
    '<MultiProcessorCompilation>true</MultiProcessorCompilation>'
)

with open(project_file, 'w', encoding='utf-8', newline='') as f:
    f.write(content)

print("Successfully enabled MultiProcessorCompilation for Release configuration")
