#include "../common/common_utils.h"

int main(int argc, char** argv)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool bEnableOutput = true; // if true, removes all YUV file writing

    // =====================================================================
    // Intel Media SDK decode pipeline setup
    // - In this example we are decoding an AVC (H.264) stream
    // - Video memory surfaces are used to store the decoded frames
    //   (Note that when using HW acceleration video surfaces are prefered, for better performance)
    //

    // Open input H.264 elementary stream (ES) file
    FILE* fSource;
    MSDK_FOPEN(fSource, "/tmp/test.264", "rb");
    MSDK_CHECK_POINTER(fSource, MFX_ERR_NULL_PTR);

    // Create output YUV file
    FILE* fSink = NULL;
    if (bEnableOutput) {
        MSDK_FOPEN(fSink, "out.yuv", "wb");
        MSDK_CHECK_POINTER(fSink, MFX_ERR_NULL_PTR);
    }

    //2. Initialize Intel Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW acceleration if available (on any adapter)
    // - Version 1.0 is selected for greatest backwards compatibility.
    // OS specific notes
    // - On Windows both SW and HW libraries may present
    // - On Linux only HW library only is available
    //   If more recent API features are needed, change the version accordingly
    mfxIMPL impl = MFX_IMPL_HARDWARE;
    mfxVersion ver = { {0, 1} };
    MFXVideoSession session;

    mfxFrameAllocator mfxAllocator;
    sts = Initialize(impl, ver, &session, &mfxAllocator);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Create Media SDK decoder
    MFXVideoDECODE mfxDEC(session);

    //3. Set required video parameters for decode
    mfxVideoParam mfxVideoParams;
    memset(&mfxVideoParams, 0, sizeof(mfxVideoParams));
    mfxVideoParams.mfx.CodecId = MFX_CODEC_AVC;
    mfxVideoParams.IOPattern = MFX_IOPATTERN_OUT_VIDEO_MEMORY;

    //4. Prepare Media SDK bit stream buffer
    // - Arbitrary buffer size for this example
    mfxBitstream mfxBS;
    memset(&mfxBS, 0, sizeof(mfxBS));
    mfxBS.MaxLength = 1024 * 1024;
    mfxBS.Data = new mfxU8[mfxBS.MaxLength];
    MSDK_CHECK_POINTER(mfxBS.Data, MFX_ERR_MEMORY_ALLOC);

    // Read a chunk of data from stream file into bit stream buffer
    // - Parse bit stream, searching for header and fill video parameters structure
    // - Abort if bit stream header is not found in the first bit stream buffer chunk
    sts = ReadBitStreamData(&mfxBS, fSource);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    sts = mfxDEC.DecodeHeader(&mfxBS, &mfxVideoParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Validate video decode parameters (optional)
    // sts = mfxDEC.Query(&mfxVideoParams, &mfxVideoParams);

    // Query number of required surfaces for decoder
    mfxFrameAllocRequest Request;
    memset(&Request, 0, sizeof(Request));
    sts = mfxDEC.QueryIOSurf(&mfxVideoParams, &Request);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    mfxU16 numSurfaces = Request.NumFrameSuggested;

    Request.Type |= WILL_READ; // This line is only required for Windows DirectX11 to ensure that surfaces can be retrieved by the application

    //5. Allocate surfaces for decoder
    mfxFrameAllocResponse mfxResponse;
    sts = mfxAllocator.Alloc(mfxAllocator.pthis, &Request, &mfxResponse);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Allocate surface headers (mfxFrameSurface1) for decoder
    mfxFrameSurface1** pmfxSurfaces = new mfxFrameSurface1 *[numSurfaces];
    MSDK_CHECK_POINTER(pmfxSurfaces, MFX_ERR_MEMORY_ALLOC);
    for (int i = 0; i < numSurfaces; i++) {
        pmfxSurfaces[i] = new mfxFrameSurface1;
        memset(pmfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&(pmfxSurfaces[i]->Info), &(mfxVideoParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
        pmfxSurfaces[i]->Data.MemId = mfxResponse.mids[i];      // MID (memory id) represents one video NV12 surface
    }

    //6. Initialize the Media SDK decoder
    sts = mfxDEC.Init(&mfxVideoParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // ===============================================================
    // 7. Start decoding the frames
    //

    mfxTime tStart, tEnd;
    mfxGetTime(&tStart);

    mfxSyncPoint syncp;
    mfxFrameSurface1* pmfxOutSurface = NULL;
    int nIndex = 0;
    mfxU32 nFrame = 0;

    //
    // Stage 1: Main decoding loop
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts) {
        if (MFX_WRN_DEVICE_BUSY == sts)
            MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call to DecodeFrameAsync

        if (MFX_ERR_MORE_DATA == sts) {
            sts = ReadBitStreamData(&mfxBS, fSource);       // Read more data into input bit stream
            MSDK_BREAK_ON_ERROR(sts);
        }

        if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts) {
            nIndex = GetFreeSurfaceIndex(pmfxSurfaces, numSurfaces);        // Find free frame surface
            MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);
        }
        // Decode a frame asychronously (returns immediately)
        //  - If input bitstream contains multiple frames DecodeFrameAsync will start decoding multiple frames, and remove them from bitstream
        sts = mfxDEC.DecodeFrameAsync(&mfxBS, pmfxSurfaces[nIndex], &pmfxOutSurface, &syncp);

        // Ignore warnings if output is available,
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncp)
            sts = MFX_ERR_NONE;

        if (MFX_ERR_NONE == sts)
            sts = session.SyncOperation(syncp, 60000);      // Synchronize. Wait until decoded frame is ready

        if (MFX_ERR_NONE == sts) {
            ++nFrame;
            if (bEnableOutput) {
                // Surface locking required when read/write video surfaces
                sts = mfxAllocator.Lock(mfxAllocator.pthis, pmfxOutSurface->Data.MemId, &(pmfxOutSurface->Data));
                MSDK_BREAK_ON_ERROR(sts);

                sts = WriteRawFrame(pmfxOutSurface, fSink);
                MSDK_BREAK_ON_ERROR(sts);

                sts = mfxAllocator.Unlock(mfxAllocator.pthis, pmfxOutSurface->Data.MemId, &(pmfxOutSurface->Data));
                MSDK_BREAK_ON_ERROR(sts);

                printf("Frame number: %d\r", nFrame);
                fflush(stdout);
            }
        }
    }

    // MFX_ERR_MORE_DATA means that file has ended, need to go to buffering loop, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //
    // Stage 2: Retrieve the buffered decoded frames
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_SURFACE == sts) {
        if (MFX_WRN_DEVICE_BUSY == sts)
            MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call to DecodeFrameAsync

        nIndex = GetFreeSurfaceIndex(pmfxSurfaces, numSurfaces);        // Find free frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);

        // Decode a frame asychronously (returns immediately)
        sts = mfxDEC.DecodeFrameAsync(NULL, pmfxSurfaces[nIndex], &pmfxOutSurface, &syncp);

        // Ignore warnings if output is available,
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncp)
            sts = MFX_ERR_NONE;

        if (MFX_ERR_NONE == sts)
            sts = session.SyncOperation(syncp, 60000);      // Synchronize. Waits until decoded frame is ready

        if (MFX_ERR_NONE == sts) {
            ++nFrame;
            if (bEnableOutput) {
                // Surface locking required when read/write D3D surfaces
                sts = mfxAllocator.Lock(mfxAllocator.pthis, pmfxOutSurface->Data.MemId, &(pmfxOutSurface->Data));
                MSDK_BREAK_ON_ERROR(sts);

                sts = WriteRawFrame(pmfxOutSurface, fSink);
                MSDK_BREAK_ON_ERROR(sts);

                sts = mfxAllocator.Unlock(mfxAllocator.pthis, pmfxOutSurface->Data.MemId, &(pmfxOutSurface->Data));
                MSDK_BREAK_ON_ERROR(sts);

                printf("Frame number: %d\r", nFrame);
                fflush(stdout);
            }
        }
    }

    // MFX_ERR_MORE_DATA indicates that all buffers has been fetched, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    mfxGetTime(&tEnd);
    double elapsed = TimeDiffMsec(tEnd, tStart) / 1000;
    double fps = ((double)nFrame / elapsed);
    printf("\nExecution time: %3.2f s (%3.2f fps)\n", elapsed, fps);

    // ===================================================================
    //8. Clean up resources
    //  - It is recommended to close Media SDK components first, before releasing allocated surfaces, since
    //    some surfaces may still be locked by internal Media SDK resources.

    mfxDEC.Close();
    // session closed automatically on destruction

    for (int i = 0; i < numSurfaces; i++)
        delete pmfxSurfaces[i];
    MSDK_SAFE_DELETE_ARRAY(pmfxSurfaces);
    MSDK_SAFE_DELETE_ARRAY(mfxBS.Data);

    mfxAllocator.Free(mfxAllocator.pthis, &mfxResponse);

    fclose(fSource);
    if (fSink) fclose(fSink);

    Release();

    return 0;
}


