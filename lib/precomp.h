
#pragma once

#include <Windows.h>
#include <MsiQuery.h>
#include <MsiDefs.h>
#include <SetupAPI.h>
#include <wtypes.h>
#include <process.h>
#include <string>
#include <fstream>
using namespace std;

#include "MsiDumpPublic.h"
#include "MsiTable.h"
#include "MsiUtils.h"

#define MSISOURCE_COMPRESSED 0x00000002

#define TEST_FLAG(field, flag) (((field) & (flag)) != 0)

extern wofstream trace;

// TODO: implement a different trace_error
//
#define trace_error trace << __FUNCTION__ << L" @line " << __LINE__ << L" : "
