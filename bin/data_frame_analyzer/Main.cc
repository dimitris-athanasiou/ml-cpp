/*
 * Copyright Elasticsearch B.V. and/or licensed to Elasticsearch B.V. under one
 * or more contributor license agreements. Licensed under the Elastic License;
 * you may not use this file except in compliance with the Elastic License.
 */
//! \brief
//! Group machine generated messages into categories by similarity.
//!
//! DESCRIPTION:\n
//! Expects to be streamed CSV or length encoded data on STDIN or a named pipe,
//! and sends its JSON results to STDOUT or another named pipe.
//!
//! IMPLEMENTATION DECISIONS:\n
//! Standalone program.
//!
#include <core/CJsonOutputStreamWrapper.h>
#include <core/CLogger.h>
#include <core/CoreTypes.h>
#include <core/CProcessPriority.h>


#include <ver/CBuildInfo.h>

#include <api/CCmdSkeleton.h>
#include <api/CIoManager.h>
#include <api/CLengthEncodedInputParser.h>
#include <api/COutputChainer.h>

#include "CCmdLineParser.h"

#include <boost/scoped_ptr.hpp>

#include <string>

#include <stdlib.h>


int main(int argc, char **argv)
{
    // Read command line options
    std::string       configFile;
    std::string       logProperties;
    std::string       logPipe;
    std::string       inputFileName;
    bool              isInputFileNamedPipe(false);
    std::string       outputFileName;
    bool              isOutputFileNamedPipe(false);
    std::string       categorizationFieldName;
    if (ml::data_frame_analyzer::CCmdLineParser::parse(argc,
                                                       argv,
                                                       configFile,
                                                       logProperties,
                                                       logPipe,
                                                       inputFileName,
                                                       isInputFileNamedPipe,
                                                       outputFileName,
                                                       isOutputFileNamedPipe) == false)
    {
        return EXIT_FAILURE;
    }

    // Construct the IO manager before reconfiguring the logger, as it performs
    // std::ios actions that only work before first use
    ml::api::CIoManager ioMgr(inputFileName,
                              isInputFileNamedPipe,
                              outputFileName,
                              isOutputFileNamedPipe,
                              "",
                              false,
                              "",
                              false);

    if (ml::core::CLogger::instance().reconfigure(logPipe, logProperties) == false)
    {
        LOG_FATAL("Could not reconfigure logging");
        return EXIT_FAILURE;
    }

    // Log the program version immediately after reconfiguring the logger.  This
    // must be done from the program, and NOT a shared library, as each program
    // statically links its own version library.
    LOG_DEBUG(ml::ver::CBuildInfo::fullInfo());

    ml::core::CProcessPriority::reducePriority();

    if (ioMgr.initIo() == false)
    {
        LOG_FATAL("Failed to initialise IO");
        return EXIT_FAILURE;
    }

    typedef boost::scoped_ptr<ml::api::CInputParser> TScopedInputParserP;
    TScopedInputParserP inputParser;
    inputParser.reset(new ml::api::CLengthEncodedInputParser(ioMgr.inputStream()));

    ml::core::CJsonOutputStreamWrapper wrappedOutputStream(ioMgr.outputStream());

    // output writer for CFieldDataTyper and persistence callback
    // ml::api::CJsonOutputWriter outputWriter(jobId, wrappedOutputStream);

    // The skeleton avoids the need to duplicate a lot of boilerplate code
    // ml::api::CCmdSkeleton skeleton(restoreSearcher.get(),
    //                                persister.get(),
    //                                *inputParser,
    //                                typer);
    // bool ioLoopSucceeded(skeleton.ioLoop());

    // Unfortunately we cannot rely on destruction to finalise the output writer
    // as it must be finalised before the skeleton is destroyed, and C++
    // destruction order means the skeleton will be destroyed before the output
    // writer as it was constructed last.
    // outputWriter.finalise();

    // if (!ioLoopSucceeded)
    // {
    //     LOG_FATAL("Ml categorization job failed");
    //     return EXIT_FAILURE;
    // }

    // This message makes it easier to spot process crashes in a log file - if
    // this isn't present in the log for a given PID and there's no other log
    // message indicating early exit then the process has probably core dumped
    LOG_DEBUG("Ml analysis pipeline exiting");

    return EXIT_SUCCESS;
}
