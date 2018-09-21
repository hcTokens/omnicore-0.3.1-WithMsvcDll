#include "omnicoreApi.h"

#ifdef _WIN32
#ifdef MYDLL_IMPORTS
#define MYDLLAPI extern "C" __declspec(dllimport)
#else
#define MYDLLAPI extern "C" __declspec(dllexport)
#endif
#else
#define MYDLLAPI extern "C"
#endif

extern void PropertyToJSON(const CMPSPInfo::Entry& sProperty, UniValue& property_obj);



MYDLLAPI const char* JsonCmdReq(char* pcReq)
{
	UniValue root;
    if (root.read(pcReq))
	{
        if (!root["method"].isNull())
		{
			std::string method = root["method"].getValStr();
			if (method == "ProcessTx" )
			{
                 CMPTransaction mp_obj;
                 std::string Sender = root["Sender"].get_str();
                 std::string Reference = root["Reference"].get_str();

                 std::vector<unsigned char> vecTxHash = ParseHex(root["TxHash"].get_str());
				 std::vector<unsigned char> vecBlockHash = ParseHex(root["BlockHash"].get_str());

                 int64_t Block = root["Block"].get_int64();
                 int64_t Idx = root["Idx"].get_int64();
                 std::string ScriptEncode = root["ScriptEncode"].get_str();
                 std::vector<unsigned char> Script = ParseHex(ScriptEncode);
                 int64_t Time = root["Time"].get_int64();
                 int64_t Fee = root["Fee"].get_int64();

                 mp_obj.unlockLogic();
                 mp_obj.Set(uint256(vecTxHash), Block, Idx, Time);
                 mp_obj.SetBlockHash(uint256(vecBlockHash));
                 mp_obj.Set(Sender, Reference, Block, uint256(vecTxHash), Block, Idx, &(Script[0]), Script.size(), 3, Fee);
                 mp_obj.interpretPacket();
            }
			else {
                std::string strReq = std::string(pcReq);
                std::string strReply = HTTPReq_JSONRPC_Simple(strReq);
                static char acTemp[1024000];
                memset(acTemp, 0, sizeof(char) * 1024000);
                strncpy(acTemp, strReply.c_str(), strReply.size() < 1024000 ? strReply.size() : 1024000);

                printf("in C Reply acTemp=%s", acTemp);

                return acTemp;
            }
		}
	} 
    return "";
}

MYDLLAPI void SetCallback(unsigned int uiIndx, void* pGoJsonCmdReq)
{ //	now just set one callback function,other may add later
  /*
  gFunGoJsonCmdReq = (FunGoJsonCmdReq)pGoJsonCmdReq;

    if (gFunGoJsonCmdReq == NULL) {
        printf("in C gFunGoJsonCmdReq==NULL");
        return;
    }

    char* pcRsp = gFunGoJsonCmdReq("{\"jsonrpc\" : \"1.0\", \"method\" : \"getinfo\", \"params\" : [], \"id\" : 1}");
    printf("in C gFunGoJsonCmdReq pcRsp=%s", pcRsp);]
	*/
}

int parse_cmdline(char* line, char*** argvp)
{
    char** argv = (char**)malloc(sizeof(char*));
    int argc = 0;
    while (*line != '\0') {
        char quote = 0;
        while (strchr("\t ", *line)) /* Skips white spaces */
            line++;
        if (!*line)
            break;
        argv = (char**)realloc(argv, (argc + 2) * sizeof(char*)); /* Starts a new parameter */
        if (*line == '"') {
            quote = '"';
            line++;
        }
        argv[argc++] = line;
    more:
        while (*line && !strchr("\t ", *line))
            line++;

        if (line > argv[argc - 1] && line[-1] == quote) // End of quoted parameter
            line[-1] = 0;
        else if (*line && quote) { // Space within a quote

            line++;
            goto more;
        } else // End of unquoted parameter
            if (*line)
            *line++ = 0;
    }
    argv[argc] = NULL;
    *argvp = argv;
    return argc;
}

extern bool AppInit(int argc, char* argv[]);

MYDLLAPI void OmniStart(char* pcArgs)
{
  
    printf(" in OmniStart\n");

	char* argv[] = {"-exe", "-regtest", "-txindex", "-server=1", "-addnode=192.168.1.24", "-reindex-chainstate", "-debug=1"};
   //  int argc = parse_cmdline(7, &argv);


    //below copy from main()
    //char* argv[] = {"exeName", "-regtest", "-txindex"};
    SetupEnvironment();
  //  fQtMode = false; // Indicate no-UI mode
    noui_connect();  // Connect bitcoind signal handlers
    AppInit(7, argv);
    //main(3, argv);

}
