#include <iostream>  // cout
#include <assert.h>  // assert()
#include <string.h>  // strstr()
#include <vector>
#include <stdlib.h>  // system()
#include <stdio.h>
#include <direct.h>  // _chdir()
#include <sstream>   // stringstream
#include <angelscript.h>
#include "../../../add_on/scriptbuilder/scriptbuilder.h"
#include "../../../add_on/scriptstdstring/scriptstdstring.h"
#include "../../../add_on/scriptarray/scriptarray.h"
#include "../../../add_on/scriptdictionary/scriptdictionary.h"
#include "../../../add_on/scriptfile/scriptfile.h"
#include "../../../add_on/scriptfile/scriptfilesystem.h"
#include "../../../add_on/scripthelper/scripthelper.h"
#include "../../../add_on/debugger/debugger.h"
#include "../../../add_on/contextmgr/contextmgr.h"
#include "../../../add_on/datetime/datetime.h"

#ifdef _WIN32
#include <Windows.h> // WriteConsoleW
#include <TlHelp32.h> // CreateToolhelp32Snapshot, Process32First, Process32Next

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

#if defined(_MSC_VER)
#include <crtdbg.h>   // MSVC debugging routines
#endif

using namespace std;

// Function prototypes
int               ConfigureEngine(asIScriptEngine *engine);
void              InitializeDebugger(asIScriptEngine *engine);
int               CompileScript(asIScriptEngine *engine, const char *scriptFile);
int               ExecuteScript(asIScriptEngine *engine, const char *scriptFile);
void              MessageCallback(const asSMessageInfo *msg, void *param);
asIScriptContext *RequestContextCallback(asIScriptEngine *engine, void *param);
void              ReturnContextCallback(asIScriptEngine *engine, asIScriptContext *ctx, void *param);
void              PrintString(const string &str);
string            GetInput();
int               ExecSystemCmd(const string &cmd);
int               ExecSystemCmd(const string &str, string &out);
CScriptArray     *GetCommandLineArgs();
void              SetWorkDir(const string &file);
void              WaitForUser();
int               PragmaCallback(const string &pragmaText, CScriptBuilder &builder, void *userParam);

// The command line arguments
CScriptArray *g_commandLineArgs = 0;
int           g_argc = 0;
char        **g_argv = 0;

// The context manager is used to manage the execution of co-routines
CContextMgr *g_ctxMgr = 0;

// The debugger is used to debug the script
bool       g_doDebug = false;
CDebugger *g_dbg = 0;

// Context pool
vector<asIScriptContext*> g_ctxPool;

int main(int argc, char **argv)
{
#if defined(_MSC_VER)
	// Tell MSVC to report any memory leaks
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
	_CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);

	// Use _CrtSetBreakAlloc(n) to find a specific memory leak
#endif

#if defined(_WIN32)
	// Turn on support for virtual terminal sequences to add support for colored text in the console
	// Ref: https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
	// Ref: https://stackoverflow.com/questions/2048509/how-to-echo-with-different-colors-in-the-windows-command-line
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
		return -1;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
		return -1;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode))
		return -1;
#endif

	int r;

	// Validate the command line arguments
	bool argsValid = true;
	if( argc < 2 ) 
		argsValid = false;
	else if( argc == 2 && strcmp(argv[1], "-d") == 0 )
		argsValid = false;

	if( !argsValid )
	{
		cout << "AngelScript command line runner. Version " << ANGELSCRIPT_VERSION_STRING << endl << endl;
		cout << "Usage: " << endl;
		cout << "asrun [-d] <script file> [<args>]" << endl;
		cout << " -d             inform if the script should be runned with debug" << endl;
		cout << " <script file>  is the script file that should be runned" << endl;
		cout << " <args>         zero or more args for the script" << endl;

		WaitForUser();
		return -1;
	}

	// Create the script engine
	asIScriptEngine *engine = asCreateScriptEngine();
	if( engine == 0 )
	{
		cout << "Failed to create script engine." << endl;
		return -1;
	}

	// Configure the script engine with all the functions, 
	// and variables that the script should be able to use.
	r = ConfigureEngine(engine);
	if( r < 0 ) return -1;
	
	// Check if the script is to be debugged
	if( strcmp(argv[1], "-d") == 0 )
		g_doDebug = true;

	// Store the command line arguments for the script
	int scriptArg = g_doDebug ? 2 : 1;
	g_argc = argc - (scriptArg + 1);
	g_argv = argv + (scriptArg + 1);

	// Set the current work dir according to the script's location
	SetWorkDir(argv[scriptArg]);

	// Compile the script code
	r = CompileScript(engine, argv[scriptArg]);
	if (r < 0)
	{
		WaitForUser();
		return -1;
	}

	// Execute the script
	r = ExecuteScript(engine, argv[scriptArg]);
	
	// Shut down the engine
	if( g_commandLineArgs )
		g_commandLineArgs->Release();
	engine->ShutDownAndRelease();

	if (r < 0)
		WaitForUser();
	return r;
}

// This message callback is used by the engine to send compiler messages
void MessageCallback(const asSMessageInfo *msg, void *param)
{
	const char *type = "ERR ";
	if( msg->type == asMSGTYPE_WARNING ) 
		type = "WARN";
	else if( msg->type == asMSGTYPE_INFORMATION ) 
		type = "INFO";

	printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
}

// This function will register the application interface
int ConfigureEngine(asIScriptEngine *engine)
{
	int r;

	// The script compiler will send any compiler messages to the callback
	r = engine->SetMessageCallback(asFUNCTION(MessageCallback), 0, asCALL_CDECL); assert( r >= 0 );

	// Register the standard add-ons that we'll allow the scripts to use
	RegisterStdString(engine);
	RegisterScriptArray(engine, false);
	RegisterStdStringUtils(engine);
	RegisterScriptDictionary(engine);
	RegisterScriptDateTime(engine);
	RegisterScriptFile(engine);
	RegisterScriptFileSystem(engine);
	RegisterExceptionRoutines(engine);

	// Register a couple of extra functions for the scripts
	r = engine->RegisterGlobalFunction("void print(const string &in)", asFUNCTION(PrintString), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("string getInput()", asFUNCTION(GetInput), asCALL_CDECL); assert(r >= 0);
	r = engine->RegisterGlobalFunction("array<string> @getCommandLineArgs()", asFUNCTION(GetCommandLineArgs), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("int exec(const string &in)", asFUNCTIONPR(ExecSystemCmd, (const string &), int), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterGlobalFunction("int exec(const string &in, string &out)", asFUNCTIONPR(ExecSystemCmd, (const string &, string &), int), asCALL_CDECL); assert( r >= 0 );

	// Setup the context manager and register the support for co-routines
	g_ctxMgr = new CContextMgr();
	g_ctxMgr->RegisterCoRoutineSupport(engine);

	// Tell the engine to use our context pool. This will also 
	// allow us to debug internal script calls made by the engine
	r = engine->SetContextCallbacks(RequestContextCallback, ReturnContextCallback, 0); assert( r >= 0 );

	// TODO: There should be an option of outputting the engine 
	//       configuration for use with the offline compiler asbuild.
	//       It should then be possible to execute pre-compiled bytecode.

	return 0;
}

// This is the to-string callback for the string type
std::string StringToString(void *obj, int /* expandMembers */, CDebugger * /* dbg */)
{
	// We know the received object is a string
	std::string *val = reinterpret_cast<std::string*>(obj);

	// Format the output string
	// TODO: Should convert non-readable characters to escape sequences
	std::stringstream s;
	s << "(len=" << val->length() << ") \"";
	if( val->length() < 20 )
		s << *val << "\"";
	else
		s << val->substr(0, 20) << "...";

	return s.str();
}

// This is the to-string callback for the array type
// This is generic and will take care of all template instances based on the array template
std::string ArrayToString(void *obj, int expandMembers, CDebugger *dbg)
{
	CScriptArray *arr = reinterpret_cast<CScriptArray*>(obj);

	std::stringstream s;
	s << "(len=" << arr->GetSize() << ")";
	
	if( expandMembers > 0 )
	{
		s << " [";
		for( asUINT n = 0; n < arr->GetSize(); n++ )
		{
			s << dbg->ToString(arr->At(n), arr->GetElementTypeId(), expandMembers - 1, arr->GetArrayObjectType()->GetEngine());
			if( n < arr->GetSize()-1 )
				s << ", ";
		}
		s << "]";
	}

	return s.str();
}

// This is the to-string callback for the dictionary type
std::string DictionaryToString(void *obj, int expandMembers, CDebugger *dbg)
{
	CScriptDictionary *dic = reinterpret_cast<CScriptDictionary*>(obj);
 
	std::stringstream s;
	s << "(len=" << dic->GetSize() << ")";
 
	if( expandMembers > 0 )
	{
		s << " [";
		asUINT n = 0;
		for( CScriptDictionary::CIterator it = dic->begin(); it != dic->end(); it++, n++ )
		{
			s << "[" << it.GetKey() << "] = ";

			// Get the type and address of the value
			const void *val = it.GetAddressOfValue();
			int typeId = it.GetTypeId();

			// Use the engine from the currently active context (if none is active, the debugger
			// will use the engine held inside it by default, but in an environment where there
			// multiple engines this might not be the correct instance).
			asIScriptContext *ctx = asGetActiveContext();

			s << dbg->ToString(const_cast<void*>(val), typeId, expandMembers - 1, ctx ? ctx->GetEngine() : 0);
			
			if( n < dic->GetSize() - 1 )
				s << ", ";
		}
		s << "]";
	}
 
	return s.str();
}

// This is the to-string callback for the dictionary type
std::string DateTimeToString(void *obj, int expandMembers, CDebugger *dbg)
{
	CDateTime *dt = reinterpret_cast<CDateTime*>(obj);
	
	std::stringstream s;
	s << "{" << dt->getYear() << "-" << dt->getMonth() << "-" << dt->getDay() << " ";
	s << dt->getHour() << ":" << dt->getMinute() << ":" << dt->getSecond() << "}";
	
	return s.str(); 
}

// This function initializes the debugger and let's the user set initial break points
void InitializeDebugger(asIScriptEngine *engine)
{
	// Create the debugger instance and store it so the context callback can attach
	// it to the scripts contexts that will be used to execute the scripts
	g_dbg = new CDebugger();

	// Let the debugger hold an engine pointer that can be used by the callbacks
	g_dbg->SetEngine(engine);

	// Register the to-string callbacks so the user can see the contents of strings
	g_dbg->RegisterToStringCallback(engine->GetTypeInfoByName("string"), StringToString);
	g_dbg->RegisterToStringCallback(engine->GetTypeInfoByName("array"), ArrayToString);
	g_dbg->RegisterToStringCallback(engine->GetTypeInfoByName("dictionary"), DictionaryToString);
	g_dbg->RegisterToStringCallback(engine->GetTypeInfoByName("datetime"), DateTimeToString);

	// Allow the user to initialize the debugging before moving on
	cout << "Debugging, waiting for commands. Type 'h' for help." << endl;
	g_dbg->TakeCommands(0);
}

// This is where the script is compiled into bytecode that can be executed
int CompileScript(asIScriptEngine *engine, const char *scriptFile)
{
	int r;

	// We will only initialize the global variables once we're 
	// ready to execute, so disable the automatic initialization
	engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, false);

	CScriptBuilder builder;

	// Set the pragma callback so we can detect if the script needs debugging
	builder.SetPragmaCallback(PragmaCallback, 0);

	// Compile the script
	r = builder.StartNewModule(engine, "script");
	if( r < 0 ) return -1;

	r = builder.AddSectionFromFile(scriptFile);
	if( r < 0 ) return -1;

	r = builder.BuildModule();
	if( r < 0 )
	{
		engine->WriteMessage(scriptFile, 0, 0, asMSGTYPE_ERROR, "Script failed to build");
		return -1;
	}

	return 0;
}

// Execute the script by calling the main() function
int ExecuteScript(asIScriptEngine *engine, const char *scriptFile)
{
	asIScriptModule *mod = engine->GetModule("script", asGM_ONLY_IF_EXISTS);
	if( !mod ) return -1;

	// Find the main function
	asIScriptFunction *func = mod->GetFunctionByDecl("int main()");
	if( func == 0 )
	{
		// Try again with "void main()"
		func = mod->GetFunctionByDecl("void main()");
	}

	if( func == 0 )
	{
		engine->WriteMessage(scriptFile, 0, 0, asMSGTYPE_ERROR, "Cannot find 'int main()' or 'void main()'");
		return -1;
	}

	if(g_doDebug)
		InitializeDebugger(engine);

	// Once we have the main function, we first need to initialize the global variables
	// Since we've set up the request context callback we will be able to debug the 
	// initialization without passing in a pre-created context
	int r = mod->ResetGlobalVars(0);
	if( r < 0 )
	{
		engine->WriteMessage(scriptFile, 0, 0, asMSGTYPE_ERROR, "Failed while initializing global variables");
		return -1;
	}

	// Set up a context to execute the script
	// The context manager will request the context from the 
	// pool, which will automatically attach the debugger
	asIScriptContext *ctx = g_ctxMgr->AddContext(engine, func, true);

	// Execute the script until completion
	// The script may create co-routines. These will automatically
	// be managed by the context manager
	while( g_ctxMgr->ExecuteScripts() );

	// Check if the main script finished normally
	r = ctx->GetState();
	if( r != asEXECUTION_FINISHED )
	{
		if( r == asEXECUTION_EXCEPTION )
		{
			cout << "The script failed with an exception" << endl;
			cout << GetExceptionInfo(ctx, true).c_str();
			r = -1;
		}
		else if( r == asEXECUTION_ABORTED )
		{
			cout << "The script was aborted" << endl;
			r = -1;
		}
		else
		{
			cout << "The script terminated unexpectedly (" << r << ")" << endl;
			r = -1;
		}
	}
	else
	{
		// Get the return value from the script
		if( func->GetReturnTypeId() == asTYPEID_INT32 )
		{
			r = *(int*)ctx->GetAddressOfReturnValue();
		}
		else
			r = 0;
	}

	// Return the context after retrieving the return value
	g_ctxMgr->DoneWithContext(ctx);

	// Destroy the context manager
	if( g_ctxMgr )
	{
		delete g_ctxMgr;
		g_ctxMgr = 0;
	}

	// Before leaving, allow the engine to clean up remaining objects by 
	// discarding the module and doing a full garbage collection so that 
	// this can also be debugged if desired
	mod->Discard();
	engine->GarbageCollect();

	// Release all contexts that have been allocated
#if AS_CAN_USE_CPP11
	for( auto ctx : g_ctxPool )
		ctx->Release();
#else
	for( size_t n = 0; n < g_ctxPool.size(); n++ )
		g_ctxPool[n]->Release();
#endif

	g_ctxPool.clear();

	// Destroy debugger
	if( g_dbg )
	{
		delete g_dbg;
		g_dbg = 0;
	}

	return r;
}

// This little function allows the script to print a string to the screen
void PrintString(const string &str)
{
#ifdef _WIN32
	// Unless the std out has been redirected to file we'll need to allow Windows to convert
	// the text to the current locale so that characters will be displayed appropriately.
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	if( console != INVALID_HANDLE_VALUE && GetConsoleMode(console, &mode) != 0 ) 
	{
		// We're writing to a console window, so convert the UTF8 string to UTF16 and write with
		// WriteConsoleW. Windows will then automatically display the characters correctly according
		// to the user's settings
		// TODO: buffer size needs to be dynamic to handle large strings
		//       must split the string correctly between UTF8 unicode sequences
		wchar_t bufUTF16[100000];
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, bufUTF16, 100000);
		WriteConsoleW(console, bufUTF16, lstrlenW(bufUTF16), 0, 0);
	}
	else
	{
		// We're writing to a file, so just write the bytes as-is without any conversion
		cout << str;
	}
#else
	cout << str;
#endif
}

// Retrieve a line from stdin
string GetInput()
{
	string line;
	getline(cin, line);
	return line;
}

// TODO: Perhaps it might be interesting to implement pipes so that the script can receive input from stdin, 
//       or execute commands that return output similar to how popen is used

// This function calls the system command, captures the stdout and return the status
// Return of -1 indicates an error. Else the return code is the return status of the executed command
// ref: https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
int ExecSystemCmd(const string &str, string &out)
{
#ifdef _WIN32
	// Convert the command to UTF16 to properly handle unicode path names
	wchar_t bufUTF16[10000];
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, bufUTF16, 10000);
	
	// Create a pipe to capture the stdout from the system command
	HANDLE pipeRead, pipeWrite;
	SECURITY_ATTRIBUTES secAttr = {0};
	secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAttr.bInheritHandle = TRUE;
    secAttr.lpSecurityDescriptor = NULL;
    if( !CreatePipe(&pipeRead, &pipeWrite, &secAttr, 0) )
		return -1;
	
	// Start the process for the system command, informing the pipe to 
	// capture stdout, and also to skip showing the command window
	STARTUPINFOW si = {0};
	si.cb          = sizeof(STARTUPINFOW);
    si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdOutput  = pipeWrite;
    si.hStdError   = pipeWrite;
    si.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION pi = {0};
	BOOL success = CreateProcessW(NULL, bufUTF16, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
    if( !success )
    {
        CloseHandle(pipeWrite);
        CloseHandle(pipeRead);
        return -1;
    }
	
	// Run the command until the end, while capturing stdout
	for(;;)
	{
		// Wait for a while to allow the process to work
		DWORD ret = WaitForSingleObject(pi.hProcess, 50);
		
		// Read from the stdout if there is any data
        for (;;)
        {
            char buf[1024];
            DWORD readCount = 0;
            DWORD availCount = 0;

            if( !::PeekNamedPipe(pipeRead, NULL, 0, NULL, &availCount, NULL) )
                break;

            if( availCount == 0 )
                break;

            if( !::ReadFile(pipeRead, buf, sizeof(buf) - 1 < availCount ? sizeof(buf) - 1 : availCount, &readCount, NULL) || !readCount )
                break;

            buf[readCount] = 0;
            out += buf;
        }

		// End the loop if the process finished
		if( ret == WAIT_OBJECT_0 )
			break;
	}
	
	// Get the return status from the process
	DWORD status = 0;
	GetExitCodeProcess(pi.hProcess, &status);
	
	CloseHandle(pipeRead);
	CloseHandle(pipeWrite);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
		
	return status;
#else
	// TODO: Implement suppor for ExecSystemCmd(const string &, string&) on non-Windows platforms
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx )
		ctx->SetException("Oops! This is not yet implemented on non-Windows platforms. Sorry!\n");
	return -1;	
#endif
}

// This function simply calls the system command and returns the status
// Return of -1 indicates an error. Else the return code is the return status of the executed command
int ExecSystemCmd(const string &str)
{
	// Check if the command line processor is available
	if( system(0) == 0 )
	{
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
			ctx->SetException("Command interpreter not available\n");
		return -1;
	}

#ifdef _WIN32
	// Convert the command to UTF16 to properly handle unicode path names
	wchar_t bufUTF16[10000];
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, bufUTF16, 10000);
	return _wsystem(bufUTF16);
#else
	return system(cmd.c_str());
#endif
}

// This function returns the command line arguments that were passed to the script
CScriptArray *GetCommandLineArgs()
{
	if( g_commandLineArgs )
	{
		g_commandLineArgs->AddRef();
		return g_commandLineArgs;
	}

	// Obtain a pointer to the engine
	asIScriptContext *ctx = asGetActiveContext();
	asIScriptEngine *engine = ctx->GetEngine();

	// Create the array object
	asITypeInfo *arrayType = engine->GetTypeInfoById(engine->GetTypeIdByDecl("array<string>"));
	g_commandLineArgs = CScriptArray::Create(arrayType, (asUINT)0);

	// Find the existence of the delimiter in the input string
	for( int n = 0; n < g_argc; n++ )
	{
		// Add the arg to the array
		g_commandLineArgs->Resize(g_commandLineArgs->GetSize()+1);
		((string*)g_commandLineArgs->At(n))->assign(g_argv[n]);
	}

	// Return the array by handle
	g_commandLineArgs->AddRef();
	return g_commandLineArgs;
}

// This function is called by the engine whenever a context is needed for an 
// execution we use it to pool contexts and to attach the debugger if needed.
asIScriptContext *RequestContextCallback(asIScriptEngine *engine, void * /*param*/)
{
	asIScriptContext *ctx = 0;

	// Check if there is a free context available in the pool
	if( g_ctxPool.size() )
	{
		ctx = g_ctxPool.back();
		g_ctxPool.pop_back();
	}
	else
	{
		// No free context was available so we'll have to create a new one
		ctx = engine->CreateContext();
	}

	// Attach the debugger if needed
	if( ctx && g_dbg )
	{
		// Set the line callback for the debugging
		ctx->SetLineCallback(asMETHOD(CDebugger, LineCallback), g_dbg, asCALL_THISCALL);
	}

	return ctx;
}

// This function is called by the engine when the context is no longer in use
void ReturnContextCallback(asIScriptEngine *engine, asIScriptContext *ctx, void * /*param*/)
{
	// We can also check for possible script exceptions here if so desired

	// Unprepare the context to free any objects it may still hold (e.g. return value)
	// This must be done before making the context available for re-use, as the clean
	// up may trigger other script executions, e.g. if a destructor needs to call a function.
	ctx->Unprepare();

	// Place the context into the pool for when it will be needed again
	g_ctxPool.push_back(ctx);
}

void SetWorkDir(const string &file)
{
	_chdir(file.c_str());
}

// This function is used to allow the user to read the output to the console before exiting 
// when the process is initiated from the file explorer on Windows.
void WaitForUser()
{
#ifdef _WIN32
	// Check if the script is initiated directly from the file explorer in which
	// case it is necessary to wait for some user input before exiting so the
	// user has time to check the compiler error messages.
	// Ref: https://msdn.microsoft.com/en-us/library/windows/desktop/ms686701(v=vs.85).aspx
	DWORD parentPid = 0;
	DWORD  myPid = GetCurrentProcessId();

	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32W procEntry;
	procEntry.dwSize = sizeof(PROCESSENTRY32W);

	// Find the pid of the parent process
	Process32FirstW(hSnapShot, &procEntry);
	do
	{
		if (myPid == procEntry.th32ProcessID)
		{
			parentPid = procEntry.th32ParentProcessID;
			break;
		}
	} while (Process32NextW(hSnapShot, &procEntry));

	// Find the name of the parent process
	wstring name;
	Process32FirstW(hSnapShot, &procEntry);
	do
	{
		if (parentPid == procEntry.th32ProcessID)
		{
			name = procEntry.szExeFile;
			break;
		}
	} while (Process32NextW(hSnapShot, &procEntry));

	CloseHandle(hSnapShot);

	if (name == L"explorer.exe")
	{
		PrintString("\nPress enter to exit\n");
		GetInput();
	}
#endif
}

int PragmaCallback(const string &pragmaText, CScriptBuilder &builder, void * /*userParam*/)
{
	asIScriptEngine *engine = builder.GetEngine();

	// Filter the pragmaText so only what is of interest remains 
	// With this the user can add comments and use different whitespaces without affecting the result
	asUINT pos = 0;
	asUINT length = 0;
	string cleanText;
	while( pos < pragmaText.size() )
	{
		asETokenClass tokenClass = engine->ParseToken(pragmaText.c_str() + pos, 0, &length);
		if (tokenClass == asTC_IDENTIFIER || tokenClass == asTC_KEYWORD || tokenClass == asTC_VALUE)
		{
			string token = pragmaText.substr(pos, length);
			cleanText += " " + token;
		}
		if (tokenClass == asTC_UNKNOWN)
			return -1;
		pos += length;
	}

	// Interpret the result
	if (cleanText == " debug")
	{
		g_doDebug = true;
		return 0;
	}

	// The #pragma directive was not accepted
	return -1;
}


