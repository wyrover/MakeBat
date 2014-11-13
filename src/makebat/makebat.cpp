//
// Project: MakeBat
// Date:    2014-02-17
// Author:  Ruslan Zaporojets | ruzzzua[]gmail.com
//

// ADD_RESOURCE: "makebat.rc"
// TODO Compile as UNICODE !!!

#include <io.h>     // _access
#include <iostream>
#include <fstream>
#include <string>
#include <regex>

#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")

using namespace std;

//
//  Defs
//

const char HELLO[] = "MakeBat v0.5a [2014/11/10] by Ruslan Zaporojets\n";
const char USAGE[] =
{
    "Create -make.bat from template file\n"
    "Usage: makebat source-file [-t]\n"
    "Option '-t' - run selector before make.\n"
};

const char TEMPLATE_SELECTOR_FILENAME[] = "selector.exe";
const char TEMPLATES_SUB_FOLDER[] = "templates\\";
const char TEMPLATE_EXT[] = ".bat-template";
const char DEFAULT_MAKE_EXT[] = "-make.bat";
const int SELECTOR_OK = 0;
const int SELECTOR_CANCEL = 1;

// TODO Invalid chars "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\\/:*?\"<>|"
const char REGEX_TEMPLATE_NAME[] = "MakeBat-Template[ \\t]*:[ \\t]*\"([^\\t:*?\"<>|\\r\\n]+)\"[ \\t]*";

const char REGEX_INCLUDE_PATH[] = "^[ \\t]*//[ \\t]*INCLUDE_PATH:[ \\t]*\"([^\\t:*?\"<>|\\r\\n]+)\"[ \\t]*";
const char REGEX_LIB_PATH[]     = "^[ \\t]*//[ \\t]*LIB_PATH:[ \\t]*\"([^\\t:*?\"<>|\\r\\n]+)\"[ \\t]*";
const char REGEX_LIB[]          = "^[ \\t]*\\#pragma[ \\t]+comment[ \\t]*\\([ \\t]*lib[ \\t]*,[ \\t]*\"(\\w+\\.lib)\"[ \\t]*\\)[ \\t]*";
const char REGEX_SOURCE[]       = "^[ \\t]*//[ \\t]*ADD_SOURCE:[ \\t]*\"([^\\t:*?\"<>|\\r\\n]+)\"[ \\t]*";
const char REGEX_RESOURCE[]     = "^[ \\t]*//[ \\t]*ADD_RESOURCE:[ \\t]*\"([^\\t:*?\"<>|\\r\\n]+)\"[ \\t]*";
const char REGEX_CL_PARAMS[]    = "^[ \\t]*//[ \\t]*CL_PARAMS:[ \\t]*\"([^\\t:*?\"<>|\\r\\n]+)\"[ \\t]*";
const char REGEX_LINK_PARAMS[]  = "^[ \\t]*//[ \\t]*LINK_PARAMS:[ \\t]*\"([^\\t:*?\"<>|\\r\\n]+)\"[ \\t]*";

const char VAR_INCLUDE_PATHS[] = "{INCLUDE_PATHS}";
const char VAR_LIB_PATHS[]     = "{LIB_PATHS}";
const char VAR_LIBS[]          = "{LIBS}";
const char VAR_SOURCES[]       = "{SOURCES}";
const char VAR_RESOURCES[]     = "{RESOURCES}";
const char VAR_CL_PARAMS[]     = "{CL_PARAMS}";
const char VAR_LINK_PARAMS[]   = "{LINK_PARAMS}";
const char VAR_OUT[]           = "{OUT}";

enum
{
    RESULT_OK = 0,
    RESULT_CANCEL,
    RESULT_ERROR_UNKNOWN,
    RESULT_ERROR_CANNOT_RUN_SELECTOR,
    RESULT_ERROR_CANNOT_READ_MAKE_BAT,
    RESULT_ERROR_TEMPLATE_NAME_IN_MAKE_BAT,
    RESULT_ERROR_CANNOT_READ_CPP,
    RESULT_ERROR_CANNOT_READ_TEMPLATE,
    RESULT_ERROR_CANNOT_WRITE_RESULT,
    RESULT_ERROR_TEMPLATE_TAG_SOURCES
};

const char *MESSAGE[] =
{
    "Created: ",
    "User cancel.",
    "Internal error: ",
    "Error. Cannot run selector: ",
    "Error. Cannot read make-bat file: ",
    "Error. Make-bat file does not contain 'MakeBat-Template': ",
    "Error. Cannot read source file: ",
    "Error. Cannot read template file: ",
    "Error. Cannot write make-bat file: ",
    "Error. Template does not contain tag {SOURCES}: "
};

typedef void (*TParserFunc)(string &result, const string &element);

//
//  Helpers
//

bool run(const char *path, DWORD &exitCode)
{
    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = { 0 };

    string command(path);
    if (!::CreateProcess(NULL, &command[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return false;
    BOOL result = ::WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_OBJECT_0
               && ::GetExitCodeProcess(pi.hProcess, &exitCode);
    ::CloseHandle(pi.hProcess);
    ::CloseHandle(pi.hThread);
    return result != FALSE;
}

string getCurrentExeFileName()
{
    string path;
    DWORD size;
    do
    {
        if (path.empty())
            path.resize(MAX_PATH);
        else
            path.resize(path.size() * 2);
        size = ::GetModuleFileName(NULL, &path[0], path.size());

    } while (size == path.size());
    path.resize(size);
    return path;
}

inline bool isSpace(const char c)
{
    #ifdef _WIN32
    return isspace(static_cast<unsigned char>(c)) != 0;
    #else
    return isspace(c) != 0;
    #endif
}

void trim(string &s)
{
    if (!s.empty())
    {
        // TODO string::const_iterator
        auto first = s.cbegin(), last = s.cend();
        for (; first != last && isSpace(*first); ++first)
            ;
        if (first == last)
        {
            s.clear();
            return;
        }
        --last;
        for (; first != last && isSpace(*last); --last)
            ;
        s.assign(first, last + 1);  // TODO its norm?
    }
}

void trimLeft(string &s)
{
    if (!s.empty())
    {
        auto first = s.cbegin(), last = s.cend();
        --last;
        for (; first != last && isSpace(*last); --last)
            ;
        s.assign(first, last + 1);
    }
}

void replace(std::string &str, const std::string &from, const std::string &to)
{
    if (str.empty() || from.empty())
        return;
    // TODO: if to.empty() - use erase
    size_t pos = str.find(from);
    if (pos != std::wstring::npos)
        str.replace(pos, from.length(), to);
}

inline bool hasSubstr(const std::string &str, const std::string &substr)
{
    if (str.empty() || substr.empty())
        return false;
    size_t pos = str.find(substr);
    return pos != std::wstring::npos;
}

string getFileName(const string &fullPath)
{
    const size_t indexOfSeparator = fullPath.find_last_of("\\");
    if (indexOfSeparator != string::npos)
        return fullPath.substr(indexOfSeparator + 1);
    else
        return fullPath;
}

inline string getFileName(const char *fullPath)
{
    return getFileName(string(fullPath));
}

string changeExt(const string &fileName, const string &newExt)
{
    const size_t indexOfSeparator = fileName.find_last_of(".");
    if (indexOfSeparator != string::npos)
        return fileName.substr(0, indexOfSeparator) + newExt;
    else
        return fileName + newExt;
}

// Return path with backsplash on end or empty string
string getPath(const string &fullPath)
{
    const size_t indexOfSeparator = fullPath.find_last_of("\\");
    if (indexOfSeparator != string::npos)
        return fullPath.substr(0, indexOfSeparator + 1);
    else
        return string();
}

inline string getPath(const char *fullPath)
{
    return getPath(string(fullPath));
}

inline bool fileExists(const char *fileName)
{
  return _access(fileName, 0) != -1;
}

bool readString(const char *fileName, string &content)
{
    ifstream file(fileName);
    if (!file)
        return false;
    content.assign((istreambuf_iterator<char>(file)), (istreambuf_iterator<char>()));
    if (!file)
        return false;
    return  true;
}

bool writeString(const char *fileName, const string &content)
{
    ofstream file(fileName, ios::out);
    if (!file)
        return false;
    file << content;
    if (!file)
        return false;
    return true;
}

//
//  Core
//

void collectorRes(string &result, const string &element)
{
    if (!result.empty())
        result += " ";
    result += '"' + changeExt(element, ".res") + '"';
}

string parseSourceEx(const string &sourceContent,
                     const regex &rule,
                     TParserFunc func)
{
    string s(sourceContent);
    string result;
    smatch m;
    while (regex_search(s, m, rule))
    {
        if (m.size() < 2)
            break;
        func(result, m[1]);
        s = m.suffix().str();
    }
    return result;
}

string getTemplateNameFromMakeBat(const string &makeBatContent)
{
    smatch m;
    if (regex_search(makeBatContent, m, regex(REGEX_TEMPLATE_NAME)) && m.size() == 2)
        return string(m[1]);
    return string();
}

string parseSource(const string &sourceContent,
                   const regex &rule,
                   const char *prefix = nullptr)
{
    string s(sourceContent);
    string result;
    smatch m;
    while (regex_search(s, m, rule))
    {
        if (m.size() < 2)
            break;

        if (!result.empty())
            result += " ";
        if (prefix)
            result += prefix;
        result += '"' + string(m[1]) + '"';
        s = m.suffix().str();
    }
    return result;
}

bool selectTemplate(const string &selectorPath, bool &userSelected)
{
    DWORD exitCode;
    if (run(selectorPath.c_str(), exitCode))
    {
        userSelected = exitCode == SELECTOR_OK;
        return true;
    }
return false;
}

#define CHECK_ARG__T (argv[2][0] == '-' && argv[2][1] == 't' && argv[2][2] == '\0')

int main(int argc, char const *argv[])
{
    cout << HELLO;

    if (argc < 2 || argc > 3 || (argc == 3 && !CHECK_ARG__T))
    {
        cout << USAGE;
        return 1;
    }

    string sourceFileName(getFileName(argv[1]));
    string makeBatFileName(sourceFileName + DEFAULT_MAKE_EXT);
    string sourcePath(getPath(argv[1]));
    ::SetCurrentDirectory(sourcePath.c_str());
    string currenExeDir(getPath(getCurrentExeFileName()));

    if ((argc == 2 && !fileExists(makeBatFileName.c_str())) ||
        (argc == 3 && CHECK_ARG__T))
    {
        bool userSelected;
        string selectorPath(currenExeDir + TEMPLATE_SELECTOR_FILENAME + " " + makeBatFileName);
        if (!selectTemplate(selectorPath, userSelected))
        {
            cout << MESSAGE[RESULT_ERROR_CANNOT_RUN_SELECTOR] << selectorPath << endl;
            return RESULT_ERROR_CANNOT_RUN_SELECTOR;
        }
        if (!userSelected)
            return RESULT_CANCEL;
    }

    try
    {
        // Read template filename from current make-bat file
        string makeBatContent;
        if (!readString(makeBatFileName.c_str(), makeBatContent))
        {
            // TODO If this error then run Selector!!!
            cout << MESSAGE[RESULT_ERROR_CANNOT_READ_MAKE_BAT] << makeBatFileName << endl;
            return RESULT_ERROR_CANNOT_READ_MAKE_BAT;
        }
        string templateFileName = getTemplateNameFromMakeBat(makeBatContent);
        if (templateFileName.empty())
        {
            // TODO If this error then run Selector!!!
            cout << MESSAGE[RESULT_ERROR_TEMPLATE_NAME_IN_MAKE_BAT] << makeBatFileName << endl;
            return RESULT_ERROR_TEMPLATE_NAME_IN_MAKE_BAT;
        }
        //templateFileName += TEMPLATE_EXT;

        // Read template file
        string templateFullPath = sourcePath + templateFileName + TEMPLATE_EXT;
        if (!fileExists(templateFullPath.c_str()))
            templateFullPath = currenExeDir + TEMPLATES_SUB_FOLDER + templateFileName + TEMPLATE_EXT;
        string templateContent;
        if (!readString(templateFullPath.c_str(), templateContent))
        {
            cout << MESSAGE[RESULT_ERROR_CANNOT_READ_TEMPLATE] << templateFullPath << endl;
            return RESULT_ERROR_CANNOT_READ_TEMPLATE;
        }

        string templateFileNameInTemplate = getTemplateNameFromMakeBat(templateContent);
        if (templateFileNameInTemplate.empty() || templateFileNameInTemplate != templateFileName)
        {
            string s("@rem MakeBat-Template: \"" + templateFileName + "\"\n");
            templateContent = s + templateContent;
        }

        // Read source file
        string sourceContent;
        if (!readString(sourceFileName.c_str(), sourceContent) || sourceContent.empty())
        {
            cout << MESSAGE[RESULT_ERROR_CANNOT_READ_CPP] << sourceFileName << endl;
            return RESULT_ERROR_CANNOT_READ_CPP;
        }

        // Create MakeBat

        // Parse: // INCLUDE_PATH: "Path"
        if (hasSubstr(templateContent, VAR_INCLUDE_PATHS))
        {
            string value(parseSource(sourceContent, regex(REGEX_INCLUDE_PATH), "/I "));
            replace(templateContent, VAR_INCLUDE_PATHS, value);
        }

        // Parse: // LIB_PATH: "Path"
        if (hasSubstr(templateContent, VAR_LIB_PATHS))
        {
            string value(parseSource(sourceContent, regex(REGEX_LIB_PATH), "/LIBPATH:"));
            replace(templateContent, VAR_LIB_PATHS, value);
        }

        // Parse: #pragma comment(lib, "NNN.lib")
        if (hasSubstr(templateContent, VAR_LIBS))
        {
            string value(parseSource(sourceContent, regex(REGEX_LIB)));
            replace(templateContent, VAR_LIBS, value);
        }

        // Parse: // ADD_RESOURCE: "name.rc"
        string resources;
        if (hasSubstr(templateContent, VAR_RESOURCES))
        {
            string value(parseSource(sourceContent, regex(REGEX_RESOURCE)));
            replace(templateContent, VAR_RESOURCES, value);
            resources = parseSourceEx(sourceContent, regex(REGEX_RESOURCE), collectorRes);
        }

        // Parse: // CL_PARAMS: "..."
        if (hasSubstr(templateContent, VAR_CL_PARAMS))
        {
            string value(parseSource(sourceContent, regex(REGEX_CL_PARAMS)));
            replace(templateContent, VAR_CL_PARAMS, value);
        }

        // Parse: // LINK_PARAMS: "..."
        if (hasSubstr(templateContent, VAR_LINK_PARAMS))
        {
            string value(parseSource(sourceContent, regex(REGEX_LINK_PARAMS)));
            replace(templateContent, VAR_LINK_PARAMS, value);
        }

        // {OUT} -> source_name_without_ext
        if (hasSubstr(templateContent, VAR_OUT))
        {
            replace(templateContent, VAR_OUT, changeExt(sourceFileName, ""));
        }

        // Parse: // ADD_SOURCE: "name.cpp"
        if (!hasSubstr(templateContent, VAR_SOURCES))
        {
            cout << MESSAGE[RESULT_ERROR_TEMPLATE_TAG_SOURCES] << templateFullPath << endl;
            return RESULT_ERROR_TEMPLATE_TAG_SOURCES;
        }
        string addSources(parseSource(sourceContent, regex(REGEX_SOURCE)));
        string sources;
        sources = '"' + sourceFileName + '"';
        if (!addSources.empty())
            sources += " " + addSources;
        if (!resources.empty())
            sources += " " + resources;
        replace(templateContent, VAR_SOURCES, sources);

        // Write result
        if (!writeString(makeBatFileName.c_str(), templateContent))
        {
            cout << MESSAGE[RESULT_ERROR_CANNOT_WRITE_RESULT] << makeBatFileName << endl;
            return RESULT_ERROR_CANNOT_WRITE_RESULT;
        }
        else
            cout << MESSAGE[RESULT_OK] << makeBatFileName << endl;
    }
    catch (const exception &e)
    {
        cout << MESSAGE[RESULT_ERROR_UNKNOWN] << e.what() << endl;
        return RESULT_ERROR_UNKNOWN;
    }

    return RESULT_OK;
}
