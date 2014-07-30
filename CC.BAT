@echo off
if exist errors del errors
cl /AL /c /Od /W4   /DNETWORK /DALPHA_TEST /DIBM /DSWAP /DOLDHELP %1  >>errors

