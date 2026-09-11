rem Requires doxygen and the HTML Help Workshop's hhc.exe on PATH -- the
rem bundled Utils/doxygen/{doxygen,hhc}.exe copies were removed (backlog
rem item 26, unused by any build/CI step); install your own.
del /q ..\stepmania.chm
del /s /q temp
rmdir /s /q temp

doxygen doxygen_config

hhc temp\html\index.hhp
move temp\html\index.chm ..\stepmania.chm

