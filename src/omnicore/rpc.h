/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   rpc.h
 * Author: dexx
 *
 * Created on 13. Oktober 2016, 23:32
 */

#ifndef RPC_H
#define RPC_H

#include "omnicore/dbspinfo.h"

/** Throws a JSONRPCError, depending on error code. */
void PopulateFailure(int error);

void PropertyToJSON(const CMPSPInfo::Entry& sProperty, UniValue& property_obj);
UniValue omni_getbalance(const UniValue& params, bool fHelp);
#endif /* RPC_H */

