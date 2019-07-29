
#include<iostream>

using namespace std;

#include "mfxcommon.h"
#include "mfxmvc.h"
#include "mfxjpeg.h"
#include "mfxplugin.h"
#include "mfxplugin++.h"
#include "mfxvideo.h"
#include "mfxvideo++.h"
#include "mfxvp8.h"

#define MSDK_CHECK_RESULT(P, X, ERR)    {if ((X) > (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}

int main()
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxIMPL impl = MFX_IMPL_AUTO_ANY;
    mfxVersion ver = { {0, 1} };
    MFXVideoSession session;
    mfxFrameAllocator mfxAlloc;
    MFXVideoDECODE *pmfxDEC;



    cout<<"execution done!\n";
    return 0;
}