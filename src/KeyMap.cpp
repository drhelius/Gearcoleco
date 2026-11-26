/*
 * Gearcoleco - ColecoVision Emulator
 * Copyright (C) 2021  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#include "KeyMap.h"
#include "HostKeys.h"
#include "AdamKeys.h"
#include "common.h"

static const GC_KeyMapEntry kKeyMapEmpty = { false, 0, { 0, 0, 0, 0 } };

static std::string KeyMapTrim(const std::string& text)
{
    size_t start = text.find_first_not_of(" \t\r\n");

    if (start == std::string::npos)
        return "";

    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

static std::string KeyMapLower(const std::string& text)
{
    std::string result(text);

    for (size_t i = 0; i < result.size(); i++)
        result[i] = ((result[i] >= 'A') && (result[i] <= 'Z')) ? (char)(result[i] + 32) : result[i];

    return result;
}

KeyMap::KeyMap()
{
    InitPointer(m_Callback);
    InitPointer(m_pContext);
    memset(m_Layers, 0, sizeof(m_Layers));
    memset(m_HostTargets, 0, sizeof(m_HostTargets));
    memset(m_HostDown, 0, sizeof(m_HostDown));
    memset(m_HostDeferred, 0, sizeof(m_HostDeferred));
    memset(m_References, 0, sizeof(m_References));
    m_bEnabled = true;
    m_bBlocked = false;
    memset(m_bChanged, 0, sizeof(m_bChanged));
    m_EditLayer = GC_KEY_MAP_USER;
    ResetBuiltIn();
}

void KeyMap::Init(GC_KeyEventFn callback, void* context)
{
    m_Callback = callback;
    m_pContext = context;
}

void KeyMap::HostKeyPressed(u16 usage)
{
    if (!IsHostKey(usage) || m_HostDown[usage])
        return;

    m_HostDown[usage] = true;
    m_HostTargets[usage] = kKeyMapEmpty;

    if (IsDeferredHostKey(usage))
    {
        m_HostDeferred[usage] = true;
        return;
    }

    if (usage < kHostKeyFirstModifier)
        FlushDeferredHostKeys();

    PressHostTargets(usage);
}

void KeyMap::HostKeyReleased(u16 usage)
{
    if (!IsHostKey(usage) || !m_HostDown[usage])
        return;

    if (m_HostDeferred[usage])
    {
        m_HostDeferred[usage] = false;
        PressHostTargets(usage);
    }

    m_HostDown[usage] = false;
    GC_KeyMapEntry& targets = m_HostTargets[usage];

    for (int i = targets.count - 1; i >= 0; i--)
        ReleaseKey(targets.keys[i]);

    targets = kKeyMapEmpty;
}

void KeyMap::IgnoreHostKey(u16 usage)
{
    if (!IsHostKey(usage) || m_HostDown[usage])
        return;

    m_HostDown[usage] = true;
    m_HostTargets[usage] = kKeyMapEmpty;
}

void KeyMap::DiscardDeferredHostKeys()
{
    m_HostDeferred[kHostKeyLeftGui] = false;
    m_HostDeferred[kHostKeyRightGui] = false;
}

bool KeyMap::IsHostKeyDown(u16 usage) const
{
    return IsHostKey(usage) && m_HostDown[usage];
}

void KeyMap::ReleaseAll()
{
    for (int i = 0; i < GC_ADAM_KEY_COUNT; i++)
    {
        if (m_References[i] == 0)
            continue;

        m_References[i] = 0;

        if (IsValidPointer(m_Callback))
            m_Callback(m_pContext, (GC_AdamKey)i, false);
    }

    memset(m_HostTargets, 0, sizeof(m_HostTargets));
    memset(m_HostDown, 0, sizeof(m_HostDown));
    memset(m_HostDeferred, 0, sizeof(m_HostDeferred));
}

void KeyMap::Suspend()
{
    DiscardDeferredHostKeys();

    for (int usage = 0; usage < GC_HOST_KEY_COUNT; usage++)
    {
        GC_KeyMapEntry& targets = m_HostTargets[usage];

        for (int i = targets.count - 1; i >= 0; i--)
            ReleaseKey(targets.keys[i]);

        targets = kKeyMapEmpty;
    }
}

void KeyMap::Resync()
{
    for (int i = 0; i < GC_ADAM_KEY_COUNT; i++)
    {
        if ((m_References[i] == 0) && !IsModifier((u8)i) && IsValidPointer(m_Callback))
            m_Callback(m_pContext, (GC_AdamKey)i, false);
    }

    for (int i = 0; i < kModifierKeyCount; i++)
        SyncKey((u8)kModifierKeys[i]);

    for (int i = 0; i < GC_ADAM_KEY_COUNT; i++)
    {
        if ((m_References[i] > 0) && !IsModifier((u8)i) && IsValidPointer(m_Callback))
            m_Callback(m_pContext, (GC_AdamKey)i, true);
    }
}

void KeyMap::SetBlocked(bool blocked)
{
    if (blocked && !m_bBlocked)
        Suspend();

    m_bBlocked = blocked;
}

bool KeyMap::IsBlocked() const
{
    return m_bBlocked;
}

void KeyMap::SetEnabled(bool enabled)
{
    if (enabled == m_bEnabled)
        return;

    Suspend();
    m_bEnabled = enabled;
}

bool KeyMap::IsEnabled() const
{
    return m_bEnabled;
}

void KeyMap::ResetBuiltIn(bool specials)
{
    memset(m_Layers[GC_KEY_MAP_BUILT_IN], 0, sizeof(m_Layers[GC_KEY_MAP_BUILT_IN]));

    for (int i = 0; i < kDefaultKeyCount; i++)
    {
        GC_KeyMapEntry& entry = m_Layers[GC_KEY_MAP_BUILT_IN][kDefaultKeys[i].usage];
        entry.set = true;
        entry.count = 1;
        entry.keys[0] = (u8)kDefaultKeys[i].key;
    }

    for (int i = 0; specials && (i < kDefaultSpecialKeyCount); i++)
    {
        GC_KeyMapEntry& entry = m_Layers[GC_KEY_MAP_BUILT_IN][kDefaultSpecialKeys[i].usage];
        entry.set = true;
        entry.count = 1;
        entry.keys[0] = (u8)kDefaultSpecialKeys[i].key;
    }
}

bool KeyMap::SetLayer(GC_KeyMapLayer layer, const char* text, std::string* error)
{
    GC_KeyMapEntry entries[GC_HOST_KEY_COUNT];
    memset(entries, 0, sizeof(entries));
    std::string data = IsValidPointer(text) ? text : "";
    size_t start = 0;
    int number = 0;

    while (start < data.size())
    {
        size_t end = data.find('\n', start);
        end = end == std::string::npos ? data.size() : end;
        std::string line = KeyMapTrim(data.substr(start, end - start));
        start = end + 1;
        number++;

        if (line.empty() || (line[0] == ';') || (line[0] == '#') || (line[0] == '['))
            continue;

        size_t equals = line.find('=');
        std::string message;
        u16 usage = 0;
        GC_KeyMapEntry entry;

        if (equals == std::string::npos)
            message = "Expected host_key = keys";
        else if (ParseEntry(line.substr(0, equals), line.substr(equals + 1), &usage, &entry, &message))
        {
            if (entries[usage].set)
                message = std::string(GetHostKeyName(usage)) + " is set twice";
            else
                entries[usage] = entry;
        }

        if (!message.empty())
        {
            if (IsValidPointer(error))
                *error = "Line " + int_to_string(number) + ": " + message;

            return false;
        }
    }

    if (layer == GC_KEY_MAP_BUILT_IN)
    {
        ResetBuiltIn();

        for (int i = 0; i < GC_HOST_KEY_COUNT; i++)
        {
            if (entries[i].set)
                m_Layers[layer][i] = entries[i];
        }
    }
    else
        memcpy(m_Layers[layer], entries, sizeof(entries));

    return true;
}

std::string KeyMap::GetLayer(GC_KeyMapLayer layer) const
{
    return FormatLayers(layer, layer);
}

std::string KeyMap::GetMerged() const
{
    return FormatLayers(GC_KEY_MAP_GAME, GC_KEY_MAP_USER);
}

void KeyMap::ClearLayer(GC_KeyMapLayer layer)
{
    if (layer == GC_KEY_MAP_BUILT_IN)
        ResetBuiltIn();
    else
        memset(m_Layers[layer], 0, sizeof(m_Layers[layer]));
}

bool KeyMap::IsLayerEmpty(GC_KeyMapLayer layer) const
{
    for (int i = 0; i < GC_HOST_KEY_COUNT; i++)
    {
        if (m_Layers[layer][i].set)
            return false;
    }

    return true;
}

void KeyMap::SetEntry(GC_KeyMapLayer layer, u16 usage, const GC_KeyMapEntry& entry)
{
    if (IsHostKey(usage))
        m_Layers[layer][usage] = entry;
}

const GC_KeyMapEntry& KeyMap::GetLayerEntry(GC_KeyMapLayer layer, u16 usage) const
{
    return IsHostKey(usage) ? m_Layers[layer][usage] : kKeyMapEmpty;
}

const GC_KeyMapEntry& KeyMap::GetEntry(u16 usage, GC_KeyMapLayer* layer) const
{
    return GetEntryUpTo(m_EditLayer, usage, layer);
}

const GC_KeyMapEntry& KeyMap::GetEntryUpTo(int top, u16 usage, GC_KeyMapLayer* layer) const
{
    if (!IsHostKey(usage))
    {
        if (IsValidPointer(layer))
            *layer = GC_KEY_MAP_BUILT_IN;

        return kKeyMapEmpty;
    }

    for (int l = top; l > GC_KEY_MAP_BUILT_IN; l--)
    {
        if (m_Layers[l][usage].set)
        {
            if (IsValidPointer(layer))
                *layer = (GC_KeyMapLayer)l;

            return m_Layers[l][usage];
        }
    }

    if (IsValidPointer(layer))
        *layer = GC_KEY_MAP_BUILT_IN;

    return m_Layers[GC_KEY_MAP_BUILT_IN][usage];
}

bool KeyMap::Bind(u16 usage, const GC_AdamKey* keys, int count)
{
    if (!IsValidPointer(GetHostKeyName(usage)) || (count < 0) || (count > GC_KEY_MAP_TARGETS))
        return false;

    GC_KeyMapEntry entry = kKeyMapEmpty;
    entry.set = true;

    for (int i = 0; i < count; i++)
    {
        if (!IsValidPointer(GetKeyName(keys[i])))
            return false;

        entry.keys[entry.count++] = (u8)keys[i];
    }

    SetEditEntry(usage, entry);
    return true;
}

bool KeyMap::Unbind(u16 usage, GC_AdamKey key)
{
    if (!IsValidPointer(GetHostKeyName(usage)))
        return false;

    GC_KeyMapLayer layer = GC_KEY_MAP_BUILT_IN;
    const GC_KeyMapEntry& current = GetEntry(usage, &layer);
    GC_KeyMapEntry entry = kKeyMapEmpty;
    entry.set = true;

    for (int i = 0; i < current.count; i++)
    {
        if (current.keys[i] != (u8)key)
            entry.keys[entry.count++] = current.keys[i];
    }

    if (entry.count == current.count)
        return false;

    if ((entry.count == 0) && (layer == m_EditLayer))
    {
        m_Layers[m_EditLayer][usage] = kKeyMapEmpty;
        m_bChanged[m_EditLayer] = true;
        return true;
    }

    SetEditEntry(usage, entry);
    return true;
}

bool KeyMap::RestoreBuiltIn(u16 usage)
{
    if (!IsValidPointer(GetHostKeyName(usage)))
        return false;

    GC_KeyMapEntry entry = m_Layers[GC_KEY_MAP_BUILT_IN][usage];
    entry.set = true;
    SetEditEntry(usage, entry);
    return true;
}

void KeyMap::SetEditLayer(GC_KeyMapLayer layer)
{
    m_EditLayer = layer == GC_KEY_MAP_GLOBAL ? GC_KEY_MAP_GLOBAL : GC_KEY_MAP_USER;
}

GC_KeyMapLayer KeyMap::GetEditLayer() const
{
    return m_EditLayer;
}

bool KeyMap::IsChanged(GC_KeyMapLayer layer) const
{
    return m_bChanged[layer];
}

void KeyMap::SetChanged(GC_KeyMapLayer layer, bool changed)
{
    m_bChanged[layer] = changed;
}

bool KeyMap::IsHostKey(u16 usage)
{
    return usage < GC_HOST_KEY_COUNT;
}

int KeyMap::FindHostKey(const char* name)
{
    for (int i = 0; IsValidPointer(name) && (i < kHostKeyCount); i++)
    {
        if (strcmp(name, kHostKeys[i].name) == 0)
            return kHostKeys[i].usage;
    }

    return -1;
}

const char* KeyMap::GetHostKeyName(u16 usage)
{
    for (int i = 0; i < kHostKeyCount; i++)
    {
        if (kHostKeys[i].usage == usage)
            return kHostKeys[i].name;
    }

    return NULL;
}

const char* KeyMap::GetHostKeyLabel(u16 usage)
{
    for (int i = 0; i < kHostKeyCount; i++)
    {
        if (kHostKeys[i].usage == usage)
            return kHostKeys[i].label;
    }

    return NULL;
}

int KeyMap::FindKey(const char* name)
{
    for (int i = 0; IsValidPointer(name) && (i < kAdamKeyInfoCount); i++)
    {
        if (strcmp(name, kAdamKeyInfos[i].name) == 0)
            return kAdamKeyInfos[i].key;
    }

    return -1;
}

const char* KeyMap::GetKeyName(GC_AdamKey key)
{
    for (int i = 0; i < kAdamKeyInfoCount; i++)
    {
        if (kAdamKeyInfos[i].key == key)
            return kAdamKeyInfos[i].name;
    }

    return NULL;
}

const char* KeyMap::GetKeyLabel(GC_AdamKey key)
{
    for (int i = 0; i < kAdamKeyInfoCount; i++)
    {
        if (kAdamKeyInfos[i].key == key)
            return kAdamKeyInfos[i].label;
    }

    return NULL;
}

int KeyMap::GetDefaultKey(u16 usage)
{
    for (int i = 0; i < kDefaultKeyCount; i++)
    {
        if (kDefaultKeys[i].usage == usage)
            return kDefaultKeys[i].key;
    }

    return -1;
}

bool KeyMap::ParseEntry(const std::string& host, const std::string& value, u16* usage, GC_KeyMapEntry* entry,
    std::string* error)
{
    std::string name = KeyMapLower(KeyMapTrim(host));
    int found = FindHostKey(name.c_str());

    if (found < 0)
    {
        *error = "Unknown host key " + name;
        return false;
    }

    *usage = (u16)found;
    *entry = kKeyMapEmpty;
    entry->set = true;
    std::string targets = KeyMapLower(KeyMapTrim(value));

    if (targets.empty())
    {
        *error = "No keys for " + name;
        return false;
    }

    if (targets == "none")
        return true;

    size_t start = 0;

    while (start <= targets.size())
    {
        size_t plus = targets.find('+', start);
        plus = plus == std::string::npos ? targets.size() : plus;
        std::string target = KeyMapTrim(targets.substr(start, plus - start));
        start = plus + 1;
        int key = FindKey(target.c_str());

        if (key < 0)
        {
            *error = target.empty() ? "Empty key in " + name : "Unknown key " + target;
            return false;
        }

        if (entry->count == GC_KEY_MAP_TARGETS)
        {
            *error = "More than " + int_to_string(GC_KEY_MAP_TARGETS) + " keys for " + name;
            return false;
        }

        for (int i = 0; i < entry->count; i++)
        {
            if (entry->keys[i] == (u8)key)
            {
                *error = target + " is twice in " + name;
                return false;
            }
        }

        entry->keys[entry->count++] = (u8)key;
    }

    return true;
}

std::string KeyMap::FormatTargets(const GC_KeyMapEntry& entry)
{
    if (entry.count == 0)
        return "none";

    std::string out;

    for (int i = 0; i < entry.count; i++)
    {
        const char* name = GetKeyName((GC_AdamKey)entry.keys[i]);
        out += std::string(i > 0 ? " + " : "") + (IsValidPointer(name) ? name : "none");
    }

    return out;
}

const GC_KeyMapEntry& KeyMap::GetActiveEntry(u16 usage) const
{
    return GetEntryUpTo(m_bEnabled ? m_EditLayer : GC_KEY_MAP_GLOBAL, usage, NULL);
}

const GC_KeyMapEntry& KeyMap::GetBaseEntry(GC_KeyMapLayer layer, u16 usage) const
{
    for (int l = (int)layer - 1; l > GC_KEY_MAP_BUILT_IN; l--)
    {
        if (m_Layers[l][usage].set)
            return m_Layers[l][usage];
    }

    return m_Layers[GC_KEY_MAP_BUILT_IN][usage];
}

void KeyMap::SetEditEntry(u16 usage, const GC_KeyMapEntry& entry)
{
    if (SameTargets(entry, GetBaseEntry(m_EditLayer, usage)))
        m_Layers[m_EditLayer][usage] = kKeyMapEmpty;
    else
        m_Layers[m_EditLayer][usage] = entry;

    m_bChanged[m_EditLayer] = true;
}

void KeyMap::PressHostTargets(u16 usage)
{
    if (m_bBlocked)
        return;

    const GC_KeyMapEntry& entry = GetActiveEntry(usage);
    m_HostTargets[usage] = entry;

    for (int i = 0; i < entry.count; i++)
        PressKey(entry.keys[i]);
}

void KeyMap::FlushDeferredHostKeys()
{
    if (m_HostDeferred[kHostKeyLeftGui])
    {
        m_HostDeferred[kHostKeyLeftGui] = false;
        PressHostTargets(kHostKeyLeftGui);
    }

    if (m_HostDeferred[kHostKeyRightGui])
    {
        m_HostDeferred[kHostKeyRightGui] = false;
        PressHostTargets(kHostKeyRightGui);
    }
}

bool KeyMap::IsDeferredHostKey(u16 usage)
{
    return (usage == kHostKeyLeftGui) || (usage == kHostKeyRightGui);
}

void KeyMap::PressKey(u8 key)
{
    if ((m_References[key]++ == 0) && IsValidPointer(m_Callback))
        m_Callback(m_pContext, (GC_AdamKey)key, true);
}

void KeyMap::ReleaseKey(u8 key)
{
    if ((m_References[key] > 0) && (--m_References[key] == 0) && IsValidPointer(m_Callback))
        m_Callback(m_pContext, (GC_AdamKey)key, false);
}

void KeyMap::SyncKey(u8 key)
{
    if (IsValidPointer(m_Callback))
        m_Callback(m_pContext, (GC_AdamKey)key, m_References[key] > 0);
}

bool KeyMap::IsModifier(u8 key)
{
    for (int i = 0; i < kModifierKeyCount; i++)
    {
        if ((u8)kModifierKeys[i] == key)
            return true;
    }

    return false;
}

bool KeyMap::SameTargets(const GC_KeyMapEntry& a, const GC_KeyMapEntry& b)
{
    if (a.count != b.count)
        return false;

    for (int i = 0; i < a.count; i++)
    {
        if (a.keys[i] != b.keys[i])
            return false;
    }

    return true;
}

std::string KeyMap::FormatLayers(int first, int last) const
{
    std::string out;

    for (int usage = 0; usage < GC_HOST_KEY_COUNT; usage++)
    {
        const char* name = GetHostKeyName((u16)usage);

        for (int l = last; IsValidPointer(name) && (l >= first); l--)
        {
            if (!m_Layers[l][usage].set)
                continue;

            out += std::string(name) + " = " + FormatTargets(m_Layers[l][usage]) + "\n";
            break;
        }
    }

    return out;
}
