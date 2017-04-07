
#pragma once

#define _ATL_NO_MSIMG
#define _ATL_NO_OPENGL
#include <atlbase.h>
#include <atlwin.h>
#include <wtl/atlapp.h>
#include <wtl/atlframe.h>
#include <wtl/atlctrls.h>
#include <wtl/atldlgs.h>
#include <ShlObj.h>
#include <shellapi.h>
#include <string>
#include <fstream>
using namespace std;

#include "MsiDumpPublic.h"
#include "Resource.h"
#include "CUnknown.h"
#include "DragDrop.h"
#include "AboutDlg.h"
#include "MainFrame.h"

#define TEST_FLAG(field, flag) (((field) & (flag)) != 0)
