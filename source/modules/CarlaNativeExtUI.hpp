/*
 * Carla Native Plugin API (C++)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef CARLA_NATIVE_EXTERNAL_UI_HPP_INCLUDED
#define CARLA_NATIVE_EXTERNAL_UI_HPP_INCLUDED

#include "CarlaNative.hpp"

#include "CarlaExternalUI.hpp"

/*!
 * @defgroup CarlaNativeAPI Carla Native API
 * @{
 */

// -----------------------------------------------------------------------
// Native Plugin and External UI class

class NativePluginAndUiClass : public NativePluginClass,
                               public CarlaExternalUI
{
public:
    NativePluginAndUiClass(const NativeHostDescriptor* const host, const char* const extUiPath)
        : NativePluginClass(host),
          fExtUiPath(extUiPath)
    {
    }

    ~NativePluginAndUiClass() override
    {
    }

protected:
    // -------------------------------------------------------------------
    // Plugin UI calls

    void uiShow(const bool show) override
    {
        if (show)
        {
            CarlaString path(getResourceDir() + fExtUiPath);
            carla_stdout("Trying to start UI using \"%s\"", path.getBuffer());

            CarlaExternalUI::setData(path.getBuffer(), getSampleRate(), getUiName());
            CarlaExternalUI::start();
        }
        else
        {
            CarlaExternalUI::stop();
        }
    }

    void uiIdle() override
    {
        CarlaExternalUI::idle();

        if (! CarlaExternalUI::isOk())
            return;

        switch (CarlaExternalUI::getAndResetUiState())
        {
        case CarlaExternalUI::UiNone:
        case CarlaExternalUI::UiShow:
            break;
        case CarlaExternalUI::UiCrashed:
            hostUiUnavailable();
            break;
        case CarlaExternalUI::UiHide:
            uiClosed();
            break;
        }
    }

    void uiSetParameterValue(const uint32_t index, const float value) override
    {
        CARLA_SAFE_ASSERT_RETURN(index < getParameterCount(),);

        char tmpBuf[0xff+1];

        writeMsg("control\n", 8);
        std::sprintf(tmpBuf, "%i\n", index);
        writeAndFixMsg(tmpBuf);
        std::sprintf(tmpBuf, "%f\n", value);
        writeAndFixMsg(tmpBuf);
    }

    void uiSetMidiProgram(const uint8_t channel, const uint32_t bank, const uint32_t program) override
    {
        CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS,);

        char tmpBuf[0xff+1];

        writeMsg("program\n", 8);
        std::sprintf(tmpBuf, "%i\n", channel);
        writeAndFixMsg(tmpBuf);
        std::sprintf(tmpBuf, "%i\n", bank);
        writeAndFixMsg(tmpBuf);
        std::sprintf(tmpBuf, "%i\n", program);
        writeAndFixMsg(tmpBuf);
    }

    void uiSetCustomData(const char* const key, const char* const value) override
    {
        CARLA_SAFE_ASSERT_RETURN(key != nullptr,);
        CARLA_SAFE_ASSERT_RETURN(value != nullptr,);

        writeMsg("configure\n", 10);
        writeAndFixMsg(key);
        writeAndFixMsg(value);
    }

    // -------------------------------------------------------------------
    // Pipe Server calls

    bool msgReceived(const char* const msg) override
    {
        if (CarlaExternalUI::msgReceived(msg))
            return true;

        if (std::strcmp(msg, "control") == 0)
        {
            uint32_t param;
            float value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(param), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsFloat(value), true);

            uiParameterChanged(param, value);
        }
        else if (std::strcmp(msg, "program") == 0)
        {
            uint32_t channel, bank, program;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(channel), true);;
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(bank), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsUInt(program), true);
            CARLA_SAFE_ASSERT_RETURN(channel < MAX_MIDI_CHANNELS, true);

            uiMidiProgramChanged(channel, bank, program);
        }
        else if (std::strcmp(msg, "configure") == 0)
        {
            const char* key;
            const char* value;

            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(key), true);
            CARLA_SAFE_ASSERT_RETURN(readNextLineAsString(value), true);

            uiCustomDataChanged(key, value);

            delete[] key;
            delete[] value;
        }
        else if (std::strcmp(msg, "exiting") == 0)
        {
            waitChildClose();
            fUiState = UiHide;

            uiClosed();
        }
        else
        {
            carla_stderr("msgReceived : %s", msg);
            return false;
        }

        return true;
    }

private:
    CarlaString fExtUiPath;

    CARLA_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativePluginAndUiClass)
};

/**@}*/

// -----------------------------------------------------------------------

#endif // CARLA_NATIVE_EXTERNAL_UI_HPP_INCLUDED