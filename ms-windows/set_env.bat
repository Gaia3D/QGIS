@echo off
set PYTHONPATH=c:\OSGeo4W\apps\Python27
set VS90COMNTOOLS=%PROGRAMFILES(x86)%\Microsoft Visual Studio 10.0\Common7\Tools\
call "%PROGRAMFILES(x86)%\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x86
set INCLUDE=%INCLUDE%;%PROGRAMFILES%\Microsoft SDKs\Windows\v7.1\include
set LIB=%LIB%;%PROGRAMFILES%\Microsoft SDKs\Windows\v7.1\lib
set OSGEO4W_ROOT=C:\OSGeo4W
call "%OSGEO4W_ROOT%\bin\o4w_env.bat"
path %PATH%;%PROGRAMFILES(x86)%\Microsoft Visual Studio 10.0\Common7\IDE\;%SYSTEMROOT%\system32;%SYSTEMROOT%;%SYSTEMROOT%\System32\Wbem;%PROGRAMFILES(x86)%\CMake\bin\;c:\cygwin64\bin\;c:\NSIS;C:\NSIS\bin
@set GRASS_PREFIX=c:/OSGeo4W/apps/grass/grass-6.4.4
@set INCLUDE=%INCLUDE%;%OSGEO4W_ROOT%\include
@set LIB=%LIB%;%OSGEO4W_ROOT%\lib;%OSGEO4W_ROOT%\lib
@cmd