#include <Windows.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include "logger.h"  
#include "GeneralFunctions.h"  
#include "EventSinks.h"  

//bool isSerializing = false;

void Install() {
    gfuncs::Install();
    eventSinks::Install();
}

void MessageListener(SKSE::MessagingInterface::Message* message) {
    switch (message->type) {
        // Descriptions are taken from the original skse64 library
        // See:
        // https://github.com/ianpatt/skse64/blob/09f520a2433747f33ae7d7c15b1164ca198932c3/skse64/PluginAPI.h#L193-L212
     //case SKSE::MessagingInterface::kPostLoad: //
        //    logger::info("kPostLoad: sent to registered plugins once all plugins have been loaded");
        //    break;

    //case SKSE::MessagingInterface::kPostPostLoad:
        //    logger::info(
        //        "kPostPostLoad: sent right after kPostLoad to facilitate the correct dispatching/registering of "
        //        "messages/listeners");
        //    break;

    //case SKSE::MessagingInterface::kPreLoadGame:
        //    // message->dataLen: length of file path, data: char* file path of .ess savegame file
        //    logger::info("kPreLoadGame: sent immediately before savegame is read");
        //    break;

    //case SKSE::MessagingInterface::kPostLoadGame:
        // You will probably want to handle this event if your plugin uses a Preload callback
        // as there is a chance that after that callback is invoked the game will encounter an error
        // while loading the saved game (eg. corrupted save) which may require you to reset some of your
        // plugin state.
        

        //logger::trace("kPostLoadGame: sent after an attempt to load a saved game has finished");
        //break;

        //case SKSE::MessagingInterface::kSaveGame:
            //    logger::info("kSaveGame");
            //    break;

        //case SKSE::MessagingInterface::kDeleteGame:
            //    // message->dataLen: length of file path, data: char* file path of .ess savegame file
            //    logger::info("kDeleteGame: sent right before deleting the .skse cosave and the .ess save");
            //    break;

    //case SKSE::MessagingInterface::kInputLoaded:
        //logger::info("kInputLoaded: sent right after game input is loaded, right before the main menu initializes");

        //break;

    //case SKSE::MessagingInterface::kNewGame:
        //logger::trace("kNewGame: sent after a new game is created, before the game has loaded");
        //break;

    case SKSE::MessagingInterface::kDataLoaded:
        Install();
        break;

        //default: //
            //    logger::info("Unknown system message of type: {}", message->type);
            //    break;
    }
}

void LoadCallback(SKSE::SerializationInterface* a_intfc) {
    //logger::trace("LoadCallback started");

    //std::uint32_t type, version, length;

    //if (a_intfc) {
    //    if (!isSerializing) {
    //        isSerializing = true;
    //        while (a_intfc->GetNextRecordInfo(type, version, length)) {
    //            if (type == 'DBT0') {
    //            }
    //            else if (type == 'DBT1') {
    //            }
    //        }

    //        logger::trace("LoadCallback complete"); 
    //    }
    //    else {
    //        logger::debug("{}: already loading or saving, aborting load.", __func__);
    //    }
    //}
    //else {
    //    logger::error("{}: a_intfc doesn't exist, aborting load.", __func__);
    //}
}

void SaveCallback(SKSE::SerializationInterface* a_intfc) {
    //logger::trace("SaveCallback started");

    //if (a_intfc) {
    //    if (!isSerializing) {
    //        isSerializing = true;
    //        
    //        isSerializing = false;
    //        logger::trace("SaveCallback complete");
    //    }
    //    else {
    //        logger::debug("{}: already loading or saving, aborting save.", __func__);
    //    }
    //}
    //else {
    //    logger::error("{}: a_intfc doesn't exist, aborting save.", __func__);
    //}
}

//init================================================================================================================================================================
SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    SetupLog("Data/SKSE/Plugins/ConsoleLogViewer.ini");

    SKSE::GetMessagingInterface()->RegisterListener(MessageListener);

    //auto* serialization = SKSE::GetSerializationInterface();
    //serialization->SetUniqueID('CLVw');
    //serialization->SetSaveCallback(SaveCallback);
    //serialization->SetLoadCallback(LoadCallback);

    return true;
}
