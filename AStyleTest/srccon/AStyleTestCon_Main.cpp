// AStyleTestCon tests the ASConsole class only. This class is used only in
// the console build. It also tests the parseOption function for options used
// by only by the console build (e.g. recursive, preserve-date, verbose). It
// does not explicitely test the ASStreamIterator class or any other part
// of the program.

//----------------------------------------------------------------------------
// headers
//----------------------------------------------------------------------------

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

#include "gtest/gtest.h"
#include "TersePrinter.h"
#include "AStyleTestCon.h"

//-------------------------------------------------------------------------
// macro
//-------------------------------------------------------------------------

// ASTYLE_ABORT(message) macro
// Print an error message, including the file and line number,
//  and then abort the program.
#define ASTYLE_ABORT(message) \
	{ \
		(*_err) << endl << __FILE__ << " (" << __LINE__ << ")" << endl; \
		(*_err) << (message) << endl; \
		(*_err) << "\nThe test has terminated!\n" << endl; \
		exit(EXIT_FAILURE); \
	}

//----------------------------------------------------------------------------
// global variables
//----------------------------------------------------------------------------

// directory paths to use for testing
#ifdef _WIN32
string g_testDirectory = "%USERPROFILE%\\Projects\\AstyleTest\\ut-testcon";
#else
string g_testDirectory = "$HOME/Projects/AstyleTest/ut-testcon";
#endif

//----------------------------------------------------------------------------
// main function
//----------------------------------------------------------------------------

int main(int argc, char **argv)
{
	// set global variable g_testDirectory
	setTestDirectory();
	// the following statement will be printed at beginning of job
	// and before a death test
//	printf("Test directory: %s.\n", (*g_testDirectory).c_str());

	// parse command line BEFORE InitGoogleTest
	bool use_terse_printer = false;
	bool use_color = true;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--terse_printer") == 0 )
			use_terse_printer = true;
		else if (strcmp(argv[i], "--gtest_color=no") == 0 )
			use_color = false;
	}

	// do this after parsing the command line but before changing printer
	testing::InitGoogleTest(&argc, argv);

	// change to TersePrinter
	if (use_terse_printer)
	{
		UnitTest& unit_test = *UnitTest::GetInstance();
		testing::TestEventListeners& listeners = unit_test.listeners();
		delete listeners.Release(listeners.default_result_printer());
		listeners.Append(new TersePrinter(use_color));
	}

	// begin unit testing
	createTestDirectory(getTestDirectory());
	int retval = RUN_ALL_TESTS();

	// end of unit testing
	removeTestDirectory(getTestDirectory());
//	system("pause");		// sometimes needed for debug
	return retval;
}

//----------------------------------------------------------------------------
// support functions
//----------------------------------------------------------------------------

//char** buildArgv(const vector<string>& argIn)
//// build an array of pointers to be used as argv variable in function calls
//// the calling program must delete[] argv
//// argc will equal (argIn.size() + 1)
//{
//	// build argv array of pointers for input
//	int argc = argIn.size() + 1;
//
//	char** argv = new char* [sizeof(char*) * argc];
//	argv[0] = (char*)"AStyleTestCon.exe";	// first value is the program name
//	if (argIn.size() > 0)
//		for (int i = 1; i < argc; i++)		// next are the options
//			argv[i] = const_cast<char*>(argIn[i-1].c_str());
////	for (int i = 0; i < argc; i++)
////		cout << argv[i] << endl;
//	return argv;
//}

#ifdef _WIN32
void cleanTestDirectory(const string &directory)
// Windows remove files and sub directories from the test directory
{
	WIN32_FIND_DATA FindFileData;

	// Find the first file in the directory
	// Find will get at least "." and "..".
	string firstFile = directory + "\\*";
	HANDLE hFind = FindFirstFile(firstFile.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		ASTYLE_ABORT("Cannot open directory for clean: " + directory);

	// remove files and sub directories
	do
	{
		// skip these
		if (strcmp(FindFileData.cFileName, ".") == 0
				||  strcmp(FindFileData.cFileName, "..") == 0)
			continue;
		// clean and remove sub directories
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			string subDirectoryPath = directory + '\\' + FindFileData.cFileName;
			cleanTestDirectory(subDirectoryPath);
			BOOL isRemoved = RemoveDirectory(subDirectoryPath.c_str());
			if (!isRemoved)
				retryRemoveDirectory(subDirectoryPath);
			continue;
		}
		// remove the file
		string filePathName = directory + '\\' + FindFileData.cFileName;
		BOOL isRemoved = DeleteFile(filePathName.c_str());
		if (!isRemoved)
		{
			cout << "Cannot remove file for clean: " << filePathName << endl;
			displayLastError();
			systemPause();
		}
	}
	while (FindNextFile(hFind, &FindFileData) != 0);

	// check for processing error
	FindClose(hFind);
	DWORD dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
		ASTYLE_ABORT("Error processing directory for clean: " + directory);
}

void displayLastError()
{
	LPSTR msgBuf;
	DWORD lastError = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		lastError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  // Default language
		(LPSTR) &msgBuf,
		0,
		NULL
	);
	// Display the string.
	cout << "Error (" << lastError << ") " << msgBuf << endl;
	// Free the buffer.
	LocalFree(msgBuf);
}

void retryRemoveDirectory(const string& directory)
// wait for files and sub-directories to be removed
{
	// sleep a max of 20 seconds for the remove
	for (size_t seconds = 1; seconds <= 20; seconds++)
	{
		sleep(1);
		BOOL isRemoved = RemoveDirectory(directory.c_str());
		if (isRemoved)
		{
//			cout << "remove retry: " << seconds << " seconds" << endl;
			return;
		}
	}
	cout << "Cannot remove file for clean: " << directory << endl;
	displayLastError();
	systemPause();
}

void sleep(int seconds)
{
	clock_t endwait;
	endwait = clock_t (clock () + seconds * CLOCKS_PER_SEC);
	while (clock() < endwait) {}
}

#else

void cleanTestDirectory(const string &directory)
// POSIX remove files and sub directories from the test directory
{
	struct dirent *entry;           // entry from readdir()
	struct stat statbuf;            // entry from stat()
	vector<string> subDirectory;    // sub directories of this directory

	// errno is defined in <errno.h> and is set for errors in opendir, readdir, or stat
	errno = 0;

	// open directory stream
	DIR *dp = opendir(directory.c_str());
	if (errno)
		ASTYLE_ABORT(string(strerror(errno))
					 +"\nCannot open directory for clean: " + directory);

	// remove files and sub directories
	while ((entry = readdir(dp)) != NULL)
	{
		// get file status
		string entryFilepath = directory + '/' + entry->d_name;
		stat(entryFilepath.c_str(), &statbuf);
		if (errno)
			ASTYLE_ABORT(string(strerror(errno))
						 +"\nError getting file status for clean: " + directory);
		// skip these
		if (strcmp(entry->d_name, ".") == 0
				|| strcmp(entry->d_name, "..") == 0)
			continue;
		// clean and remove sub directories
		if (S_ISDIR(statbuf.st_mode))
		{
			string subDirectoryPath = directory + '/' + entry->d_name;
			cleanTestDirectory(subDirectoryPath);
			rmdir(subDirectoryPath.c_str());
			if (errno)
				ASTYLE_ABORT(string(strerror(errno))
							 +"\nCannot remove directory for clean: " + subDirectoryPath);
			continue;
		}

		// remove the file
		if (S_ISREG(statbuf.st_mode))
		{
			string filePathName = directory + '/' + entry->d_name;
			remove(filePathName.c_str());
			if (errno)
				ASTYLE_ABORT(string(strerror(errno))
							 + "\nCannot remove file for clean: " + filePathName);
		}
	}
	closedir(dp);

	if (errno)
		ASTYLE_ABORT(string(strerror(errno))
					 + "\nError processing directory for clean: " + directory);
}
#endif

void createConsoleGlobalObject(ASFormatter& formatter)
// creates the g_console object
{
	if (g_console)
	{
		systemPause("Global object not deleted by previous test.");
		deleteConsoleGlobalObject();
	}
	g_console = new ASConsole(formatter);
}

void createTestDirectory(const string &dirPath)
// create a test directory
{
#ifdef _WIN32
	BOOL ok = ::CreateDirectory(dirPath.c_str(), NULL);
	if (!ok && GetLastError() != ERROR_ALREADY_EXISTS)
	{
		displayLastError();
		ASTYLE_ABORT("Cannot create directory: " + dirPath)
	}
#else
	mkdir(dirPath.c_str(), 00770);
	chmod(dirPath.c_str(), 00770);
#endif
		// clean a pre-existing directory
		cleanTestDirectory(dirPath);
}

void createTestFile(const string& testFilePath, const char* testFileText, int size /*0*/)
// write a test file to the test directory
// the size option is for 16 and 32 bit UTF files with NULs in the text
{
	// verify test directory
	string testDir = getTestDirectory();
	if (testFilePath.compare(0, testDir.length(), testDir) != 0
			|| !(testFilePath[testDir.length()] == '/'
				 || testFilePath[testDir.length()] == '\\'))
		ASTYLE_ABORT("File not written to test directory: " + testFilePath);

	// write the output file
	ofstream fout(testFilePath.c_str(), ios::binary | ios::trunc);
	if (!fout)
		ASTYLE_ABORT("Cannot open output file: " + testFilePath);

	if (size == 0)
		fout << testFileText;
	else
		fout.write(testFileText, size);
	fout.close();
}

void deleteConsoleGlobalObject()
// deletes the g_console object
{
	delete g_console;
	g_console = NULL;
	// check global error display
	if (_err != &cerr)
		systemPause("_err ostream not replaced");
	_err = &cerr;
}

string getCurrentDirectory()
// get the current working directory
{
	bool gotDirectory = true;
	string currentDirectory;
#ifdef _WIN32
	char currdir[MAX_PATH];
	currdir[0] = '\0';
	if (GetCurrentDirectory(sizeof(currdir), currdir))
		currentDirectory = currdir;
	else
		gotDirectory = false;
#else
	char* currdir = getenv("PWD");
	if (currdir != NULL)
		currentDirectory = currdir;
	else
		gotDirectory = false;
#endif
	if (!gotDirectory)
		ASTYLE_ABORT("Cannot get current directory");
	return currentDirectory;
}

string& getTestDirectory()
// return file path of the global test directory
{
	return (g_testDirectory);
}

void removeTestDirectory(const string &dirName)
// remove a test directory
{
	cleanTestDirectory(dirName);
#ifdef _WIN32
	RemoveDirectory(dirName.c_str());
#else
	rmdir(dirName.c_str());
#endif
}

void removeTestFile(const string& testFileName)
// remove a test file
{
	errno = 0;
	remove(testFileName.c_str());
	if (errno)
		ASTYLE_ABORT(string(strerror(errno))
					 + "\nCannot remove test file: " + testFileName);
}

void setTestDirectory()
// set the global variable g_testDirectory
{
	// define unicode and multi-byte variables
#ifdef _WIN32
	string envVar = "%USERPROFILE%";
	string varName = envVar.substr(1, envVar.length()-2);
#else
	string envVar = "$HOME";
	string varName = envVar.substr(1, envVar.length()-1);
#endif
	// get replacement for environment variable
	char* envPath = getenv(varName.c_str());
	if (envPath == NULL)
		ASTYLE_ABORT("Bad char in wide character string: " + envVar);
	// replace the environment variable
	size_t iEnv = g_testDirectory.find(envVar.c_str());
	if (iEnv == string::npos)
		ASTYLE_ABORT("Cannot find environmental variable: " + envVar);
	g_testDirectory.replace(iEnv, envVar.length(), envPath);
	// check that the primary directory exists
	size_t iPrim = g_testDirectory.find_last_of("\\/");
	if (iPrim == string::npos)
		ASTYLE_ABORT("Cannot find ending separator: " + g_testDirectory);
	string primaryDirectory = g_testDirectory.substr(0, iPrim);
	struct stat stBuf;
	if (stat(primaryDirectory.c_str(), &stBuf) == -1)
		ASTYLE_ABORT("Primary directory does not exist: " + primaryDirectory);
}

//void setTestDirectoryX(char *argv)
//// set the global variable *g_testDirectory
//{
//	assert(g_testDirectory == NULL);
//	string testDirectory = argv;
//
//	// remove "build" directories
//#ifdef _WIN32
//	string buildDirectory = "\\build\\";
//	string separator = "\\";
//#else
//	string buildDirectory = "/build/";
//	string separator = "/";
//#endif
//	size_t i = testDirectory.rfind(buildDirectory);
//	if (i == string::npos)
//		ASTYLE_ABORT("Cannot extract test directory from: " + testDirectory);
//	testDirectory = testDirectory.substr(0, i);
//
//	// create global *_projectDirectory
//	testDirectory += separator + "ut-testcon";
//	g_testDirectory = new string(testDirectory);
//}

void systemAbort(const string& message)
// accept keyboard input then abort
// assures a console message is noticed
{
	systemPause(message);
	exit(EXIT_FAILURE);
}

void systemPause(const string& message)
// accept keyboard input to continue
// assures a console message is noticed
{
	if (message.length() > 4)
		cout << message << endl;
#ifdef _WIN32
	system("pause");
#else
	cout << "Press ENTER to continue." << endl;
	if (system("read x") > 0)
		cout << "Bad return from 'system' call." << endl;
#endif
}

bool writeOptionsFile(const string& optionsFileName, const char* fileIn)
// write a test options file
{
	// write the file
	ofstream fileOut(optionsFileName.c_str(), ios::binary);
	if (!fileOut)
	{
		systemPause("Cannot write options test file: " + optionsFileName);
		return false;
	}
	fileOut << fileIn;
	fileOut.close();
	return true;
}