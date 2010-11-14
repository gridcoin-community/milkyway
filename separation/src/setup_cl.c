/* Copyright 2010 Matthew Arsenault, Travis Desell, Boleslaw
Szymanski, Heidi Newberg, Carlos Varela, Malik Magdon-Ismail and
Rensselaer Polytechnic Institute.

This file is part of Milkway@Home.

Milkyway@Home is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Milkyway@Home is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "milkyway_util.h"
#include "milkyway_cl.h"
#include "mw_cl.h"
#include "setup_cl.h"
#include "separation_cl_buffers.h"
#include "separation_cl_defs.h"
#include "separation_binaries.h"

#if SEPARATION_INLINE_KERNEL
  #include "integral_kernel.h"
#endif /* SEPARATION_INLINE_KERNEL */


/* Only sets the constant arguments, not the outputs which we double buffer */
static inline cl_int separationSetKernelArgs(CLInfo* ci, SeparationCLMem* cm)
{
    cl_int err = CL_SUCCESS;

    /* Set output buffer arguments */
    err |= clSetKernelArg(ci->kern, 0, sizeof(cl_mem), &cm->outMu);
    err |= clSetKernelArg(ci->kern, 1, sizeof(cl_mem), &cm->outProbs);

    /* The constant arguments */
    err |= clSetKernelArg(ci->kern, 2, sizeof(cl_mem), &cm->ap);
    err |= clSetKernelArg(ci->kern, 3, sizeof(cl_mem), &cm->ia);
    err |= clSetKernelArg(ci->kern, 4, sizeof(cl_mem), &cm->sc);
    err |= clSetKernelArg(ci->kern, 5, sizeof(cl_mem), &cm->rc);
    err |= clSetKernelArg(ci->kern, 6, sizeof(cl_mem), &cm->sg_dx);
    err |= clSetKernelArg(ci->kern, 7, sizeof(cl_mem), &cm->rPts);
    err |= clSetKernelArg(ci->kern, 8, sizeof(cl_mem), &cm->lbts);

    if (err != CL_SUCCESS)
    {
        warn("Error setting kernel arguments: %s\n", showCLInt(err));
        return err;
    }

    return CL_SUCCESS;
}

#if SEPARATION_INLINE_KERNEL

char* findKernelSrc()
{
    return integral_kernel_src;
}

void freeKernelSrc(char* src)
{
  #pragma unused(src)
}

#else

/* Reading from the file is more convenient for actually working on
 * it. Inlining is more useful for releasing when we don't want to
 * deal with the hassle of distributing more files. */
char* findKernelSrc()
{
    char* kernelSrc = NULL;
    kernelSrc = mwReadFile("../kernels/integrals.cl");
    if (!kernelSrc)
        warn("Failed to read kernel file\n");

    return kernelSrc;
}

void freeKernelSrc(char* src)
{
    free(src);
}

#endif


#define NUM_CONST_BUF_ARGS 5

/* Check that the device has the necessary resources */
static cl_bool separationCheckDevCapabilities(const DevInfo* di, const SeparationSizes* sizes)
{
    size_t totalOut;
    size_t totalConstBuf;
    size_t totalGlobalConst;
    size_t totalMem;

    totalOut = 2 * sizes->outMu + 2 * sizes->outProbs; /* 2 buffers for double buffering */
    totalConstBuf = sizes->ap + sizes->ia + sizes->sc + sizes->rc + sizes->sg_dx;
    totalGlobalConst = sizes->lbts + sizes->rPts;

    totalMem = totalOut + totalConstBuf + totalGlobalConst;

    if (totalMem > di->memSize)
    {
        warn("Total required device memory (%zu) > available (%zu)\n", totalMem, di->memSize);
        return CL_FALSE;
    }

    /* Check individual allocations. Right now ATI has a fairly small
     * maximum allowed allocation compared to the actual memory
     * available. */
    if (totalOut > di->memSize)
    {
        warn("Device has insufficient global memory for output buffers\n");
        return CL_FALSE;
    }

    if (sizes->outMu > di->maxMemAlloc || sizes->outProbs > di->maxMemAlloc)
    {
        warn("An output buffer would exceed CL_DEVICE_MAX_MEM_ALLOC_SIZE\n");
        return CL_FALSE;
    }

    if (sizes->lbts > di->maxMemAlloc || sizes->rPts > di->maxMemAlloc)
    {
        warn("A global constant buffer would exceed CL_DEVICE_MAX_MEM_ALLOC_SIZE\n");
        return CL_FALSE;
    }

    if (NUM_CONST_BUF_ARGS > di->maxConstArgs)
    {
        warn("Need more constant arguments than available\n");
        return CL_FALSE;
    }

    if (totalConstBuf > di-> maxConstBufSize)
    {
        warn("Device doesn't have enough constant buffer space\n");
        return CL_FALSE;
    }

  #if DOUBLEPREC
    if (!mwSupportsDoubles(di))
    {
        warn("Device doesn't support double precision\n");
        return CL_FALSE;
    }
  #endif

    return CL_TRUE;
}

/* Get string of options to pass to the CL compiler. */
static char* getCompilerFlags(const ASTRONOMY_PARAMETERS* ap, const DevInfo* di)
{
    char* compileFlags = NULL;
    char cwd[1024] = "";
    char extraFlags[1024] = "";
    char includeFlags[4096] = "";

    /* Math options for CL compiler */
    const char* mathFlags = "-cl-mad-enable "
                            "-cl-no-signed-zeros "
                            "-cl-strict-aliasing "
                            "-cl-finite-math-only ";

    /* Build options used by milkyway_math stuff */
    const char* mathOptions = "-DUSE_CL_MATH_TYPES=0 "
                              "-DUSE_MAD=1 "
                              "-DUSE_FMA=0 ";

    /* Extra flags for different compilers */
    const char* nvidiaOptFlags = "-cl-nv-maxrregcount=32 ";
    const char* atiOptFlags    = "";

  #if DOUBLEPREC
    const char* precDefStr   = "-DDOUBLEPREC=1 ";
    const char* atiPrecStr   = "";
    const char* otherPrecStr = "";
  #else
    const char* precDefStr = "-DDOUBLEPREC=0 ";
    const char* atiPrecStr = "--single_precision_constant ";
    const char* clPrecStr  = "-cl-single-precision-constant ";
  #endif

    /* Constants compiled into kernel */
    const char* apDefStr = "-DNSTREAM=%u "
                           "-DFAST_H_PROB=%d "
                           "-DAUX_BG_PROFILE=%d ";

    const char* includeStr = "-I%s/../include "
                             "-I%s/../../include "
                             "-I%s/../../milkyway/include ";

    /* Big enough. Also make sure to count for the extra characters of the format specifiers */
    char apDefBuf[sizeof(apDefStr) + 3 * 12 + 6];
    char precDefBuf[2 * sizeof(atiPrecStr) + sizeof(precDefStr)];

    size_t totalSize = 3 * sizeof(cwd) + (sizeof(includeStr) + 6)
                     + sizeof(precDefBuf)
                     + sizeof(apDefBuf)
                     + sizeof(mathOptions)
                     + sizeof(mathFlags)
                     + sizeof(extraFlags);

    if (snprintf(apDefBuf, sizeof(apDefBuf), apDefStr,
                 ap->number_streams,
                 ap->fast_h_prob,
                 ap->aux_bg_profile) < 0)
    {
        warn("Error getting ap constant definitions\n");
        return NULL;
    }

    /* Always use this flag */
    strncpy(precDefBuf, precDefStr, sizeof(precDefBuf));

  #if !DOUBLEPREC
    /* The ATI compiler rejects the one you're supposed to use, in
     * favor of a totally undocumented flag. */
    strncat(precDefBuf,
            di->vendorID != MW_AMD_ATI ? clPrecStr : atiPrecStr,
            sizeof(2 * atiPrecStr));
  #endif /* !DOUBLEPREC */


    if (di->vendorID == MW_NVIDIA)
        strncat(extraFlags, nvidiaOptFlags, sizeof(extraFlags));
    else if (di->vendorID == MW_AMD_ATI)
        strncat(extraFlags, atiOptFlags, sizeof(extraFlags));
    else
        warn("Unknown vendor ID: 0x%x\n", di->vendorID);

    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("getcwd");
        return NULL;
    }

    if (snprintf(includeFlags, sizeof(includeFlags), includeStr, cwd, cwd, cwd) < 0)
    {
        warn("Failed to get include flags\n");
        return NULL;
    }

    compileFlags = mallocSafe(totalSize);
    if (snprintf(compileFlags, totalSize, "%s %s %s %s %s %s ",
                 includeFlags,
                 precDefBuf,
                 apDefBuf,
                 mathOptions,
                 mathFlags,
                 extraFlags) < 0)
    {
        warn("Failed to get compile flags\n");
        free(compileFlags);
        return NULL;
    }

    return compileFlags;
}

cl_int setupSeparationCL(CLInfo* ci,
                         SeparationCLMem* cm,
                         const ASTRONOMY_PARAMETERS* ap,
                         const INTEGRAL_AREA* ia,
                         const STREAM_CONSTANTS* sc,
                         const STREAM_GAUSS sg,
                         const CLRequest* clr)
{
    cl_int err;
    char* compileFlags;
    char* kernelSrc;

    DevInfo di;
    SeparationSizes sizes;

    err = mwSetupCL(ci, &di, clr);
    if (err != CL_SUCCESS)
    {
        warn("Error getting device and context: %s\n", showCLInt(err));
        return err;
    }

    compileFlags = getCompilerFlags(ap, &di);
    if (!compileFlags)
    {
        warn("Failed to get compiler flags\n");
        return -1;
    }

    kernelSrc = findKernelSrc();
    if (!kernelSrc)
    {
        warn("Failed to read CL kernel source\n");
        return -1;
    }

    warn("\nCompiler flags:\n%s\n\n", compileFlags);
    err = mwSetProgramFromSrc(ci, "mu_sum_kernel", &kernelSrc, 1, compileFlags);

    freeKernelSrc(kernelSrc);
    free(compileFlags);

    if (err != CL_SUCCESS)
    {
        warn("Error creating program from source: %s\n", showCLInt(err));
        return err;
    }

    calculateSizes(&sizes, ap, ia);

    if (!separationCheckDevCapabilities(&di, &sizes))
    {
        warn("Device failed capability check\n");
        return -1;
    }

    err = createSeparationBuffers(ci, cm, ap, ia, sc, sg, &sizes);
    if (err != CL_SUCCESS)
    {
        warn("Failed to create CL buffers: %s\n", showCLInt(err));
        return err;
    }

    err = separationSetKernelArgs(ci, cm);
    if (err != CL_SUCCESS)
    {
        warn("Failed to set integral kernel arguments: %s\n", showCLInt(err));
        return err;
    }

    return CL_SUCCESS;
}

