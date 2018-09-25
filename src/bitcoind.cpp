// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "chainparams.h"
#include "clientversion.h"
#include "httprpc.h"
#include "httpserver.h"
#include "init.h"
#include "noui.h"
#include "rpc/server.h"
#include "scheduler.h"
#include "util.h"
#include "utilstrencodings.h"

#include "omnicore/utilsui.h"
#include "omnicore/omnicore.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include "rpc/register.h"

#include <stdio.h>
//#include "createpayload.h"
/* Introduction text for doxygen: */

/*! \mainpage Developer documentation
 *
 * \section intro_sec Introduction
 *
 * This is the developer documentation of the reference client for an experimental new digital currency called Bitcoin (https://www.bitcoin.org/),
 * which enables instant payments to anyone, anywhere in the world. Bitcoin uses peer-to-peer technology to operate
 * with no central authority: managing transactions and issuing money are carried out collectively by the network.
 *
 * The software is a community-driven open source project, released under the MIT license.
 *
 * \section Navigation
 * Use the buttons <code>Namespaces</code>, <code>Classes</code> or <code>Files</code> at the top of the page to start navigating the code.
 */

static bool fDaemon;

void WaitForShutdown(boost::thread_group* threadGroup)
{
    bool fShutdown = ShutdownRequested();
    // Tell the main threads to shutdown.
    while (!fShutdown) {
        MilliSleep(200);
        fShutdown = ShutdownRequested();
    }
    if (threadGroup) {
        Interrupt(*threadGroup);
        threadGroup->join_all();
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Start
//
bool AppInitEx(char* netName)
{
	if (!boost::filesystem::is_directory(GetDataDir(false))) {
		return false;
    }
	if( netName ){
		SelectParams(netName);
	}

	RegisterAllCoreRPCCommands(tableRPC);
	SetRPCWarmupFinished();
	mastercore_init_ex();
	return true;
}
//////////////////////////////////////////////////////////////////////////////
//
// Start
//
bool AppInit(int argc, char* argv[])
{
    boost::thread_group threadGroup;
    CScheduler scheduler;

    bool fRet = false;

    //
    // Parameters
    //
    // If Qt is used, parameters/bitcoin.conf are parsed in qt/bitcoin.cpp's main()
    ParseParameters(argc, argv);

    // Process help and version before taking care about datadir
    if (mapArgs.count("-?") || mapArgs.count("-h") || mapArgs.count("-help") || mapArgs.count("-version")) {
        std::string strUsage = strprintf(_("%s Daemon"), _(PACKAGE_NAME)) + " " + _("version") + " " + FormatFullVersion() + "\n";

        if (mapArgs.count("-version")) {
            strUsage += FormatParagraph(LicenseInfo());
        } else {
            strUsage += "\n" + _("Usage:") + "\n" +
                        "  omnicored [options]                     " + strprintf(_("Start %s Daemon"), _(PACKAGE_NAME)) + "\n";

            strUsage += "\n" + HelpMessage(HMM_BITCOIND);
        }

        fprintf(stdout, "%s", strUsage.c_str());
        return true;
    }

    try {
        if (!boost::filesystem::is_directory(GetDataDir(false))) {
            fprintf(stderr, "Error: Specified data directory \"%s\" does not exist.\n", mapArgs["-datadir"].c_str());
            return false;
        }
        try {
            ReadConfigFile(mapArgs, mapMultiArgs);
        } catch (const std::exception& e) {
            fprintf(stderr, "Error reading configuration file: %s\n", e.what());
            return false;
        }
        // Check for -testnet or -regtest parameter (Params() calls are only valid after this clause)
        try {
            SelectParams(ChainNameFromCommandLine());
        } catch (const std::exception& e) {
            fprintf(stderr, "Error: %s\n", e.what());
            return false;
        }

        // Command-line RPC
        bool fCommandLine = false;
        for (int i = 1; i < argc; i++)
            if (!IsSwitchChar(argv[i][0]) && !boost::algorithm::istarts_with(argv[i], "bitcoin:"))
                fCommandLine = true;

        if (fCommandLine) {
            fprintf(stderr, "Error: There is no RPC client functionality in omnicored anymore. Use the omnicore-cli utility instead.\n");
            exit(EXIT_FAILURE);
        }
#ifndef WIN32
        fDaemon = GetBoolArg("-daemon", false);
        if (fDaemon) {
            fprintf(stdout, "Omni Core server starting\n");

            // Daemonize
            pid_t pid = fork();
            if (pid < 0) {
                fprintf(stderr, "Error: fork() returned %d errno %d\n", pid, errno);
                return false;
            }
            if (pid > 0) // Parent process, pid is child process id
            {
                return true;
            }
            // Child process falls through to rest of initialization

            pid_t sid = setsid();
            if (sid < 0)
                fprintf(stderr, "Error: setsid() returned %d errno %d\n", sid, errno);
        }
#endif
        SoftSetBoolArg("-server", true);

        // Set this early so that parameter interactions go to console
        InitLogging();
        InitParameterInteraction();
        fRet = AppInit2(threadGroup, scheduler);
    } catch (const std::exception& e) {
        PrintExceptionContinue(&e, "AppInit()");
    } catch (...) {
        PrintExceptionContinue(NULL, "AppInit()");
    }

    if (!fRet) {
        Interrupt(threadGroup);
        // threadGroup.join_all(); was left out intentionally here, because we didn't re-test all of
        // the startup-failure cases to make sure they don't result in a hang due to some
        // thread-blocking-waiting-for-another-thread-during-startup case
    } else {
        WaitForShutdown(&threadGroup);
    }
    Shutdown();

    return fRet;
}


int main_actual(int argc, char* argv[])
{
    SetupEnvironment();

    // Indicate no-UI mode
    fQtMode = false;

    // Connect bitcoind signal handlers
    noui_connect();

    return (AppInit(argc, argv) ? EXIT_SUCCESS : EXIT_FAILURE);
}

//
//int parse_cmdline(char* line, char*** argvp)
//{
//    char** argv = (char**)malloc(sizeof(char*));
//    int argc = 0;
//    while (*line != '\0') {
//        char quote = 0;
//        while (strchr("\t ", *line)) /* Skips white spaces */
//            line++;
//        if (!*line)
//            break;
//        argv = (char**)realloc(argv, (argc + 2) * sizeof(char*)); /* Starts a new parameter */
//        if (*line == '"') {
//            quote = '"';
//            line++;
//        }
//        argv[argc++] = line;
//    more:
//        while (*line && !strchr("\t ", *line))
//            line++;
//
//        if (line > argv[argc - 1] && line[-1] == quote) // End of quoted parameter
//            line[-1] = 0;
//        else if (*line && quote) { // Space within a quote
//
//            line++;
//            goto more;
//        } else // End of unquoted parameter
//            if (*line)
//            *line++ = 0;
//    }
//    argv[argc] = NULL;
//    *argvp = argv;
//    return argc;
//}

 
        /*
//below add by ycj after 20180908
typedef char* (*FunGoJsonCmdReq)(char*);
FunGoJsonCmdReq gFunGoJsonCmdReq = NULL;

 std::string HTTPReq_JSONRPC_Simple(const std::string& strReq);//extern

extern "C" __declspec(dllexport) char* JsonCmdReq(char* pcReq)
{
	
	{"jsonrpc":"1.0","method":"omni_sendissuancefixed","params":["Tsk6gAJ7X9wjihFPo4nt5HHa9GNZysTyugn",2,1,0,"Companies","Bitcoin Mining","Quantum Miner","","","1000000"],"id":1}
	
	

	UniValue valRequest;
        if (!valRequest.read(pcReq))
	{
			if (!valRequest["method"].isNull())
			{
				std::string method = valRequest["method"].getValStr();
				if (method == "omni_sendissuancefixed") 
				{
                     std::vector<unsigned char> payload = CreatePayload_IssuanceFixed(ecosystem, type, previousId, category, subcategory, name, url, data, amount);

                     std::vector<unsigned char> vchData;
                     std::vector<unsigned char> vchOmBytes = GetOmMarker();
                     vchData.insert(vchData.end(), vchOmBytes.begin(), vchOmBytes.end());
                     vchData.insert(vchData.end(), payload.begin(), payload.end());
				}
			}

	}
     
    std::string strReq = std::string(pcReq);
    std::string strReply = HTTPReq_JSONRPC_Simple(strReq);
    static char acTemp[1024000];
    memset(acTemp, 0, sizeof(0));
    strncpy(acTemp, strReply.c_str(), sizeof(acTemp) - 1);

	printf("in C Reply acTemp=%s", acTemp);

    return acTemp;
}

extern "C" __declspec(dllexport) void SetCallback(UINT uiIndx,void* pGoJsonCmdReq)
{//	now just set one callback function,other may add later
    gFunGoJsonCmdReq = (FunGoJsonCmdReq)pGoJsonCmdReq;

	if (gFunGoJsonCmdReq == NULL) {
        printf("in C gFunGoJsonCmdReq==NULL");
        return;
    }
		
    char* pcRsp = gFunGoJsonCmdReq("{\"jsonrpc\" : \"1.0\", \"method\" : \"getinfo\", \"params\" : [], \"id\" : 1}");
    printf("in C gFunGoJsonCmdReq pcRsp=%s", pcRsp);
}

extern "C" __declspec(dllexport) void OmniStart(char* pcArgs)
{
    printf(" in OmniStart\n");

	char** argv;
    int argc = parse_cmdline(pcArgs, &argv);


    //below copy from main()
    //char* argv[] = {"exeName", "-regtest", "-txindex"};
    SetupEnvironment();
    fQtMode = false; // Indicate no-UI mode
    noui_connect();  // Connect bitcoind signal handlers
    AppInit(argc, argv);
    //main(3, argv);
}


*/