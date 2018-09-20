#include "omnicoinApi.h"


extern "C" __declspec(dllexport) const char* JsonCmdReq(char* pcReq)
{
	UniValue root;
    if (root.read(pcReq))
	{
        if (!root["method"].isNull())
		{
			std::string method = root["method"].getValStr();
			if (method == "omni_sendissuancefixed") 
			{
			   if (root["params"].size() != 10)
					return "";
                
			   std::vector<unsigned char> payload = CreatePayload_IssuanceFixed(
				   root["params"][1].get_int(),
				   root["params"][2].get_int(),
				   root["params"][3].get_int(),
				   root["params"][4].getValStr().c_str(),
				   root["params"][5].getValStr().c_str(),
				   root["params"][6].getValStr().c_str(),
				   root["params"][7].getValStr().c_str(),
				   root["params"][8].getValStr().c_str(),
                   atoi64(root["params"][9].getValStr()));

				std::vector<unsigned char> vchData;
				std::vector<unsigned char> vchOmBytes = GetOmMarker();
				vchData.insert(vchData.end(), vchOmBytes.begin(), vchOmBytes.end());
				vchData.insert(vchData.end(), payload.begin(), payload.end());
				std::string payLoad = HexStr(vchData.begin(), vchData.end());

				static char buf[1024000] = {0};
                memset(buf, 0, sizeof(char) * 1024000);
                strncpy(buf, payLoad.c_str(), payLoad.size());
				return buf;
            }
			else if (method == "ProcessTx" )
			{
                 CMPTransaction mp_obj;
                 std::string Sender = root["Sender"].get_str();
                 std::string Reference = root["Reference"].get_str();

                 std::vector<unsigned char> vecTxHash = ParseHex(root["TxHash"].get_str());
				 std::vector<unsigned char> vecBlockHash = ParseHex(root["BlockHash"].get_str());

                 INT64 Block = root["Block"].get_int64();
                 INT64 Idx = root["Idx"].get_int64();
                 std::string ScriptEncode = root["ScriptEncode"].get_str();
                 std::vector<unsigned char> Script = ParseHex(ScriptEncode);
                 INT64 Time = root["Time"].get_int64();
                 INT64 Fee = root["Fee"].get_int64();

                 mp_obj.unlockLogic();
                 mp_obj.Set(uint256(vecTxHash), Block, Idx, Time);
                 mp_obj.SetBlockHash(uint256(vecBlockHash));
                 mp_obj.Set(Sender, Reference, Block, uint256(vecTxHash), Block, Idx, &(Script[0]), Script.size(), 3, Fee);

                 mp_obj.interpretPacket();
            } 
			else if (method == "omni_listproperties"){
                UniValue response(UniValue::VARR);
                uint32_t nextSPID = mastercore::_my_sps->peekNextSPID(1);
                for (uint32_t propertyId = 1; propertyId < nextSPID; propertyId++) {
                    CMPSPInfo::Entry sp;
                    if (mastercore::_my_sps->getSP(propertyId, sp)) {
                        UniValue propertyObj(UniValue::VOBJ);
                        propertyObj.push_back(Pair("propertyid", (uint64_t)propertyId));
                        PropertyToJSON(sp, propertyObj); // name, category, subcategory, data, url, divisible

                        response.push_back(propertyObj);
                    }
                }

                uint32_t nextTestSPID = mastercore::_my_sps->peekNextSPID(2);
                for (uint32_t propertyId = TEST_ECO_PROPERTY_1; propertyId < nextTestSPID; propertyId++) {
                    CMPSPInfo::Entry sp;
                    if (mastercore::_my_sps->getSP(propertyId, sp)) {
                        UniValue propertyObj(UniValue::VOBJ);
                        propertyObj.push_back(Pair("propertyid", (uint64_t)propertyId));
                        PropertyToJSON(sp, propertyObj); // name, category, subcategory, data, url, divisible

                        response.push_back(propertyObj);
                    }
                }
                std::string ret = response.write();
                static char buf[1024000] = {0};
                memset(buf, 0, sizeof(char) * 1024000);
                strncpy(buf, ret.c_str(), ret.size());
                return buf;
            }
			else {
                std::string strReq = std::string(pcReq);
                std::string strReply = HTTPReq_JSONRPC_Simple(strReq);
                static char acTemp[1024000];
                memset(acTemp, 0, sizeof(0));
                strncpy(acTemp, strReply.c_str(), sizeof(acTemp) - 1);

                printf("in C Reply acTemp=%s", acTemp);

                return acTemp;
            }
		}
	} 
    return "";
}

extern "C" __declspec(dllexport) void SetCallback(unsigned int uiIndx, void* pGoJsonCmdReq)
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

extern "C" __declspec(dllexport) void OmniStart(char* pcArgs)
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
