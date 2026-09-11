rem Requires pngcrush on PATH -- the bundled Utils/pngcrush.exe /
rem Utils/crush copies were removed (backlog item 26, unused by any
rem build/CI step); install your own. forfiles is a Windows built-in.
forfiles -p.. -s -m*.png -c"pngcrushinplace.bat 0x22@FILE0x22"
