MakeBat
=======

Позволяет быстро компилировать .с/.cpp файлы в Sublime Text 3 используя компилятор Visual Studio C++. Создает file.cpp.make.bat файл сборки для VC++ на основе указанного шаблона .bat.template. Предварительно может извлеч из исходного файла дополнительную информацию и использовать ее в шаблоне.

Переменные внутри исходного файла:
- // INCLUDE_PATH: "Path"
- // LIB_PATH: "Path"
- #pragma comment(lib, "NNN.lib")
- // ADD_RESOURCE: "name.rc"
- // ADD_SOURCE: "name.cpp"

Переменные шаблона:
- {INCLUDE_PATHS} -> /I "Path1" /I "Path2" ...
- {LIB_PATHS}     -> /LIBPATH:"Path1" /LIBPATH:"Path2" ...
- {LIBS}          -> "file1.lib" "file2.lib" ...
- {RC}            -> "file1.rc" "file2.rc" ...
- {SOURCES}       -> "file1.cpp" "file2.cpp" "file1.res" "file2.res" ...

Установка
---------

1. Скопировать папку package\MakeBat\ в папку %AppData%\Sublime Text 3\Packages\
2. Выбрать MakeBat как систему сборки.

Использование
-------------

- Select and Build (Ctrl+Alt+F7)
    - В сплывающем окне выбрать шаблон.
    - Создать make-файл.
    - Скомпилировать.
- Build (F7)
    - Если не найден шаблон по умолчанию для текущего файла в его make.bat файле, то  выбрать шаблон как в пункте выше. Иначе пункт пропускаем.
    - Создать make-файл.
    - Скомпилировать.
- Compile (Ctrl+F7)
    - Скомпилировать.
- Run (Ctrl+F5)
    - Запустить программу.