/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2012 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <signal.h>
#include "libcec/include/cec.h"

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

extern "C" {
#include "/home/aep/proj/ubus/lib/ubus.h"
}

using namespace CEC;
using namespace std;


#define CStdString std::string

#define CEC_CONFIG_VERSION CEC_CLIENT_VERSION_CURRENT;

#include "libcec/include/cecloader.h"

ICECCallbacks        g_callbacks;
libcec_configuration g_config;
int                  g_cecLogLevel(-1);
int                  g_cecDefaultLogLevel(CEC_LOG_ALL);
ofstream             g_logOutput;
bool                 g_bShortLog(false);
CStdString           g_strPort;
bool                 g_bSingleCommand(false);
bool                 g_bExit(false);
bool                 g_bHardExit(false);


int main (int argc, char *argv[])
{
    if(argc<2){
        fprintf(stderr,"usage: echo /path/to/echo.method\n");
        exit(1);
    }
    ubus_t * service=ubus_create(argv[1]);
    if(service==0){
        perror("ubus_create");
        exit(0);
    }



    g_config.Clear();
    g_callbacks.Clear();
    snprintf(g_config.strDeviceName, 13, "XBMC");
    g_config.clientVersion       = CEC_CONFIG_VERSION;
    g_config.bActivateSource     = 0;
    //    g_callbacks.CBCecLogMessage  = &CecLogMessage;
    //    g_callbacks.CBCecKeyPress    = &CecKeyPress;
    //    g_callbacks.CBCecCommand     = &CecCommand;
    //    g_callbacks.CBCecAlert       = &CecAlert;
    //    g_config.callbacks           = &g_callbacks;


    g_config.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);
    ICECAdapter *parser = LibCecInitialise(&g_config);
    if (!parser)
    {
#ifdef __WINDOWS__
        cout << "Cannot load libcec.dll" << endl;
#else
        cout << "Cannot load libcec.so" << endl;
#endif
        if (parser)
            UnloadLibCec(parser);
        return 1;
    }

    // init video on targets that need this
    parser->InitVideoStandalone();


    if (g_strPort.empty())
    {
        if (!g_bSingleCommand)
            cout << "no serial port given. trying autodetect: ";
        cec_adapter devices[10];
        uint8_t iDevicesFound = parser->FindAdapters(devices, 10, NULL);
        if (iDevicesFound <= 0)
        {
            if (g_bSingleCommand)
                cout << "autodetect ";
            cout << "FAILED" << endl;
            UnloadLibCec(parser);
            return 1;
        }
        else
        {
            if (!g_bSingleCommand)
            {
                cout << endl << " path:     " << devices[0].path << endl <<
                    " com port: " << devices[0].comm << endl << endl;
            }
            g_strPort = devices[0].comm;
        }
    }

    fprintf(stderr,"opening a connection to the CEC adapter...");

    if (!parser->Open(g_strPort.c_str()))
    {
        fprintf(stderr,"unable to open the device on port %s", g_strPort.c_str());
        UnloadLibCec(parser);
        return 1;
    }

    if (!g_bSingleCommand)
        fprintf(stderr,"waiting for input\n");




    signal(SIGPIPE, SIG_IGN);


    fd_set rfds;
    char buff [2000];
    for(;;) {
        FD_ZERO (&rfds);
        int maxfd=ubus_select_all(service,&rfds);
        if (select(maxfd+2, &rfds, NULL, NULL, NULL) < 0){
            perror("select");
            exit(1);
        }
        ubus_activate_all(service,&rfds,0);
        ubus_chan_t * chan=0;
        while((chan=ubus_ready_chan(service))){
            int len = ubus_read(chan,&buff,2000);
            if (len < 1) {
                ubus_disconnect(chan);
            }else {
                if (strncmp(buff, "volup", 5) == 0) {
                    parser->VolumeUp();
                    ubus_write(chan,&buff,len);
                } else if (strncmp(buff, "voldown", 7) == 0) {
                    parser->VolumeDown();
                    ubus_write(chan,&buff,len);
                } else if (strncmp(buff, "mute", 4) == 0) {
                    parser->AudioToggleMute();
                    ubus_write(chan,&buff,len);
                }
            }
        }
    }
    fprintf(stderr,"bye\n");
    ubus_destroy(service);


    parser->Close();
    UnloadLibCec(parser);

    if (g_logOutput.is_open())
        g_logOutput.close();

    return 0;
}


