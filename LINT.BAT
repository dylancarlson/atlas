@echo off
if exist errors del errors
cl /AL /c /Od /W4 /Zs  /DNETWORK /DALPHA_TEST /DIBM /DSWAP /DOLDHELP %1  >>errors
type errors
