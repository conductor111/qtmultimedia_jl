call set_env_var.cmd
set PATH=%PYTHON_2_7_DIR%;%PYTHON_2_7_DIR%\Scripts;%PERL_64_DIR%;%PERL_64_DIR%\bin;%RUBY_BIN_DIR%;%QT_BASE_DIR%\qtbase\bin;%QT_BASE_DIR%\Src\gnuwin32\bin;%PATH%
set INCLUDE=%GSTREAMER_DIR%lib\gstreamer-1.0\;%INCLUDE%
set INCLUDE=%GSTREAMER_DIR%lib\gstreamer-1.0\include\;%INCLUDE%
set INCLUDE=%GSTREAMER_DIR%lib\gstreamer-1.0\include\gst\;%INCLUDE%
set INCLUDE=%GSTREAMER_DIR%lib\gstreamer-1.0\include\gst\gl\;%INCLUDE%
set INCLUDE=%GSTREAMER_DIR%;%INCLUDE%
set INCLUDE=%GSTREAMER_DIR%include\;%INCLUDE%
set INCLUDE=%GSTREAMER_DIR%include\gstreamer-1.0\;%INCLUDE%
set INCLUDE=%GSTREAMER_DIR%include\gstreamer-1.0\gst\;%INCLUDE%
set INCLUDE=%GSTREAMER_DIR%include\gstreamer-1.0\ges\;%INCLUDE%
set INCLUDE=%GSTREAMER_DIR%include\glib-2.0\;%INCLUDE%
set INCLUDE=%GSTREAMER_DIR%lib\glib-2.0\include\;%INCLUDE%
set INCLUDE=%ADDITIONAL_INCLUDE%;%INCLUDE%
cd %QT_BASE_DIR%

configure -no-mp -debug-and-release -developer-build -platform win32-msvc2015 -opensource -force-debug-info -confirm-license -c++std c++11 -make libs -make tools -no-openssl -gstreamer 1.0 -L%GSTREAMER_DIR%lib\ -L%GSTREAMER_DIR%bin\ -L%GSTREAMER_DIR% -skip qtwebchannel -skip qtwebengine -skip qtwebsockets -skip qtwebview
SET CL=/MP1

cd ..\..