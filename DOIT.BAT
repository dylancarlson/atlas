@echo off
if exist errors del errors
nmake CFLAGS="/AL /c /Od /W4"  DEFS="/DNETWORK /DALPHA_TEST /DIBM /DSWAP /DOLDHELP /DCRON" 

